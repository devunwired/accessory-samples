package com.example.BluetoothAudioProxy;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

/**
 * Created by Dave Smith
 * Double Encore, Inc.
 * Date: 5/9/13
 * HeadsetService
 * Service to interact with BluetoothHeadset profile
 */
public class HeadsetService extends Service {
    private static final String TAG = "BluetoothProxyMonitorService";
    private static final int NOTE_ID = 1000;

    BluetoothAdapter mBluetoothAdapter;
    BluetoothHeadset mBluetoothHeadset;

    NotificationManager mNotificationManager;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "Creating HeadsetService...");

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        // Establish connection to the proxy.
        mBluetoothAdapter.getProfileProxy(this, mProfileListener, BluetoothProfile.HEADSET);

        //Monitor profile events
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED);
        filter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothHeadset.ACTION_VENDOR_SPECIFIC_HEADSET_EVENT);
        registerReceiver(mProfileReceiver, filter);

        buildNotification("Service Started...");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "Destroying HeadsetService...");
        mBluetoothAdapter.closeProfileProxy(BluetoothProfile.HEADSET, mBluetoothHeadset);

        unregisterReceiver(mProfileReceiver);
        mNotificationManager.cancel(NOTE_ID);
    }

    private void buildNotification(String text) {
        Notification note = new Notification.Builder(this)
                .setContentTitle("BluetoothProxyService")
                .setContentText(text)
                .setSmallIcon(R.drawable.ic_launcher)
                .getNotification();
        mNotificationManager.notify(NOTE_ID, note);
    }

    private BluetoothProfile.ServiceListener mProfileListener = new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (profile == BluetoothProfile.HEADSET) {
                Log.d(TAG, "Connecting HeadsetService...");
                mBluetoothHeadset = (BluetoothHeadset) proxy;
            }
        }
        public void onServiceDisconnected(int profile) {
            if (profile == BluetoothProfile.HEADSET) {
                Log.d(TAG, "Unexpected Disconnect of HeadsetService...");
                mBluetoothHeadset = null;
            }
        }
    };

    private BroadcastReceiver mProfileReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED.equals(action)) {
                notifyAudioState(intent);
            }
            if (BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED.equals(action)) {
                notifyConnectState(intent);
            }
            if (BluetoothHeadset.ACTION_VENDOR_SPECIFIC_HEADSET_EVENT.equals(action)) {
                notifyATEvent(intent);
            }
        }
    };

    private void notifyAudioState(Intent intent) {
        final int state = intent.getIntExtra(BluetoothHeadset.EXTRA_STATE, -1);
        String message;
        switch (state) {
            case BluetoothHeadset.STATE_AUDIO_CONNECTED:
                message = "Audio Connected";
                break;
            case BluetoothHeadset.STATE_AUDIO_CONNECTING:
                message = "Audio Connecting";
                break;
            case BluetoothHeadset.STATE_AUDIO_DISCONNECTED:
                message = "Audio Disconnected";
                break;
            default:
                message = "Audio Unknown";
                break;
        }

        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
        buildNotification(message);

    }

    private void notifyConnectState(Intent intent) {
        final int state = intent.getIntExtra(BluetoothHeadset.EXTRA_STATE, -1);
        String message;
        switch (state) {
            case BluetoothHeadset.STATE_CONNECTED:
                message = "Connected";
                break;
            case BluetoothHeadset.STATE_CONNECTING:
                message = "Connecting";
                break;
            case BluetoothHeadset.STATE_DISCONNECTING:
                message = "Disconnecting";
                break;
            case BluetoothHeadset.STATE_DISCONNECTED:
                message = "Disconnected";
                break;
            default:
                message = "Connect Unknown";
                break;
        }

        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
        buildNotification(message);
    }

    private void notifyATEvent(Intent intent) {
        String command = intent.getStringExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD);
        int type = intent.getIntExtra(BluetoothHeadset.EXTRA_VENDOR_SPECIFIC_HEADSET_EVENT_CMD_TYPE, -1);

        String typeString;
        switch (type) {
            case BluetoothHeadset.AT_CMD_TYPE_ACTION:
                typeString = "AT Action";
                break;
            case BluetoothHeadset.AT_CMD_TYPE_READ:
                typeString = "AT Read";
                break;
            case BluetoothHeadset.AT_CMD_TYPE_TEST:
                typeString = "AT Test";
                break;
            case BluetoothHeadset.AT_CMD_TYPE_SET:
                typeString = "AT Set";
                break;
            case BluetoothHeadset.AT_CMD_TYPE_BASIC:
                typeString = "AT Basic";
                break;
            default:
                typeString = "AT Unknown";
                break;
        }

        Toast.makeText(this, typeString+": "+command, Toast.LENGTH_SHORT).show();
        buildNotification(typeString+": "+command);
    }

    public IBinder onBind(Intent intent) {
        return null;
    }
}
