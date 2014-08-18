package org.subsurface.downloader;

import java.io.File;

import android.app.AlertDialog;
import android.os.AsyncTask;
import android.widget.Toast;

public class DcImportTask extends AsyncTask<Void, Void, Boolean> {

        // The calling activity. Useful for UI tasks.
        private Main activity;
        private DcData dcData;

        public DcImportTask(Main activity, DcData dcData) {
                this.activity = activity;
                this.dcData = dcData;
        }

        private void showErrorDialog(int title, int message) {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                builder.setTitle(title);
                builder.setMessage(message);
                AlertDialog dialog = builder.create();
                dialog.show();
        }

        @Override
        protected void onPreExecute() {
                // Check once more before execution of import.
                try {
                        dcData.validateData();
                } catch (DcException e) {
                        showErrorDialog(R.string.error, Integer.valueOf(e.getMessage()));
                }
                activity.setProgressBarIndeterminateVisibility(true);
                super.onPreExecute();
        }

        @Override
        protected Boolean doInBackground(Void... params) {
                try {
                        // If dive file of the same name is already stored, load
                        // it first.
                        String diveFileName = dcData.getOutfilepath();
                        File diveFile = new File(diveFileName);
                        if (!dcData.isDump() && diveFile.exists()) {
                                dcData.nativeDoParseDives();
                        }
                        // Download dive data from the dive computer.
                        dcData.nativeDoDcImport();
                        // Process the dives to extract valuable information.
                        dcData.nativeDoProcessDives();
                        // Save the dives in form of an XML file.
                        dcData.nativeDoSaveDives();
                } catch (DcException e) {
                        return false;
                }
                return true;
        }

        @Override
        protected void onPostExecute(Boolean result) {
                activity.setProgressBarIndeterminateVisibility(false);
                activity.finishImport();
                if (!result) {
                        showErrorDialog(R.string.error, R.string.dcdata_import_error);

                } else {
                        Toast.makeText(activity, R.string.dcdata_import_success, Toast.LENGTH_SHORT).show();
                }
                super.onPostExecute(result);
        }

        @Override
        protected void onProgressUpdate(Void... values) {
                // TODO Auto-generated method stub
                super.onProgressUpdate(values);
        }

        @Override
        protected void onCancelled(Boolean result) {
                activity.setProgressBarIndeterminateVisibility(false);
                activity.finishImport();
                showErrorDialog(R.string.error, R.string.dcdata_import_interrupted);
                super.onCancelled(result);
        }

}
