package org.libdivecomputer;

public class DcData {
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
}
