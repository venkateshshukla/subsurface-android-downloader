/* helper code for interfacing libdivecomputer and susburface commands with
 * android front end using JNI.
 */
#include "subsurface_jni.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include <libdivecomputer/context.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include "libdivecomputer.h"

#include <jni.h>

#define LOG_TAG "subsurface_jni.c"
#define LOG_F(fn_name) __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "Called : " fn_name )

#define STRSIZE 64
#define PATHSIZE 128

static device_data_t dcdata;

static char *xmlfile_name;
static bool prefer_downloaded;

// Reset the dcdata
void reset_dcdata(JNIEnv *env, jobject jobj)
{
	LOG_F("reset_dcdata");
	memset(&dcdata, 0, sizeof (device_data_t));
}

// Set the usb file descriptor of dcdata
int set_usb_fd(JNIEnv *env, jobject jobj, jint usb_fd)
{
	LOG_F("set_usb_fd");
	if (usb_fd > 0) {
		dcdata.fd = usb_fd;
		return 0;
	} else {
		return -1; // Invalid file descriptor
	}
}

// Prefer downloaded dives or already existing
void set_prefer(JNIEnv *env, jobject jobj, jboolean jprefer)
{
	LOG_F("set_prefer");
        prefer_downloaded = jprefer;
}

// Force download of dives even when they are already present?
void set_force(JNIEnv *env, jobject jobj, jboolean jforce)
{
	LOG_F("set_force");
        dcdata.force_download = jforce;
}

// Should logging be done?
void set_log(JNIEnv *env, jobject jobj, jboolean jlog)
{
	LOG_F("set_log");
        dcdata.libdc_log = jlog;
}

// Should file dumping be done?
void set_dump(JNIEnv *env, jobject jobj, jboolean jdump)
{
	LOG_F("set_dump");
        dcdata.libdc_dump = jdump;
}

// Set the vendor name
int set_vendor_name(JNIEnv *env, jobject jobj, jstring jvendor)
{
	LOG_F("set_vendor_name");
	if (jvendor != NULL) {
		const char *vn = (*env)->GetStringUTFChars(env, jvendor, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jvendor);
		char *vendor = (char *) malloc(len + 1);
		if (vendor == NULL)
			return -1; // Memory error
		strncpy(vendor, vn, len);
		vendor[len] = 0;
		(*env)->ReleaseStringUTFChars(env, jvendor, vn);
		dcdata.vendor = vendor;
		return 0;
	}
	return -2; // Null filename
}

// Set product name
int set_product_name(JNIEnv *env, jobject jobj, jstring jproduct)
{
	LOG_F("set_product_name");
	if (jproduct != NULL) {
		const char *pr = (*env)->GetStringUTFChars(env, jproduct, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jproduct);
		char *product = (char *) malloc(len + 1);
		if (product == NULL)
			return -1; // Memory error
		strncpy(product, pr, len);
		product[len] = 0;
		(*env)->ReleaseStringUTFChars(env, jproduct, pr);
		dcdata.product = product;
		return 0;
	}
	return -2; // Null filename
}

// Set the logfile and
int  set_logfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	LOG_F("set_logfile");
	if (jfilename != NULL) {
		const char *lf = (*env)->GetStringUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *logfile = (char *) malloc(len + 1);
		if (logfile == NULL)
			return -1; // Memory error
		strncpy(logfile, lf, len);
		logfile[len] = 0;
		(*env)->ReleaseStringUTFChars(env, jfilename, lf);
		logfile_name = logfile;
		return 0;
	}
	return -2; // Null filename
}

// Set the dumpfile
int  set_dumpfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	LOG_F("set_dumpfile");
	if (jfilename != NULL) {
		const char *df = (*env)->GetStringUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *dumpfile = (char *) malloc(len + 1);
		if (dumpfile == NULL)
			return -1; // Memory error
		strncpy(dumpfile, df, len);
		dumpfile[len] = 0;
		(*env)->ReleaseStringUTFChars(env, jfilename, df);
		dumpfile_name = dumpfile;
		return 0;
	}
	return -2; // Null filename
}

