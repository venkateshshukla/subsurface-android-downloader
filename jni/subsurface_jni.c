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

#include <jni.h>

#define LOG_TAG "subsurface_jni.c"
#define LOG_F(fn_name) __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "Called : " fn_name )

// A single JavaVM is shared by all threads in a process.
// Hence it can be cached.
static JavaVM *java_vm;

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

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	java_vm = vm;

	// Get JNI Env for all function calls
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGD ("GetEnv failed.\n");
		return -1;
	}

	// Find the class calling native function
	// org/libdivecomputer/NativeHelper.java
	jclass NativeHelper = (*env)->FindClass(env, "org/libdivecomputer/Main");
	if (NativeHelper == NULL) {
		LOGD ("FindClass failed : org.libdivecomputer.Main\n");
		return -1;
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm[] = {
		{ "getDeviceMap",	"(Ljava/util/HashMap;)V",	get_device_map },
	};

	if ((*env)->RegisterNatives(env, NativeHelper, nm , sizeof (nm) / sizeof (nm[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	}

	return JNI_VERSION_1_6;
}

