/* helper code for interfacing libdivecomputer and susburface commands with
 * android front end using JNI.
 */
#include "libdivecomputer_jni.c"
#include "universal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include <libdivecomputer/context.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include <jni.h>

#define LOG_TAG "libdivecomputer_jni.c"
#define LOG_F(fn_name) __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "Called : " fn_name )

#define STRSIZE 64
#define PATHSIZE 128

typedef struct device_data_t {
	dc_context_t *context;
	dc_descriptor_t *descriptor;
	dc_loglevel_t loglevel;
	int fd;
	const char *logfile;
	const char *xmlfile;
	const char *dumpfile;
	bool dump;
} device_data_t;

static device_data_t dcdata;

// Reset the dcdata
void reset_dcdata(JNIEnv *env, jobject jobj)
{
	memset(&dcdata, 0, sizeof (device_data_t));
}

// Set the usb file descriptor of dcdata
int set_usb_fd(JNIEnv *env, jobject jobj, jint usb_fd)
{
	if (usb_fd > 0) {
		dcdata.fd = usb_fd;
		return 0;
	} else {
		return -1; // Invalid file descriptor
	}
}

// Set the logfile and set loglevel to DC_LOGLEVEL_INFO
int  set_logfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	if (jfilename != NULL) {
		const char *lf = (*env)->GetUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *logfile = (char *) malloc(len + 1);
		if (logfile == NULL)
			return -1; // Memory error
		strncpy(logfile, lf, len);
		(*env)->ReleaseStringUTFChars(env, jfilename, lf);
		dcdata.logfile = logfile;
		return 0;
	}
	return -2; // Null filename
}

// Set the dumpfile
int  set_dumpfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	if (jfilename != NULL) {
		const char *df = (*env)->GetUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *dumpfile = (char *) malloc(len + 1);
		if (dumpfile == NULL)
			return -1; // Memory error
		strncpy(dumpfile, df, len);
		(*env)->ReleaseStringUTFChars(env, jfilename, df);
		dcdata.dumpfile = dumpfile;
		dcdata.dump = 1;
		return 0;
	}
	return -2; // Null filename
}

// Set the xmlfile
int  set_xmlfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	if (jfilename != NULL) {
		const char *xf = (*env)->GetUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *xmlfile = (char *) malloc(len + 1);
		if (xmlfile == NULL)
			return -1; // Memory error
		strncpy(xmlfile, xf, len);
		(*env)->ReleaseStringUTFChars(env, jfilename, xf);
		dcdata.xmlfile = xmlfile;
		dcdata.dump = 0;
		return 0;
	}
	return -2; // Null filename
}

// Initialise a dc_context for libdivecomputer functions
int init_dc_context(JNIEnv *env, jobject jobj)
{
	dc_context_t *context = NULL;
	dc_status_t rc = dc_context_new (&context);
	if (rc != DC_STATUS_SUCCESS) {
		message_set_logfile (NULL);
		return -1; // Error initialising context
	}
	dcdata.context = context;
	return 0;
}

// Search for matching dc_descriptor and if found, add it to dcdata.
int init_dc_descriptor(JNIEnv *env, jobject jobj, jstring jvendor, jstring, jproduct)
{
	if (jvendor != NULL && jproduct != NULL) {
		const char *vn = (*env)->GetUTFChars(env, jvendor, NULL);
		const char *pr = (*env)->GetUTFChars(env, jproduct, NULL);

		dc_iterator_t *iterator = NULL;
		dc_descriptor_t *descriptor = NULL;

		int found = 0;
		dc_descriptor_iterator(&iterator);
		while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
			const char *vendor = dc_descriptor_get_vendor(descriptor);
			const char *product = dc_descriptor_get_product(descriptor);
			if (!strcmp(vn, vendor) && !strcmp(pr, product)) {
				found = 1;
				dcdata.descriptor = descriptor;
				break;
			}
		}
		dc_iterator_free(iterator);
		(*env)->ReleaseStringUTFChars(env, jvendor, vn);
		(*env)->ReleaseStringUTFChars(env, jproduct, pr);
		return found ? 0 : -1;
	}
}

// interrupt the import by raising a interrupt signal
int interrupt_import(JNIEnv *env, jobject jobj)
{
	return raise(SIGINT);
}

