/**
 * MainBluetoothActivity is a GameActivity that monitors a Bluetooth
 * accessory device and uses the input from that device to control
 * the game.
 */

package com.examples.accessory.controller;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelUuid;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.UUID;

public class MainBluetoothActivity extends GameActivity implements Runnable {

    //Friendly name of the Bluetooth device we are looking to connect with
    private static final String CONTROLLER_NAME = "adccontroller";
    private static final int CONTROLLER_CLASS = BluetoothClass.Device.TOY_CONTROLLER;
    //RFCOMM Service UUID on our device
    private static final UUID BT_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private static final int MESSAGE_SWITCH = 1;
    private static final int MESSAGE_TEMPERATURE = 2;
    private static final int MESSAGE_LIGHT = 3;
    private static final int MESSAGE_JOY = 4;
    private static final int MESSAGE_VIBE = 5;
    private static final int MESSAGE_CONNECTED = 100;
    private static final int MESSAGE_CONNECTFAIL = 101;
    
    ProgressDialog mProgress;
    BluetoothDevice mControllerDevice;
    BluetoothSocket mSocket;
    InputStream mInputStream;
    OutputStream mOutputStream;    

    @Override
    protected void onResume() {
        super.onResume();
        mSocket = null;
    }

    @Override
    protected void onPause() {
        super.onPause();
        try {
            mSocket.close();
        } catch (Exception e) { }
    }

    @Override
    protected boolean isControllerConnected() {
        return (mSocket != null);
    }

    @Override
    protected void sendVibeControl(boolean longDuration) {
        byte[] command = {0x02,
                longDuration ? (byte)0x64 : (byte)0x32,
                0x00};
        Message msg = Message.obtain(null, MESSAGE_VIBE, command);
        mHandler.sendMessage(msg);
    }

    @Override
    protected void hideControls() {
        setContentView(R.layout.no_device_bluetooth);
        super.hideControls();
    }

    public void onConnectClick(View v) {
        //Register for the device discovery callbacks
        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        registerReceiver(mDiscoveryReceiver, filter);
        //Begin device discovery
        BluetoothAdapter.getDefaultAdapter().startDiscovery();
        mProgress = ProgressDialog.show(this, "", "Scanning for Devices...", true);
    }

    /*
     * Helper method to create a polling thread for the accessory and
     * load the game.
     */
    private void beginGame() {
        //Create a new thread using this Activity's Runnable as the execution block
        Thread thread = new Thread(null, this, "BluetoothController");
        thread.start();
        //Show the game view
        Log.d(TAG, "Bluetooth Connected");
        enableControls(true);
    }
    
