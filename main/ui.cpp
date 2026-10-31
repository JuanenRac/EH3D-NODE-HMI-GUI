// EH3D-NODE-HMI-GUI - main/ui.cpp: the screens of the touch display, in the look of the desktop program (Forge theme: dark, amber,
// cards with a thin amber outline) and the same order: machine, job, viewer, machines and stations, settings. Lying down the machine screen
// has three columns above the bar of the position; standing up the columns stack. The stop button is always on the bar and always works.
// This has not been run on a module yet. GPL-3.0-or-later.
#include "ui.hpp"

#include <cstdio>

#include "lvgl.h"

#include "../core/ui_text.hpp"

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
lv_obj_t* g_pages[5] = {nullptr};
lv_obj_t* g_tabs[5] = {nullptr};
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
lv_obj_t* g_move_buttons[10] = {nullptr};
int g_move_count = 0;

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
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_24, 0);
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
    for (int i = 0; i < 5; ++i) {
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

void on_file(lv_event_t* e) {
    Request r;
    r.what = What::OpenCard;
    std::snprintf(r.file, sizeof r.file, "%s", static_cast<const char*>(lv_event_get_user_data(e)));
    send(r);
}

void on_send_job(lv_event_t*) { Request r; r.what = What::SendJob; send(r); }
void on_refresh_card(lv_event_t*) { Request r; r.what = What::RefreshCard; send(r); }

const hmi::Action kJog[6] = {{hmi::Kind::Motion, "jog X -"}, {hmi::Kind::Motion, "jog X +"}, {hmi::Kind::Motion, "jog Y -"},
                             {hmi::Kind::Motion, "jog Y +"}, {hmi::Kind::Motion, "jog Z -"}, {hmi::Kind::Motion, "jog Z +"}};
const hmi::Action kStart{hmi::Kind::Motion, "start job"};
const hmi::Action kPause{hmi::Kind::Stop, "pause"};
const hmi::Action kHome{hmi::Kind::Motion, "home"};
const hmi::Action kSpindle{hmi::Kind::Spindle, "spindle on"};

void remember_move(lv_obj_t* b) {
    if (g_move_count < 10) g_move_buttons[g_move_count++] = b;
}

}  // namespace

void build(const hmi::Settings& s, const hmi::Layout& layout, void (*post)(const Request&)) {
    using hmi::T;
    g_post = post;
    g_layout = layout;
    g_move_count = 0;
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, kBg, 0);
    lv_obj_set_style_text_color(scr, kText, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 8, 0);

    // the top bar: the title, the five windows as chips, and the state of the link
    lv_obj_t* top = lv_obj_create(scr);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, LV_PCT(100), layout.top_height);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* title = lv_label_create(top);
    lv_label_set_text(title, "EH3D  BUNGARD");
    lv_obj_set_style_text_color(title, kAmber, 0);
    const T names[5] = {T::Machine, T::Job, T::Viewer, T::Fleet, T::Settings};
    for (int i = 0; i < 5; ++i) {
        g_tabs[i] = lv_button_create(top);
        lv_obj_set_style_bg_opa(g_tabs[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_tabs[i], 2, 0);
        lv_obj_set_style_border_color(g_tabs[i], i == 0 ? kAmber : kDim, 0);
        lv_obj_set_style_radius(g_tabs[i], 9, 0);
        lv_obj_add_event_cb(g_tabs[i], on_tab, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_t* l = lv_label_create(g_tabs[i]);
        lv_label_set_text(l, hmi::text(names[i], s.language));
    }
    g_armed = lv_label_create(top);
    g_banner = lv_label_create(top);
    lv_obj_set_style_text_color(g_banner, LV_COLOR_MAKE(0xff, 0x52, 0x52), 0);

    // the pages share the middle of the screen; lying down the machine page is three columns, standing up one column
    lv_obj_t* body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    for (int i = 0; i < 5; ++i) {
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
    if (hmi::controls_for(s.machine).spindle) remember_move(icon_button(jog, LV_SYMBOL_REFRESH, on_action, const_cast<hmi::Action*>(&kSpindle), kSlate, 120, 80));

    // job: the files of the microSD card, and the button that sends the chosen one to the server as a job
    lv_obj_t* files = card(g_pages[1], "microSD");
    lv_obj_set_size(files, layout.portrait ? LV_PCT(100) : LV_PCT(66), LV_PCT(100));
    g_card_list = lv_list_create(files);
    lv_obj_set_size(g_card_list, LV_PCT(100), LV_PCT(80));
    lv_obj_t* side = card(g_pages[1], hmi::text(T::Job, s.language));
    lv_obj_set_size(side, layout.portrait ? LV_PCT(100) : LV_PCT(32), LV_PCT(100));
    icon_button(side, LV_SYMBOL_SD_CARD " " LV_SYMBOL_REFRESH, on_refresh_card, nullptr, kSlate, 180, 64);
    remember_move(icon_button(side, LV_SYMBOL_UPLOAD, on_send_job, nullptr, kGo, 180, 64));

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
    remember_move(icon_button(g_bar, LV_SYMBOL_PAUSE, on_action, const_cast<hmi::Action*>(&kPause), kHold, 150, 80));
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

void show_fleet(const hmi::FleetView& fleet, const hmi::Settings&) {
    lv_obj_clean(g_fleet_list);
    for (const auto& m : fleet.machines) {
        char row[160];
        if (m.percent >= 0) std::snprintf(row, sizeof row, "%s   %s   %s %d%%", m.name.c_str(), m.state.c_str(), m.job_name.c_str(), m.percent);
        else std::snprintf(row, sizeof row, "%s   %s   (%d)", m.name.c_str(), m.state.c_str(), m.ready);
        lv_list_add_button(g_fleet_list, nullptr, row);
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
