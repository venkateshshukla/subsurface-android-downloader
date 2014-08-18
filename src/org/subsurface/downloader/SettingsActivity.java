package org.subsurface.downloader;

import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.preference.PreferenceFragment;

public class SettingsActivity extends Activity {

        public static class SettingsFragment extends PreferenceFragment {
                @Override
                public void onCreate(Bundle savedInstanceState) {
                        super.onCreate(savedInstanceState);
                        addPreferencesFromResource(R.xml.preferences);

                }
        }

        @Override
        protected void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                ActionBar actionBar = getActionBar();
                actionBar.setDisplayHomeAsUpEnabled(true);
                getFragmentManager().beginTransaction().replace(android.R.id.content, new SettingsFragment()).commit();
        }

}
