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

void device_descriptor_lookup(device_data_t *d)
{
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		if (!strcmp(vendor, d->vendor) && !strcmp(product, d->product)) {
			LOGD("DC Descriptor found : %s / %s\n", vendor, product);
			d->descriptor = descriptor;
			break;
		}
	}
	dc_iterator_free(iterator);

}

JNIEXPORT void JNICALL get_device_map(JNIEnv *env, jobject jobj, jobject jhashMap)
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
	struct mydescriptor *mydescriptor;

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

JNIEXPORT void JNICALL init_dcdata(JNIEnv *env, jobject jobj, jobject jdcdata)
{
	LOG_F("init_dcdata");
	jclass DcData = (*env)->GetObjectClass(env, jdcdata);
	jmethodID getFd = (*env)->GetMethodID(env, DcData, "getFd", "()I");
	jmethodID getVendor = (*env)->GetMethodID(env, DcData, "getVendor", "()Ljava/lang/String;");
	jmethodID getProduct = (*env)->GetMethodID(env, DcData, "getProduct", "()Ljava/lang/String;");
	jmethodID isForce = (*env)->GetMethodID(env, DcData, "isForce", "()Z");
	jmethodID isPrefer = (*env)->GetMethodID(env, DcData, "isPrefer", "()Z");
	jmethodID isLog = (*env)->GetMethodID(env, DcData, "isLog", "()Z");
	jmethodID isDump = (*env)->GetMethodID(env, DcData, "isDump", "()Z");
	jmethodID getLogfilepath = (*env)->GetMethodID(env, DcData, "getLogfilepath", "()Ljava/lang/String;");
	jmethodID getOutfilepath = (*env)->GetMethodID(env, DcData, "getOutfilepath", "()Ljava/lang/String;");

	jint jfd =(*env)->CallIntMethod(env, jdcdata, getFd);
	jstring jvendor = (*env)->CallObjectMethod(env, jdcdata, getVendor);
	jstring jproduct = (*env)->CallObjectMethod(env, jdcdata, getProduct);
	jboolean jforce = (*env)->CallBooleanMethod(env, jdcdata, isForce);
	jboolean jprefer = (*env)->CallBooleanMethod(env, jdcdata, isPrefer);
	jboolean jlog = (*env)->CallBooleanMethod(env, jdcdata, isLog);
	jboolean jdump = (*env)->CallBooleanMethod(env, jdcdata, isDump);
	jstring jlogfilepath = (*env)->CallObjectMethod(env, jdcdata, getLogfilepath);
	jstring joutfilepath = (*env)->CallObjectMethod(env, jdcdata, getOutfilepath);

	dcdata.fd = jfd;
	dcdata.force_download = jforce;
	dcdata.libdc_log = jlog;
	dcdata.libdc_dump = jdump;
	dcdata.deviceid = 0;
	dcdata.diveid = 0;

	LOGD("file descriptor received : %d\n", jfd);

	if (jvendor != NULL) {
		const char *vndr = (*env)->GetStringUTFChars(env, jvendor, NULL);
		char *v = (char *) malloc(STRSIZE);
		strncpy(v, vndr, STRSIZE);
		dcdata.vendor = v;
		(*env)->ReleaseStringUTFChars(env, jvendor, vndr);
	}
	if (jproduct != NULL) {
		const char *prdt = (*env)->GetStringUTFChars(env, jproduct, NULL);
		char *p = (char *) malloc(STRSIZE);
		strncpy(p, prdt, STRSIZE);
		dcdata.product = p;
		(*env)->ReleaseStringUTFChars(env, jproduct, prdt);

	}

	device_descriptor_lookup(&dcdata);

	if (jlogfilepath != NULL) {
		const char *logfp = (*env)->GetStringUTFChars(env, jlogfilepath, NULL);
		(*env)->ReleaseStringUTFChars(env, jlogfilepath, logfp);
		LOGD("logfilename received : %s\n", logfp);
		logfile_name = (char *) logfp;

	}
	if (joutfilepath != NULL) {
		const char *dumpfp = (*env)->GetStringUTFChars(env, joutfilepath, NULL);
		(*env)->ReleaseStringUTFChars(env, joutfilepath, dumpfp);
		LOGD("dumpfilename received : %s\n", dumpfp);
		dumpfile_name = (char *) dumpfp;
	}

	LOGD("successfully finished init_dcdata");
}

JNIEXPORT void JNICALL do_dc_import(JNIEnv *env, jobject jobj)
{
	LOG_F("do_dc_import");
	const char *error_text;
	error_text = do_libdivecomputer_import(&dcdata);
	if (error_text)
		LOGD("error while import : %s\n", error_text);
	LOGD("Finished do_dc_import\n");
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

	// Find the class calling native function
	// org/libdivecomputer/ImportProgress.java
	jclass ImportProgress = (*env)->FindClass(env, "org/libdivecomputer/ImportProgress");
	if (ImportProgress == NULL) {
		LOGD ("FindClass failed : org.libdivecomputer.ImportProgress\n");
		return -1;
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm2[] = {
		{ "initDcData",	"(Lorg/libdivecomputer/DcData;)V", init_dcdata },

	};

	if ((*env)->RegisterNatives(env, ImportProgress, nm2 , sizeof (nm2) / sizeof (nm2[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}

	jclass DcImportTask = (*env)->FindClass(env, "org/libdivecomputer/ImportProgress$DcImportTask");
	if (ImportProgress == NULL) {
		LOGD ("FindClass failed : org.libdivecomputer.ImportProgress$DcImportTask\n");
		return -1;
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm3[] = {
		{ "doDcImport",	"()Z", do_dc_import },
	};

	if ((*env)->RegisterNatives(env, DcImportTask, nm3 , sizeof (nm3) / sizeof (nm3[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}


	return JNI_VERSION_1_6;
}

