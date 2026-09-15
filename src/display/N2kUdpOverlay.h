#pragma once
#include <lvgl.h>

// ============================================================
// N2kUdpOverlay - settings sub-page for the Actisense/UDP NMEA 2000 source.
//
// Opened from the "NMEA/UDP" button next to Licences on the main settings
// screen (ConfigOverlay). Same lifecycle as ConfigOverlay/LicenseOverlay:
// built on lv_layer_top() in open(), fully torn down in close().
// ============================================================
class N2kUdpOverlay {
public:
    void open();
    void close();
    bool isOpen() const { return _open; }

private:
    bool      _open = false;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_kb   = nullptr;
    lv_obj_t *_swEnabled  = nullptr;
    lv_obj_t *_taRemoteIp = nullptr;
    lv_obj_t *_taLocalPort  = nullptr;
    lv_obj_t *_taRemotePort = nullptr;
    lv_obj_t *_status = nullptr;

    void showKeyboard(lv_obj_t *ta, bool numeric);
    void hideKeyboard();

    static void cbClose(lv_event_t *e);
    static void cbSave(lv_event_t *e);
    static void cbTaClickedNumeric(lv_event_t *e);
    static void cbKbEvent(lv_event_t *e);
};

extern N2kUdpOverlay n2kUdpOverlay;
