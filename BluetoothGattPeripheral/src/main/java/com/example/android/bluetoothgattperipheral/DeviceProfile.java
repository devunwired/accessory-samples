package com.example.android.bluetoothgattperipheral;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothProfile;

import java.util.UUID;

/**
 * Dave Smith
 * Date: 11/13/14
 * DeviceProfile
 * Service/Characteristic constant for our custom peripheral
 */
public class DeviceProfile {

    /* Unique ids generated for this device by 'uuidgen'. Doesn't conform to any SIG profile. */

    public static UUID SERVICE_UUID = UUID.fromString("1706BBC0-88AB-4B8D-877E-2237916EE929");

    public static UUID CHARACTERISTIC_READ_UUID = UUID.fromString("275348FB-C14D-4FD5-B434-7C3F351DEA5F");

    public static UUID CHARACTERISTIC_WRITE_UUID = UUID.fromString("BD28E457-4026-4270-A99F-F9BC20182E15");

    public static String getStateDescription(int state) {
        switch (state) {
            case BluetoothProfile.STATE_CONNECTED:
                return "Connected";
            case BluetoothProfile.STATE_CONNECTING:
                return "Connecting";
            case BluetoothProfile.STATE_DISCONNECTED:
                return "Disconnected";
            case BluetoothProfile.STATE_DISCONNECTING:
                return "Disconnecting";
            default:
                return "Unknown State "+state;
        }
    }

    public static String getStatusDescription(int status) {
        switch (status) {
            case BluetoothGatt.GATT_SUCCESS:
                return "SUCCESS";
            default:
                return "Unknown Status "+status;
        }
    }
}
