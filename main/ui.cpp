// EH3D-NODE-HMI-GUI - main/ui.cpp: the screens of the touch display, in the look of the desktop program (Forge theme: dark, amber,
// cards with a thin amber outline) and the same order: machine, job, viewer, machines and stations, settings. Lying down the machine screen
// has three columns above the bar of the position; standing up the columns stack. The stop button is always on the bar and always works.
// This has not been run on a module yet. GPL-3.0-or-later.
#include "ui.hpp"

#include <cstdio>

#include "lvgl.h"

#include "../core/ui_text.hpp"

// the fonts of the screen (tools/make_fonts.py): the characters of the thirty-seven languages, with the symbols of LVGL as fallback
extern "C" {
LV_FONT_DECLARE(eh3d_font_16)
LV_FONT_DECLARE(eh3d_font_24)
}

namespace ui {

namespace {

const lv_color_t kBg = LV_COLOR_MAKE(0x14, 0x11, 0x0d);
const lv_color_t kCard = LV_COLOR_MAKE(0x18, 0x14, 0x10);
const lv_color_t kAmber = LV_COLOR_MAKE(0xff, 0xb3, 0x00);
const lv_color_t kText = LV_COLOR_MAKE(0xf0, 0xe6, 0xd0);
const lv_color_t kDim = LV_COLOR_MAKE(0xa8, 0x98, 0x78);
const lv_color_t kSlate = LV_COLOR_MAKE(0x37, 0x47, 0x4f);

void (*g_post)(const Request&) = nullptr;
hmi::Layout g_layout;
constexpr int kPages = 6;  // machine, job, viewer, machines and stations, settings, quality check
int g_pages_n = 5;         // the quality check is the sixth page and only a machine with the camera of the drill head has it
lv_obj_t* g_pages[kPages] = {nullptr};
lv_obj_t* g_tabs[kPages] = {nullptr};
lv_obj_t* g_armed = nullptr;
lv_obj_t* g_banner = nullptr;
lv_obj_t* g_ack = nullptr;
lv_obj_t* g_note = nullptr;
lv_obj_t* g_bar = nullptr;
lv_obj_t* g_dialog = nullptr;
lv_obj_t* g_dialog_text = nullptr;
lv_obj_t* g_fleet_list = nullptr;
lv_obj_t* g_card_list = nullptr;
lv_obj_t* g_selected_file = nullptr;
lv_obj_t* g_job_summary = nullptr;
lv_obj_t* g_tools_box = nullptr;
lv_obj_t* g_flow_label = nullptr;
lv_obj_t* g_resume_button = nullptr;
lv_obj_t* g_continue_button = nullptr;
lv_obj_t* g_toolchange_button = nullptr;
lv_obj_t* g_over_label = nullptr;
bool g_over_refuse = false;
lv_obj_t* g_move_buttons[10] = {nullptr};
int g_move_count = 0;
// the quality check
lv_obj_t* g_q_badge = nullptr;
lv_obj_t* g_q_badge_text = nullptr;
lv_obj_t* g_q_detail = nullptr;
lv_obj_t* g_q_light_label = nullptr;
lv_obj_t* g_q_hole_label = nullptr;
lv_obj_t* g_q_run = nullptr;
int g_q_light = 2;  // the index in hmi::kLightLevels of the light for the picture
int g_q_hole = 2;   // the index in hmi::kHoleWidths of the hole in the centre
hmi::Lang g_q_lang = hmi::Lang::En;

void send(const Request& r) {
    if (g_post != nullptr) g_post(r);
}

void send_action(const hmi::Action& a) {
    Request r;
    r.what = What::Action;
    r.action = a;
    send(r);
}

void style_card(lv_obj_t* o) {
    lv_obj_set_style_bg_color(o, kCard, 0);
    lv_obj_set_style_border_color(o, kAmber, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_radius(o, 10, 0);
    lv_obj_set_style_pad_all(o, 12, 0);
    lv_obj_set_style_text_color(o, kText, 0);
}

lv_obj_t* card(lv_obj_t* parent, const char* title) {
    lv_obj_t* c = lv_obj_create(parent);
    style_card(c);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* t = lv_label_create(c);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, kAmber, 0);
    return c;
}

lv_obj_t* button(lv_obj_t* parent, const char* label, lv_event_cb_t cb, void* user, lv_color_t colour, int w, int h) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, colour, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, label);
    lv_obj_center(l);
    return b;
}

