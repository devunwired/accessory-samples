/**
 * MainBluetoothActivity is a GameActivity that monitors a Bluetooth
 * accessory device and uses the input from that device to control
 * the game.
 */

package com.examples.accessory.controller;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
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

public class MainBluetoothActivity extends GameActivity implements Runnable {

    //Friendly name of the Bluetooth device we are looking to connect with
    private static final String CONTROLLER_NAME = "itead";

    private static final int MESSAGE_SWITCH = 1;
    private static final int MESSAGE_TEMPERATURE = 2;
    private static final int MESSAGE_LIGHT = 3;
    private static final int MESSAGE_JOY = 4;
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
                if (CONTROLLER_NAME.equals(device.getName())) {
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
            ParcelUuid[] uuids = servicesFromDevice(mControllerDevice);
            try {
                //Create a socket to the RFCOMM service
                //BT Modem only has one service to publish, so it is the first UUID
                mSocket = mControllerDevice.createInsecureRfcommSocketToServiceRecord(uuids[0].getUuid());
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
        int ret = 0;
        byte[] buffer = new byte[16384];
        int i;

        while (ret >= 0) {
            try {
                ret = mInputStream.read(buffer);
            } catch (IOException e) {
                break;
            }

            i = 0;
            while (i < ret) {
                int len = ret - i;

                switch (buffer[i]) {
                case 0x1:
                    if (len >= 3) {
                        Message m = Message.obtain(mHandler, MESSAGE_SWITCH);
                        m.obj = new SwitchMsg(buffer[i + 1], buffer[i + 2]);
                        mHandler.sendMessage(m);
                    }
                    i += 3;
                    break;

                case 0x4:
                    if (len >= 3) {
                        Message m = Message.obtain(mHandler, MESSAGE_TEMPERATURE);
                        m.obj = new TemperatureMsg(composeInt(buffer[i + 1], buffer[i + 2]));
                        mHandler.sendMessage(m);
                    }
                    i += 3;
                    break;

                case 0x5:
                    if (len >= 3) {
                        Message m = Message.obtain(mHandler, MESSAGE_LIGHT);
                        m.obj = new LightMsg(composeInt(buffer[i + 1],
                                buffer[i + 2]));
                        mHandler.sendMessage(m);
                    }
                    i += 3;
                    break;

                case 0x6:
                    if (len >= 3) {
                        Message m = Message.obtain(mHandler, MESSAGE_JOY);
                        m.obj = new JoyMsg(buffer[i + 1], buffer[i + 2]);
                        mHandler.sendMessage(m);
                    }
                    i += 3;
                    break;

                default:
                    Log.d(TAG, "unknown msg: " + buffer[i]);
                    i = len;
                    break;
                }
            }
            
            //Bluetooth + UART is slow, allow data to buffer
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) { }
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
                if(mProgress != null) {
                    mProgress.dismiss();
                    mProgress = null;
                }
                break;
                
            case MESSAGE_SWITCH:
                SwitchMsg o = (SwitchMsg) msg.obj;
                handleSwitchMessage(o);
                break;

            case MESSAGE_TEMPERATURE:
                TemperatureMsg t = (TemperatureMsg) msg.obj;
                handleTemperatureMessage(t);
                break;

            case MESSAGE_LIGHT:
                LightMsg l = (LightMsg) msg.obj;
                handleLightMessage(l);
                break;

            case MESSAGE_JOY:
                JoyMsg j = (JoyMsg) msg.obj;
                handleJoyMessage(j);
                break;

            }
        }
    };

}
