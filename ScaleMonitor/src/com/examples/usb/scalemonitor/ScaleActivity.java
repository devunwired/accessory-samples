/**
 * ScaleActivity monitors a USB HID Point of Sale
 * weight scale device.  It interprets the scale data
 * packet and displays the resulting weight on the screen.
 *
 * USB HID Class Specification:
 * http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 *
 * USB HID POS Specification:
 * http://www.usb.org/developers/devclass_docs/pos1_02.pdf
 */

package com.examples.usb.scalemonitor;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import java.util.HashMap;

public class ScaleActivity extends Activity implements Runnable {

    private static final String TAG = "ScaleMonitor";

    // Dymo USB VID/PID values
    // These are the same as the XML filter
    private static final int VID = 24726;
    private static final int PID = 344;

    private TextView mWeight, mStatus, mAttributes;
    private PendingIntent mPermissionIntent;

    private int mWeightLimit;

    private UsbManager mUsbManager;
    private UsbDevice mDevice;
    private UsbDeviceConnection mConnection;
    private UsbEndpoint mEndpointIntr;

    private boolean mRunning = false;

    // USB HIS POS Constants
    private static final int UNITS_MILLIGRAM = 0x01;
    private static final int UNITS_GRAM = 0x02;
    private static final int UNITS_KILOGRAM = 0x03;
    private static final int UNITS_CARAT = 0x04;
    private static final int UNITS_TAEL = 0x05;
    private static final int UNITS_GRAIN = 0x06;
    private static final int UNITS_PENNYWEIGHT = 0x07;
    private static final int UNITS_MTON = 0x08;
    private static final int UNITS_ATON = 0x09;
    private static final int UNITS_TROYOUNCE = 0x0A;
    private static final int UNITS_OUNCE = 0x0B;
    private static final int UNITS_POUND = 0x0C;

