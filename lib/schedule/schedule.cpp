#include "schedule.hpp"

int save_schedule_to_json_file(schedule_t *sched, fs::FS fs, const char *file)
{
    StaticJsonDocument<512> json_doc;

    File fschedule = fs.open(file, "w");
    if (!fschedule) 
    {
        (void)Serial.printf("Error: Could not open file \"%s\" for writing\n", file);
        return -1;
    }

    json_doc["use_schedule" ] = sched->use_schedule;
    
    json_doc["action1"      ] = sched->action_1  ;
    json_doc["action2"      ] = sched->action_2  ;
    json_doc["date1"        ] = sched->date_1    ;           
    json_doc["date2"        ] = sched->date_2    ;
    json_doc["time1"        ] = sched->time_1    ;
    json_doc["time2"        ] = sched->time_2    ;
    json_doc["repeat"       ] = sched->repeat    ;

    json_doc["monday"       ] = sched->by_days[0];
    json_doc["tuesday"      ] = sched->by_days[1];
    json_doc["wednesday"    ] = sched->by_days[2];
    json_doc["thursday"     ] = sched->by_days[3];
    json_doc["friday"       ] = sched->by_days[4];
    json_doc["saturday"     ] = sched->by_days[5];
    json_doc["sunday"       ] = sched->by_days[6];

    (void)serializeJson(json_doc, fschedule);
    fschedule.close();

    return 0;
}

int load_schedule_from_json_file(schedule_t *sched, fs::FS fs, const char *file)
{
    StaticJsonDocument<512> json_doc;

    File fschedule = LittleFS.open(file, "r");
    if (!fschedule) 
    {
        (void)Serial.printf("Error: Could not open file %s for reading\n", file);
        return -1;
    }

    DeserializationError error = deserializeJson(json_doc, fschedule); 
    if (error)
    {
        (void)Serial.printf("Error: %s\n", error.c_str());
        return -1;
    }

    fschedule.close();

    sched->use_schedule    = json_doc["use_schedule"];
    
    (void)strcpy( sched->action_1 , json_doc["action1"] );
    (void)strcpy( sched->action_2 , json_doc["action2"] );
    (void)strcpy( sched->date_1   , json_doc["date1"  ] );
    (void)strcpy( sched->date_2   , json_doc["date2"  ] );
    (void)strcpy( sched->time_1   , json_doc["time1"  ] );
    (void)strcpy( sched->time_2   , json_doc["time2"  ] );
    (void)strcpy( sched->repeat   , json_doc["repeat" ] );

    sched->by_days[0] = json_doc["monday"   ];
    sched->by_days[1] = json_doc["tuesday"  ];
    sched->by_days[2] = json_doc["wednesday"];
    sched->by_days[3] = json_doc["thursday" ];
    sched->by_days[4] = json_doc["friday"   ];
    sched->by_days[5] = json_doc["saturday" ];
    sched->by_days[6] = json_doc["sunday"   ];

    return 0;
}

int update_schedule_from_json_doc(schedule_t *sched, fs::FS fs, const char *file, JsonVariant *json)
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

    File fschedule = LittleFS.open(file, "w");
    if (!fschedule) 
    {
        (void)Serial.printf("Error: Could not open file %s for writing\n", file);
        return -1;
    }

    sched->use_schedule = json_doc["use_schedule"];
    
    (void)strcpy( sched->action_1 , json_doc["action1"] );
    (void)strcpy( sched->action_2 , json_doc["action2"] );
    (void)strcpy( sched->date_1   , json_doc["date1"  ] );
    (void)strcpy( sched->date_2   , json_doc["date2"  ] );
    (void)strcpy( sched->time_1   , json_doc["time1"  ] );
    (void)strcpy( sched->time_2   , json_doc["time2"  ] );
    (void)strcpy( sched->repeat   , json_doc["repeat" ] );

    sched->by_days[0] = json_doc["monday"   ];
    sched->by_days[1] = json_doc["tuesday"  ];
    sched->by_days[2] = json_doc["wednesday"];
    sched->by_days[3] = json_doc["thursday" ];
    sched->by_days[4] = json_doc["friday"   ];
    sched->by_days[5] = json_doc["saturday" ];
    sched->by_days[6] = json_doc["sunday"   ];

    (void)serializeJson(json_doc, fschedule);
    fschedule.close();

    return 0;
}   