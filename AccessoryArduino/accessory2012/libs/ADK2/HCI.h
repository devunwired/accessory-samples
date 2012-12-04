/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _HCI_H_
#define _HCI_H_


//http://webee.technion.ac.il/labs/comnet/projects/winter02/cn08w02/HCI_spec.pdf


#define HCI_OPCODE(ogf, ocf)	((((unsigned short)(ogf)) << 10) | (ocf))

typedef struct{

	unsigned short	opcode;
	unsigned char	totalParamLen;
	unsigned char	params[];

}HCI_Cmd;

typedef struct{

	unsigned char	eventCode;
	unsigned char	totalParamLen;
	unsigned char	params[];

}HCI_Event;

#define HCI_DATA_INFO(conn, pbf, bcf)	(((conn & 0xEFF) | (((unsigned short)(pbf)) << 12) | (((unsigned short)(bcf)) << 14))
#define HCI_CONN(info)			((info) & 0xEFF)
#define HCI_PBF(info)			(((info) >> 12) & 3)
#define HCI_BCF(info)			(((info) >> 14) & 3)

#define HCI_PBF_First_Message		2
#define HCI_PBF_Continuing_Message	1

//BCF Common
#define HCI_BCF_Point_To_Point		0

//BCF from host to controller
#define HCI_BCF_Active_Broadcast	1
#define HCI_BCF_Piconet_Broadcast	2

//BCF from controller
#define HCI_BCF_RXed_In_Active_State	1
#define HCI_BCF_RXed_In_Parked_State	2

typedef struct{

	unsigned short	info;		//connection handle & flags
	unsigned short	totalDataLen;
	unsigned char	data[];

}HCI_ACL_Data;


typedef struct{

	unsigned short	info;		//connection handle & flags [reserved]
	unsigned char	totalDataLen;
	unsigned char	data[];

}HCI_SCO_Data;


#define HCI_LAP_Unlimited_Inquiry	0x9E8B33
#define HCI_LAP_Limited_Inquiry		0x9E8B00

#define UART_PKT_TYP_CMD	1
#define UART_PKT_TYP_ACL	2
#define UART_PKT_TYP_SCO	3
#define UART_PKT_TYP_EVT	4

#define PAGE_STATE_INQUIRY	1
#define PAGE_STATE_PAGE		2

#define DEVICE_CLASS_SERVICE_POSITIONING	0x010000	//GPS
#define DEVICE_CLASS_SERVICE_NETWORKING		0x020000	//LAN
#define DEVICE_CLASS_SERVICE_RENDERING		0x040000	//PRINTER, SPEAKER
#define DEVICE_CLASS_SERVICE_CAPTURING		0x080000	//SCANNER, MICROPHONE
#define DEVICE_CLASS_SERVICE_OBEX		0x100000	//v-Inbox, v-Folder, v-$WORD
#define DEVICE_CLASS_SERVICE_AUDIO		0x200000	//SPEAKER, MIC, HEADSET
#define DEVICE_CLASS_SERVICE_TELEPHONY		0x400000	//MODEM, HEADSET
#define DEVICE_CLASS_SERVICE_INFORMATION	0x800000	//WEB-server, WAP-server

#define DEVICE_CLASS_MAJOR_SHIFT		8
#define DEVICE_CLASS_MAJOR_MASK			0x001F00

#define DEVICE_CLASS_MAJOR_MISC			0
#define DEVICE_CLASS_MAJOR_COMPUTER		1
#define DEVICE_CLASS_MAJOR_PHONE		2
#define DEVICE_CLASS_MAJOR_LAN			3
#define DEVICE_CLASS_MAJOR_AV			4
#define DEVICE_CLASS_MAJOR_PERIPH		5
#define DEVICE_CLASS_MAJOR_IMAGING		6
#define DEVICE_CLASS_MAJOR_WEARABLE		7
#define DEVICE_CLASS_MAJOR_TOY			8
#define DEVICE_CLASS_MAJOR_HEALTH		9
#define DEVICE_CLASS_MAJOR_UNCATEGORIZED	31

