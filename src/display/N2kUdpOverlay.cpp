#include "N2kUdpOverlay.h"
#if defined(BOARD_PANEL_1024X600)
#include "../BoardConfig.h"
#endif
#include "Theme.h"
#include "DisplayManager.h"
#include "../i18n/I18n.h"
#include "../config/Config.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

N2kUdpOverlay n2kUdpOverlay;

namespace {

// Manual dotted-quad check - kept free of WiFi.h/IPAddress so this file
// compiles unchanged in the PC simulator (env:simulator has no ESP32 WiFi
// stack; ConfigOverlay.cpp/LicenseOverlay.cpp use the same trick with #ifndef
// SIMULATOR around actual radio calls, but here there is no radio call at all
// to guard - only the syntax of the address needs checking).
bool looksLikeIPv4(const char *s) {
    if (!s || !*s) return false;
    int dots = 0, digits = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '.') {
            if (digits == 0 || digits > 3) return false;
            dots++; digits = 0;
        } else if (*p >= '0' && *p <= '9') {
            digits++;
        } else {
            return false;
        }
    }
    return dots == 3 && digits >= 1 && digits <= 3;
}

lv_obj_t *mkLabel(lv_obj_t *parent, const char *txt, int x, int y,
                   const lv_font_t *font, lv_color_t col) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, col, 0);
    return l;
}

lv_obj_t *mkButton(lv_obj_t *parent, const char *txt, int x, int y, int w, int h,
                    lv_color_t bg, lv_color_t fg, lv_event_cb_t cb) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, bg, 0);
    lv_obj_set_style_bg_color(b, CLR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, fg, 0);
    lv_obj_set_style_text_font(l, FONT_MED, 0);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    return b;
}

lv_obj_t *mkField(lv_obj_t *parent, const char *value, const char *placeholder,
                   int x, int y, int w, int h, lv_event_cb_t cb) {
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, value ? value : "");
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_obj_set_pos(ta, x, y);
    lv_obj_set_size(ta, w, h);
    lv_obj_set_style_text_font(ta, FONT_MED, 0);
    lv_obj_set_style_bg_color(ta, CLR_SURFACE, 0);
    lv_obj_set_style_text_color(ta, CLR_TEXT, 0);
    lv_obj_set_style_border_color(ta, CLR_BORDER, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 6, 0);
    lv_obj_set_style_pad_top(ta, 6, 0);
    lv_obj_set_style_pad_bottom(ta, 6, 0);
    if (cb) lv_obj_add_event_cb(ta, cb, LV_EVENT_CLICKED, nullptr);
    return ta;
}

lv_obj_t *mkSwitch(lv_obj_t *parent, int x, int y, bool on, lv_event_cb_t cb) {
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 46, 24);
    lv_obj_set_pos(sw, x, y);
    lv_obj_set_style_bg_color(sw, CLR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_color(sw, CLR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, CLR_GREEN, LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    if (cb) lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, nullptr);
    return sw;
}

} // namespace

void N2kUdpOverlay::open() {
    if (_open) return;
    _open = true;

#if defined(BOARD_PANEL_1024X600)
    const int W = uiScreenW(), H = uiScreenH();
#else
    const int W = SCREEN_W, H = SCREEN_H;
#endif
    const int CX = 20;
    const int FIELD_W = (W - CX * 2 - 140 > 360) ? 360 : (W - CX * 2 - 140);

    _root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_root, W, H);
    lv_obj_set_pos(_root, 0, 0);
    lv_obj_set_style_bg_color(_root, CLR_BG, 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_radius(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);   // modal: absorb touches

    mkLabel(_root, T(STR_CFG_UDP_TITLE), CX, 12, FONT_LARGE, CLR_TEXT);
    mkButton(_root, LV_SYMBOL_CLOSE, W - 12 - 44, 8, 44, 36, CLR_SURFACE, CLR_TEXT, cbClose);

    mkLabel(_root, T(STR_CFG_UDP_ENABLE), CX, 64, FONT_MED, CLR_ACCENT);
    _swEnabled = mkSwitch(_root, CX + 300, 60, appConfig.cfg.n2kUdpEnabled, nullptr);

    mkLabel(_root, T(STR_CFG_UDP_REMOTE_IP), CX, 106, FONT_SMALL, CLR_TEXT_DIM);
    _taRemoteIp = mkField(_root, appConfig.cfg.n2kUdpRemoteIp, "192.168.1.140",
                           CX + 140, 96, FIELD_W, 38, cbTaClickedNumeric);

    char portBuf[8];
    mkLabel(_root, T(STR_CFG_UDP_LOCAL_PORT), CX, 152, FONT_SMALL, CLR_TEXT_DIM);
    snprintf(portBuf, sizeof(portBuf), "%u", (unsigned)appConfig.cfg.n2kUdpLocalPort);
    _taLocalPort = mkField(_root, portBuf, "10120",
                            CX + 140, 142, 140, 38, cbTaClickedNumeric);

    mkLabel(_root, T(STR_CFG_UDP_REMOTE_PORT), CX, 198, FONT_SMALL, CLR_TEXT_DIM);
    snprintf(portBuf, sizeof(portBuf), "%u", (unsigned)appConfig.cfg.n2kUdpRemotePort);
    _taRemotePort = mkField(_root, appConfig.cfg.n2kUdpRemotePort ? portBuf : "",
                             "any", CX + 140, 188, 140, 38, cbTaClickedNumeric);

    _status = mkLabel(_root, "", CX, 240, FONT_SMALL, CLR_RED);

    mkButton(_root, T(STR_CFG_UDP_SAVE_REBOOT), CX, 280, 300, 42,
             CLR_ACCENT, CLR_ON_ACCENT, cbSave);
}

