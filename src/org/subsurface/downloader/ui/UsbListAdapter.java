package org.subsurface.downloader.ui;

import java.util.ArrayList;

import org.subsurface.downloader.R;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class UsbListAdapter extends BaseAdapter {
        private static final String TAG = "UsbListAdapter";
        private ArrayList<UsbDevice> devicelist;
        private final LayoutInflater inflater;

        public UsbListAdapter(Context context) {
                super();
                this.devicelist = new ArrayList<UsbDevice>();
                this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        public UsbListAdapter(Context context, ArrayList<UsbDevice> devicelist) {
                super();
                this.devicelist = devicelist;
                this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public int getCount() {
                return devicelist.size();
        }

        @Override
        public Object getItem(int position) {
                return devicelist.get(position);
        }

        @Override
        public long getItemId(int position) {
                return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
                ViewHolder holder = null;
                if (convertView == null) {
                        convertView = inflater.inflate(R.layout.usblistitem, parent, false);
                        holder = new ViewHolder();
                        holder.tvUsbDevName = (TextView) convertView.findViewById(R.id.tvUsbDevName);
                        holder.tvUsbDevPID = (TextView) convertView.findViewById(R.id.tvUsbDevPID);
                        holder.tvUsbDevVID = (TextView) convertView.findViewById(R.id.tvUsbDevVID);
                        convertView.setTag(holder);
                } else {
                        holder = (ViewHolder) convertView.getTag();
                }

                String nm = devicelist.get(position).getDeviceName();
                String[] ln = nm.split("/");
                String name = ln[ln.length - 1];
                int pid = devicelist.get(position).getProductId();
                int vid = devicelist.get(position).getVendorId();

                holder.tvUsbDevName.setText(name);
                holder.tvUsbDevPID.setText(String.format("%04x", pid));
                holder.tvUsbDevVID.setText(String.format("%04x", vid));
                return convertView;
        }
        public static class ViewHolder {
                public TextView tvUsbDevName;
                public TextView tvUsbDevPID;
                public TextView tvUsbDevVID;
        }

        public void setUsbList(ArrayList<UsbDevice> al) {
                this.devicelist = al;
        }

}
