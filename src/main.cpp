
#define LOGE(ERR_MSG, ...) \
    (void)Serial.printf("E: " ERR_MSG "\n", ##__VA_ARGS__); \

#define LOGI(INFO_MSG, ...) \
    (void)Serial.printf("I: " INFO_MSG "\n", ##__VA_ARGS__); \

#define TURN_ON() \
    uint8_t buff[] = {0xA0, 0x01, 0x01, 0xA2}; \
    (void)Serial.write(buff, sizeof(buff)); \
    (void)Serial.println();

#define TURN_OFF() \
    uint8_t buff[] = {0xA0, 0x01, 0x00, 0xA1}; \
    (void)Serial.write(buff, sizeof(buff)); \
    (void)Serial.println();

#include <Arduino.h> 
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <AsyncJson.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266Ping.h>
#include <ESPAsyncWebServer.h>

#include "schedule.hpp"
#include "settings.hpp"

bool relay_is_on = false;
const char *ping_url = "google.com";

WiFiUDP ntpUDP;
NTPClient time_client(ntpUDP, "pool.ntp.org");
time_t start_time, time_offset, prev_millis, current_millis;
bool is_synchronized = false;

DNSServer dns_server;

AsyncWebServer server(80);
AsyncEventSource events("/events");

schedule_t schedule;
settings_t settings;

int get_day_number_from_name(const char *);
int execute_action(bool, char *, char *, char *, time_t);
long parse_utc_time_zone(const char *);
uint64_t millis64(); 

constexpr bool ACTION_ONE = true;
constexpr bool ACTION_TWO = false;

class CaptiveRequestHandler : public AsyncWebHandler 
{
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request) 
    {
        request->redirect("/settings");
    }
};

void setup()
{
    Serial.begin(9600);

    if (!LittleFS.begin())
    {
        LOGE("Could not init LittleFS. Restarting...");
        ESP.restart();
    }
    else
    {
        int err = load_schedule_from_json_file(&schedule, LittleFS, "/schedule.json");
        if (err)
        {
            LOGE("Could not load schedule file. Restarting...");
            ESP.restart();
        }

        err = load_settings_from_json_file(&settings, LittleFS, "/settings.json");
        if (err)
        {
            LOGE("Could not load settings file. Restarting...");
            ESP.restart();
        }

        long time_offset = parse_utc_time_zone(settings.time_zone);
        if (time_offset == -1L)
        {
            LOGE("Could not parse timezone. Restarting...");
            ESP.restart();
        }

        time_client.setTimeOffset(time_offset);
    }
    
    (void)WiFi.mode(WIFI_STA);
    (void)WiFi.hostname("SmartRelay");
    
    if (settings.use_static_ip)
    {
        IPAddress wifi_ip;
        IPAddress wifi_gateway;
        IPAddress wifi_subnet;
        IPAddress wifi_dns1;
        IPAddress wifi_dns2;

        (void)wifi_ip.fromString(settings.static_ipv4);
        (void)wifi_gateway.fromString(settings.static_gateway);
        (void)wifi_subnet.fromString(settings.static_subnet);
        (void)wifi_dns1.fromString(settings.static_dns);
        (void)wifi_dns2.fromString(settings.static_dns);

       (void) WiFi.config(wifi_ip, wifi_gateway, wifi_subnet, wifi_dns1, wifi_dns2);
    }

    (void)WiFi.begin(settings.wifi_ssid, settings.wifi_password);
    
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        LOGI("Could not connect to WiFi, ssid=%s, password=%s. Going to AP mode.", settings.wifi_ssid, settings.wifi_password);

        (void)WiFi.mode(WIFI_AP);
        (void)WiFi.softAPConfig(IPAddress(192, 168, 4, 4), IPAddress(192, 168, 4, 4), IPAddress(255, 255, 255, 0));
        (void)WiFi.softAP("SmartRelay");

        LOGI("Wifi AP started, ssid=SmartRelay, Ip=%s", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        LOGI("Connected to Wifi, ssid=%s, Ip=%s", settings.wifi_ssid, WiFi.localIP().toString().c_str());
    }

    (void)server.serveStatic( "/styles/main.css"      , LittleFS, "/styles/main.css"    );
    (void)server.serveStatic( "/scripts/main.js"      , LittleFS, "/scripts/main.js"    );
    (void)server.serveStatic( "/scripts/settings.js"  , LittleFS, "/scripts/settings.js");
    (void)server.serveStatic( "/images/favicon.ico"   , LittleFS, "/images/favicon.ico" );
    (void)server.serveStatic( "/images/home.ico"      , LittleFS, "/images/home.ico"    );
    (void)server.serveStatic( "/images/settings.ico"  , LittleFS, "/images/settings.ico");

    // (void)server.serveStatic( "/logs.html"            , LittleFS, "/logs.html");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {   
        if (settings.use_auth)
            if(!request->authenticate(settings.auth_login, settings.auth_pass))
                return request->requestAuthentication();

            
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
    {   
        if (settings.use_auth)
            if(!request->authenticate(settings.auth_login, settings.auth_pass))
                return request->requestAuthentication();

            
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/settings.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server.on("/on", HTTP_PUT, [](AsyncWebServerRequest *request)
    {
        relay_is_on = true;
        TURN_ON();
        LOGI("Relay turend on");
        request->send(200, "application/json", "{\"state\":\"ON\"}");
    });

    server.on("/off", HTTP_PUT, [](AsyncWebServerRequest *request) 
    {
        relay_is_on = false;
        TURN_OFF();
        LOGI("Relay turend off");
        request->send(200, "application/json", "{\"state\":\"OFF\"}");
    });

    server.on("/index/get", HTTP_PUT, [](AsyncWebServerRequest *request) 
    {   
        StaticJsonDocument<512> scheduleJSON;

        File fschedule = LittleFS.open("/schedule.json", "r");
        if (!fschedule) 
        {
            LOGE("Could not open file \"schedule.json\" for reading. Restarting...");
            ESP.restart();
        }

        DeserializationError error = deserializeJson(scheduleJSON, fschedule); 
        if (error)
        {
            LOGE("%s. Restarting...", error.c_str());
            ESP.restart();
        }

        fschedule.close();

        scheduleJSON["state"] = relay_is_on ? "ON" : "OFF";
        
        String jsonString;
        (void)serializeJson(scheduleJSON, jsonString);
        request->send(200, "application/json", jsonString);
    });

    server.on("/settings/get", HTTP_PUT, [](AsyncWebServerRequest *request)
    {
        StaticJsonDocument<512> settingsJSON;

        File fsettings = LittleFS.open("/settings.json", "r");
        if (!fsettings) 
        {
            LOGE("Could not open file \"settings.json\" for reading. Restarting...");
            ESP.restart();
        }

        DeserializationError error = deserializeJson(settingsJSON, fsettings); 
        if (error)
        {
            LOGE("%s. Restarting...", error.c_str());
            ESP.restart();
        }

        fsettings.close();
        
        String jsonString;
        (void)serializeJson(settingsJSON, jsonString);
        request->send(200, "application/json", jsonString);
    });

    AsyncCallbackJsonWebHandler *schedule_handler = new AsyncCallbackJsonWebHandler("/schedule/set", [](AsyncWebServerRequest *request, JsonVariant &json) 
    {
        int err = update_schedule_from_json_doc(&schedule, LittleFS, "/schedule.json", &json);
        if (err)
        {
            LOGE("Could not update file \"schedule.json\".");
            request->send(500, "application/json", "{\"message\":\"Internal Server Error\"}");
            return;
        }

        request->send(200, "application/json", "{\"message\":\"OK\"}");
    });

    AsyncCallbackJsonWebHandler *settings_handler = new AsyncCallbackJsonWebHandler("/settings/set", [](AsyncWebServerRequest *request, JsonVariant &json) 
    {
        int err = update_settings_from_json_doc(&settings, LittleFS, "/settings.json", &json);
        if (err)
        {
            LOGE("Could not update file \"settings.json\".");
            request->send(500, "application/json", "{\"message\":\"Internal Server Error\"}");
            return;
        }

        long time_offset = parse_utc_time_zone(settings.time_zone);
        if (time_offset == -1L)
        {
            LOGE("Could not parse timezone.");
            request->send(500, "application/json", "{\"message\":\"Internal Server Error\"}");
            return;
        }

        time_client.setTimeOffset(time_offset);

        request->send(200, "application/json", "{\"message\":\"OK\"}");

        ESP.restart();
    });

    AsyncCallbackJsonWebHandler *time_set_handler = new AsyncCallbackJsonWebHandler("/settings/time/set", [](AsyncWebServerRequest *request, JsonVariant &json) 
    {
        StaticJsonDocument<32> json_doc;

        if (json.is<JsonArray>())
        {
            json_doc = json.as<JsonArray>();
        }
        else if (json.is<JsonObject>())
        {
            json_doc = json.as<JsonObject>();
        }

        const char *date_time_str = json_doc["date_time"];
        struct tm tm;
        if (strptime(date_time_str, "%Y-%m-%d %H:%M:%S", &tm) != NULL) 
        {
            start_time = mktime(&tm);
            time_offset = millis64() / 1000;
            is_synchronized = true;
            request->send(200, "application/json", "{\"message\":\"OK\"}");
        }
        else 
        {
            is_synchronized = false;
            request->send(500, "application/json", "{\"message\":\"Wrong time format\"}");
        }
    });

    events.onConnect([](AsyncEventSourceClient *client)
    {
        client->send("{\"message\":\"OK\"}", nullptr, millis(), 1000);
    });

    events.onConnect([](AsyncEventSourceClient *client)
    {
        client->send("{\"message\":\"OK\"}", nullptr, millis(), 1000);
    });

    server.on ( "/generate_204", [](AsyncWebServerRequest *request)
    {
        request->redirect("/settings");
    });

    (void)server.addHandler(&events);
    (void)server.addHandler(schedule_handler);
    (void)server.addHandler(settings_handler);
    (void)server.addHandler(time_set_handler);
    (void)server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

    (void)dns_server.start(53, "*", WiFi.softAPIP());
    time_client.begin();
    server.begin();

    if (time_client.update())
    {
        start_time = time_client.getEpochTime();
        time_offset = millis64() / 1000;
        is_synchronized = true;
    }
    else
    {
        start_time = 0;
        time_offset = 0;
    }
}

void loop()
{   
    if (WiFi.getMode() == WIFI_AP)
        dns_server.processNextRequest();

    if (millis() % 1000 == 0)
    {
        if (!is_synchronized)
            events.send("", "sync_time", millis());

        if (schedule.use_schedule)
        {   
            static bool execute_called = false;
            static int last_called = 0;

            time_t epoch_secs = 0;
            if (time_client.update()) // This may cause the unnessary amount of internet traffic consumption. Need to limit the amout of requests per some time period.
                epoch_secs = time_client.getEpochTime();
            else
                epoch_secs = start_time + ((millis64() / 1000) - time_offset); // Better to use local time because it does not consume internet traffic.

            char date_buff[11], time_buff[6];

            (void)sprintf(date_buff, "%04d-%02d-%02d", year(epoch_secs), month(epoch_secs), day(epoch_secs));
            (void)sprintf(time_buff, "%02d:%02d", hour(epoch_secs), minute(epoch_secs));

            if (!strcmp(date_buff, schedule.date_1) && !strcmp(time_buff, schedule.time_1) && !execute_called)
            {
                (void)execute_action(ACTION_ONE, schedule.action_1, schedule.date_1, schedule.time_1, epoch_secs);
                last_called = minute(epoch_secs);
                execute_called = true;
            }

            if (!strcmp(date_buff, schedule.date_2) && !strcmp(time_buff, schedule.time_2) && !execute_called)
            {
                (void)execute_action(ACTION_TWO, schedule.action_2, schedule.date_2, schedule.time_2, epoch_secs);
                last_called = minute(epoch_secs);
                execute_called = true;
            }

            if (last_called != minute(epoch_secs))
                execute_called = false;
        }

        char state[16];
        (void)sprintf(state, "{\"state\":\"%s\"}", relay_is_on ? "ON" : "OFF");
        events.send(state, "ping", millis());

        if (!Ping.ping(ping_url)) 
            events.send("{\"status\":\"offline\"}", "ping_status", millis());
        else 
            events.send("{\"status\":\"online\"}", "ping_status", millis());
    }
}

int get_day_number_from_name(const char *day_name)
{
    if (!strcmp(day_name, "monday"))
        return 1;
    if (!strcmp(day_name, "tuesday"))
        return 2;
    if (!strcmp(day_name, "wednesday"))
        return 3;
    if (!strcmp(day_name, "thursday"))
        return 4;
    if (!strcmp(day_name, "friday"))
        return 5;
    if (!strcmp(day_name, "saturday"))
        return 6;
    if (!strcmp(day_name, "sunday"))
        return 7;
    return -1;
}

int execute_action(const bool action_select, char *action, char *date, char *time, const time_t epoch_secs)
{
    char event_action[15], event_date[13];

    if (action_select)
    {
        (void)strcpy(event_action, "update_action1");
        (void)strcpy(event_date, "update_date1");
    }
    else 
    {
        (void)strcpy(event_action, "update_action2");
        (void)strcpy(event_date, "update_date2");
    }

    if (!strcmp(action, "turnon"))
    {
        relay_is_on = true;
        TURN_ON();
        LOGI("Relay turned on");
    }
    else if (!strcmp(action, "turnoff"))
    {
        relay_is_on = false;
        TURN_OFF();
        LOGI("Relay turned off");
    }

    if (!strcmp(schedule.repeat, "once"))
    {
        events.send("noaction", event_action, millis());
        (void)strcpy(action, "noaction");
    } 

    else if (!strcmp(schedule.repeat, "daily"))
    {
        char next_date[11];
        unsigned long next_epoch_secs = epoch_secs + SECS_PER_DAY;
        (void)sprintf(next_date, "%04d-%02d-%02d", year(next_epoch_secs), month(next_epoch_secs), day(next_epoch_secs));
        events.send(next_date, event_date, millis());
        (void)strcpy(date, next_date);
        LOGI("This action will be repeated at %s", next_date);
    }

    else if (!strcmp(schedule.repeat, "weekly"))
    {
        char next_date[11];
        unsigned long next_epoch_secs = epoch_secs + SECS_PER_DAY * 7;
        (void)sprintf(next_date, "%04d-%02d-%02d", year(next_epoch_secs), month(next_epoch_secs), day(next_epoch_secs));
        events.send(next_date, event_date, millis());
        (void)strcpy(date, next_date);
        LOGI("This action will be repeated at %s", next_date);
    }

    else if (!strcmp(schedule.repeat, "monthly"))
    {
        char next_date[11];
        uint8_t next_month, next_year;

        uint8_t current_month = month(epoch_secs);
        if (current_month == 12)
        {
            next_month = 1;
            next_year = year(epoch_secs) + 1;
        }
        else
        {
            next_month = current_month + 1;
            next_year = year(epoch_secs);
        }

        (void)sprintf(next_date, "%04hhu-%02hhu-%02d", next_year, next_month, day(epoch_secs));
        events.send(next_date, event_date, millis());

        (void)strcpy(date, next_date);
        LOGI("This action will be repeated at %s", next_date);
    }

    else if (!strcmp(schedule.repeat, "days"))
    {
        char day_of_week[10];
        tm *time_info = gmtime(&epoch_secs);
        if (time_info == nullptr) 
        {
            LOGE("Failed to convert epoch time to struct tm");
            return -1;
        }
        (void)strftime(day_of_week, sizeof(day_of_week), "%A", time_info);
        day_of_week[0] = (char)tolower((int)day_of_week[0]);
        
        int current_day_num = get_day_number_from_name(day_of_week) - 1;
        if (current_day_num == -2)
        {
            LOGE("No day named %s", day_of_week);
            return -1;
        }

        int next_day_num = current_day_num;

        int step = 0;
        for (uint8_t i = 0; i < 7; i++)
        {
            if (next_day_num == 6)
                next_day_num = 0;
            else 
                next_day_num += 1;

            if (schedule.by_days[next_day_num])
            {
                char next_date[11];
                
                if (next_day_num > current_day_num)
                    step = next_day_num - current_day_num;
                else 
                    step = next_day_num - current_day_num + 7;

                unsigned long next_epoch_secs = epoch_secs + SECS_PER_DAY * (step);
                (void)sprintf(next_date, "%04d-%02d-%02d", year(next_epoch_secs), month(next_epoch_secs), day(next_epoch_secs));
                events.send(next_date, event_date, millis());
                (void)strcpy(date, next_date);
                LOGI("This action will be repeated at %s", next_date);
                break;
            }

            if (i == 6)
            {
                events.send("", event_date, millis());
                (void)strcpy(date, "");
            }
        }
    }

    int err = save_schedule_to_json_file(&schedule, LittleFS, "/schedule.json");
    if (err)
    {
        return -1;
    }

    return 0;
}

long parse_utc_time_zone(const char *utc_str)
{
    int hours, minutes;
    sscanf(utc_str, "UTC%d:%d", &hours, &minutes);
    return (long) (3600 * hours) + (60 * minutes);
}

uint64_t millis64() 
{
    static uint32_t low32, high32; // Static variables are automatically initialized to zero

    uint32_t new_low32 = millis();

    if (new_low32 < low32) 
        high32++;

    low32 = new_low32;

    return (uint64_t) high32 << 32 | low32;
}