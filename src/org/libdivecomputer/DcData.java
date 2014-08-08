package org.libdivecomputer;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;

public class DcData implements Parcelable {
        private String vendor;
        private String product;
        private boolean force;
        private boolean prefer;
        private boolean log;
        private boolean dump;
        private String logfilepath;
        private String dumpfilepath;

        public String getVendor() {
                return vendor;
        }

        public void setVendor(String vendor) {
                this.vendor = vendor;
        }

        public String getProduct() {
                return product;
        }

        public void setProduct(String product) {
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

        public void setLogfilepath(String logfilepath) {
                this.logfilepath = logfilepath;
        }

        public String getDumpfilepath() {
                return dumpfilepath;
        }

        public void setDumpfilepath(String dumpfilepath) {
                this.dumpfilepath = dumpfilepath;
        }

        public DcData(Context c) {
                super();
                this.vendor = null;
                this.product = null;
                this.force = false;
                this.prefer = false;
                this.dump = false;
                this.log = false;
                this.logfilepath = null;
                this.logfilepath = null;
        }

        @Override
        public int describeContents() {
                // TODO Auto-generated method stub
                return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
                dest.writeString(vendor);
                dest.writeString(product);
                dest.writeByte((byte) (force ? 1 : 0));
                dest.writeByte((byte) (prefer ? 1 : 0));
                dest.writeByte((byte) (log ? 1 : 0));
                dest.writeByte((byte) (dump ? 1 : 0));
                dest.writeString(logfilepath);
                dest.writeString(dumpfilepath);
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
                vendor = in.readString();
                product = in.readString();
                force = in.readByte() != 0;
                prefer = in.readByte() != 0;
                log = in.readByte() != 0;
                dump = in.readByte() != 0;
                logfilepath = in.readString();
                dumpfilepath = in.readString();
        }
}