#define DEVICE_CLASS_MINOR_COMPUTER_SHIFT	2
#define DEVICE_CLASS_MINOR_COMPUTER_MASK	0xFC
#define DEVICE_CLASS_MINOR_COMPUTER_UNCATEG	0
#define DEVICE_CLASS_MINOR_COMPUTER_DESKTOP	1
#define DEVICE_CLASS_MINOR_COMPUTER_SERVER	2
#define DEVICE_CLASS_MINOR_COMPUTER_LAPTOP	3
#define DEVICE_CLASS_MINOR_COMPUTER_CLAM_PDA	4
#define DEVICE_CLASS_MINOR_COMPUTER_PALM_PDA	5
#define DEVICE_CLASS_MINOR_COMPUTER_WEARABLE	6

#define DEVICE_CLASS_MINOR_PHONE_SHIFT		2
#define DEVICE_CLASS_MINOR_PHONE_MASK		0xFC
#define DEVICE_CLASS_MINOR_PHONE_UNCATEG	0
#define DEVICE_CLASS_MINOR_PHONE_CELL		1
#define DEVICE_CLASS_MINOR_PHONE_CORDLESS	2
#define DEVICE_CLASS_MINOR_PHONE_SMART		3
#define DEVICE_CLASS_MINOR_PHONE_MODEM		4
#define DEVICE_CLASS_MINOR_PHONE_ISDN		5

#define DEVICE_CLASS_MINOR_AV_SHIFT		2
#define DEVICE_CLASS_MINOR_AV_MASK		0xFC
#define DEVICE_CLASS_MINOR_AV_UNCATEG		0
#define DEVICE_CLASS_MINOR_AV_HEADSET		1
#define DEVICE_CLASS_MINOR_AV_HANDSFREE		2
#define DEVICE_CLASS_MINOR_AV_MIC		4
#define DEVICE_CLASS_MINOR_AV_LOUDSPEAKER	5
#define DEVICE_CLASS_MINOR_AV_HEADPHONES	6
#define DEVICE_CLASS_MINOR_AV_PORTBL_AUDIO	7
#define DEVICE_CLASS_MINOR_AV_CAR_AUDIO		8
#define DEVICE_CLASS_MINOR_AV_SET_TOP_BOX	9
#define DEVICE_CLASS_MINOR_AV_HIFI		10
#define DEVICE_CLASS_MINOR_AV_VCR		11
#define DEVICE_CLASS_MINOR_AV_VID_CAM		12
#define DEVICE_CLASS_MINOR_AV_CAMCORDER		13
#define DEVICE_CLASS_MINOR_AV_VID_MONITOR	14
#define DEVICE_CLASS_MINOR_AV_DISPLAY_AND_SPKR	15
#define DEVICE_CLASS_MINOR_AV_VC		16
#define DEVICE_CLASS_MINOR_AV_TOY		18


//COMMANDS

#define HCI_OGF_Link_Control					1

