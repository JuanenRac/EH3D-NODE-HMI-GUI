// EH3D-NODE-HMI-GUI - main/module_p4.c: the Guition JC8012P4A1C module with the single drivers of the component registry
// (all of them Apache-2.0 or MIT). Not run on a module: the panel timing and its initialisation commands are the defaults
// of the JD9365 driver, and the pins of docs/HARDWARE.md are to be checked against the schematic. GPL-3.0-or-later.
#include "module_p4.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "esp_codec_dev_defaults.h"
#include "esp_lcd_jd9365.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"

static const char *TAG = "module";

#define PANEL_H_RES 800
#define PANEL_V_RES 1280
#define PANEL_RESET_GPIO GPIO_NUM_27
#define TOUCH_RESET_GPIO GPIO_NUM_22
#define TOUCH_INT_GPIO GPIO_NUM_21
#define I2C_SDA_GPIO GPIO_NUM_7
#define I2C_SCL_GPIO GPIO_NUM_8
#define I2S_MCLK_GPIO GPIO_NUM_13
#define I2S_SCLK_GPIO GPIO_NUM_12
#define I2S_LRCK_GPIO GPIO_NUM_10
#define I2S_DOUT_GPIO GPIO_NUM_9
#define I2S_DIN_GPIO GPIO_NUM_11
#define DSI_PHY_LDO_CHANNEL 3
#define DSI_PHY_LDO_MV 2500
#define SD_LDO_CHANNEL 4
#define BACKLIGHT_CHANNEL LEDC_CHANNEL_0
#define BACKLIGHT_TIMER LEDC_TIMER_1
#define DRAW_BUFFER_LINES 50

static i2c_master_bus_handle_t s_i2c = NULL;

static bool i2c_start(void) {
    if (s_i2c != NULL) return true;
    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };
    if (i2c_new_master_bus(&config, &s_i2c) != ESP_OK) {
        s_i2c = NULL;
        return false;
    }
    return true;
}

static esp_err_t backlight_start(void) {
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = BACKLIGHT_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) return err;
    const ledc_channel_config_t channel = {
        .gpio_num = CONFIG_HMI_BACKLIGHT_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BACKLIGHT_CHANNEL,
        .timer_sel = BACKLIGHT_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&channel);
}

void module_set_brightness(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, (1023u * (unsigned)percent) / 100u);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
}

static esp_err_t panel_start(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel) {
    static esp_ldo_channel_handle_t phy_power = NULL;
    const esp_ldo_channel_config_t ldo = {.chan_id = DSI_PHY_LDO_CHANNEL, .voltage_mv = DSI_PHY_LDO_MV};
    esp_err_t err = esp_ldo_acquire_channel(&ldo, &phy_power);
    if (err != ESP_OK) return err;

    esp_lcd_dsi_bus_handle_t bus = NULL;
    const esp_lcd_dsi_bus_config_t bus_config = JD9365_PANEL_BUS_DSI_2CH_CONFIG();
    err = esp_lcd_new_dsi_bus(&bus_config, &bus);
    if (err != ESP_OK) return err;

    const esp_lcd_dbi_io_config_t io_config = JD9365_PANEL_IO_DBI_CONFIG();
    err = esp_lcd_new_panel_io_dbi(bus, &io_config, io);
    if (err != ESP_OK) return err;

    static esp_lcd_dpi_panel_config_t timing = JD9365_800_1280_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    jd9365_vendor_config_t vendor = {
        .mipi_config = {.dsi_bus = bus, .dpi_config = &timing, .lane_num = 2},
    };
    const esp_lcd_panel_dev_config_t device = {
        .reset_gpio_num = PANEL_RESET_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor,
    };
    err = esp_lcd_new_panel_jd9365(*io, &device, panel);
    if (err != ESP_OK) return err;
    err = esp_lcd_panel_reset(*panel);
    if (err != ESP_OK) return err;
    err = esp_lcd_panel_init(*panel);
    if (err != ESP_OK) return err;
    return esp_lcd_panel_disp_on_off(*panel, true);
}

