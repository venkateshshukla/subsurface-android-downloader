#ifndef HELP_H
#define HELP_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "dive.h"
#include "divelist.h"

// global pointers for our translation

char *weight_string(int weight_in_grams);
bool gpsHasChanged(struct dive *dive, struct dive *master, const char *gps_text, bool *parsed_out);
char *printGPSCoords(int lat, int lon);
int *getDivesInTrip(dive_trip_t *trip);
char *gasToStr(struct gasmix gas);

#endif // HELP_H
