#include "N2kUdpSource.h"
#include "N2kHandler.h"
#include "../config/Config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ActisenseReader.h>
#include <N2kMsg.h>

N2kUdpSource n2kUdpSource;

namespace {

// Ring buffer feeding tActisenseReader, which wants a Stream (N2kStream ==
// Stream on Arduino, see NMEA2000-library/src/N2kStream.h). UDP payloads are
// appended here as they arrive; the reader drains it byte by byte in loop().
class UdpActisenseStream final : public Stream {
public:
    size_t write(uint8_t) override { return 0; }
    size_t write(const uint8_t *, size_t) override { return 0; }

    int available() override { return (int)_count; }

    int read() override {
        if (_count == 0) return -1;
        const uint8_t v = _buf[_head];
        _head = (_head + 1) % sizeof(_buf);
        --_count;
        return v;
    }

    int peek() override { return _count == 0 ? -1 : _buf[_head]; }
    void flush() override {}

    // Returns how many bytes were actually accepted (buffer may be full -
    // that only happens if loop() has fallen far behind ParseMessages()).
    size_t append(const uint8_t *data, size_t len) {
        size_t n = 0;
        while (n < len && _count < sizeof(_buf)) {
            _buf[_tail] = data[n++];
            _tail = (_tail + 1) % sizeof(_buf);
            ++_count;
        }
        return n;
    }

private:
    uint8_t _buf[4096] = {};
    size_t  _head = 0, _tail = 0, _count = 0;
};

WiFiUDP             udp;
UdpActisenseStream  stream;
tActisenseReader    reader;
IPAddress           allowedRemoteIp;
bool                filterRemoteIp = false;

void onActisenseMessage(const tN2kMsg &msg) {
    N2kHandler::dispatch(msg);
}

// Non-blocking: pulls every datagram already sitting in the socket buffer.
void receiveDatagrams() {
    int packetSize;
    while ((packetSize = udp.parsePacket()) > 0) {
        const bool sourceOk =
            !filterRemoteIp || udp.remoteIP() == allowedRemoteIp;
        const bool portOk =
            appConfig.cfg.n2kUdpRemotePort == 0 ||
            udp.remotePort() == appConfig.cfg.n2kUdpRemotePort;

        if (!sourceOk || !portOk) {
            while (udp.available() > 0) udp.read();   // discard, keep listening
            continue;
        }

        uint8_t packet[1500];
        int remaining = packetSize;
        while (remaining > 0) {
            const int chunk = remaining > (int)sizeof(packet) ? (int)sizeof(packet) : remaining;
            const int got = udp.read(packet, chunk);
            if (got <= 0) break;
            stream.append(packet, (size_t)got);   // silently drops if buffer is full
            remaining -= got;
        }
    }
}

} // namespace

bool N2kUdpSource::begin() {
    _started = false;

    if (appConfig.cfg.n2kUdpLocalPort == 0) {
        Serial.println("[n2k-udp] local port is 0 - not starting");
        return false;
    }

    filterRemoteIp = appConfig.cfg.n2kUdpRemoteIp[0] != '\0';
    if (filterRemoteIp && !allowedRemoteIp.fromString(appConfig.cfg.n2kUdpRemoteIp)) {
        Serial.printf("[n2k-udp] invalid remote IP '%s' - not starting\n",
                      appConfig.cfg.n2kUdpRemoteIp);
        return false;
    }

    if (!udp.begin(appConfig.cfg.n2kUdpLocalPort)) {
        Serial.printf("[n2k-udp] could not bind local port %u\n",
                      (unsigned)appConfig.cfg.n2kUdpLocalPort);
        return false;
    }

    reader.SetReadStream(&stream);
    reader.SetMsgHandler(onActisenseMessage);
    _started = true;

    if (appConfig.cfg.n2kUdpRemotePort) {
        Serial.printf("[n2k-udp] listening on :%u, source %s:%u\n",
                      (unsigned)appConfig.cfg.n2kUdpLocalPort,
                      filterRemoteIp ? appConfig.cfg.n2kUdpRemoteIp : "any",
                      (unsigned)appConfig.cfg.n2kUdpRemotePort);
    } else {
        Serial.printf("[n2k-udp] listening on :%u, source %s:any port\n",
                      (unsigned)appConfig.cfg.n2kUdpLocalPort,
                      filterRemoteIp ? appConfig.cfg.n2kUdpRemoteIp : "any");
    }
    return true;
}

void N2kUdpSource::loop() {
    if (!_started) return;
    receiveDatagrams();
    reader.ParseMessages();
}
