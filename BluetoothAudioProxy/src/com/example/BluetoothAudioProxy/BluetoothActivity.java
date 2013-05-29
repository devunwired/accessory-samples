package com.example.BluetoothAudioProxy;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHeadset;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

public class BluetoothActivity extends Activity {

    private BluetoothAdapter mBluetoothAdapter;
    private SpeechRecognizer mRecognizer;

    private TextView mResultText;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mResultText = (TextView) findViewById(R.id.text_result);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mRecognizer = SpeechRecognizer.createSpeechRecognizer(this);
        mRecognizer.setRecognitionListener(mRecognitionListener);
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(mAudioReceiver,
                new IntentFilter(BluetoothHeadset.ACTION_AUDIO_STATE_CHANGED));
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mAudioReceiver);
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
        sendStartRecognitionCommand();
    }

    /**
     * Send a command to the service to enable voice on the headset, if successful
     * this will trigger our broadcast receiver when the audio state changes
     */
    private void sendStartRecognitionCommand() {
        Intent intent = new Intent(this, HeadsetService.class);
        intent.setAction(HeadsetService.ACTION_STARTVOICE);
        startService(intent);
    }

    /**
     * Send a command to the service to disable voice on the headset
     */
    private void sendStopRecognitionCommand() {
        Intent intent = new Intent(this, HeadsetService.class);
        intent.setAction(HeadsetService.ACTION_STOPVOICE);
        startService(intent);
    }


    private void processRecognitionResults(ArrayList<String> results) {
        if (!results.isEmpty()) {
            mResultText.setText(results.get(0));
        } else {
            mResultText.setText(null);
        }
    }

    private void processRecognitionError(int error) {
        mResultText.setText(null);
        Toast.makeText(this, "Error Recognizing Speech", Toast.LENGTH_SHORT).show();
    }

    /**
     * Callbacks for events related to speech recognition service on the device
     */
    private RecognitionListener mRecognitionListener = new RecognitionListener() {
        @Override
        public void onReadyForSpeech(Bundle params) { }

        @Override
        public void onBeginningOfSpeech() { }

        @Override
        public void onRmsChanged(float rmsdB) { }

        @Override
        public void onBufferReceived(byte[] buffer) { }

        @Override
        public void onEndOfSpeech() {
            //When we detect no more speech, disable the audio connection
            sendStopRecognitionCommand();
        }

        @Override
        public void onError(int error) {
            //If something went wrong, disable the audio connection
            sendStopRecognitionCommand();
            processRecognitionError(error);
        }

        @Override
        public void onResults(Bundle results) {
            ArrayList<String> items = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            processRecognitionResults(items);
        }

        @Override
        public void onPartialResults(Bundle partialResults) {
            ArrayList<String> items = partialResults.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            processRecognitionResults(items);
        }

        @Override
        public void onEvent(int eventType, Bundle params) { }
    };

    /**
     * This receiver will trigger when the audio state for a headset changes to connected,
     * at which point we start voice recognition.
     */
    private BroadcastReceiver mAudioReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final int currentState = intent.getIntExtra(BluetoothHeadset.EXTRA_STATE, -1);
            if (BluetoothHeadset.STATE_AUDIO_CONNECTED == currentState) {
                //Start voice recognition
                mRecognizer.startListening(RecognizerIntent.getVoiceDetailsIntent(context));
            }
        }
    };
}