void N2kUdpOverlay::close() {
    if (!_open) return;
    _open = false;
    if (_root) lv_obj_del_async(_root);
    _root = _kb = _swEnabled = _taRemoteIp = _taLocalPort = _taRemotePort = _status = nullptr;
}

void N2kUdpOverlay::showKeyboard(lv_obj_t *ta, bool numeric) {
    if (!_root) return;
    if (!_kb) {
        _kb = lv_keyboard_create(_root);
#if defined(BOARD_PANEL_1024X600)
        lv_obj_set_size(_kb, uiScreenW(), 280);
#else
        lv_obj_set_size(_kb, SCREEN_W, 200);
#endif
        lv_obj_align(_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(_kb, cbKbEvent, LV_EVENT_READY, nullptr);
        lv_obj_add_event_cb(_kb, cbKbEvent, LV_EVENT_CANCEL, nullptr);
    }
    lv_keyboard_set_mode(_kb, numeric ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(_kb, ta);
    lv_obj_clear_flag(_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_kb);
}

void N2kUdpOverlay::hideKeyboard() {
    if (_kb) {
        lv_keyboard_set_textarea(_kb, nullptr);
        lv_obj_add_flag(_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

void N2kUdpOverlay::cbClose(lv_event_t *e)  { n2kUdpOverlay.close(); }
void N2kUdpOverlay::cbKbEvent(lv_event_t *) { n2kUdpOverlay.hideKeyboard(); }

void N2kUdpOverlay::cbTaClickedNumeric(lv_event_t *e) {
    n2kUdpOverlay.showKeyboard(lv_event_get_target(e), true);
}

void N2kUdpOverlay::cbSave(lv_event_t *e) {
    N2kUdpOverlay &s = n2kUdpOverlay;
    const char *ip = lv_textarea_get_text(s._taRemoteIp);
    if (ip[0] && !looksLikeIPv4(ip)) {
        lv_label_set_text(s._status, T(STR_CFG_UDP_BAD_IP));
        return;
    }
    const uint16_t localPort  = (uint16_t)strtoul(lv_textarea_get_text(s._taLocalPort), nullptr, 10);
    const uint16_t remotePortText = lv_textarea_get_text(s._taRemotePort)[0]
        ? (uint16_t)strtoul(lv_textarea_get_text(s._taRemotePort), nullptr, 10) : 0;

    strlcpy(appConfig.cfg.n2kUdpRemoteIp, ip, sizeof(appConfig.cfg.n2kUdpRemoteIp));
    appConfig.cfg.n2kUdpLocalPort  = localPort ? localPort : 10120;
    appConfig.cfg.n2kUdpRemotePort = remotePortText;
    appConfig.cfg.n2kUdpEnabled    = lv_obj_has_state(s._swEnabled, LV_STATE_CHECKED);
    appConfig.save();

#ifndef SIMULATOR
    s.close();
    dispMgr.requestReboot(T(STR_CFG_RB_UDP));
#else
    Serial.printf("[sim] UDP cfg saved: enabled=%d ip='%s' local=%u remote=%u -> reboot (skipped)\n",
                  (int)appConfig.cfg.n2kUdpEnabled, appConfig.cfg.n2kUdpRemoteIp,
                  (unsigned)appConfig.cfg.n2kUdpLocalPort, (unsigned)appConfig.cfg.n2kUdpRemotePort);
    s.close();
#endif
}
