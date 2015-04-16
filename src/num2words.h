#pragma once

#include "pebble.h"
#include "string.h"
#define BUFFER_SIZE 32
#define DATE_FORMAT "%b %e"
  
#define DEBUG_NUM 1

void fuzzy_minutes_to_words(struct tm *t, char* words);
void fuzzy_hours_to_words(struct tm *t, char* words);
void fuzzy_sminutes_to_words(struct tm *t, char* words);
void fuzzy_dates_to_words(struct tm *t, char* words);
void fuzzy_set_date_lower(int isLower);