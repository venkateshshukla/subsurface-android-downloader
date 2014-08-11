/* universal.h */

#ifndef UNIVERSAL_H
#define UNIVERSAL_H

#include <stdio.h>	// fopen, fwrite, fclose
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define DC_TICKS_FORMAT "%lld"

#include <unistd.h>

#include <libdivecomputer/context.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include "utils.h"
#include "common.h"

typedef struct device_data_t {
	dc_event_devinfo_t devinfo;
	dc_event_clock_t clock;
} device_data_t;

typedef struct dive_data_t {
	dc_device_t *device;
	FILE* fp;
	unsigned int number;
	dc_buffer_t *fingerprint;
} dive_data_t;

typedef struct sample_data_t {
	FILE* fp;
	unsigned int nsamples;
} sample_data_t;

typedef struct backend_table_t {
	const char *name;
	dc_family_t type;
} backend_table_t;

dc_family_t lookup_type (const char *);
dc_buffer_t *fpconvert (const char *);
void sighandler (int);
dc_status_t search (dc_descriptor_t **, const char *, dc_family_t, unsigned int);
dc_status_t dowork (dc_context_t *, dc_descriptor_t *, int, const char *, const char *, int, int, dc_buffer_t *);
#endif // UNIVERSAL_H