    /*
     * This receiver will listen during the discovery process for each device located.
     * If a device with the same friendly name as our controller is found, we abort
     * discovery and move to the connecting phase.  If no device has been located by
     * the time discovery finishes, we alert the user.
     */
    private BroadcastReceiver mDiscoveryReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (BluetoothDevice.ACTION_FOUND.equals(intent.getAction())) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                Log.d("AccessoryController", "Device Found: "+device.getName());
                if (CONTROLLER_NAME.equals(device.getName())
                        && CONTROLLER_CLASS == device.getBluetoothClass().getDeviceClass()) {
                    // We found our controller
                    mControllerDevice = device;
                    mConnectThread.start();
                    finishDiscovery();
                    mProgress = ProgressDialog.show(MainBluetoothActivity.this, "", "Connecting...", true);
                }
            }
            if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(intent
                    .getAction())) {
                if (mControllerDevice == null) {
                    // We didn't find anything
                    Toast.makeText(context, "No Controller Device Found.", Toast.LENGTH_SHORT).show();
                    finishDiscovery();
                }
            }
        }

        private void finishDiscovery() {
            BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
            unregisterReceiver(this);

            if (mProgress != null) {
                mProgress.dismiss();
                mProgress = null;
            }
        }
    };

    /*
     * We create a new thread to do the work of connecting to the remote device
     * and setting up the data transfer because this process is blocking and
     * can take some time to complete.
     */
    private Thread mConnectThread = new Thread() {
        public void run() {
            try {
                //Create a socket to the RFCOMM service
                mSocket = mControllerDevice.createInsecureRfcommSocketToServiceRecord(BT_UUID);
                // Connect the device through the socket. This will block
                // until it succeeds or throws an exception
                
                mSocket.connect();
                //Once connected, get the read/write data streams
                mInputStream = mSocket.getInputStream();
                mOutputStream = mSocket.getOutputStream();
            } catch (IOException connectException) {
                Log.w(TAG, "Error Connecting Bluetooth");
                // Unable to connect; close the socket and get out
                try {
                    mSocket.close();
                } catch (IOException closeException) { }
                mHandler.sendEmptyMessage(MESSAGE_CONNECTFAIL);
                return;
            }
            
            mHandler.sendEmptyMessage(MESSAGE_CONNECTED);
        }

        //In SDK15 (4.0.3) this method is now public as
        //Bluetooth.fetchUuisWithSdp() and BluetoothDevice.getUuids()
        public ParcelUuid[] servicesFromDevice(BluetoothDevice device) {
            try {
                Class cl = Class.forName("android.bluetooth.BluetoothDevice");
                Class[] par = {};
                Method method = cl.getMethod("getUuids", par);
                Object[] args = {};
                ParcelUuid[] retval = (ParcelUuid[]) method.invoke(device, args);
                return retval;
            } catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
    };
    
    private int composeInt(byte hi, byte lo) {
        int val = (int) hi & 0xff;
        val *= 256;
        val += (int) lo & 0xff;
        return val;
    }
    
    /*
     * Runnable block that will poll the accessory InputStream for
     * regular data updates, and post each message it encounters to
     * a Handler.  This is executed on a spawned background thread.
     */
    public void run() {
        byte[] buffer = new byte[16];
        int i;
        boolean running = true;
        //Wrap the stream to allow for fixed reads
        DataInputStream input = new DataInputStream(mInputStream);

        while (running) {
            try {
                input.readFully(buffer, 0, 3);
            } catch (IOException e) {
                break;
            }

            switch (buffer[0]) {
                case 0x1: {
                    Message m = Message.obtain(mHandler, MESSAGE_SWITCH);
                    m.obj = new SwitchMsg(buffer[1], buffer[2]);
                    mHandler.sendMessage(m);
                    break;
                }
                case 0x6: {
                    Message m = Message.obtain(mHandler, MESSAGE_JOY);
                    m.obj = new JoyMsg(buffer[1], buffer[2]);
                    mHandler.sendMessage(m);
                    break;
                }
                default:
                    Log.d(TAG, "unknown msg: " + buffer[0]);
                    break;
            }
        }
    }

    /*
     * This Handler receives messages from the polling thread and
     * injects them into the GameActivity methods on the main thread.
     * 
     * It also responds to connect events posted from the connect thread
     * in order to update the UI state.
     */
    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_CONNECTED:
                    beginGame();
                case MESSAGE_CONNECTFAIL:
                    if (mProgress != null) {
                        mProgress.dismiss();
                        mProgress = null;
                    }
                    break;

                case MESSAGE_SWITCH:
                    SwitchMsg o = (SwitchMsg) msg.obj;
                    handleSwitchMessage(o);
                    break;

                case MESSAGE_JOY:
                    JoyMsg j = (JoyMsg) msg.obj;
                    handleJoyMessage(j);
                    break;

                case MESSAGE_VIBE:
                    try {
                        byte[] v = (byte[]) msg.obj;
                        mOutputStream.write(v);
                        mOutputStream.flush();
                    } catch (IOException e) {
                        Log.w("AccessoryController", "Error writing vibe output");
                    }
                    break;
            }
        }
    };

}
