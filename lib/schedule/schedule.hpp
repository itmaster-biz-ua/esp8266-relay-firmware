#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include <LittleFS.h>
#include <ArduinoJson.h>

struct schedule_t
{
    bool use_schedule ;
    char action_1 [9] ;
    char action_2 [9] ;
    char date_1   [11];
    char date_2   [11];
    char time_1   [6] ;
    char time_2   [6] ;
    char repeat   [8] ;
    bool by_days  [7] ;   
};

int save_schedule_to_json_file(schedule_t *, fs::FS, const char *);

int load_schedule_from_json_file(schedule_t *, fs::FS, const char *);

int update_schedule_from_json_doc(schedule_t *, fs::FS, const char *, JsonVariant *);

#endif // SCHEDULE_H_
