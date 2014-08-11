package org.libdivecomputer;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

import org.libdivecomputer.ui.UsbListAdapter;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;

public class Main extends Activity implements OnItemSelectedListener, OnClickListener {
        static {
                System.loadLibrary("libdivecomputer_jni");
        }

        private native void getDeviceMap(HashMap<String, ArrayList<String>> hm);

        private Spinner spVendor;
        private Spinner spProduct;
        private Button bOk;
        private Button bCancel;
        private CheckBox cbForce;
        private CheckBox cbPrefer;
        private CheckBox cbLogfile;
        private CheckBox cbDumpfile;
        private EditText etXmlfile;
        private EditText etLogfile;
        private EditText etDumpfile;

        private HashMap<String, ArrayList<String>> deviceMap;
        private ArrayAdapter<String> vendorAdapter;
        private ArrayAdapter<String> productAdapter;
        private ArrayList<String> vendorList;
        private ArrayList<String> productList;

        private DcData dcData;

        private UsbManager usbManager;
        private UsbDevice usbDevice;
        private HashMap<String, UsbDevice> usbDeviceMap;
        private UsbListAdapter usbListAdapter;
        private PendingIntent usbPendingIntent;
        private BroadcastReceiver usbPermissionReceiver;

        private static final String TAG = "Main";
        private static final String DCDATA = "DivecomputerData";
        private static final String ACTION_USB_PERMISSION = "org.libdivecomputer.USB_PERMISSION";

        @Override
        public void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                setContentView(R.layout.activity_main);
                initialiseVars();
                initialiseViews();
                addListeners();

                getDeviceMap(deviceMap);