// A button with a symbol in place of a word: the symbols are the ones of LVGL (the same meaning in every language), drawn in the larger font.
lv_obj_t* icon_button(lv_obj_t* parent, const char* symbol, lv_event_cb_t cb, void* user, lv_color_t colour, int w, int h) {
    lv_obj_t* b = button(parent, symbol, cb, user, colour, w, h);
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &eh3d_font_24, 0);
    return b;
}

// the colours of the buttons of a job, as in the desktop program: green to start, blue to pause, red to stop
const lv_color_t kGo = LV_COLOR_MAKE(0x28, 0xe6, 0x5a);
const lv_color_t kHold = LV_COLOR_MAKE(0x28, 0x96, 0xff);
const lv_color_t kHalt = LV_COLOR_MAKE(0xff, 0x28, 0x28);

void on_stop(lv_event_t*) { send_action({hmi::Kind::Stop, "stop"}); }
void on_ack(lv_event_t*) { Request r; r.what = What::AckRelease; send(r); }
void on_confirm(lv_event_t*) { Request r; r.what = What::Confirm; send(r); }
void on_cancel(lv_event_t*) { Request r; r.what = What::Cancel; send(r); }
void on_action(lv_event_t* e) { send_action(*static_cast<const hmi::Action*>(lv_event_get_user_data(e))); }

void on_tab(lv_event_t* e) {
    const int idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    for (int i = 0; i < g_pages_n; ++i) {
        if (i == idx) lv_obj_remove_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_border_color(g_tabs[i], i == idx ? kAmber : kDim, 0);
    }
}

void on_orientation(lv_event_t* e) {
    Request r;
    r.what = What::SetOrientation;
    r.orientation = static_cast<hmi::Orientation>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    send(r);
}

void on_language(lv_event_t* e) {
    Request r;
    r.what = What::SetLanguage;
    r.language = static_cast<hmi::Lang>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    send(r);
}

void on_file(lv_event_t* e) {
    Request r;
    r.what = What::OpenCard;
    std::snprintf(r.file, sizeof r.file, "%s", static_cast<const char*>(lv_event_get_user_data(e)));
    send(r);
}

void on_light(lv_event_t* e) {
    Request r;
    r.what = What::LightSet;
    r.light = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    send(r);
}

// the light for the picture and the hole in the centre are chosen here; the check itself asks the server
void on_pick_light(lv_event_t* e) {
    g_q_light = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    char line[64];
    std::snprintf(line, sizeof line, "%s: %d %%", hmi::quality_text(hmi::Q::Light, g_q_lang), hmi::kLightLevels[g_q_light]);
    lv_label_set_text(g_q_light_label, line);
}

void on_hole(lv_event_t*) {
    g_q_hole = (g_q_hole + 1) % 5;
    char line[96];
    if (hmi::kHoleWidths[g_q_hole] > 0.0) std::snprintf(line, sizeof line, "%s: %.1f", hmi::quality_text(hmi::Q::HoleWidth, g_q_lang), hmi::kHoleWidths[g_q_hole]);
    else std::snprintf(line, sizeof line, "%s: %s", hmi::quality_text(hmi::Q::HoleWidth, g_q_lang), hmi::quality_text(hmi::Q::HoleNone, g_q_lang));
    lv_label_set_text(g_q_hole_label, line);
}

void on_inspect(lv_event_t*) {
    Request r;
    r.what = What::InspectRun;
    r.light = g_q_light;
    r.hole = g_q_hole;
    send(r);
}

void on_preset(lv_event_t* e) {
    Request r;
    r.what = What::InspectPreset;
    r.preset = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    send(r);
}

void on_calibrate(lv_event_t*) {
    Request r;
    r.what = What::InspectCalibrate;
    send(r);
}

void on_tool(lv_event_t* e) {
    Request r;
    r.what = What::ToggleTool;
    r.tool = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    send(r);
}

void on_resume(lv_event_t*) { Request r; r.what = What::Resume; send(r); }

void on_spindle_over(lv_event_t*) {
    g_over_refuse = !g_over_refuse;
    lv_label_set_text(lv_obj_get_child(g_over_label, 0), g_over_refuse ? "refuse" : "clip");
    Request r;
    r.what = What::SetSpindleOver;
    r.refuse = g_over_refuse;
    send(r);
}

void on_send_job(lv_event_t*) { Request r; r.what = What::SendJob; send(r); }
void on_refresh_card(lv_event_t*) { Request r; r.what = What::RefreshCard; send(r); }