// Using the values stored in dcdata, start importing data from divecomputer.
int start_import (JNIEnv *env, jobject jobj)
{
	signal (SIGINT, sighandler);

	message_set_logfile (dcdata.logfile);
	dcdata.loglevel = DC_LOGLEVEL_INFO;

	dc_context_set_loglevel (dcdata.context, dcdata.loglevel);
	dc_context_set_logfunc (dcdata.context, logfunc, NULL);

	rc = dowork (dcdata.context, dcdata.descriptor, dcdata.fd, dcdata.dumpfile, dcdata.xmlfile, dcdata.dump, !dcdata.dump, NULL);
	message ("Result: %s\n", errmsg (rc));

	dc_descriptor_free (dcdata.descriptor);
	dc_context_free (dcdata.context);

	message_set_logfile (NULL);

	return rc != DC_STATUS_SUCCESS ? -1 : 0;
}

// Iterate through all the devices and generate a java HashMap<String,
// ArrayList<String>> containing all the Vendors and Products.
void  get_device_map(JNIEnv *env, jobject jobj, jobject jhashMap)
{
	jclass HashMap = (*env)->GetObjectClass(env, jhashMap);
	jmethodID hmInit = (*env)->GetMethodID(env, HashMap, "<init>", "()V");
	jmethodID containsKey = (*env)->GetMethodID(env, HashMap, "containsKey", "(Ljava/lang/Object;)Z");
	jmethodID hmGet = (*env)->GetMethodID(env, HashMap, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
	jmethodID hmPut = (*env)->GetMethodID(env, HashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	jclass ArrayList = (*env)->FindClass(env, "java/util/ArrayList");
	jmethodID alInit = (*env)->GetMethodID(env, ArrayList, "<init>", "()V");
	jmethodID alAdd = (*env)->GetMethodID(env, ArrayList, "add", "(Ljava/lang/Object;)Z");

	jstring jvendor, jproduct;
	jobject strarr;

	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;

	bool isPresent;
	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		jvendor = (*env)->NewStringUTF(env, vendor);
		jproduct = (*env)->NewStringUTF(env, product);

		isPresent = (*env)->CallBooleanMethod(env, jhashMap, containsKey, jvendor);
		if (isPresent) {
			strarr = (*env)->CallObjectMethod(env, jhashMap, hmGet, jvendor);
			(*env)->CallBooleanMethod(env, strarr, alAdd, jproduct);
		} else {
			strarr = (*env)->NewObject(env, ArrayList, alInit);
			(*env)->CallBooleanMethod(env, strarr, alAdd, jproduct);
			(*env)->CallObjectMethod(env, jhashMap, hmPut, jvendor, strarr);
		}
	}
	dc_iterator_free(iterator);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	// Get JNI Env for all function calls
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGD ("GetEnv failed.\n");
		return -1;
	}

	// Find the class calling native function
	// org/libdivecomputer/Main.java
	jclass Main = (*env)->FindClass(env, "org/libdivecomputer/Main");
	if (Main == NULL) {
		LOGD ("FindClass failed : org.libdivecomputer.Main\n");
		return -1;
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm1[] = {
		{ "getDeviceMap", "(Ljava/util/HashMap;)V", get_device_map },

	};

	if ((*env)->RegisterNatives(env, Main, nm1 , sizeof (nm1) / sizeof (nm1[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}

	// Find the class org/libdivecomputer/DcData containing all native
	// functions
	jclass DcData = (*env)->FindClass(env, "org/libdivecomputer/DcData");
	if (DcData == NULL) {
		LOGD ("FindClass failed : org.libdivecomputer.DcData\n");
		return -1;
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm2[] = {
		{ "resetDcData", "()V", reset_dc_data },
		{ "setUsbFd", "(I)I", set_usb_fd},
		{ "setLogFile",	"(Ljava/lang/String;)I", set_log_file },
		{ "setDumpFile", "(Ljava/lang/String;)I", set_dump_file },
		{ "setXmlFile",	"(Ljava/lang/String;)I", set_xml_file },
		{ "initDcContext", "()I", init_dc_context },
		{ "initDcDescriptor", "(Ljava/lang/String;Ljava/lang/String;)", init_dc_descriptor },
		{ "interruptImport", "()I", interrupt_import },
		{ "startImport", "()I", start_import },
	};

	if ((*env)->RegisterNatives(env, DcData, nm2 , sizeof (nm2) / sizeof (nm2[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}

	return JNI_VERSION_1_6;
}