// After a reset pulse: a GT911 read by polling, or else a GSL3680 (main/touch_gsl3680.cpp).
static void touch_start(lv_display_t *display) {
    if (!i2c_start()) {
        ESP_LOGW(TAG, "no touch: the I2C bus does not start");
        return;
    }
    gpio_config_t reset = {.pin_bit_mask = 1ULL << TOUCH_RESET_GPIO, .mode = GPIO_MODE_OUTPUT};
    gpio_config_t interrupt = {.pin_bit_mask = 1ULL << TOUCH_INT_GPIO, .mode = GPIO_MODE_INPUT};
    gpio_config(&reset);
    gpio_config(&interrupt);
    gpio_set_level(TOUCH_RESET_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TOUCH_RESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(60));

    uint8_t address = 0;
    if (i2c_master_probe(s_i2c, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 50) == ESP_OK) {
        address = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
    } else if (i2c_master_probe(s_i2c, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, 50) == ESP_OK) {
        address = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
    }
    if (address == 0) {
        if (!touch_gsl3680_start(s_i2c, display, TOUCH_RESET_GPIO)) ESP_LOGW(TAG, "no touch: no GT911 answers and no GSL3680 starts");
        return;
    }

    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_config.dev_addr = address;
    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_touch_config_t config = {
        .x_max = PANEL_H_RES,
        .y_max = PANEL_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
    };
    esp_lcd_touch_handle_t touch = NULL;
    if (esp_lcd_new_panel_io_i2c(s_i2c, &io_config, &io) != ESP_OK || esp_lcd_touch_new_i2c_gt911(io, &config, &touch) != ESP_OK) {
        ESP_LOGW(TAG, "no touch: the GT911 does not start");
        return;
    }
    const lvgl_port_touch_cfg_t port = {.disp = display, .handle = touch};
    if (lvgl_port_add_touch(&port) == NULL) ESP_LOGW(TAG, "no touch: LVGL does not take it");
}

bool module_display_start(void) {
    const lvgl_port_cfg_t port = ESP_LVGL_PORT_INIT_CONFIG();
    if (lvgl_port_init(&port) != ESP_OK) return false;
    if (backlight_start() != ESP_OK) return false;

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_handle_t panel = NULL;
    const esp_err_t err = panel_start(&io, &panel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "the panel does not start: %s", esp_err_to_name(err));
        return false;
    }

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = PANEL_H_RES * DRAW_BUFFER_LINES,
        .hres = PANEL_H_RES,
        .vres = PANEL_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {.buff_dma = 1, .sw_rotate = 1},
    };
    const lvgl_port_display_dsi_cfg_t dsi_config = {.flags = {.avoid_tearing = 0}};
    lv_display_t *display = lvgl_port_add_disp_dsi(&display_config, &dsi_config);
    if (display == NULL) return false;
    touch_start(display);
    return true;
}

esp_codec_dev_handle_t module_speaker_start(void) {
    if (!i2c_start()) return NULL;

    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    channel.auto_clear = true;
    i2s_chan_handle_t tx = NULL;
    if (i2s_new_channel(&channel, &tx, NULL) != ESP_OK) return NULL;
    const i2s_std_config_t standard = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(22050),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk = I2S_MCLK_GPIO,
                .bclk = I2S_SCLK_GPIO,
                .ws = I2S_LRCK_GPIO,
                .dout = I2S_DOUT_GPIO,
                .din = I2S_DIN_GPIO,
            },
    };
    if (i2s_channel_init_std_mode(tx, &standard) != ESP_OK || i2s_channel_enable(tx) != ESP_OK) return NULL;

    audio_codec_i2s_cfg_t data_config = {.port = I2S_NUM_1, .tx_handle = tx};
    const audio_codec_data_if_t *data = audio_codec_new_i2s_data(&data_config);
    audio_codec_i2c_cfg_t control_config = {.port = I2C_NUM_1, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = s_i2c};
    const audio_codec_ctrl_if_t *control = audio_codec_new_i2c_ctrl(&control_config);
    const audio_codec_gpio_if_t *gpio = audio_codec_new_gpio();
    if (data == NULL || control == NULL || gpio == NULL) return NULL;

    es8311_codec_cfg_t codec_config = {
        .ctrl_if = control,
        .gpio_if = gpio,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = CONFIG_HMI_AMP_GPIO,
        .use_mclk = true,
        .hw_gain = {.pa_voltage = 5.0, .codec_dac_voltage = 3.3},
    };
    const audio_codec_if_t *codec = es8311_codec_new(&codec_config);
    if (codec == NULL) return NULL;
    esp_codec_dev_cfg_t device = {.dev_type = ESP_CODEC_DEV_TYPE_OUT, .codec_if = codec, .data_if = data};
    return esp_codec_dev_new(&device);
}

bool module_sd_mount(const char *mount_point) {
    static sd_pwr_ctrl_handle_t power = NULL;
    static bool mounted = false;  // the start of the touch may have mounted it already
    if (mounted) return true;
    if (power == NULL) {
        const sd_pwr_ctrl_ldo_config_t ldo = {.ldo_chan_id = SD_LDO_CHANNEL};
        if (sd_pwr_ctrl_new_on_chip_ldo(&ldo, &power) != ESP_OK) {
            power = NULL;
            return false;
        }
    }
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    host.pwr_ctrl_handle = power;

    sdmmc_slot_config_t slot;
    memset(&slot, 0, sizeof slot);
    slot.cd = SDMMC_SLOT_NO_CD;
    slot.wp = SDMMC_SLOT_NO_WP;
    slot.width = 4;

    const esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 64 * 1024,
    };
    sdmmc_card_t *card = NULL;
    mounted = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot, &mount, &card) == ESP_OK;
    return mounted;
}
