package com.example.bluetoothgatt;

import java.util.List;

/**
* Created by Dave Smith
* Double Encore, Inc.
* TemperatureBeacon
*/
class TemperatureBeacon {

    /* Short-form UUID that defines the Health Thermometer service */
    private static final int UUID_SERVICE_THERMOMETER = 0x1809;

    private String mName;
    private float mCurrentTemp;
    //Device metadata
    private int mSignal;
    private String mAddress;

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
                /*
                 * Temperature data is two bytes, and precision is 0.5degC.
                 * LSB contains temperature whole number
                 * MSB contains a bit flag noting if fractional part exists
                 */
                mCurrentTemp = (data[0] & 0xFF);
                if ((data[1] & 0x80) != 0) {
                    mCurrentTemp += 0.5f;
                }
            }
        }
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