const hmi::Action kJog[6] = {{hmi::Kind::Motion, "jog X -"}, {hmi::Kind::Motion, "jog X +"}, {hmi::Kind::Motion, "jog Y -"},
                             {hmi::Kind::Motion, "jog Y +"}, {hmi::Kind::Motion, "jog Z -"}, {hmi::Kind::Motion, "jog Z +"}};
const hmi::Action kStart{hmi::Kind::Start, "start job"};
const hmi::Action kPause{hmi::Kind::Pause, "pause"};
const hmi::Action kContinue{hmi::Kind::Motion, "resume job"};
const hmi::Action kToolChanged{hmi::Kind::Motion, "tool changed"};
const hmi::Action kHome{hmi::Kind::Motion, "home"};
hmi::Action g_spindle_action{hmi::Kind::Spindle, "spindle on", 0};  // the turns come from the setting `spindle_rpm`; the gate cuts them to what the spindle gives

void remember_move(lv_obj_t* b) {
    if (g_move_count < 10) g_move_buttons[g_move_count++] = b;
}

}  // namespace

void build(const hmi::Settings& s, const hmi::Layout& layout, void (*post)(const Request&)) {
    using hmi::T;
    g_post = post;
    g_layout = layout;
    g_move_count = 0;
    g_spindle_action.rpm = static_cast<int>(s.spindle_rpm);
    g_over_refuse = s.spindle_over == hmi::SpindleOver::Refuse;
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, kBg, 0);
    lv_obj_set_style_text_color(scr, kText, 0);
    lv_obj_set_style_text_font(scr, &eh3d_font_16, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 8, 0);

    // the top bar: the title, the five windows as chips, and the state of the link
    lv_obj_t* top = lv_obj_create(scr);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, LV_PCT(100), layout.top_height);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* title = lv_label_create(top);
    lv_label_set_text(title, "EH3D");
    lv_obj_set_style_text_color(title, kAmber, 0);
    const T names[5] = {T::Machine, T::Job, T::Viewer, T::Fleet, T::Settings};
    g_q_lang = s.language;
    const bool with_quality = hmi::controls_for(s.machine).quality_check;
    g_pages_n = with_quality ? kPages : kPages - 1;
    for (int i = 0; i < g_pages_n; ++i) {
        g_tabs[i] = lv_button_create(top);
        lv_obj_set_style_bg_opa(g_tabs[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_tabs[i], 2, 0);
        lv_obj_set_style_border_color(g_tabs[i], i == 0 ? kAmber : kDim, 0);
        lv_obj_set_style_radius(g_tabs[i], 9, 0);
        lv_obj_add_event_cb(g_tabs[i], on_tab, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_t* l = lv_label_create(g_tabs[i]);
        lv_label_set_text(l, i < 5 ? hmi::text(names[i], s.language) : hmi::quality_text(hmi::Q::Title, s.language));
    }
    g_armed = lv_label_create(top);
    g_banner = lv_label_create(top);
    lv_obj_set_style_text_color(g_banner, LV_COLOR_MAKE(0xff, 0x52, 0x52), 0);

    // the pages share the middle of the screen; lying down the machine page is three columns, standing up one column
    lv_obj_t* body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    for (int i = 0; i < g_pages_n; ++i) {
        g_pages[i] = lv_obj_create(body);
        lv_obj_remove_style_all(g_pages[i]);
        lv_obj_set_size(g_pages[i], LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_flow(g_pages[i], layout.portrait ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
        if (i != 0) lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    const int col_w = layout.portrait ? LV_PCT(100) : hmi::column_width(layout, 12);

    // machine: the job card, the plan view and the jog pad
    lv_obj_t* job = card(g_pages[0], hmi::text(T::Job, s.language));
    lv_obj_set_size(job, col_w, layout.portrait ? 180 : LV_PCT(100));
    lv_obj_t* plan = card(g_pages[0], "2D");
    lv_obj_set_size(plan, col_w, layout.portrait ? 360 : LV_PCT(100));
    lv_obj_t* jog = card(g_pages[0], hmi::text(T::Jog, s.language));
    lv_obj_set_size(jog, col_w, layout.portrait ? 380 : LV_PCT(100));
    lv_obj_set_flex_flow(jog, LV_FLEX_FLOW_ROW_WRAP);
    // the jog pad: the letter of the axis and an arrow, which mean the same in every language
    static const char* const arrows[6] = {"X " LV_SYMBOL_LEFT, "X " LV_SYMBOL_RIGHT, "Y " LV_SYMBOL_DOWN, "Y " LV_SYMBOL_UP, "Z " LV_SYMBOL_DOWN, "Z " LV_SYMBOL_UP};
    for (int i = 0; i < 6; ++i) remember_move(icon_button(jog, arrows[i], on_action, const_cast<hmi::Action*>(&kJog[i]), kSlate, 120, 80));
    remember_move(icon_button(jog, LV_SYMBOL_HOME, on_action, const_cast<hmi::Action*>(&kHome), kSlate, 120, 80));
    // only a machine with a spindle gets the button that starts it: the screen never fires a laser or starts a pick and place head
    if (hmi::controls_for(s.machine).spindle) remember_move(icon_button(jog, LV_SYMBOL_REFRESH, on_action, &g_spindle_action, kSlate, 120, 80));

    // job: the files of the microSD card, and the button that sends the chosen one to the server as a job
    lv_obj_t* files = card(g_pages[1], "microSD");
    lv_obj_set_size(files, layout.portrait ? LV_PCT(100) : LV_PCT(66), LV_PCT(100));
    g_card_list = lv_list_create(files);
    lv_obj_set_size(g_card_list, LV_PCT(100), LV_PCT(80));
    lv_obj_t* side = card(g_pages[1], hmi::text(T::Job, s.language));
    lv_obj_set_size(side, layout.portrait ? LV_PCT(100) : LV_PCT(32), LV_PCT(100));
    icon_button(side, LV_SYMBOL_SD_CARD " " LV_SYMBOL_REFRESH, on_refresh_card, nullptr, kSlate, 180, 64);
    remember_move(icon_button(side, LV_SYMBOL_UPLOAD, on_send_job, nullptr, kGo, 180, 64));
    // what the chosen file holds: the summary, and a chip for each tool that switches it off or on
    g_job_summary = lv_label_create(side);
    lv_obj_set_width(g_job_summary, LV_PCT(100));
    lv_label_set_long_mode(g_job_summary, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_job_summary, "");
    g_tools_box = lv_obj_create(side);
    lv_obj_remove_style_all(g_tools_box);
    lv_obj_set_size(g_tools_box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(g_tools_box, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(g_tools_box, 6, 0);
    // where the job is: what a stopped job did, the button that goes on from there, and the program that comes next
    g_flow_label = lv_label_create(side);
    lv_obj_set_width(g_flow_label, LV_PCT(100));
    lv_label_set_long_mode(g_flow_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_flow_label, "");
    {
        char label[64];
        std::snprintf(label, sizeof label, "%s  %s", LV_SYMBOL_PLAY, hmi::text(T::Resume, s.language));
        g_resume_button = button(side, label, on_resume, nullptr, kGo, 220, 64);
        lv_obj_add_flag(g_resume_button, LV_OBJ_FLAG_HIDDEN);
        // a held job goes on, and a job at a tool change is answered, from the buttons that show only then
        g_continue_button = button(side, label, on_action, const_cast<hmi::Action*>(&kContinue), kGo, 220, 64);
        lv_obj_add_flag(g_continue_button, LV_OBJ_FLAG_HIDDEN);
        std::snprintf(label, sizeof label, "%s  %s", LV_SYMBOL_OK, hmi::text(T::ToolChanged, s.language));
        g_toolchange_button = button(side, label, on_action, const_cast<hmi::Action*>(&kToolChanged), kGo, 300, 64);
        lv_obj_add_flag(g_toolchange_button, LV_OBJ_FLAG_HIDDEN);
    }

    // viewer: the plan of the chosen job
    lv_obj_t* view = card(g_pages[2], hmi::text(T::Viewer, s.language));
    lv_obj_set_size(view, LV_PCT(100), LV_PCT(100));

    // machines and stations: one row per machine of the server
    lv_obj_t* fleet = card(g_pages[3], hmi::text(T::Fleet, s.language));
    lv_obj_set_size(fleet, LV_PCT(100), LV_PCT(100));
    g_fleet_list = lv_list_create(fleet);
    lv_obj_set_size(g_fleet_list, LV_PCT(100), LV_PCT(90));

    // settings: the orientation of the screen
    lv_obj_t* set = card(g_pages[4], hmi::text(T::Settings, s.language));
    lv_obj_set_size(set, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(set, LV_FLEX_FLOW_ROW_WRAP);
    const char* o_names[3] = {"auto", "landscape", "portrait"};
    for (int i = 0; i < 3; ++i) {
        button(set, o_names[i], on_orientation, reinterpret_cast<void*>(static_cast<intptr_t>(i)), kSlate, 220, 80);
    }

    // the spindle: an order for more turns than the spindle gives is cut to its top speed (clip) or refused (refuse); kept until the next start, hmi.cfg holds the lasting choice
    if (hmi::controls_for(s.machine).spindle) {
        lv_obj_t* box = card(set, hmi::text(T::Spindle, s.language));
        lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
        g_over_label = button(box, g_over_refuse ? "refuse" : "clip", on_spindle_over, nullptr, kSlate, 220, 64);
    }

    // the language: the thirty-seven languages, each named in itself, in a list that scrolls; the one in use is outlined
    lv_obj_t* langs = card(set, hmi::text(T::Language, s.language));
    lv_obj_set_size(langs, LV_PCT(100), 300);
    lv_obj_set_flex_flow(langs, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scroll_dir(langs, LV_DIR_VER);
    for (std::size_t i = 0; i < hmi::kLangCount; ++i) {
        lv_obj_t* b = button(langs, hmi::lang_name(static_cast<hmi::Lang>(i)), on_language, reinterpret_cast<void*>(static_cast<intptr_t>(i)), kSlate, 220, 64);
        if (static_cast<hmi::Lang>(i) == s.language) {
            lv_obj_set_style_border_color(b, kAmber, 0);
            lv_obj_set_style_border_width(b, 3, 0);
        }
    }

    // quality check: the verdict, big enough to read from across the room, what was found, and the controls of the ring of LEDs and of the check
    if (with_quality) {
        using hmi::Q;
        const hmi::Lang lang = s.language;
        lv_obj_t* verdict = card(g_pages[5], hmi::quality_text(Q::Title, lang));
        lv_obj_set_size(verdict, layout.portrait ? LV_PCT(100) : LV_PCT(55), LV_PCT(100));
        g_q_badge = lv_obj_create(verdict);
        lv_obj_set_size(g_q_badge, LV_PCT(100), 150);
        lv_obj_set_style_radius(g_q_badge, 16, 0);
        lv_obj_set_style_bg_color(g_q_badge, kSlate, 0);
        lv_obj_set_style_border_width(g_q_badge, 0, 0);
        g_q_badge_text = lv_label_create(g_q_badge);
        lv_obj_set_style_text_font(g_q_badge_text, &eh3d_font_24, 0);
        lv_obj_set_style_text_color(g_q_badge_text, lv_color_white(), 0);
        lv_label_set_text(g_q_badge_text, hmi::quality_text(Q::NotChecked, lang));
        lv_obj_center(g_q_badge_text);
        g_q_detail = lv_label_create(verdict);
        lv_label_set_long_mode(g_q_detail, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(g_q_detail, LV_PCT(100));
        lv_label_set_text(g_q_detail, "");

        lv_obj_t* controls = card(g_pages[5], hmi::quality_text(Q::Light, lang));
        lv_obj_set_size(controls, layout.portrait ? LV_PCT(100) : LV_PCT(43), LV_PCT(100));
        lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW_WRAP);
        // the ring: off and three levels (the node keeps its own ceiling)
        for (int i = 0; i < 4; ++i) {
            char label[24];
            if (hmi::kLightLevels[i] == 0) std::snprintf(label, sizeof label, "%s", hmi::quality_text(Q::LightOff, lang));
            else std::snprintf(label, sizeof label, "%d %%", hmi::kLightLevels[i]);
            button(controls, label, on_light, reinterpret_cast<void*>(static_cast<intptr_t>(i)), kSlate, 120, 70);
        }
        // the light the check uses for its picture
        g_q_light_label = lv_label_create(controls);
        lv_obj_set_width(g_q_light_label, LV_PCT(100));
        char line[96];
        std::snprintf(line, sizeof line, "%s: %d %%", hmi::quality_text(Q::Light, lang), hmi::kLightLevels[g_q_light]);
        lv_label_set_text(g_q_light_label, line);
        for (int i = 1; i < 4; ++i) {
            char label[24];
            std::snprintf(label, sizeof label, "%d %%", hmi::kLightLevels[i]);
            button(controls, label, on_pick_light, reinterpret_cast<void*>(static_cast<intptr_t>(i)), kSlate, 120, 56);
        }
        // the hole of the job in the centre of the picture, from the tools of the machine
        g_q_hole_label = button(controls, "", on_hole, nullptr, kSlate, LV_PCT(100), 70);
        lv_obj_t* hole_text = lv_obj_get_child(g_q_hole_label, 0);
        std::snprintf(line, sizeof line, "%s: %.1f", hmi::quality_text(Q::HoleWidth, lang), hmi::kHoleWidths[g_q_hole]);
        lv_label_set_text(hole_text, line);
        g_q_hole_label = hole_text;
        // the profile of the shop: the presets (the tolerances are edited in the desktop program or in Studio)
        static const Q presets[3] = {Q::Standard, Q::FinePitch, Q::Prototype};
        for (int i = 0; i < 3; ++i) button(controls, hmi::quality_text(presets[i], lang), on_preset, reinterpret_cast<void*>(static_cast<intptr_t>(i)), kSlate, 150, 60);
        button(controls, hmi::quality_text(Q::Calibrate, lang), on_calibrate, nullptr, kSlate, LV_PCT(100), 60);
        g_q_run = button(controls, hmi::quality_text(Q::CheckNow, lang), on_inspect, nullptr, kGo, LV_PCT(100), 90);
        lv_obj_set_style_text_font(lv_obj_get_child(g_q_run, 0), &eh3d_font_24, 0);
    }

    g_note = lv_label_create(scr);
    lv_obj_set_style_text_color(g_note, kAmber, 0);
    lv_label_set_text(g_note, "");

    // the bar of the position: position, progress, the round stop button, pause, start and stop
    g_bar = lv_obj_create(scr);
    style_card(g_bar);
    lv_obj_set_size(g_bar, LV_PCT(100), layout.bar_height);
    lv_obj_set_flex_flow(g_bar, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(g_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* pos = lv_label_create(g_bar);
    lv_label_set_text(pos, "X 19.5   Y 77.5   Z -0.4 mm");
    lv_obj_t* stop = icon_button(g_bar, LV_SYMBOL_WARNING, on_stop, nullptr, LV_COLOR_MAKE(0xc6, 0x28, 0x28), 130, 130);
    lv_obj_set_style_radius(stop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(stop, 6, 0);
    lv_obj_set_style_border_color(stop, kAmber, 0);
    icon_button(g_bar, LV_SYMBOL_PAUSE, on_action, const_cast<hmi::Action*>(&kPause), kHold, 150, 80);  // a pause is never dimmed: it only holds the job
    remember_move(icon_button(g_bar, LV_SYMBOL_PLAY, on_action, const_cast<hmi::Action*>(&kStart), kGo, 150, 80));
    icon_button(g_bar, LV_SYMBOL_STOP, on_stop, nullptr, kHalt, 150, 80);
    g_ack = button(g_bar, hmi::text(T::AckRelease, s.language), on_ack, nullptr, LV_COLOR_MAKE(0xf9, 0xa8, 0x25), 260, 80);
    lv_obj_add_flag(g_ack, LV_OBJ_FLAG_HIDDEN);

    // the question of the gate: a move is done only after this
    g_dialog = lv_obj_create(scr);
    style_card(g_dialog);
    lv_obj_add_flag(g_dialog, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(g_dialog, 560, 300);
    lv_obj_center(g_dialog);
    g_dialog_text = lv_label_create(g_dialog);
    lv_obj_align(g_dialog_text, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_t* yes = icon_button(g_dialog, LV_SYMBOL_OK, on_confirm, nullptr, kGo, 220, 90);
    lv_obj_align(yes, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    lv_obj_t* no = icon_button(g_dialog, LV_SYMBOL_CLOSE, on_cancel, nullptr, LV_COLOR_MAKE(0x54, 0x6e, 0x7a), 220, 90);
    lv_obj_align(no, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    lv_obj_add_flag(g_dialog, LV_OBJ_FLAG_HIDDEN);
}

void show(const State& st, const hmi::Settings& s) {
    using hmi::T;
    char line[96];
    std::snprintf(line, sizeof line, "%s   %s", hmi::text(st.armed ? T::Armed : T::NotArmed, s.language), st.server_ok ? "server" : "-");
    lv_label_set_text(g_armed, line);
    lv_label_set_text(g_banner, st.stop_pressed ? hmi::text(T::StopPressed, s.language) : "");
    if (st.stop_latched && !st.stop_pressed) lv_obj_remove_flag(g_ack, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_ack, LV_OBJ_FLAG_HIDDEN);
    // while the stop is pressed or not acknowledged the buttons that move are dimmed (the gate refuses them anyway)
    const bool blocked = st.stop_latched || st.stop_pressed;
    for (int i = 0; i < g_move_count; ++i) {
        if (blocked) lv_obj_add_state(g_move_buttons[i], LV_STATE_DISABLED); else lv_obj_remove_state(g_move_buttons[i], LV_STATE_DISABLED);
    }
    if (!st.note.empty()) lv_label_set_text(g_note, st.note.c_str());
}

void ask(const hmi::Action* action, const hmi::Settings& s) {
    using hmi::T;
    if (action == nullptr) {
        lv_obj_add_flag(g_dialog, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    static char text[96];
    lv_snprintf(text, sizeof text, "%s\n%s", hmi::text(action->kind == hmi::Kind::Spindle ? T::AskSpindle : T::AskMove, s.language), action->what.c_str());
    lv_label_set_text(g_dialog_text, text);
    lv_obj_remove_flag(g_dialog, LV_OBJ_FLAG_HIDDEN);
}

void refused(hmi::Why why, const hmi::Settings& s) { lv_label_set_text(g_note, hmi::why_text(why, s.language)); }

void show_quality(const hmi::QualityView& q, const hmi::LightView& light, bool running, const hmi::Settings& s) {
    using hmi::Q;
    if (g_q_badge == nullptr || g_pages_n < kPages) return;  // this machine has no visual check
    const char* word = hmi::quality_text(Q::NotChecked, s.language);
    lv_color_t colour = kSlate;
    if (running) {
        word = hmi::quality_text(Q::Checking, s.language);
    } else if (q.verdict == hmi::QualityVerdict::Ok) {
        word = hmi::quality_text(Q::VerdictOk, s.language);
        colour = kGo;
    } else if (q.verdict == hmi::QualityVerdict::NotOk) {
        word = hmi::quality_text(Q::VerdictNotOk, s.language);
        colour = kHalt;
    } else if (q.verdict == hmi::QualityVerdict::Unknown) {
        word = hmi::quality_text(Q::VerdictUnknown, s.language);
        colour = LV_COLOR_MAKE(0xe8, 0xa4, 0x20);
    }
    lv_obj_set_style_bg_color(g_q_badge, colour, 0);
    lv_label_set_text(g_q_badge_text, word);
    lv_obj_center(g_q_badge_text);
    // what was found: the counts, the width of the channels, and a line for each finding (the first six)
    std::string detail;
    if (!running && q.verdict != hmi::QualityVerdict::None) {
        char line[96];
        if (q.holes_total > 0) {
            std::snprintf(line, sizeof line, "%d/%d %s\n", q.holes_ok, q.holes_total, hmi::quality_text(Q::HolesOk, s.language));
            detail += line;
        }
        if (q.channel_mm > 0.0) {
            std::snprintf(line, sizeof line, "%s %.2f mm\n", hmi::quality_text(Q::ChannelWidth, s.language), q.channel_mm);
            detail += line;
        }
        int shown = 0;
        for (const auto& f : q.findings) {
            if (shown++ >= 6) break;
            detail += hmi::finding_line(f, s.language) + "\n";
        }
    }
    if (light.known) {
        char line[64];
        std::snprintf(line, sizeof line, "%s: %s", hmi::quality_text(Q::Light, s.language), light.on ? (std::to_string(light.level) + " %").c_str() : hmi::quality_text(Q::LightOff, s.language));
        detail += line;
    }
    lv_label_set_text(g_q_detail, detail.c_str());
    if (running) lv_obj_add_state(g_q_run, LV_STATE_DISABLED); else lv_obj_remove_state(g_q_run, LV_STATE_DISABLED);
}

void show_fleet(const hmi::FleetView& fleet, const hmi::Settings&) {
    lv_obj_clean(g_fleet_list);
    for (const auto& m : fleet.machines) {
        char row[160];
        if (m.percent >= 0) std::snprintf(row, sizeof row, "%s   %s   %s %d%%", m.name.c_str(), m.state.c_str(), m.job_name.c_str(), m.percent);
        else std::snprintf(row, sizeof row, "%s   %s   (%d)", m.name.c_str(), m.state.c_str(), m.ready);
        lv_list_add_button(g_fleet_list, nullptr, row);
    }
}

void show_job(const hmi::ToolSwitch& tools, const hmi::SpindleView& spindle, const hmi::Settings& s) {
    using hmi::T;
    if (g_tools_box == nullptr || g_job_summary == nullptr) return;
    lv_obj_clean(g_tools_box);
    const hmi::FileSummary& file = tools.summary();
    std::string text;
    if (!file.tools.empty()) {
        // the chips: the number, the diameter when the file says it and how many holes or cuts; a tool that is off is dim, with a cross
        for (const hmi::ToolInfo& t : file.tools) {
            char label[64];
            const bool off = tools.is_off(t.number);
            if (t.diameter_mm > 0) std::snprintf(label, sizeof label, "%s %d  %.2f mm  (%d)", off ? LV_SYMBOL_CLOSE : LV_SYMBOL_OK, t.number, t.diameter_mm, t.count);
            else std::snprintf(label, sizeof label, "%s %s %d  (%d)", off ? LV_SYMBOL_CLOSE : LV_SYMBOL_OK, hmi::text(T::Tool, s.language), t.number, t.count);
            lv_obj_t* b = button(g_tools_box, label, on_tool, reinterpret_cast<void*>(static_cast<intptr_t>(t.number)), off ? kDim : kSlate, 200, 56);
            if (off) lv_obj_set_style_text_opa(lv_obj_get_child(b, 0), LV_OPA_60, 0);
        }
        text = std::to_string(tools.remaining()) + "/" + std::to_string(file.total) + " " + hmi::text(file.route ? T::Cuts : T::Holes, s.language);
        if (tools.nothing_left()) text += std::string("  ") + LV_SYMBOL_WARNING;
    }
    if (spindle.known) {
        char line[96];
        std::snprintf(line, sizeof line, "%s%s: %s %d-%d rpm", text.empty() ? "" : "\n", hmi::text(T::Spindle, s.language), spindle.kind.c_str(), spindle.min_rpm, spindle.max_rpm);
        text += line;
    }
    lv_label_set_text(g_job_summary, text.c_str());
}

void show_flow(const FlowState& flow, const hmi::Settings& s) {
    using hmi::T;
    if (g_flow_label == nullptr || g_resume_button == nullptr) return;
    const char* what = hmi::text(flow.route ? T::Cuts : T::Holes, s.language);
    std::string text;
    char line[160];
    if (flow.stopped) {
        std::snprintf(line, sizeof line, "%s: %d/%d %s", hmi::text(T::Stopped, s.language), flow.skipped + flow.done, flow.skipped + flow.total, what);
        text = line;
        if (!flow.message.empty()) text += "\n" + flow.message;
    } else if (flow.skipped > 0) {
        std::snprintf(line, sizeof line, "%d %s", flow.skipped, what);
        text = line;
    }
    if (!flow.resumed_from.empty() && !flow.stopped) text += std::string(text.empty() ? "" : "\n") + hmi::text(T::ResumedFrom, s.language) + ": " + flow.resumed_from;
    if (!flow.next.empty()) text += std::string(text.empty() ? "" : "\n") + hmi::text(T::Next, s.language) + ": " + flow.next;
    lv_label_set_text(g_flow_label, text.c_str());
    if (flow.can_resume) lv_obj_remove_flag(g_resume_button, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_resume_button, LV_OBJ_FLAG_HIDDEN);
    if (g_continue_button != nullptr) {
        if (flow.held) lv_obj_remove_flag(g_continue_button, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_continue_button, LV_OBJ_FLAG_HIDDEN);
        if (flow.tool_wait) lv_obj_remove_flag(g_toolchange_button, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(g_toolchange_button, LV_OBJ_FLAG_HIDDEN);
    }
}

void show_card(const std::vector<hmi::Entry>& files, const hmi::Settings&) {
    lv_obj_clean(g_card_list);
    static char names[64][64];
    int n = 0;
    for (const auto& e : files) {
        if (e.folder || !hmi::is_job_file(e.name) || n >= 64) continue;
        std::snprintf(names[n], sizeof names[n], "%s", e.name.c_str());
        lv_obj_t* b = lv_list_add_button(g_card_list, nullptr, (e.name + "   " + hmi::size_text(e.size)).c_str());
        lv_obj_add_event_cb(b, on_file, LV_EVENT_CLICKED, names[n]);
        ++n;
    }
}

}  // namespace ui
