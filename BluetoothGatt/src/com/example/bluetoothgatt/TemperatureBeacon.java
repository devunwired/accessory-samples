package com.example.bluetoothgatt;

import android.annotation.TargetApi;
import android.bluetooth.le.ScanRecord;
import android.os.Build;
import android.os.ParcelUuid;

import java.util.List;

/**
* Created by Dave Smith
* Double Encore, Inc.
* TemperatureBeacon
*/
class TemperatureBeacon {

    /* Full Bluetooth UUID that defines the Health Thermometer Service */
    public static final ParcelUuid THERM_SERVICE = ParcelUuid.fromString("00001809-0000-1000-8000-00805f9b34fb");
    /* Short-form UUID that defines the Health Thermometer service */
    private static final int UUID_SERVICE_THERMOMETER = 0x1809;

    private String mName;
    private float mCurrentTemp;
    //Device metadata
    private int mSignal;
    private String mAddress;

    /* Builder for Lollipop+ */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public TemperatureBeacon(ScanRecord record, String deviceAddress, int rssi) {
        mSignal = rssi;
        mAddress = deviceAddress;

        mName = record.getDeviceName();

        byte[] data = record.getServiceData(THERM_SERVICE);
        if (data != null) {
            mCurrentTemp = parseTemp(data);
        } else {
            mCurrentTemp = 0f;
        }
    }

    /* Builder for pre-Lollipop */
    public TemperatureBeacon(List<AdRecord> records, String deviceAddress, int rssi) {
        mSignal = rssi;
        mAddress = deviceAddress;

        for(AdRecord packet : records) {
            //Find the device name record
            if (packet.getType() == AdRecord.TYPE_NAME) {
                mName = AdRecord.getName(packet);
            }
            //Find the service data record that contains our service's UUID
            if (packet.getType() == AdRecord.TYPE_SERVICEDATA
                    && AdRecord.getServiceDataUuid(packet) == UUID_SERVICE_THERMOMETER) {
                byte[] data = AdRecord.getServiceData(packet);
                mCurrentTemp = parseTemp(data);
            }
        }
    }

    private float parseTemp(byte[] serviceData) {
        /*
         * Temperature data is two bytes, and precision is 0.5degC.
         * LSB contains temperature whole number
         * MSB contains a bit flag noting if fractional part exists
         */
        float temp = (serviceData[0] & 0xFF);
        if ((serviceData[1] & 0x80) != 0) {
            temp += 0.5f;
        }

        return temp;
    }

    public String getName() {
        return mName;
    }

    public int getSignal() {
        return mSignal;
    }

    public float getCurrentTemp() {
        return mCurrentTemp;
    }

    public String getAddress() {
        return mAddress;
    }

    @Override
    public String toString() {
        return String.format("%s (%ddBm): %.1fC", mName, mSignal, mCurrentTemp);
    }
}
