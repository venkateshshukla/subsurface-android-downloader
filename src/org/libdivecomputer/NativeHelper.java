package org.libdivecomputer;

public class NativeHelper {
	static {
		System.loadLibrary("nativehelper");
	}
	public static native String[] getVendorList();
	public static native String[] getProductList(String vendor);
}
