package org.libdivecomputer;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import org.libdivecomputer.R;

public class ImportProgress extends Activity {

        @Override
        protected void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                setContentView(R.layout.activity_import_progress);
        }


        @Override
        public boolean onCreateOptionsMenu(Menu menu) {
                getMenuInflater().inflate(R.menu.import_progress, menu);
                return true;
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.action_cancel) {
                        return true;
                }
                return super.onOptionsItemSelected(item);
        }

	public void onImportCancelClicked(MenuItem item) {
		// Undertake steps to safely cancel import and reset to
		// begenning
	}
}