#define HCI_CMD_Inquiry						0x0001	//enter Inquiry mode where it discovers other Bluetooth devices.
#define HCI_CMD_Inquiry_Cancel					0x0002	//cancel the Inquiry mode in which the Bluetooth device is in.
#define HCI_CMD_Periodic_Inquiry_Mode				0x0003	//set the device to enter Inquiry modes periodically according to the time interval set.
#define HCI_CMD_Exit_Periodic_Inquiry_Mode			0x0004	//exit the periodic Inquiry mode
#define HCI_CMD_Create_Connection				0x0005	//create an ACL connection to the device specified by the BD_ADDR in the parameters.
#define HCI_CMD_Disconnect					0x0006	//terminate the existing connection to a device
#define HCI_CMD_Add_SCO_Connection				0x0007	//Create an SCO connection defined by the connection handle parameters.
#define HCI_CMD_Accept_Connection_Request			0x0009	//accept a new connection request
#define HCI_CMD_Reject_Connection_Request			0x000A	//reject a new connection request
#define HCI_CMD_Link_Key_Request_Reply				0x000B	//Reply to a link key request event sent from controller to the host
#define HCI_CMD_Link_Key_Request_Negative_Reply			0x000C	//Reply to a link key request event from the controller to the host if there is no link key associated with the connection.
#define HCI_CMD_PIN_Code_Request_Reply				0x000D	//Reply to a PIN code request event sent from a controller to the host.
#define HCI_CMD_PIN_Code_Request_Negative_Reply			0x000E	//Reply to a PIN code request event sent from the controller to the host if there is no PIN associated with the connection.
#define HCI_CMD_Change_Connection_Packet_Type			0x000F	//change the type of packets to be sent for an existing connection.
#define HCI_CMD_Authentication_Requested			0x0011	//establish authentication between two devices specified by the connection handle.
#define HCI_CMD_Set_Connection_Encryption			0x0013	//enable or disable the link level encryption.
#define HCI_CMD_Change_Connection_Link_Key			0x0015	//force the change of a link key to a new one between two connected devices.
#define HCI_CMD_Master_Link_Key					0x0017	//force two devices to use the master's link key temporarily.
#define HCI_CMD_Remote_Name_Request				0x0019	//determine the user friendly name of the connected device.
#define HCI_CMD_Read_Remote_Supported_Features			0x001B	//determine the features supported by the connected device.
#define HCI_CMD_Read_Remote_Version_Information			0x001D	//determine the version information of the connected device.
#define HCI_CMD_Read_Clock_Offset				0x001F	//read the clock offset of the remote device.
//BT2.1 SSP
#define HCI_CMD_IO_Capability_Request_Reply			0x002B
#define HCI_CMD_User_Confirmation_Request_Reply			0x002C
#define HCI_CMD_User_Confirmation_Request_Negative_Reply	0x002D
#define HCI_CMD_User_Passkey_Request_Reply			0x002E
#define HCI_CMD_User_Passkey_Request_Negative_Reply		0x002F
#define HCI_CMD_Remote_OOB_Data_Request_Reply			0x0030
#define HCI_CMD_Remote_OOB_Data_Request_Negative_Reply		0x0033
#define HCI_CMD_IO_Capability_Request_Negative_Reply		0x0034

#define HCI_OGF_Policy						2

#define HCI_CMD_Hold_Mode					0x0001	//place the current or remote device into the Hold mode state.
#define HCI_CMD_Sniff_Mode					0x0003	//place the current or remote device into the Sniff mode state.
#define HCI_CMD_Exit_Sniff_Mode					0x0004	//exit the current or remote device from the Sniff mode state.
#define HCI_CMD_Park_Mode					0x0005	//place the current or remote device into the Park mode state.
#define HCI_CMD_Exit_Park_Mode					0x0006	//exit the current or remote device from the Park mode state.
#define HCI_CMD_QoS_Setup					0x0007	//setup the Quality of Service parameters of the device.
#define HCI_CMD_Role_Discovery					0x0009	//determine the role of the device for a particular connection.
#define HCI_CMD_Switch_Role					0x000B	//allow the device to switch roles for a particular connection.
#define HCI_CMD_Read_Link_Policy_Settings			0x000C	//determine the link policy that the LM can use to establish connections.
#define HCI_CMD_Write_Link_Policy_Settings			0x000D	//set the link policy that the LM can use for a particular connection.


#define HCI_OGF_Controller_and_Baseband				3