    private static final int STATUS_FAULT = 0x01;
    private static final int STATUS_STABLEZERO = 0x02;
    private static final int STATUS_INMOTION = 0x03;
    private static final int STATUS_STABLE = 0x04;
    private static final int STATUS_UNDERZERO = 0x05;
    private static final int STATUS_OVERLIMIT = 0x06;
    private static final int STATUS_NEEDCAL = 0x07;
    private static final int STATUS_NEEDZERO = 0x08;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.launcher);
        mWeight = (TextView) findViewById(R.id.text_weight);
        mStatus = (TextView) findViewById(R.id.text_status);
        mAttributes = (TextView) findViewById(R.id.text_attributes);

        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        //Create the intent to fire with permission results
        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);

        Intent intent = getIntent();
        String action = intent.getAction();
        /* Check if this was launched from a device attached intent, and
         * obtain the device instance if it was.  We do this in onStart()
         * because permissions dialogs may pop up as a result, which will
         * cause an infinite loop in onResume().
         */
        UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
            setDevice(device);
        } else {
            searchForDevice();
        }
    }

    @Override
    public void onStart() {
        super.onStart();

        //Keep the screen on while using the application
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        //Register receiver for further events
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(mUsbReceiver, filter);
    }
    
    @Override
    protected void onStop() {
        super.onStop();
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        unregisterReceiver(mUsbReceiver);
        mRunning = false;
    }

    private void searchForDevice() {
        //If we find our device already attached, connect to it
        HashMap<String, UsbDevice> devices = mUsbManager.getDeviceList();
        UsbDevice selected = null;
        for (UsbDevice device : devices.values()) {
            if (device.getVendorId() == VID && device.getProductId() == PID) {
                selected = device;
                break;
            }
        }
        //Request a connection
        if (selected != null) {
            if (mUsbManager.hasPermission(selected)) {
                setDevice(selected);
            } else {
                mUsbManager.requestPermission(selected, mPermissionIntent);
            }
        }
    }

    private static final String ACTION_USB_PERMISSION = "com.examples.usb.scalemonitor.USB_PERMISSION";
    private BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            //If our device is detached, disconnect
            if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                Log.i(TAG, "Device Detached");
                if (mDevice != null && mDevice.equals(device)) {
                    setDevice(null);
                }
            }
            //If a new device is attached, connect to it
            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                Log.i(TAG, "Device Attached");
                mUsbManager.requestPermission(device, mPermissionIntent);
            }
            //If this is our permission request, check result
            if (ACTION_USB_PERMISSION.equals(action)) {
                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                        && device != null) {
                    //Connect to the device
                    setDevice(device);
                } else {
                    Log.d(TAG, "permission denied for device " + device);
                    setDevice(null);
                }
            }
        }
    };

    public void onZeroClick(View view) {
        if (mConnection != null) {
            setZeroScale(mConnection);
        }
    }

    /*
     * Methods to update the UI with the weight value read
     * from the USB scale.
     */

    private void resetDisplay() {
        mWeight.setText("---");
        setWeightColor(0);

        mStatus.setText("{ }");
        mAttributes.setText("{ }");
    }

    private void setWeightColor(int status) {
        switch(status) {
        case STATUS_FAULT:
        case STATUS_UNDERZERO:
            mWeight.setTextColor(Color.RED);
            break;
        case STATUS_STABLE:
        case STATUS_STABLEZERO:
            mWeight.setTextColor(Color.GREEN);
            break;
        default:
            mWeight.setTextColor(Color.WHITE);
            break;
        }
    }
    
    // Value is the number of oz. times 100
    private static final int OZ_PER_POUND = 160; // 16oz per 0.1lb
    private void updateWeightPounds(int weight) {
        String result;

        if (weight >= OZ_PER_POUND) {
            float pounds = weight / OZ_PER_POUND;
            float remainder = (weight % OZ_PER_POUND) / 10.0f;
            result = String.format("%.0f lb, %.1f oz", pounds, remainder);
        } else {
            float ounces = weight / 10.0f;
            result = String.format("%.1f oz", ounces);
        }

        mWeight.setText(result);
    }

    // Value is the number of grams
    private static final int GRAMS_PER_KG = 1000;
    private void updateWeightGrams(int weight) {
        String result;

        if (weight >= GRAMS_PER_KG) {
            float kg = weight / GRAMS_PER_KG;
            float remainder = (weight % GRAMS_PER_KG);
            result = String.format("%.0f kg, %.1f g", kg, remainder);
        } else {
            float grams = weight;
            result = String.format("%.0f g", grams);
        }

        mWeight.setText(result);
    }

    /*
     * Enumerate the endpoints and interfaces on the connected
     * device to find the Interrupt endpoint we need to poll
     * for the scale data.
     */
    private void setDevice(UsbDevice device) {
        Log.d(TAG, "setDevice " + device);
        if (device == null) {
            //Cancel connections
            mConnection = null;
            mRunning = false;

            resetDisplay();
            return;
        }

        //Verify the device has what we need
        if (device.getInterfaceCount() != 1) {
            Log.e(TAG, "could not find interface");
            return;
        }
        UsbInterface intf = device.getInterface(0);
        // device should have one endpoint
        if (intf.getEndpointCount() != 1) {
            Log.e(TAG, "could not find endpoint");
            return;
        }
        // endpoint should be of type interrupt
        UsbEndpoint ep = intf.getEndpoint(0);
        if (ep.getType() != UsbConstants.USB_ENDPOINT_XFER_INT
                || ep.getDirection() != UsbConstants.USB_DIR_IN) {
            Log.e(TAG, "endpoint is not Interrupt IN");
            return;
        }
        mDevice = device;
        mEndpointIntr = ep;

        UsbDeviceConnection connection = mUsbManager.openDevice(device);
        if (connection != null && connection.claimInterface(intf, true)) {
            mConnection = connection;

            //Get scale attributes
            updateAttributesData(mConnection);
            //Update the weight limit information
            updateLimitData(mConnection);

            //Start the polling thread
            mRunning = true;
            Thread thread = new Thread(null, this, "ScaleMonitor");
            thread.start();
        } else {
            mConnection = null;
            mRunning = false;
        }
    }

    private String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for(byte b : bytes) {
            sb.append(String.format("%02X ", b));
        }

        return sb.toString();
    }

    private static final int MSG_DATA = 101;
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_DATA:
                byte[] data = (byte[]) msg.obj;
                Log.d(TAG, "Raw: "+bytesToHex(data));

                //This value will always be 0x03 for the data report
                byte reportId = data[0];
                //Maps to a status constant defined for HID POS
                byte status = data[1];
                setWeightColor(status);

                //Maps to a units constant defined for HID POS
                int units = (data[2] & 0xFF);

                //Scaling applied to the weight value, if any
                byte scaling = data[3];

                //Two byte value representing the weight itself
                int weight = (data[5] & 0xFF) << 8;
                weight += (data[4] & 0xFF);

                //Validate result
                if (mWeightLimit > 0 && weight > mWeightLimit) {
                    mWeight.setText("---");
                    return;
                }

                switch (units) {
                    case UNITS_GRAM:
                        updateWeightGrams(weight);
                        break;
                    case UNITS_OUNCE:
                        updateWeightPounds(weight);
                        break;
                    default:
                        mWeight.setText("---");
                        break;
                }
                break;
            default:
                break;
            }
        }
    };

    public void postWeightData(byte[] data) {
        mHandler.sendMessage(Message.obtain(mHandler, MSG_DATA, data));
    }

    /*
     * Get the current measurement for the scale by requesting a
     * Scale Data Input Report
     */
    private void updateWeightData(UsbDeviceConnection connection, byte[] buffer) {
        int requestType = 0xA1; // 1010 0001b
        int request = 0x01; //HID GET_REPORT
        int value = 0x0103; //Input report, ID = 3
        int index = 0; //Interface 0
        int length = 6;

        connection.controlTransfer(requestType, request, value, index, buffer, length, 2000);
    }

    /*
     * Get the attribute information for the scale by requesting a
     * Scale Attributes Feature Report
     */
    private void updateAttributesData(UsbDeviceConnection connection) {
        int requestType = 0xA1; // 1010 0001b
        int request = 0x01; //HID GET_REPORT
        int value = 0x0301; //Feature report, ID = 1
        int index = 0; //Interface 0
        int length = 3;

        byte[] buffer = new byte[length];
        connection.controlTransfer(requestType, request, value, index, buffer, length, 2000);

        //Parse the data and fill the display
        byte reportId = buffer[0]; //Always 0x01

        int scaleClass = (buffer[1] & 0xFF);
        //Maps to a units constant defined for HID POS
        int defaultUnits = (buffer[2] & 0xFF);

        String message = String.format("Class: %d, Default Units: %s", scaleClass, getUnitDisplayString(defaultUnits));
        Log.d(TAG, message);
        mAttributes.setText(message);
    }

    /*
     * Get the weight limit of the connected scale by requesting a
     * Weight Limit Feature Report.
     */
    private void updateLimitData(UsbDeviceConnection connection) {
        int requestType = 0xA1; // 1010 0001b
        int request = 0x01; //HID GET_REPORT
        int value = 0x0305; //Feature report, ID = 5
        int index = 0; //Interface 0
        int length = 5;

        byte[] buffer = new byte[length];
        connection.controlTransfer(requestType, request, value, index, buffer, length, 2000);

        //Parse the data and fill the display
        byte reportId = buffer[0]; //Always 0x05
        //Maps to a units constant defined for HID POS
        int unit = (buffer[1] & 0xFF);
        //Scaling applied to the weight value, if any
        byte scaling = buffer[2];

        //Two byte value representing the weight itself
        int weight = (buffer[4] & 0xFF) << 8;
        weight += (buffer[3] & 0xFF);
        mWeightLimit = weight;

        String message = String.format("Weight Limit: %d %s", weight, getUnitDisplayString(unit));
        Log.d(TAG, message);
        mStatus.setText(message);
    }

    /*
     * Set the scale to zero its current reading by sending a
     * Scale Control Report
     */
    private void setZeroScale(UsbDeviceConnection connection) {
        int requestType = 0x21; // 0010 0001b
        int request = 0x09; //HID SET_REPORT
        int value = 0x0302; //Feature report, ID = 2
        int index = 0; //Interface 0
        int length = 2;

        byte[] buffer = new byte[length];
        buffer[0] = 0x02; //Report ID = 2
        buffer[1] = 0x02; //Set the zero flag

        connection.controlTransfer(requestType, request, value, index, buffer, length, 2000);
    }

    private String getUnitDisplayString(int unitConstant) {
        switch (unitConstant) {
            case UNITS_MILLIGRAM:
                return "Milligrams";
            case UNITS_GRAM:
                return "Grams";
            case UNITS_KILOGRAM:
                return "Kilograms";
            case UNITS_CARAT:
                return "Carats";
            case UNITS_TAEL:
                return "Taels";
            case UNITS_GRAIN:
                return "Grains";
            case UNITS_PENNYWEIGHT:
                return "Pennyweights";
            case UNITS_MTON:
                return "Metric Tons";
            case UNITS_ATON:
                return "Avoir Tons";
            case UNITS_TROYOUNCE:
                return "Troy Ounces";
            case UNITS_OUNCE:
                return "Ounces";
            case UNITS_POUND:
                return "Pounds";
            default:
                return "";
        }
    }
    
    /*
     * Runnable block to poll the interrupt endpoint on the scale.
     * This will return a Scale Data Report, which we pass on to
     * the Handler for parsing.
     */
    @Override
    public void run() {
        byte[] buffer = new byte[6];
        while (mRunning) {
            updateWeightData(mConnection, buffer);
            postWeightData(buffer);

            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
                Log.w(TAG, "Read Interrupted");
            }
        }
    }
}
