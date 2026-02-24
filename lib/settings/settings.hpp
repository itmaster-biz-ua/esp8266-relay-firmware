#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <LittleFS.h>
#include <ArduinoJson.h>

struct settings_t
{
    bool use_auth          ;
    bool use_static_ip     ;
    char auth_pass     [16];
    char auth_login    [16];
    char time_zone     [16];
    char wifi_ssid     [33];
    char wifi_password [33];
    char static_ipv4   [16];
    char static_gateway[16];
    char static_subnet [16];
    char static_dns    [16];  
};

int save_settings_to_json_file(settings_t *, fs::FS, const char *);

int load_settings_from_json_file(settings_t *, fs::FS, const char *);

int update_settings_from_json_doc(settings_t *, fs::FS, const char *, JsonVariant *);

#endif // SETTINGS_H_