#define HCI_CMD_Set_Event_Mask					0x0001	//set which events are generated by the HCI for the host.
#define HCI_CMD_Reset						0x0003	//reset the host controller, link manager and the radio module.
#define HCI_CMD_Set_Event_Filter				0x0005	//used by host to set the different types of event filters that the host needs to receive.
#define HCI_CMD_Flush						0x0008	//flush all pending data packets for transmission for a particular connection handle.
#define HCI_CMD_Read_PIN_Type					0x0009	//used by host to determine if the link manager assumes that the host requires a variable PIN type or fixed PIN code. PIN is used during pairing.
#define HCI_CMD_Write_PIN_Type					0x000A	//used by host to write to the host controller on the PIN type supported by the host.
#define HCI_CMD_Create_New_Unit_Key				0x000B	//create a new unit key.
#define HCI_CMD_Read_Stored_Link_Key				0x000D	//read the link key stored in the host controller.
#define HCI_CMD_Write_Stored_Link_Key				0x0011	//write the link key to the host controller.
#define HCI_CMD_Delete_Stored_Link_Key				0x0012	//delete a stored link key in the host controller.
#define HCI_CMD_Change_Local_Name				0x0013	//modify the user friendly name of the device.
#define HCI_CMD_Read_Local_Name					0x0014	//read the user friendly name of the device.
#define HCI_CMD_Read_Connection_Accept_Timeout			0x0015	//determine the timeout session before the host denies and rejects a new connection request.
#define HCI_CMD_Write_Connection_Accept_Timeout			0x0016	//set the timeout session before a device can deny or reject a connection request.
#define HCI_CMD_Read_Page_Timeout				0x0017	//read the timeout value where a device will wait for a connection acceptance before sending a connection failure is returned.
#define HCI_CMD_Write_Page_Timeout				0x0018	//write the timeout value where a device will wait for a connection acceptance before sending a connection failure is returned.
#define HCI_CMD_Read_Scan_Enable				0x0019	//read the status of the Scan_Enable configuration.
#define HCI_CMD_Write_Scan_Enable				0x001A	//set the status of the Scan_Enable configuration.
#define HCI_CMD_Read_Page_Scan_Activity				0x001B	//read the value of the Page_Scan_Interval and Page_Scan_Window configurations.
#define HCI_CMD_Write_Page_Scan_Activity			0x001C	//write the value of the Page_Scan_Interval and Page_Scan_Window configurations.
#define HCI_CMD_Read_Inquiry_Scan_Activity			0x001D	//read the value of the Inquiry_Scan_Interval and Inquiry_Scan_Window configurations.
#define HCI_CMD_Write_Inquiry_Scan_Activity			0x001E	//set the value of the Inquiry_Scan_Interval and Inquiry_Scan_Window configurations.
#define HCI_CMD_Authentication_Enable				0x001F	//read the Authentication_Enable parameter.
#define HCI_CMD_Write_Authentication_Enable			0x0020	//set the Authentication_Enable parameter.
#define HCI_CMD_Read_Encryption_Mode				0x0021	//read the Encryption_Mode parameter.
#define HCI_CMD_Write_Encryption_Mode				0x0022	//write the Encryption_Mode parameter.
#define HCI_CMD_Read_Class_Of_Device				0x0023	//read the Class_Of_Device parameter.
#define HCI_CMD_Write_Class_Of_Device				0x0024	//set the Class_Of_Device parameter.
#define HCI_CMD_Read_Voice_Setting				0x0025	//read the Voice_Setting parameter. Used for voice connections.
#define HCI_CMD_Voice_Setting					0x0026	//set the Voice_Setting parameter. Used for voice connections.
#define HCI_CMD_Read_Automatic_Flush_Timeout			0x0027	//read the Flush_Timeout parameter. Used for ACL connections only.
#define HCI_CMD_Write_Automatic_Flush_Timeout			0x0028	//set the Flush_Timeout parameter. Used for ACL connections only.
#define HCI_CMD_Read_Num_Broadcast_Retransmissions		0x0029	//read the number of time a broadcast message is retransmitted.
#define HCI_CMD_Write_Num_Broadcast_Retransmissions		0x002A	//set the number of time a broadcast message is retransmitted.
#define HCI_CMD_Read_Hold_Mode_Activity				0x002B	//set the Hold_Mode activity to instruct the device to perform an activity during hold mode.
#define HCI_CMD_Write_Hold_Mode_Activity			0x002C	//set the Hold_Mode_Activity parameter.
#define HCI_CMD_Transmit_Power_Level				0x002D	//read the power level required for transmission for a connection handle.
#define HCI_CMD_Read_SCO_Flow_Control_Enable			0x002E	//check the current status of the flow control for the SCO connection.
#define HCI_CMD_SCO_Flow_Control_Enable				0x002F	//set the status of the flow control for a connection handle.
#define HCI_CMD_Set_Host_Controller_To_Host_Flow_Control	0x0031	//set the flow control from the host controller to host in on or off state.
#define HCI_CMD_Host_Buffer_Size				0x0033	//set by host to inform the host controller of the buffer size of the host for ACL and SCO connections.
#define HCI_CMD_Host_Number_Of_Completed_Packets		0x0035	//set from host to host controller when it is ready to receive more data packets.
#define HCI_CMD_Read_Link_Supervision_Timeout			0x0036	//read the timeout for monitoring link losses.
#define HCI_CMD_Write_Link_Supervision_Timeout			0x0037	//set the timeout for monitoring link losses.
#define HCI_CMD_Read_Number_Of_Supported_IAC			0x0038	//read the number of IACs that the device can listen on during Inquiry access.
#define HCI_CMD_Read_Current_IAC_LAP				0x0039	//read the LAP for the current IAC.
#define HCI_CMD_Write_Current_IAC_LAP				0x003A	//set the LAP for the current IAC.
#define HCI_CMD_Read_Page_Scan_Period_Mode			0x003B	//read the timeout session of a page scan.
#define HCI_CMD_Write_Page_Scan_Period_Mode			0x003C	//set the timeout session of a page scan.
#define HCI_CMD_Read_Page_Scan_Mode				0x003D	//read the default Page scan mode.
#define HCI_CMD_Write_Page_Scan_Mode				0x003E	//set the default page scan mode.
//BT2.1 SSP
#define HCI_CMD_Write_Simple_Pairing_Mode			0x0056


