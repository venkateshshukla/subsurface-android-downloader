package org.libdivecomputer;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ProgressBar;
import android.widget.TextView;

public class ImportProgress extends Activity {
        static {
                System.loadLibrary("subsurface_jni");
        }

        private native void initDcData(DcData dc);

        private static final String TAG = "ImportProgress";
        private static final String DCDATA = "DivecomputerData";
        private static final int PROGRESSMAX = 1000;

        private DcData dcData;

        private TextView tvImportPercent;
        private TextView tvImportString;
        private ProgressBar pbImport;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                setContentView(R.layout.activity_import_progress);
                Bundle extras = getIntent().getExtras();
                if (extras == null) {
                        Log.d(TAG, "No extras found.");
                        finish();
                } else {
                        dcData = extras.getParcelable(DCDATA);
                }
                initialiseViews();
                pbImport.setMax(PROGRESSMAX);
                initDcData(dcData);
                DcImportTask importTask = new DcImportTask(dcData);
                importTask.execute();
        }

        private void initialiseViews() {
                tvImportPercent = (TextView) findViewById(R.id.tvImportPercent);
                tvImportString = (TextView) findViewById(R.id.tvImportString);
                pbImport = (ProgressBar) findViewById(R.id.pbImportProgress);
        }

        @Override
        public boolean onCreateOptionsMenu(Menu menu) {
                getMenuInflater().inflate(R.menu.import_progress, menu);
                return true;
        }

        public void onImportCancelClicked(MenuItem item) {
                // Undertake steps to safely cancel import and reset to
                // beginning
                // Close USB device. How?? Maybe natively.
                finish();
        }

        private class DcImportTask extends AsyncTask<Void, Integer, Boolean> {

                private DcData dcData;
                private native boolean doDcImport();

                public DcImportTask(DcData d) {
                        this.dcData = d;
                }

                @Override
                protected void onPreExecute() {
                        // logging and dumping is decided in the native part.
                }

                @Override
                protected void onPostExecute(Boolean result) {
                        super.onPostExecute(result);
                }

                @Override
                protected void onProgressUpdate(Integer... values) {
                        pbImport.setProgress(values[0]);
                }

                @Override
                protected void onCancelled() {
                        super.onCancelled();
                }

                @Override
                protected Boolean doInBackground(Void... params) {
                        return doDcImport();
                }
        }

}
