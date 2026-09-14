#include "udp_nmea2000_config.h"

#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

namespace {
constexpr const char *kNamespace = "udp_n2k";
}

void udp_config_load(UdpNmea2000Config *config) {
  if (config == nullptr) return;

  Serial.println("NVS UDP: load begin");

  memset(config, 0, sizeof(*config));
  config->enabled = N2K_UDP_DEFAULT_ENABLED;
  strncpy(config->wifi_ssid, N2K_UDP_DEFAULT_WIFI_SSID, sizeof(config->wifi_ssid) - 1);
  strncpy(config->wifi_password, N2K_UDP_DEFAULT_WIFI_PASSWORD, sizeof(config->wifi_password) - 1);
  config->local_port = N2K_UDP_DEFAULT_LOCAL_PORT;
  strncpy(config->remote_ip, N2K_UDP_DEFAULT_REMOTE_IP, sizeof(config->remote_ip) - 1);
  config->remote_port = N2K_UDP_DEFAULT_REMOTE_PORT;

  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    Serial.println("NVS UDP: begin failed");
    return;
  }
  Serial.println("NVS UDP: begin ok");
  config->enabled = preferences.getBool("enabled", config->enabled);
  Serial.println("NVS UDP: enabled read ok");
  preferences.getString("ssid", config->wifi_ssid, sizeof(config->wifi_ssid));
  Serial.printf("NVS UDP: ssid read ok len=%u\n", static_cast<unsigned>(strlen(config->wifi_ssid)));
  preferences.getString("password", config->wifi_password, sizeof(config->wifi_password));
  Serial.printf("NVS UDP: password read ok len=%u\n", static_cast<unsigned>(strlen(config->wifi_password)));
  config->local_port = preferences.getUShort("local_port", config->local_port);
  Serial.println("NVS UDP: local_port read ok");
  preferences.getString("remote_ip", config->remote_ip, sizeof(config->remote_ip));
  Serial.printf("NVS UDP: remote_ip read ok len=%u\n", static_cast<unsigned>(strlen(config->remote_ip)));
  config->remote_port = preferences.getUShort("remote_port", config->remote_port);
  Serial.println("NVS UDP: remote_port read ok");
  preferences.end();
  Serial.println("NVS UDP: load end");
}

bool udp_config_save(const UdpNmea2000Config *config) {
  if (config == nullptr) return false;

  Serial.println("NVS UDP: save begin");

  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    Serial.println("NVS UDP: save begin failed");
    return false;
  }
  preferences.putBool("enabled", config->enabled);
  Serial.println("NVS UDP: enabled written");
  preferences.putString("ssid", config->wifi_ssid);
  Serial.println("NVS UDP: ssid written");
  preferences.putString("password", config->wifi_password);
  Serial.println("NVS UDP: password written");
  preferences.putUShort("local_port", config->local_port);
  Serial.println("NVS UDP: local_port written");
  preferences.putString("remote_ip", config->remote_ip);
  Serial.println("NVS UDP: remote_ip written");
  preferences.putUShort("remote_port", config->remote_port);
  Serial.println("NVS UDP: remote_port written");
  preferences.end();
  Serial.println("NVS UDP: save end");
  return true;
}