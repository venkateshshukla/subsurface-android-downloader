package org.libdivecomputer;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;

public class Main extends Activity
{
	static {
		System.loadLibrary("subsurface_jni");
	}
	private native String[] getVendorList();
	private native String[] getProductList(String vndr);

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

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
	    super.onCreate(savedInstanceState);
	    setContentView(R.layout.main);
	    initialiseViews();
	    String[] vendorlist = getVendorList();
	}

	private void initialiseViews()
	{
		spVendor = (Spinner) findViewById(R.id.spnVendor);
		spProduct = (Spinner) findViewById(R.id.spnProduct);
		bOk = (Button) findViewById(R.id.bOK);
		bCancel = (Button) findViewById(R.id.bCancel);
		cbForce = (CheckBox) findViewById(R.id.cbForce);
		cbPrefer = (CheckBox) findViewById(R.id.cbPrefer);
		cbLogfile = (CheckBox) findViewById(R.id.cbLogFile);
		cbDumpfile = (CheckBox) findViewById(R.id.cbDumpFile);
	}

	public void onCheckboxClicked(View v)
	{
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

	public void onOkClicked(View v)
	{
	}

	public void onCancelClicked(View v)
	{
	}

}
