package org.libdivecomputer;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

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
                if (!isExternalStorageWritable()) {
                        Log.d(TAG, "External storage not writable.");
                        Toast.makeText(this,
                                        "External storage is not writable.",
                                        Toast.LENGTH_LONG).show();
                        finish();
                }
                initDcData(dcData);
                DcImportTask importTask = new DcImportTask(dcData);
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
        }

        private class DcImportTask extends AsyncTask<DcData, Integer, Boolean> {

                private DcData dcData;

                public DcImportTask(DcData d) {
                        this.dcData = d;
                }

                @Override
                protected void onPreExecute() {
                        if (dcData.isLog()) {
                                // Open logfile for saving divelogs.
                        }
                        if (dcData.isDump()) {
                                // Open dumpfile for saving dump.
                        } else {
                                // Open xml file for saving divelogs
                        }
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
                protected Boolean doInBackground(DcData... params) {

                        return null;
                }

        }

        /* Checks if external storage is available for read and write */
        public boolean isExternalStorageWritable() {
                String state = Environment.getExternalStorageState();
                if (Environment.MEDIA_MOUNTED.equals(state)) {
                        return true;
                }
                return false;
        }
}