#define HCI_OGF_Informational					4

#define HCI_CMD_Read_Local_Version_Information			0x0001
#define HCI_CMD_Read_Local_Supported_Features			0x0003
#define HCI_CMD_Read_Buffer_Size				0x0005
#define HCI_CMD_Read_Country_Code				0x0007
#define HCI_CMD_Read_BD_ADDR					0x0009


//EVENTS
 
#define HCI_EVT_Inquiry_Complete_Event				0x01	//Indicates the Inquiry has finished.
#define HCI_EVT_Inquiry_Result_Event				0x02	//Indicates that Bluetooth device(s) have responded for the inquiry.
#define HCI_EVT_Connection_Complete_Event			0x03	//Indicates to both hosts that the new connection has been formed.
#define HCI_EVT_Connection_Request_Event			0x04	//Indicates that a new connection is trying to be established.
#define HCI_EVT_Disconnection_Complete_Event			0x05	//Occurs when a connection has been disconnected.
#define HCI_EVT_Authentication_Complete_Event			0x06	//Occurs when an authentication has been completed.
#define HCI_EVT_Remote_Name_Request_Complete_Event		0x07	//Indicates that the request for the remote name has been completed.
#define HCI_EVT_Encryption_Change_Event				0x08	//Indicates that a change in the encryption has been completed.
#define HCI_EVT_Change_Connection_Link_Key_Complete_Event	0x09	//Indicates that the change in the link key has been completed.
#define HCI_EVT_Master_Link_Key_Complete_Event			0x0A	//Indicates that the change in the temporary link key or semi permanent link key on the master device is complete.
#define HCI_EVT_Read_Remote_Supported_Features_Complete_Event	0x0B	//Indicates that the reading of the supported features on the remote device is complete.
#define HCI_EVT_Read_Remote_Version_Complete_Event		0x0C	//Indicates that the version number on the remote device has been read and completed.
#define HCI_EVT_QOS_Setup_Complete_Event			0x0D	//Indicates that the Quality of Service setup has been complete.
#define HCI_EVT_Command_Complete_Event				0x0E	//Used by controller to send status and event parameters to the host for the particular command.
#define HCI_EVT_Command_Status_Event				0x0F	//Indicates that the command has been received and is being processed in the host controller.
#define HCI_EVT_Hardware_Error_Event				0x10	//Indicates a hardware failure of the Bluetooth device.
#define HCI_EVT_Flush_Occured_event				0x11	//Indicates that the data has been flushed for a particular connection.
#define HCI_EVT_Role_Change_Event				0x12	//Indicates that the current bluetooth role for a connection has been changed.
#define HCI_EVT_Number_Of_Completed_Packets_Event		0x13	//Indicates to the host the number of data packets sent compared to the last time the same event was sent.
#define HCI_EVT_Mode_Change_Event				0x14	//Indicates the change in mode from hold, sniff, park or active to another mode.
#define HCI_EVT_Return_Link_Keys_Event				0x15	//Used to return stored link keys after a Read_Stored_Link_Key command was issued.
#define HCI_EVT_PIN_Code_Request_Event				0x16	//Indicates the a PIN code is required for a new connection.
#define HCI_EVT_Link_Key_Request_Event				0x17	//Indicates that a link key is required for the connection.
#define HCI_EVT_Link_Key_Notification_Event			0x18	//Indicates to the host that a new link key has been created.
#define HCI_EVT_Loopback_Command_Event				0x19	//Indicates that command sent from the host will be looped back.
#define HCI_EVT_Data_Buffer_Overflow_Event			0x1A	//Indicates that the data buffers on the host has overflowed.
#define HCI_EVT_Max_Slots_Change_Event				0x1B	//Informs the host when the LMP_Max_Slots parameter changes.
#define HCI_EVT_Read_Clock_Offset_Complete_Event		0x1C	//Indicates the completion of reading the clock offset information.
#define HCI_EVT_Connection_Packet_Type_Changed_Event		0x1D	//Indicate the completion of the packet type change for a connection.
#define HCI_EVT_QoS_Violation_Event				0x1E	//Indicates that the link manager is unable to provide the required Quality of Service.
#define HCI_EVT_Page_Scan_Mode_Change_Event			0x1F	//Indicates that the remote device has successfully changed the Page Scan mode.
#define HCI_EVT_Page_Scan_Repetition_Mode_Change_Event		0x20	//Indicates that the remote device has successfully changed the Page Scan Repetition mode.
//BT2.1 SSP
#define HCI_EVT_IO_Capability_Request_Event			0x31
#define HCI_EVT_IO_Capability_Response_Event			0x32
#define HCI_EVT_User_Confirmation_Request_Event			0x33
#define HCI_EVT_User_Passkey_Request_Event			0x34
#define HCI_EVT_Remote_OOB Data_Request_Event			0x35
#define HCI_EVT_Simple_Pairing_Complete_Event			0x36
#define HCI_EVT_User_Passkey_Notification_Event			0x3B




