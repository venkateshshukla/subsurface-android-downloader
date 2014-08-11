package org.libdivecomputer;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;

public class DcData implements Parcelable {
        static {
                System.loadLibrary("libdivecomputer_jni");
        }

        private int fd;
        private String vendor;
        private String product;
        private boolean force;
        private boolean prefer;
        private boolean log;
        private boolean dump;
        private String logfilepath;
        private String outfilepath;

        private native void resetDcData();
        private native int setUsbFd(int usbFd);
        private native int setLogFile(String flname);
        private native int setDumpFile(String flname);
        private native int setXmlFile(String flname);
        private native int initDcContext();
        private native int initDcDescriptor(String vndr, String prdt);
        private native int interruptImport();
        private native int startImport();

        public void nativeResetDcData() {
                resetDcData();
        }

        public void nativeSetUsbFd(int usbFd) throws DcException {
                int ret = setUsbFd(this.fd);
                if (ret == -1)
                        throw new DcException("Invalid USB file descriptor.");
        }

        public void nativeSetLogFile(String flname) throws DcException {
                int ret = setLogFile(this.logfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException("Memory error");
                        case -2 :
                                throw new DcException("Null Filename");
                }
        }

        public void nativeSetDumpFile(String flname) throws DcException {
                int ret = setDumpFile(this.outfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException("Memory error");
                        case -2 :
                                throw new DcException("Null Filename");
                }
        }

        public void nativeSetXmlFile(String flname) throws DcException {
                int ret = setXmlFile(this.outfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException("Memory error");
                        case -2 :
                                throw new DcException("Null Filename");
                }
        }

        public void nativeInitDcContext() throws DcException {
                int ret = initDcContext();
                if (ret == -1) {
                        throw new DcException("Error initializing libdivecomputer context.");
                }
        }

        public void nativeInitDcDescriptor(String vndr, String prdt) throws DcException {
                int ret = initDcDescriptor(this.vendor, this.product);
                if (ret == -1) {
                        throw new DcException("No device found with given information.");
                }
        }

        public void nativeInterruptImport() throws DcException {
                int ret = interruptImport();
                if (ret != 0) {
                        throw new DcException("Error interrupting import.");
                }
        }

        public void nativeStartImport() throws DcException {
                int ret = startImport();
                if (ret == -1) {
                        throw new DcException("Error importing from Divecomputer.");
                }
        }

        public int getFd() {
                return fd;
        }

        public void setFd(int fd) throws DcException {
                if (fd <= 0) {
                        throw new DcException("Usb File Descriptor is invalid.");
                }
                this.fd = fd;
        }

        public String getVendor() {
                return vendor;
        }

        public void setVendor(String vendor) throws DcException {
                // This should never happen
                if (vendor.equals(null) || vendor.isEmpty()) {
                        throw new DcException("Empty vendor string is not allowed.");
                }
                this.vendor = vendor;
        }
        public String getProduct() {
                return product;
        }

        public void setProduct(String product) throws DcException {
                // This should never happen
                if (product.equals(null) || product.isEmpty()) {
                        throw new DcException("Empty product string is not allowed.");
                }
                this.product = product;
        }

        public boolean isForce() {
                return force;
        }

        public void setForce(boolean force) {
                this.force = force;
        }

        public boolean isPrefer() {
                return prefer;
        }

        public void setPrefer(boolean prefer) {
                this.prefer = prefer;
        }

        public boolean isLog() {
                return log;
        }

        public void setLog(boolean log) {
                this.log = log;
        }

        public boolean isDump() {
                return dump;
        }

        public void setDump(boolean dump) {
                this.dump = dump;
        }

        public String getLogfilepath() {
                return logfilepath;
        }

        public void setLogfilepath(String logfilepath) throws DcException {
                if (logfilepath.equals(null) || logfilepath.isEmpty()) {
                        throw new DcException("Empty logfile string.");
                }
                this.logfilepath = logfilepath;
        }

        public String getOutfilepath() {
                return outfilepath;
        }

        public void setOutfilepath(String dumpfilepath) throws DcException {
                if (dumpfilepath.equals(null) || dumpfilepath.isEmpty()) {
                        throw new DcException("Empty output file string.");
                }
                this.outfilepath = dumpfilepath;
        }

        public DcData(Context c) {
                super();
                this.fd = -1;
                this.vendor = null;
                this.product = null;
                this.force = false;
                this.prefer = false;
                this.dump = false;
                this.log = false;
                this.logfilepath = null;
                this.outfilepath = null;
        }

        @Override
        public int describeContents() {
                // TODO Auto-generated method stub
                return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
                dest.writeInt(fd);
                dest.writeString(vendor);
                dest.writeString(product);
                dest.writeByte((byte) (force ? 1 : 0));
                dest.writeByte((byte) (prefer ? 1 : 0));
                dest.writeByte((byte) (log ? 1 : 0));
                dest.writeByte((byte) (dump ? 1 : 0));
                dest.writeString(logfilepath);
                dest.writeString(outfilepath);
        }

        public static final Parcelable.Creator<DcData> CREATOR = new Parcelable.Creator<DcData>() {
                @Override
                public DcData createFromParcel(Parcel in) {
                        return new DcData(in);
                }

                @Override
                public DcData[] newArray(int size) {
                        return new DcData[size];
                }
        };

        private DcData(Parcel in) {
                fd = in.readInt();
                vendor = in.readString();
                product = in.readString();
                force = in.readByte() != 0;
                prefer = in.readByte() != 0;
                log = in.readByte() != 0;
                dump = in.readByte() != 0;
                logfilepath = in.readString();
                outfilepath = in.readString();
        }
}
