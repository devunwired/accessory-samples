package com.example.UsbMonitor;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.HashMap;

public class USBActivity extends Activity {
    private static final String TAG = "UsbHost";

    TextView mDeviceText, mDisplayText;
    Button mConnectButton;
    
    UsbManager mUsbManager;
    UsbDevice mDevice;
    PendingIntent mPermissionIntent;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mDeviceText = (TextView) findViewById(R.id.text_status);
        mDisplayText = (TextView) findViewById(R.id.text_data);
        mConnectButton = (Button) findViewById(R.id.button_connect);
        
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        registerReceiver(mUsbReceiver, filter);        
        //Check currently connected devices
        updateDeviceList();
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mUsbReceiver);
    }

    public void onConnectClick(View v) {
        if (mDevice == null) {
            return;
        }
        mDisplayText.setText("---");
        
        //This will either prompt the user with a grant permission dialog, 
        // or immediately fire the ACTION_USB_PERMISSION broadcast if the
        // user has already granted it to us.
        mUsbManager.requestPermission(mDevice, mPermissionIntent);
    }
    
    /*
     * Receiver to catch user permission responses, which are required in order to actuall
     * interact with a connected device.
     */
    private static final String ACTION_USB_PERMISSION = "com.android.recipes.USB_PERMISSION";
    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                        && device != null) {
                        //Query the device's descriptor
                        getDeviceStatus(device);
                } else {
                    Log.d(TAG, "permission denied for device " + device);
                }
            }
        }
    };

    /*
     * Initiate a control transfer to request the first configuration
     * descriptor of the device.
     */
    //Type: Indicates whether this is a read or write
    // Matches USB_ENDPOINT_DIR_MASK for either IN or OUT
    private static final int REQUEST_TYPE = 0x80;
    //Request: GET_CONFIGURATION_DESCRIPTOR = 0x06
    private static final int REQUEST = 0x06;
    //Value: Descriptor Type (High) and Index (Low)
    // Configuration Descriptor = 0x2
    // Index = 0x0 (First configuration)
    private static final int REQ_VALUE = 0x200;
    private static final int REQ_INDEX = 0x00;
    private static final int LENGTH = 64;
    private void getDeviceStatus(UsbDevice device) {
        UsbDeviceConnection connection = mUsbManager.openDevice(device);
        //Create a sufficiently large buffer for incoming data
        byte[] buffer = new byte[LENGTH];
        connection.controlTransfer(REQUEST_TYPE, REQUEST, REQ_VALUE, REQ_INDEX,
                buffer, LENGTH, 2000);
        //Parse received data into a description
        String description = parseConfigDescriptor(buffer);
        
        mDisplayText.setText(description);
        connection.close();
    }
    
    /*
     * Parse the USB configuration descriptor response per the
     * USB Specification.  Return a printable description of
     * the connected device.
     */
    private static final int DESC_SIZE_CONFIG = 9;
    private String parseConfigDescriptor(byte[] buffer) {
        StringBuilder sb = new StringBuilder();
        //Parse configuration descriptor header
        int totalLength = (buffer[3] &0xFF) << 8;
        totalLength += (buffer[2] & 0xFF);
        //Interface count
        int numInterfaces = (buffer[5] & 0xFF);
        //Configuration attributes
        int attributes = (buffer[7] & 0xFF);        
        //Power is given in 2mA increments
        int maxPower = (buffer[8] & 0xFF) * 2;
        
        sb.append("Configuration Descriptor:\n");
        sb.append("Length: " + totalLength + " bytes\n");
        sb.append(numInterfaces + " Interfaces\n");
        sb.append(String.format("Attributes:%s%s%s\n",
                (attributes & 0x80) == 0x80 ? " BusPowered" : "",
                (attributes & 0x40) == 0x40 ? " SelfPowered" : "",
                (attributes & 0x20) == 0x20 ? " RemoteWakeup" : ""));
        sb.append("Max Power: " + maxPower + "mA\n");
        
        //The rest of the descriptor is interfaces and endpoints
        int index = DESC_SIZE_CONFIG; 
        while (index < totalLength) {
            //Read length and type
            int len = (buffer[index] & 0xFF);
            int type = (buffer[index+1] & 0xFF);
            switch (type) {
            case 0x04: //Interface Descriptor
                int intfNumber = (buffer[index+2] & 0xFF);
                int numEndpoints = (buffer[index+4] & 0xFF);
                int intfClass = (buffer[index+5] & 0xFF);
                
                sb.append(String.format("- Interface %d, %s, %d Endpoints\n",
                        intfNumber, nameForClass(intfClass), numEndpoints));
                break;
            case 0x05: //Endpoint Descriptor
                int endpointAddr = ((buffer[index+2] & 0xFF));
                //Number is lower 4 bits
                int endpointNum = (endpointAddr & 0x0F);
                //Direction is high bit
                int direction = (endpointAddr & 0x80);
                
                int endpointAttrs = (buffer[index+3] & 0xFF);
                //Type is the lower two bits
                int endpointType = (endpointAttrs & 0x3);
                
                sb.append(String.format("-- Endpoint %d, %s %s\n",
                        endpointNum,
                        nameForEndpointType(endpointType),
                        nameForDirection(direction) ));
                break;
            }
            //Advance to next descriptor
            index += len;
        }
        
        return sb.toString();
    }
    
    private void updateDeviceList() {
        HashMap<String, UsbDevice> connectedDevices = mUsbManager
                .getDeviceList();
        if (connectedDevices.isEmpty()) {
            mDevice = null;
            mDeviceText.setText("No Devices Currently Connected");
            mConnectButton.setEnabled(false);
        } else {
            StringBuilder builder = new StringBuilder();
            for (UsbDevice device : connectedDevices.values()) {
                //Use the last device detected (if multiple) to open
                mDevice = device;
                builder.append(readDevice(device));
                builder.append("\n\n");
            }
            mDeviceText.setText(builder.toString());
            mConnectButton.setEnabled(true);
        }
    }

    /*
     * Enumerate the endpoints and interfaces on the connected device.
     * We do not need permission to do anything here, it is all "publicly available"
     * until we try to connect to an actual device.
     */
    private String readDevice(UsbDevice device) {
        StringBuilder sb = new StringBuilder();
        sb.append("Device Name: " + device.getDeviceName() + "\n");
        sb.append(String.format(
                "Device Class: %s -> Subclass: 0x%02x -> Protocol: 0x%02x\n",
                nameForClass(device.getDeviceClass()),
                device.getDeviceSubclass(), device.getDeviceProtocol()));

        for (int i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface intf = device.getInterface(i);
            sb.append(String
                    .format("+--Interface %d Class: %s -> Subclass: 0x%02x -> Protocol: 0x%02x\n",
                            intf.getId(),
                            nameForClass(intf.getInterfaceClass()),
                            intf.getInterfaceSubclass(),
                            intf.getInterfaceProtocol()));

            for (int j = 0; j < intf.getEndpointCount(); j++) {
                UsbEndpoint endpoint = intf.getEndpoint(j);
                sb.append(String.format("  +---Endpoint %d: %s %s\n",
                        endpoint.getEndpointNumber(),
                        nameForEndpointType(endpoint.getType()),
                        nameForDirection(endpoint.getDirection())));
            }
        }

        return sb.toString();
    }

    /* Helper Methods to Provide Readable Names for USB Constants */
    
    private String nameForClass(int classType) {
        switch (classType) {
        case UsbConstants.USB_CLASS_APP_SPEC:
            return String.format("Application Specific 0x%02x", classType);
        case UsbConstants.USB_CLASS_AUDIO:
            return "Audio";
        case UsbConstants.USB_CLASS_CDC_DATA:
            return "CDC Control";
        case UsbConstants.USB_CLASS_COMM:
            return "Communications";
        case UsbConstants.USB_CLASS_CONTENT_SEC:
            return "Content Security";
        case UsbConstants.USB_CLASS_CSCID:
            return "Content Smart Card";
        case UsbConstants.USB_CLASS_HID:
            return "Human Interface Device";
        case UsbConstants.USB_CLASS_HUB:
            return "Hub";
        case UsbConstants.USB_CLASS_MASS_STORAGE:
            return "Mass Storage";
        case UsbConstants.USB_CLASS_MISC:
            return "Wireless Miscellaneous";
        case UsbConstants.USB_CLASS_PER_INTERFACE:
            return "(Defined Per Interface)";
        case UsbConstants.USB_CLASS_PHYSICA:
            return "Physical";
        case UsbConstants.USB_CLASS_PRINTER:
            return "Printer";
        case UsbConstants.USB_CLASS_STILL_IMAGE:
            return "Still Image";
        case UsbConstants.USB_CLASS_VENDOR_SPEC:
            return String.format("Vendor Specific 0x%02x", classType);
        case UsbConstants.USB_CLASS_VIDEO:
            return "Video";
        case UsbConstants.USB_CLASS_WIRELESS_CONTROLLER:
            return "Wireless Controller";
        default:
            return String.format("0x%02x", classType);
        }
    }

    private String nameForEndpointType(int type) {
        switch (type) {
        case UsbConstants.USB_ENDPOINT_XFER_BULK:
            return "Bulk";
        case UsbConstants.USB_ENDPOINT_XFER_CONTROL:
            return "Control";
        case UsbConstants.USB_ENDPOINT_XFER_INT:
            return "Interrupt";
        case UsbConstants.USB_ENDPOINT_XFER_ISOC:
            return "Isochronous";
        default:
            return "Unknown Type";
        }
    }

    private String nameForDirection(int direction) {
        switch (direction) {
        case UsbConstants.USB_DIR_IN:
            return "IN";
        case UsbConstants.USB_DIR_OUT:
            return "OUT";
        default:
            return "Unknown Direction";
        }
    }
}