//ERRORS

#define HCI_ERR_Unknown_HCI_Command						0x01
#define HCI_ERR_No_Connection							0x02
#define HCI_ERR_Hardware_Failure						0x03
#define HCI_ERR_Page_Timeout							0x04
#define HCI_ERR_Authentication_Failure						0x05
#define HCI_ERR_Key_Missing							0x06
#define HCI_ERR_Memory_Full							0x07
#define HCI_ERR_Connection_Timeout						0x08
#define HCI_ERR_Max_Number_Of_Connections					0x09
#define HCI_ERR_Max_Number_Of_SCO_Connections_To_A_Device			0x0A
#define HCI_ERR_ACL_Connection_Already_Exists					0x0B
#define HCI_ERR_Command_Disallowed						0x0C
#define HCI_ERR_Host_Rejected_Due_To_Limited_Resources				0x0D
#define HCI_ERR_Host_Rejected_Due_To_Security_Reasons				0x0E
#define HCI_ERR_Host_Rejected_Due_To_A_Remote_Device_Only_A_Personal_Device	0x0F
#define HCI_ERR_Host_Timeout							0x10
#define HCI_ERR_Unsupported_Feature_Or_Parameter_Value				0x11
#define HCI_ERR_Invalid_HCI_Command_Parameters					0x12
#define HCI_ERR_Other_End_Terminated_Connection_User_Ended_Connection		0x13
#define HCI_ERR_Other_End_Terminated_Connection_Low_Resources			0x14
#define HCI_ERR_Other_End_Terminated_Connection_About_To_Power_Off		0x15
#define HCI_ERR_Connection_Terminated_By_Local_Host				0x16
#define HCI_ERR_Repeated_Attempts						0x17
#define HCI_ERR_Pairing_Not_Allowed						0x18
#define HCI_ERR_Unknown_LMP_PDU							0x19
#define HCI_ERR_Unsupported_Remote_Feature					0x1A
#define HCI_ERR_SCO_Offset_Rejected						0x1B
#define HCI_ERR_SCO_Interval_Rejected						0x1C
#define HCI_ERR_SCO_Air_Mode_Rejected						0x1D
#define HCI_ERR_Invalid_LMP_Parameters						0x1E
#define HCI_ERR_Unspecified_Error						0x1F
#define HCI_ERR_Unsupported_LMP_Parameter					0x20
#define HCI_ERR_Role_Change_Not_Allowed						0x21
#define HCI_ERR_LMP_Response_Timeout						0x22
#define HCI_ERR_LMP_Error_Transaction_Collision					0x23
#define HCI_ERR_LMP_PDU_Not_Allowed						0x24
#define HCI_ERR_Encryption_Mode_Not_Acceptable					0x25
#define HCI_ERR_Unit_Key_Used							0x26
#define HCI_ERR_QoS_Not_Supported						0x27
#define HCI_ERR_Instant_Passed							0x28
#define HCI_ERR_Pairing_With_Unit_Key_Not_Supported				0x29

#endif
