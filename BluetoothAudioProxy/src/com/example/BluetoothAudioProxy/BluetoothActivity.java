package com.example.BluetoothAudioProxy;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

public class BluetoothActivity extends Activity {

    private BluetoothAdapter mBluetoothAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    public void onStartClick(View v) {
        if (mBluetoothAdapter.isEnabled()) {
            Intent intent = new Intent(this, HeadsetService.class);
            startService(intent);
        } else {
            Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivity(intent);
        }
    }

    public void onStopClick(View v) {
        Intent intent = new Intent(this, HeadsetService.class);
        stopService(intent);
    }

    public void onStartVoiceClick(View v) {
        Intent intent = new Intent(this, HeadsetService.class);
        intent.setAction(HeadsetService.ACTION_STARTVOICE);
        startService(intent);
    }

    public void onStopVoiceClick(View v) {
        Intent intent = new Intent(this, HeadsetService.class);
        intent.setAction(HeadsetService.ACTION_STOPVOICE);
        startService(intent);
    }
}