                // Fill the vendorlist spinner with the Vendor names obtained
                // from libdivecomputer
                vendorList.addAll(deviceMap.keySet());
                Collections.sort(vendorList);
                vendorAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, vendorList);
                vendorAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                spVendor.setAdapter(vendorAdapter);

                // Fill the productlist spinner with the vendor names obtained
                // from libdivecomputers.
                productList.addAll(deviceMap.get(vendorList.get(0)));
                productAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, productList);
                productAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                spProduct.setAdapter(productAdapter);

        }

        @Override
        protected void onResume() {
                refreshViews();
                super.onResume();
        }

        @Override
        protected void onDestroy() {
                try {
                        unregisterReceiver(usbPermissionReceiver);
                } catch (IllegalArgumentException e) {
                        // There is no registered reciever.
                        Log.e(TAG, e.toString());
                }
                super.onDestroy();
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
                etXmlfile = (EditText) findViewById(R.id.etXmlFile);
                etLogfile = (EditText) findViewById(R.id.etLogFile);
                etDumpfile = (EditText) findViewById(R.id.etDumpFile);
        }

        private void refreshViews() {
                boolean checked;
                checked = cbLogfile.isChecked();
                etLogfile.setEnabled(checked);
                checked = cbDumpfile.isChecked();
                etDumpfile.setEnabled(checked);
                etXmlfile.setEnabled(!checked);
        }

        private void initialiseVars() {
                dcData = new DcData(this);
                vendorList = new ArrayList<String>();
                productList = new ArrayList<String>();
                deviceMap = new HashMap<String, ArrayList<String>>();
                usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
                usbListAdapter = new UsbListAdapter(this);
                usbPermissionReceiver = new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                                String action = intent.getAction();
                                if (ACTION_USB_PERMISSION.equals(action)) {
                                        synchronized (this) {
                                                UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                                                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                                                        if (device != null) {
                                                                usbDevice = device;
                                                                openUsbAndImport();
                                                        }
                                                } else {
                                                        showInvalidDialog(R.string.dialog_error_usb, R.string.dialog_error_usbdenied);
                                                        Log.d(TAG, "permission denied for device " + device);
                                                }
                                        }
                                }
                        }
                };
                usbPendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
                IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
                registerReceiver(usbPermissionReceiver, filter);
        }

        private void addListeners() {
                spProduct.setOnItemSelectedListener(this);
                spVendor.setOnItemSelectedListener(this);
        }

        public void onCheckboxClicked(View v) {
                boolean checked = ((CheckBox) v).isChecked();
                switch (v.getId()) {
                        case R.id.cbForce :
                                dcData.setForce(checked);
                                break;
                        case R.id.cbPrefer :
                                dcData.setPrefer(checked);
                                break;
                        case R.id.cbLogFile :
                                etLogfile.setEnabled(checked);
                                dcData.setLog(checked);
                                break;
                        case R.id.cbDumpFile :
                                etDumpfile.setEnabled(checked);
                                etXmlfile.setEnabled(!checked);
                                dcData.setDump(checked);
                                break;
                }

        }

        public void onOkClicked(View v) {
                if (dcData.getVendor() == null || dcData.getProduct() == null) {
                        showInvalidDialog(R.string.dialog_error_invalid, R.string.dialog_invalid_none);
                        return;
                }
                if (!isExternalStorageWritable()) {
                        showInvalidDialog(R.string.dialog_error_storage, R.string.dialog_error_unwritable);
                }
                if (!checkUsbDevice()) {
                        showInvalidDialog(R.string.dialog_error_invalid, R.string.dialog_invalid_nousb);
                        return;
                }
                if (!createDiveFolder()) {
                        showInvalidDialog(R.string.dialog_error_storage, R.string.dialog_error_folder);
                        return;
                }
                putValDcData();
        }

        private boolean checkUsbDevice() {
                usbDeviceMap = usbManager.getDeviceList();
                if (usbDeviceMap.size() == 0) {
                        Log.d(TAG, "No USB device is attached.");
                        return false;
                }
                ArrayList<UsbDevice> al = new ArrayList<UsbDevice>();
                al.addAll(usbDeviceMap.values());
                usbListAdapter.setUsbList(al);
                showUsbListDialog();
                return true;
        }

        private boolean createDiveFolder() {
                String diveFolderName = getDiveFolderName();
                File diveFolder = new File(diveFolderName);
                if (diveFolder.exists() && diveFolder.isDirectory()) {
                        return true;
                } else {
                        return diveFolder.mkdir();
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

        private void putValDcData() {
                dcData.setPrefer(cbPrefer.isChecked());
                dcData.setForce(cbForce.isChecked());
                dcData.setLog(cbLogfile.isChecked());
                dcData.setDump(cbDumpfile.isChecked());
                String diveFolder = getDiveFolderName();
                if (dcData.isLog()) {
                        dcData.setLogfilepath(diveFolder + '/' + etLogfile.getText().toString());
                } else {
                        dcData.setLogfilepath(null);
                }
                if (dcData.isDump()) {
                        dcData.setOutfilepath(diveFolder + '/' + etDumpfile.getText().toString());
                } else {
                        dcData.setOutfilepath(diveFolder + '/' + etXmlfile.getText().toString());
                }
        }

        private String getDiveFolderName() {
                File ext = Environment.getExternalStorageDirectory();
                String divefolder = getDefaultFolderName();
                return ext.getAbsolutePath() + '/' + divefolder;
        }

        private String getDefaultFolderName() {
                // This should be done by using sharedpreferences. But for now,
                // we are using dives folder.
                return "Dives";
        }

        public void onCancelClicked(View v) {
                unregisterReceiver(usbPermissionReceiver);
                finish();
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                String s = parent.getItemAtPosition(position).toString();
                switch (parent.getId()) {
                        case R.id.spnVendor :
                                productList = deviceMap.get(s);
                                Collections.sort(productList);
                                productAdapter.clear();
                                productAdapter.addAll(productList);
                                dcData.setVendor(s);
                                break;
                        case R.id.spnProduct :
                                dcData.setProduct(s);
                                break;
                }

        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
                usbDevice = (UsbDevice) usbListAdapter.getItem(which);
                // Check if we have permission to use the USB device.
                if (!usbManager.hasPermission(usbDevice)) {
                        Log.d(TAG, "We don't have permission to use USB device");
                        // Get permission for device.
                        usbManager.requestPermission(usbDevice, usbPendingIntent);
                        return;
                }
                openUsbAndImport();
        }

        private void openUsbAndImport() {
                Log.d(TAG, "openUsbAndImport");
                UsbDeviceConnection usbCon = usbManager.openDevice(usbDevice);
                if (usbCon == null) {
                        // Failed to open device.
                        showInvalidDialog(R.string.dialog_error_usb, R.string.dialog_error_openusb);
                        Log.d(TAG, "Failed to open the device " + usbDevice.toString());
                        return;
                }

                int fd = usbCon.getFileDescriptor();
                if (fd <= 0) {
                        // Some error during opening the device. Return.
                        showInvalidDialog(R.string.dialog_error_usb, R.string.dialog_error_openusb);
                        return;
                }
                dcData.setFd(fd);

                Log.d(TAG, "openUsbAndImport closed");
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }

        private void showInvalidDialog(int title, int message) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setMessage(message).setTitle(title);
                AlertDialog dialog = builder.create();
                dialog.show();
        }

        private void showUsbListDialog() {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(R.string.dialog_usb_title);
                builder.setAdapter(usbListAdapter, this);
                AlertDialog dialog = builder.create();
                dialog.show();
        }
}
