package org.libdivecomputer;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;

public class DcData implements Parcelable {
        static {
                System.loadLibrary("subsurface_jni");
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

        private native void resetDcData();
        private native int setUsbFd(int usbFd);
        private native void setDcPrefer(boolean p);
        private native void setDcForce(boolean f);
        private native void setDcLog(boolean l);
        private native void setDcDump(boolean d);
        private native int setVendorName(String v);
        private native int setProductName(String p);
        private native int setLogFile(String flname);
        private native int setDumpFile(String flname);
        private native int setXmlFile(String flname);
        private native void doDcImport();
        private native void doProcessDives();
        private native int doSaveDives();

        public void nativeResetDcData() {
                resetDcData();
        }

        public void nativeSetUsbFd(int usbFd) throws DcException {
                int ret = setUsbFd(this.fd);
                if (ret == -1)
                        throw new DcException(R.string.dcdata_invalid_usb);
        }

        public void nativeSetDcPrefer(boolean p) {
                setDcPrefer(this.prefer);
        }

        public void nativeSetDcForce(boolean f) {
                setDcForce(this.force);
        }

        public void nativeSetDcLog(boolean l) {
                setDcForce(this.log);
        }

        public void nativeSetDcDump(boolean d) {
                setDcDump(this.dump);
        }

        public void nativeSetVendorName(String v) throws DcException{
            int ret = setVendorName(this.vendor);
            switch (ret) {
                    case -1 :
                            throw new DcException(R.string.dcdata_error_memory);
                    case -2 :
                            throw new DcException(R.string.dcdata_null_file);
            }
        }

        public void nativeSetProductName(String p) throws DcException{
            int ret = setProductName(this.product);
            switch (ret) {
                    case -1 :
                            throw new DcException(R.string.dcdata_error_memory);
                    case -2 :
                            throw new DcException(R.string.dcdata_null_file);
            }
        }

        public void nativeSetLogFile(String flname) throws DcException {
                int ret = setLogFile(this.logfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException(R.string.dcdata_error_memory);
                        case -2 :
                                throw new DcException(R.string.dcdata_null_file);
                }
        }

        public void nativeSetDumpFile(String flname) throws DcException {
                int ret = setDumpFile(this.outfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException(R.string.dcdata_error_memory);
                        case -2 :
                                throw new DcException(R.string.dcdata_null_file);
                }
        }

        public void nativeSetXmlFile(String flname) throws DcException {
                int ret = setXmlFile(this.outfilepath);
                switch (ret) {
                        case -1 :
                                throw new DcException(R.string.dcdata_error_memory);
                        case -2 :
                                throw new DcException(R.string.dcdata_null_file);
                }
        }

        public void nativeDoDcImport() {
                doDcImport();
        }

        public void nativeDoProcessDives() {
                doProcessDives();
        }

        public void nativeDoSaveDives() throws DcException {
                int ret = doSaveDives();
                if (ret != 0) {
                        throw new DcException(R.string.dcdata_error_savedives);
                }
        }

        public int getFd() {
                return fd;
        }

        public void setFd(int fd) throws DcException {
                if (fd <= 0) {
                        throw new DcException(R.string.dcdata_invalid_usb);
                }
                this.fd = fd;
        }

        public String getVendor() {
                return vendor;
        }

        public void setVendor(String vendor) throws DcException {
                // This should never happen
                if (vendor == null || vendor.isEmpty()) {
                        throw new DcException(R.string.dcdata_empty_vendor);
                }
                this.vendor = vendor;
        }
        public String getProduct() {
                return product;
        }

        public void setProduct(String product) throws DcException {
                // This should never happen
                if (product == null || product.isEmpty()) {
                        throw new DcException(R.string.dcdata_empty_product);
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
                if (logfilepath == null || logfilepath.isEmpty()) {
                        throw new DcException(R.string.dcdata_error_logfile);
                }
                this.logfilepath = logfilepath;
        }

        public String getOutfilepath() {
                return outfilepath;
        }

        public void setOutfilepath(String dumpfilepath) throws DcException {
                if (dumpfilepath == null || dumpfilepath.isEmpty()) {
                        throw new DcException(R.string.dcdata_empty_outfile);
                }
                this.outfilepath = dumpfilepath;
        }

        public void validateData() throws DcException {
                if (log && (logfilepath == null || logfilepath.isEmpty())) {
                        throw new DcException(R.string.dcdata_error_logfile);
                }
                if (outfilepath == null || outfilepath.isEmpty()) {
                        if (dump) {
                                throw new DcException(R.string.dcdata_error_dumpfile);
                        } else {
                                throw new DcException(R.string.dcdata_error_xmlfile);
                        }
                }
                // This should never happen.
                if (vendor == null || vendor.isEmpty()) {
                        throw new DcException(R.string.dcdata_empty_vendor);
                }
                if (product == null || product.isEmpty()) {
                        throw new DcException(R.string.dcdata_empty_product);
                }
                if (fd <= 0) {
                        throw new DcException(R.string.dcdata_invalid_usb);
                }
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
