package com.example.bluetoothgatt;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.HashMap;
import java.util.List;
import java.util.UUID;

/**
 * Created by Dave Smith
 * Double Encore, Inc.
 * BeaconActivity
 */
public class BeaconKitKatActivity extends Activity implements BluetoothAdapter.LeScanCallback {
    private static final String TAG = "BeaconActivity";

    private BluetoothAdapter mBluetoothAdapter;
    /* Collect unique devices discovered, keyed by address */
    private HashMap<String, TemperatureBeacon> mBeacons;
    private BeaconAdapter mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        setProgressBarIndeterminate(true);

        /*
         * We are going to display all the device beacons that we discover
         * in a list, using a custom adapter implementation
         */
        ListView list = new ListView(this);
        mAdapter = new BeaconAdapter(this);
        list.setAdapter(mAdapter);
        setContentView(list);

        /*
         * Bluetooth in Android 4.3 is accessed via the BluetoothManager, rather than
         * the old static BluetoothAdapter.getInstance()
         */
        BluetoothManager manager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        mBluetoothAdapter = manager.getAdapter();

        mBeacons = new HashMap<String, TemperatureBeacon>();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /*
         * We need to enforce that Bluetooth is first enabled, and take the
         * user to settings to enable it if they have not done so.
         */
        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            //Bluetooth is disabled
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivity(enableBtIntent);
            finish();
            return;
        }

        /*
         * Check for Bluetooth LE Support.  In production, our manifest entry will keep this
         * from installing on these devices, but this will allow test devices or other
         * sideloads to report whether or not the feature exists.
         */
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "No LE Support.", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        //Begin scanning for LE devices
        startScan();
    }

    @Override
    protected void onPause() {
        super.onPause();
        //Cancel any scans in progress
        mHandler.removeCallbacks(mStopRunnable);
        mHandler.removeCallbacks(mStartRunnable);
        mBluetoothAdapter.stopLeScan(this);
    }

    private Runnable mStopRunnable = new Runnable() {
        @Override
        public void run() {
            stopScan();
        }
    };
    private Runnable mStartRunnable = new Runnable() {
        @Override
        public void run() {
            startScan();
        }
    };

    private void startScan() {
        //Scan for devices advertising the thermometer service
        mBluetoothAdapter.startLeScan(new UUID[] {TemperatureBeacon.THERM_SERVICE.getUuid()}, this);
        setProgressBarIndeterminateVisibility(true);

        mHandler.postDelayed(mStopRunnable, 5000);
    }

    private void stopScan() {
        mBluetoothAdapter.stopLeScan(this);
        setProgressBarIndeterminateVisibility(false);

        mHandler.postDelayed(mStartRunnable, 2500);
    }

    /* BluetoothAdapter.LeScanCallback */

    @Override
    public void onLeScan(BluetoothDevice device, int rssi, byte[] scanRecord) {
        Log.i(TAG, "New LE Device: " + device.getName() + " @ " + rssi);
        /*
         * We need to parse out of the AD structures from the scan record
         */
        List<AdRecord> records = AdRecord.parseScanRecord(scanRecord);
        if (records.size() == 0) {
            Log.i(TAG, "Scan Record Empty");
        } else {
            Log.i(TAG, "Scan Record: "
                    + TextUtils.join(",", records));
        }

        /*
         * Create a new beacon from the list of obtains AD structures
         * and pass it up to the main thread
         */
        TemperatureBeacon beacon = new TemperatureBeacon(records, device.getAddress(), rssi);
        mHandler.sendMessage(Message.obtain(null, 0, beacon));
    }

        /*
         * We have a Handler to process scan results on the main thread,
         * add them to our list adapter, and update the view
         */
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            TemperatureBeacon beacon = (TemperatureBeacon) msg.obj;
            mBeacons.put(beacon.getName(), beacon);

            mAdapter.setNotifyOnChange(false);
            mAdapter.clear();
            mAdapter.addAll(mBeacons.values());
            mAdapter.notifyDataSetChanged();
        }
    };

    /*
     * A custom adapter implementation that displays the TemperatureBeacon
     * element data in columns, and also varies the text color of each row
     * by the temperature values of the beacon
     */
    private static class BeaconAdapter extends ArrayAdapter<TemperatureBeacon> {

        public BeaconAdapter(Context context) {
            super(context, 0);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext())
                        .inflate(R.layout.item_beacon_list, parent, false);
            }

            TemperatureBeacon beacon = getItem(position);
            //Set color based on temperature
            final int textColor = getTemperatureColor(beacon.getCurrentTemp());

            TextView nameView = (TextView) convertView.findViewById(R.id.text_name);
            nameView.setText(beacon.getName());
            nameView.setTextColor(textColor);

            TextView tempView = (TextView) convertView.findViewById(R.id.text_temperature);
            tempView.setText(String.format("%.1f\u00B0C", beacon.getCurrentTemp()));
            tempView.setTextColor(textColor);

            TextView addressView = (TextView) convertView.findViewById(R.id.text_address);
            addressView.setText(beacon.getAddress());
            addressView.setTextColor(textColor);

            TextView rssiView = (TextView) convertView.findViewById(R.id.text_rssi);
            rssiView.setText(String.format("%ddBm", beacon.getSignal()));
            rssiView.setTextColor(textColor);

            return convertView;
        }

        private int getTemperatureColor(float temperature) {
            //Color range from 0 - 40 degC
            float clipped = Math.max(0f, Math.min(40f, temperature));

            float scaled = ((40f - clipped) / 40f) * 255f;
            int blue = Math.round(scaled);
            int red = 255 - blue;

            return Color.rgb(red, 0, blue);
        }
    }
}