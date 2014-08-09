package org.libdivecomputer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

import org.libdivecomputer.ui.UsbListAdapter;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
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

public class Main extends Activity
                implements
                        OnItemSelectedListener,
                        OnClickListener {
        static {
                System.loadLibrary("subsurface_jni");
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
        private TextView tvLogfile;
        private TextView tvDumpfile;

        private HashMap<String, ArrayList<String>> deviceMap;
        private ArrayAdapter<String> vendorAdapter;
        private ArrayAdapter<String> productAdapter;
        private ArrayList<String> vendorList;
        private ArrayList<String> productList;

        private DcData dcData;

        private UsbManager usbManager;
        private HashMap<String, UsbDevice> usbDeviceMap;
        private UsbListAdapter usbListAdapter;

        private static final String TAG = "Main";
        private static final String DCDATA = "DivecomputerData";

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
                vendorAdapter = new ArrayAdapter<String>(this,
                                android.R.layout.simple_spinner_item,
                                vendorList);
                vendorAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                spVendor.setAdapter(vendorAdapter);

                // Fill the productlist spinner with the vendor names obtained
                // from libdivecomputers.
                productList.addAll(deviceMap.get(vendorList.get(0)));
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
                dcData = new DcData(this);
                vendorList = new ArrayList<String>();
                productList = new ArrayList<String>();
                deviceMap = new HashMap<String, ArrayList<String>>();
                usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
                usbListAdapter = new UsbListAdapter(this);
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
                                dcData.setLog(checked);
                                break;
                        case R.id.cbDumpFile :
                                dcData.setDump(checked);
                                break;
                }

        }

        public void onOkClicked(View v) {
                if (dcData.getVendor() == null || dcData.getProduct() == null) {
                        showInvalidDialog(R.string.dialog_error_invalid,
                                        R.string.dialog_invalid_none);
                        return;
                }
                if (!checkUsbDevice()) {
                        showInvalidDialog(R.string.dialog_error_invalid,
                                        R.string.dialog_invalid_nousb);
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

        private void putValDcData() {
                dcData.setPrefer(cbPrefer.isChecked());
                dcData.setForce(cbForce.isChecked());
                dcData.setLog(cbLogfile.isChecked());
                dcData.setDump(cbDumpfile.isChecked());
        }

        public void onCancelClicked(View v) {
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view,
                        int position, long id) {
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
                UsbDevice usbDevice = (UsbDevice) usbListAdapter.getItem(which);
                // Check if we have permission to use the USB device.
                if (!usbManager.hasPermission(usbDevice)) {
                        Log.d(TAG, "We don't have permission to use USB device");
                        showInvalidDialog(R.string.dialog_error_usb,
                                        R.string.dialog_error_usbpermission);
                        return;
                        // Get permission for device.
                }
                UsbDeviceConnection usbCon = usbManager.openDevice(usbDevice);
                if (usbCon == null) {
                        // Failed to open device.
                        Log.d(TAG,
                                        "Failed to open the device "
                                                        + usbDevice.toString());
                        return;
                }

                int fd = usbCon.getFileDescriptor();
                if (fd <= 0) {
                        // Some error during opening the device. Return.
                        showInvalidDialog(R.string.dialog_error_usb,
                                        R.string.dialog_error_usbpermission);
                        return;
                }
                dcData.setFd(fd);

                Intent i = new Intent(this, ImportProgress.class);
                i.putExtra(DCDATA, dcData);
                startActivity(i);
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
