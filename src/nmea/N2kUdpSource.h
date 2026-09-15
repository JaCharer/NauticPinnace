#pragma once

// ============================================================
// N2kUdpSource – alternate NMEA 2000 source: decode Actisense-format
// messages arriving over WiFi UDP (e.g. a YDWG-02/NGT-1 gateway bridging
// the boat's real CAN bus onto the network) instead of reading the local
// CAN/TWAI controller.
//
// Adapted from a standalone sketch (src/nmea_udp/ in this repo, kept only
// as reference) - here it is wired into the SAME PGN dispatcher used for
// the CAN bus (N2kHandler::handleMsg), reuses the project's own WiFi
// connection instead of opening a second one, and reads its settings from
// appConfig instead of a private NVS namespace.
//
// Mutually exclusive with the CAN bus: N2kHandler::begin()/loop() pick this
// OR NMEA2000.Open()/ParseMessages(), never both (see main.cpp's demoTask
// comment for the same reasoning applied to demo mode).
// ============================================================
class N2kUdpSource {
public:
    // Starts the UDP listener. Call only once WiFi has an IP (STA or AP) -
    // see N2kHandler::begin(). Returns false (and logs why) if the config
    // is incomplete/invalid or the socket could not be bound.
    bool begin();

    // Non-blocking: drains any buffered datagrams into the Actisense reader
    // and dispatches decoded messages via N2kHandler::handleMsg(). Call
    // every N2K task loop iteration instead of NMEA2000.ParseMessages().
    void loop();

    bool isRunning() const { return _started; }

private:
    bool _started = false;
};

extern N2kUdpSource n2kUdpSource;
