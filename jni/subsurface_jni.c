/* helper code for interfacing libdivecomputer and susburface commands with
 * android front end using JNI.
 */
#include "subsurface_jni.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <libdivecomputer/context.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include <jni.h>

#define LOG_TAG "subsurface_jni.c"
#define LOG_F(fn_name) __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "Called : " fn_name )

// A single JavaVM is shared by all threads in a process.
// Hence it can be cached.
static JavaVM *java_vm;

JNIEXPORT jobjectArray JNICALL get_vendor_list(JNIEnv *env, jobject jobj)
{
	LOG_F("get_vendor_list");
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		LOGD("vendor : %s\n", vendor);
		LOGD("product : %s\n", product);
	}
	dc_iterator_free(iterator);
	return NULL;
}

JNIEXPORT jobjectArray JNICALL get_product_list(JNIEnv *env, jobject jobj, jobject vndr)
{
	LOG_F("get_product_list");
	return NULL;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	LOG_F ("JNI_OnLoad");
	java_vm = vm;

	// Get JNI Env for all function calls
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGD ("GetEnv failed.\n");
		return -1;
	} else {
		LOGD("GetEnv succeeded.\n");
	}

	// Find the class calling native function
	// org/libdivecomputer/NativeHelper.java
	jclass NativeHelper = (*env)->FindClass(env, "org/libdivecomputer/Main");
	if (NativeHelper == NULL) {
		LOGD ("FindClass failed : No class found.\n");
		return -1;
	} else {
		LOGD("FindClass org.libdivecomputer.Main succeeded.\n");
	}

	// Register native method for getUsbPermission
	JNINativeMethod nm[] = {
		{ "getVendorList",	"()[Ljava/lang/String;",			get_vendor_list	},
		{ "getProductList",	"(Ljava/lang/String;)[Ljava/lang/String;",	get_product_list}
	};

	if ((*env)->RegisterNatives(env, NativeHelper, nm , sizeof (nm) / sizeof (nm[0]))) {
	     LOGD ("RegisterNatives Failed.\n");
	     return -1;
	} else {
		LOGD("RegisterNatives Succeeded.\n");
	}

	return JNI_VERSION_1_6;
}