// Set the xmlfile
int  set_xmlfile(JNIEnv *env, jobject jobj, jstring jfilename)
{
	LOG_F("set_xmlfile");
	if (jfilename != NULL) {
		const char *xf = (*env)->GetStringUTFChars(env, jfilename, NULL);
		jsize len = (*env)->GetStringUTFLength(env, jfilename);
		char *xmlfile = (char *) malloc(len + 1);
		if (xmlfile == NULL)
			return -1; // Memory error
		strncpy(xmlfile, xf, len);
		xmlfile[len] = 0;
		(*env)->ReleaseStringUTFChars(env, jfilename, xf);
		xmlfile_name = xmlfile;
		return 0;
	}
	return -2; // Null filename
}

// Search and initialise dcdata descriptor.
int init_dc_descriptor(JNIEnv *env, jobject jobj)
{
        dc_iterator_t *iterator = NULL;
        dc_descriptor_t *descriptor = NULL;

        dc_descriptor_iterator(&iterator);
        while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
                const char *vendor = dc_descriptor_get_vendor(descriptor);
                const char *product = dc_descriptor_get_product(descriptor);
		if (!strcmp(vendor, dcdata.vendor) && !strcmp(product, dcdata.product)) {
			dcdata.descriptor = descriptor;
			return 0;
		}
        }
        dc_iterator_free(iterator);
	return -1;

}

// Start importing data from Divecomputer.
void do_dc_import(JNIEnv *env, jobject jobj)
{
	LOG_F("do_dc_import");
	const char *error_text;
	error_text = do_libdivecomputer_import(&dcdata);
	if (error_text)
		LOGD("error while import : %s\n", error_text);
	LOGD("Finished do_dc_import\n");
}

// After download if the download is successful, start processing the dives.
void do_process_dives(JNIEnv *env, jobject jobj)
{
        LOG_F("do_process_dives");
        // TODO insert check for cancelled thread.
        process_dives(true, prefer_downloaded);
}

// Save the dives after processing them.
int do_save_dives(JNIEnv *env, jobject jobj)
{
        LOG_F("do_save_dives");
        int ret = save_dives(xmlfile_name);
        if (ret == -1)
                LOGD("Error saving the dives.\n");
        // Do some error checking as well. And return the error to user.
        return ret;
}

// Iterate through all the devices and generate a java HashMap<String,
// ArrayList<String>> containing all the Vendors and Products.
void  get_device_map(JNIEnv *env, jobject jobj, jobject jhashMap)
{
	LOG_F("get_device_map");
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
	LOG_F("JNI_OnLoad");
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
		{ "resetDcData", "()V", reset_dcdata },
		{ "setUsbFd", "(I)I", set_usb_fd},
		{ "setDcPrefer", "(Z)V", set_prefer},
		{ "setDcForce", "(Z)V", set_force},
		{ "setDcLog", "(Z)V", set_log},
		{ "setDcDump", "(Z)V", set_dump},
		{ "setVendorName", "(Ljava/lang/String;)I", set_vendor_name},
		{ "setProductName", "(Ljava/lang/String;)I", set_product_name},
		{ "setLogFile",	"(Ljava/lang/String;)I", set_logfile },
		{ "setDumpFile", "(Ljava/lang/String;)I", set_dumpfile },
		{ "setXmlFile",	"(Ljava/lang/String;)I", set_xmlfile },
		{ "initDcDescriptor", "()I", init_dc_descriptor},
		{ "doDcImport", "()V", do_dc_import },
		{ "doProcessDives", "()V", do_process_dives },
		{ "doSaveDives", "()I", do_save_dives },
	};

	if ((*env)->RegisterNatives(env, DcData, nm2 , sizeof (nm2) / sizeof (nm2[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}

	return JNI_VERSION_1_6;
}

