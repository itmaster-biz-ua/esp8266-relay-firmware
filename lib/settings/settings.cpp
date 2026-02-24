#include "settings.hpp"

int save_settings_to_json_file(settings_t *settings, fs::FS fs, const char *file)
{
    StaticJsonDocument<512> json_doc;

    File fsettings = fs.open(file, "w");
    if (!fsettings) 
    {
        (void)Serial.printf("Error: Could not open file \"%s\" for writing\n", file);
        return -1;
    }

    json_doc["use_auth"      ] = settings->use_auth      ;
    json_doc["use_static_ip" ] = settings->use_static_ip ;
    
    json_doc["auth_pass"     ] = settings->auth_pass     ;
    json_doc["auth_login"    ] = settings->auth_login    ;
    json_doc["time_zone"     ] = settings->time_zone     ;           
    json_doc["wifi_ssid"     ] = settings->wifi_ssid     ;
    json_doc["wifi_password" ] = settings->wifi_password ;
    json_doc["static_ipv4"   ] = settings->static_ipv4   ;
    json_doc["static_gateway"] = settings->static_gateway;
    json_doc["static_subnet" ] = settings->static_subnet ;
    json_doc["static_dns"    ] = settings->static_dns    ;

    (void)serializeJson(json_doc, fsettings);
    fsettings.close();

    return 0;
} 

int load_settings_from_json_file(settings_t *settings, fs::FS fs, const char *file)
{
    StaticJsonDocument<512> json_doc;

    File fsettings = LittleFS.open(file, "r");
    if (!fsettings) 
    {
        (void)Serial.printf("Error: Could not open file %s for reading\n", file);
        return -1;
    }

    DeserializationError error = deserializeJson(json_doc, fsettings); 
    if (error)
    {
        (void)Serial.printf("Error: %s\n", error.c_str());
        return -1;
    }

    fsettings.close();

    settings->use_auth = json_doc["use_auth"];
    settings->use_static_ip = json_doc["use_static_ip"];
    
    (void)strcpy( settings->auth_pass      , json_doc["auth_pass"     ] );
    (void)strcpy( settings->auth_login     , json_doc["auth_login"    ] );
    (void)strcpy( settings->time_zone      , json_doc["time_zone"     ] );
    (void)strcpy( settings->wifi_ssid      , json_doc["wifi_ssid"     ] );
    (void)strcpy( settings->wifi_password  , json_doc["wifi_password" ] );
    (void)strcpy( settings->static_ipv4    , json_doc["static_ipv4"   ] );
    (void)strcpy( settings->static_gateway , json_doc["static_gateway"] );
    (void)strcpy( settings->static_subnet  , json_doc["static_subnet" ] );
    (void)strcpy( settings->static_dns     , json_doc["static_dns"    ] );

    return 0;
}

int update_settings_from_json_doc(settings_t *settings, fs::FS fs, const char *file, JsonVariant *json)
{
    StaticJsonDocument<512> json_doc;

    if (json->is<JsonArray>())
    {
        json_doc = json->as<JsonArray>();
    }
    else if (json->is<JsonObject>())
    {
        json_doc = json->as<JsonObject>();
    }

    File fsettings = fs.open(file, "w");
    if (!fsettings) 
    {
        (void)Serial.printf("Error: Could not open file %s for writing\n", file);
        return -1;
    }

    settings->use_auth = json_doc["use_schedule"];
    settings->use_static_ip = json_doc["use_static_ip"];
    
    (void)strcpy( settings->auth_pass      , json_doc["auth_pass"     ] );
    (void)strcpy( settings->auth_login     , json_doc["auth_login"    ] );
    (void)strcpy( settings->time_zone      , json_doc["time_zone"     ] );
    (void)strcpy( settings->wifi_ssid      , json_doc["wifi_ssid"     ] );
    (void)strcpy( settings->wifi_password  , json_doc["wifi_password" ] );
    (void)strcpy( settings->static_ipv4    , json_doc["static_ipv4"   ] );
    (void)strcpy( settings->static_gateway , json_doc["static_gateway"] );
    (void)strcpy( settings->static_subnet  , json_doc["static_subnet" ] );
    (void)strcpy( settings->static_dns     , json_doc["static_dns"    ] );

    (void)serializeJson(json_doc, fsettings);
    fsettings.close();

    return 0;
}   