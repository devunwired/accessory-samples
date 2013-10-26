package com.example.bluetoothgatt;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;

import java.util.HashMap;

/**
 * Created by Dave Smith
 * Double Encore, Inc.
 * Date: 10/19/13
 * BeaconView
 */
public class BeaconView extends View {

    private static final float RADIUS = 25f;

    private static final float MAX_SIGNAL = -50;
    private static final float MIN_SIGNAL = -100;

    private HashMap<String, Integer> mPositions;
    private HashMap<String, TemperatureBeacon> mBeacons;

    private int mDrawRadius;
    private Paint mCirclePaint;
    private TextPaint mTextPaint;

    public BeaconView(Context context) {
        this(context, null);
    }

    public BeaconView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BeaconView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mPositions = new HashMap<String, Integer>();
        mBeacons = new HashMap<String, TemperatureBeacon>();

        mDrawRadius = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, RADIUS,
                getResources().getDisplayMetrics());

        mCirclePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mCirclePaint.setStyle(Paint.Style.FILL);

        mTextPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setColor(Color.WHITE);
        mTextPaint.setTextSize(mDrawRadius);
    }

    public void updateBeacon(TemperatureBeacon beacon) {
        if (!mPositions.containsKey(beacon.getAddress())) {
            //Randomize x-position
            int random = (int)(Math.random() * getWidth());
            int clamped = Math.max(mDrawRadius, Math.min(getWidth() - mDrawRadius, random));
            mPositions.put(beacon.getAddress(), clamped);
        }

        mBeacons.put(beacon.getAddress(), beacon);
        invalidate();
    }


    @Override
    protected void onDraw(Canvas canvas) {
        //Draw any backgrounds
        super.onDraw(canvas);

        //Draw user
        mCirclePaint.setColor(Color.BLACK);
        canvas.drawCircle(getWidth() / 2, getHeight(), mDrawRadius, mCirclePaint);


        for(TemperatureBeacon beacon : mBeacons.values()) {
            //Scaled temperature between 0 and 30 degrees
            float scaled = Math.max(0f, Math.min(1f,
                    beacon.getCurrentTemp() / 30f) );
            //Color circle based on scaled temperature
            // 100 degrees, solid red
            // 0 degress, solid blue
            int red = Math.round(255 * scaled);
            int blue = 255 - red;
            mCirclePaint.setColor(Color.rgb(red, 0, blue));


            int x = mPositions.get(beacon.getAddress());
            //Calculate y-position from RSSI
            scaled = Math.max(0f, Math.min(1f,
                    (beacon.getSignal() - MIN_SIGNAL) / (MAX_SIGNAL - MIN_SIGNAL) ));
            int y = Math.round( getWidth() - (scaled * getWidth()) );

            canvas.drawCircle(x, y, mDrawRadius, mCirclePaint);
            canvas.drawText(String.valueOf(beacon.getCurrentTemp()), x - mDrawRadius, y, mTextPaint);
        }
    }
}
