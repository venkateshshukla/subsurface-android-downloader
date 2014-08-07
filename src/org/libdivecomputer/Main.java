package org.libdivecomputer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;

public class Main extends Activity implements OnItemSelectedListener {
        static {
                System.loadLibrary("subsurface_jni");
        }

        private native void getDeviceMap(HashMap<String, ArrayList<String>> hm);

        private String vendor;
        private String product;
        private boolean force;
        private boolean prefer;
        private boolean logfile;
        private boolean dumpfile;
        private String logfilepath;
        private String dumpfilepath;

        private Spinner spVendor;
        private Spinner spProduct;
        private Button bOk;
        private Button bCancel;
        private CheckBox cbForce;
        private CheckBox cbPrefer;
        private CheckBox cbLogfile;
        private CheckBox cbDumpfile;
        private TextView tvLogfile;
        private TextView tvDumpfile;

        private HashMap<String, ArrayList<String>> deviceMap;
        private ArrayAdapter<String> vendorAdapter;
        private ArrayAdapter<String> productAdapter;
        private ArrayList<String> vendorList;
        private ArrayList<String> productList;

        private static final String TAG = "Main";

        @Override
        public void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                setContentView(R.layout.activity_main);

		initialiseVars();
                initialiseViews();
                addListeners();

                getDeviceMap(deviceMap);

                vendorList.addAll(deviceMap.keySet());
                Collections.sort(vendorList);
                vendorAdapter = new ArrayAdapter<String>(this,
                                android.R.layout.simple_spinner_item,
                                vendorList);
                vendorAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                spVendor.setAdapter(vendorAdapter);

                productList.addAll(deviceMap.get(vendorList.get(0)));
                Collections.sort(productList);
                productAdapter = new ArrayAdapter<String>(this,
                                android.R.layout.simple_spinner_item,
                                productList);
                productAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                spProduct.setAdapter(productAdapter);
        }

        @Override
        public boolean onCreateOptionsMenu(Menu menu) {
                getMenuInflater().inflate(R.menu.main, menu);
                return true;
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.action_settings) {
                        return true;
                }
                return super.onOptionsItemSelected(item);
        }

        private void initialiseViews() {
                spVendor = (Spinner) findViewById(R.id.spnVendor);
                spProduct = (Spinner) findViewById(R.id.spnProduct);
                bOk = (Button) findViewById(R.id.bOK);
                bCancel = (Button) findViewById(R.id.bCancel);
                cbForce = (CheckBox) findViewById(R.id.cbForce);
                cbPrefer = (CheckBox) findViewById(R.id.cbPrefer);
                cbLogfile = (CheckBox) findViewById(R.id.cbLogFile);
                cbDumpfile = (CheckBox) findViewById(R.id.cbDumpFile);
        }

        private void initialiseVars() {
                vendor = "";
                product = "";
                force = false;
                prefer = false;
                logfile = false;
                dumpfile = false;
                logfilepath = "";
                dumpfilepath = "";
		vendorList = new ArrayList<String>();
		productList = new ArrayList<String>();
                deviceMap = new HashMap<String, ArrayList<String>>();
        }

        private void addListeners() {
                spProduct.setOnItemSelectedListener(this);
                spVendor.setOnItemSelectedListener(this);
        }

        public void onCheckboxClicked(View v) {
                boolean checked = ((CheckBox) v).isChecked();
                switch (v.getId()) {
                case R.id.cbForce:
                        if (checked) {
                        } else {
                        }
                        break;
                case R.id.cbPrefer:
                        if (checked) {
                        } else {
                        }
                        break;
                case R.id.cbLogFile:
                        if (checked) {
                        } else {
                        }
                        break;
                case R.id.cbDumpFile:
                        if (checked) {
                        } else {
                        }
                        break;
                }

        }

        public void onOkClicked(View v) {
                Intent in = new Intent(this, ImportProgress.class);
                startActivity(in);
        }

        public void onCancelClicked(View v) {
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view,
                        int position, long id) {
                String s = parent.getItemAtPosition(position).toString();
                switch (parent.getId()) {
                case R.id.spnVendor:
                        productList = deviceMap.get(s);
                        Collections.sort(productList);
                        productAdapter.clear();
                        productAdapter.addAll(productList);
                        vendor = s;
                        break;
                case R.id.spnProduct:
                        product = s;
                        break;
                }

        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub

        }

}
