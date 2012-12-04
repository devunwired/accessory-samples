/* This header file is part of the AVR Software Framework 2.0.0 release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Contains some IDs (VID, PID, Class numbers) used in the various
 *        demos.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ***************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */

#ifndef _USB_IDS_H_
#define _USB_IDS_H_


//_____ I N C L U D E S ____________________________________________________

//_____ M A C R O S ________________________________________________________

//_____ D E C L A R A T I O N S ____________________________________________

/*! \name Vendor ID (VID)
 */
//! @{
#define ATMEL_VID                             0x03EB
#define APPLE_VID                             0x05AC
//! @}

/*! \name USB Product ID (PID)
 */
//! @{
#define IPOD_PID                              0x1200
#define IPOD_SHUFFLE_PID                      0x1300
#define HID_GENERIC_EXAMPLE_PID               0x2013
#define HID_QTOUCH_DEBUG_PID                  0x211F
#define ENUM_EXAMPLE_PID                      0x2300
#define MS_EXAMPLE_PID                        0x2301
#define MS_SDRAM_LOADER_PID                   0x2302
#define EVK1100_CTRL_PANEL_DEMO_PID           0x2303
#define HID_EXAMPLE_PID                       0x2304
#define EVK1101_CTRL_PANEL_DEMO_HID_PID       0x2305
#define EVK1101_CTRL_PANEL_DEMO_HID_MS_PID    0x2306
#define CDC_EXAMPLE_PID                       0x2307
#define AUDIO_MIC_EXAMPLE_PID                 0x2308
#define EVK11xx_VIRTUAL_COM_PORT_PID          0x2310
#define AUDIO_SPEAKER_MICRO_EXAMPLE_PID       0x2311
#define ISP_UC3A_PID                          0x2FF8
#define ISP_UC3A3_PID                         0x2FF1
#define ISP_UC3B_PID                          0x2FF6

//! @}

#define  USB_ENDPOINT_OUT                    0x00
#define  USB_ENDPOINT_IN                     0x80

/*! \name Generic definitions (Class, subclass and protocol)
 */
//! @{
#define NO_CLASS                              0x00
#define NO_SUBCLASS                                0x00
#define NO_PROTOCOL                                     0x00
//! @}

/*! \name Audio specific definitions (Class, subclass and protocol)
 */
//! @{
#define AUDIO_CLASS                           0x01
#define AUDIOCONTROL_SUBCLASS                      0x01
#define AUDIOSTREAMING_SUBCLASS                    0x02
#define MIDISTREAMING_SUBCLASS                     0x03
//! @}

/*! \name CDC specific definitions (Class, subclass and protocol)
 */
//! @{
#define CDC_COMM_DEVICE_CLASS                 0x02
#define CDC_COMM_CLASS                        0x02
#define CDC_COMM_DIRECT_LINE_CM_SUBCLASS           0x01
#define CDC_COMM_ABSTRACT_CM_SUBCLASS              0x02
#define CDC_COMM_TELEPHONE_CM_SUBCLASS             0x03
#define CDC_COMM_MULTICHANNEL_CM_SUBCLASS          0x04
#define CDC_COMM_CAPI_CM_SUBCLASS                  0x05
#define CDC_COMM_ETHERNET_NETWORKING_CM_SUBCLASS   0x06
#define CDC_COMM_ATM_NETWORKING_CM_SUBCLASS        0x07
#define CDC_COMM_V25ter_PROTOCOL                        0x01
#define CDC_DATA_CLASS                        0x0A
#define CDC_DATA_SUBCLASS                          0x00
#define CDC_DATA_PHYS_ISDN_BRI_PROTOCOL                 0x30
#define CDC_DATA_HDLC_PROTOCOL                          0x31
#define CDC_DATA_TRANSPARENT_PROTOCOL                   0x32
//! @}

/*! \name HID specific definitions (Class, subclass and protocol)
 */
//! @{
#define HID_CLASS                             0x03
#define BOOT_SUBCLASS                              0x01
#define KEYBOARD_PROTOCOL                               0x01
#define MOUSE_PROTOCOL                                  0x02
//! @}

/*! \name MS specific definitions (Class, subclass and protocol)
 */
//! @{
#define MS_CLASS                              0x08
#define SFF8020I_MMC2_SUBCLASS                     0x02
#define SFF8070I_SUBCLASS                          0x05
#define SCSI_SUBCLASS                              0x06
#define BULK_PROTOCOL                                   0x50
//! @}

/*! \name DFU specific definitions (Class, subclass and protocol)
 */
//! @{
#define APPLICATION_CLASS                     0xFE
#define DFU_SUBCLASS                               0x01
#define RUNTIME_PROTOCOL                                0x01
#define DFU_MODE_PROTOCOL                               0x02
//! @}

/*! \name Others specific definitions (Class, subclass and protocol)
 */
//! @{
#define VENDOR_CLASS                          0xFF
//! @}

   //! \name Requests type (bmRequestTypes)
   //! @{

      //! \name Data transfer direction
      //! bit 7,
      //! 0 = Host to device
      //! 1 = Device to host
      //! @{
#define  USB_SETUP_DIR_HOST_TO_DEVICE        (0<<7)
#define  USB_SETUP_DIR_DEVICE_TO_HOST        (1<<7)
      //! @}

      //! \name Type
      //! bit 6 to 5,
      //! 0 = Standard
      //! 1 = Class
      //! 2 = Vendor
      //! 3 = Reserved
      //! @{
#define  USB_SETUP_TYPE_STANDARD             (0<<5)
#define  USB_SETUP_TYPE_CLASS                (1<<5)
#define  USB_SETUP_TYPE_VENDOR               (2<<5)
      //! @}

      //! \name Recipient
      //! bit 4 to 0,
      //! 0 = device
      //! 1 = Interface
      //! 2 = Endpoint
      //! 3 = Other
      //! 4...31 = Reserved
      //! @{
#define  USB_SETUP_RECIPIENT_DEVICE          (0)
#define  USB_SETUP_RECIPIENT_INTERFACE       (1)
#define  USB_SETUP_RECIPIENT_ENDPOINT        (2)
#define  USB_SETUP_RECIPIENT_OTHER           (3)
      //! @}

      //! \name Request type used by standard setup request
      //! @{
#define  USB_SETUP_SET_STAND_DEVICE          (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_DEVICE)    // 0x00
#define  USB_SETUP_GET_STAND_DEVICE          (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_DEVICE)    // 0x80
#define  USB_SETUP_SET_STAND_INTERFACE       (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_INTERFACE) // 0x01
#define  USB_SETUP_GET_STAND_INTERFACE       (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_INTERFACE) // 0x81
#define  USB_SETUP_SET_STAND_ENDPOINT        (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_ENDPOINT)  // 0x02
#define  USB_SETUP_GET_STAND_ENDPOINT        (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_STANDARD |USB_SETUP_RECIPIENT_ENDPOINT)  // 0x82
      //! @}

      //! \name Request type used by specific setup request from class driver
      //! @{
#define  USB_SETUP_SET_CLASS_DEVICE          (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_DEVICE)       // 0x20
#define  USB_SETUP_GET_CLASS_DEVICE          (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_DEVICE)       // 0xA0
#define  USB_SETUP_SET_CLASS_INTER           (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_INTERFACE)    // 0x21
#define  USB_SETUP_GET_CLASS_INTER           (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_INTERFACE)    // 0xA1
#define  USB_SETUP_SET_CLASS_ENDPOINT        (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_ENDPOINT)     // 0x22
#define  USB_SETUP_GET_CLASS_ENDPOINT        (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_ENDPOINT)     // 0xA2
#define  USB_SETUP_SET_CLASS_OTHER           (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_OTHER)        // 0x23
#define  USB_SETUP_GET_CLASS_OTHER           (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_CLASS |USB_SETUP_RECIPIENT_OTHER)        // 0xA3
#define  USB_SETUP_SET_VENDOR_DEVICE         (USB_SETUP_DIR_HOST_TO_DEVICE |USB_SETUP_TYPE_VENDOR |USB_SETUP_RECIPIENT_DEVICE)      // 0x40
#define  USB_SETUP_GET_VENDOR_DEVICE         (USB_SETUP_DIR_DEVICE_TO_HOST |USB_SETUP_TYPE_VENDOR |USB_SETUP_RECIPIENT_DEVICE)      // 0xC0
      //! @}
   //! @}

   //! \name Standard Requests (bRequest)
   //! @{
#define  SETUP_GET_STATUS                    0x00
#define  SETUP_GET_DEVICE                    0x01
#define  SETUP_CLEAR_FEATURE                 0x01
#define  SETUP_GET_STRING                    0x03
#define  SETUP_SET_FEATURE                   0x03
#define  SETUP_SET_ADDRESS                   0x05
#define  SETUP_GET_DESCRIPTOR                0x06
#define  SETUP_SET_DESCRIPTOR                0x07
#define  SETUP_GET_CONFIGURATION             0x08
#define  SETUP_SET_CONFIGURATION             0x09
#define  SETUP_GET_INTERFACE                 0x0A
#define  SETUP_SET_INTERFACE                 0x0B
#define  SETUP_SYNCH_FRAME                   0x0C
   //! @}

   //! \name Descriptor types used in several setup requests
   //! @{
#define  DESCRIPTOR_DEVICE                   0x01
#define  DESCRIPTOR_CONFIGURATION            0x02
#define  DESCRIPTOR_STRING                   0x03
#define  DESCRIPTOR_INTERFACE                0x04
#define  DESCRIPTOR_ENDPOINT                 0x05
#define  DESCRIPTOR_DEVICE_QUALIFIER         0x06
#define  DESCRIPTOR_CONF_OTHER_SPEED         0x07
#define  DESCRIPTOR_OTG                      0x09
#define  DESCRIPTOR_IAD                      0x0B
   //! @}

   //! \name Feature types for SETUP_X_FEATURE standard request
   //! @{
#define  FEATURE_DEVICE_REMOTE_WAKEUP        0x01
#define  FEATURE_DEVICE_TEST                 0x02
#define  FEATURE_DEVICE_OTG_B_HNP_ENABLE     0x03
#define  FEATURE_DEVICE_OTG_A_HNP_SUPPORT    0x04
#define  FEATURE_DEVICE_OTG_A_ALT_HNP_SUPPORT 0x05
#define  FEATURE_ENDPOINT_HALT               0x00
   //! @}

   //! \name Feature types for SETUP_X_FEATURE standard test request
   //! @{
#define  FEATURE_DEVICE_TEST_J               0x01
#define  FEATURE_DEVICE_TEST_K               0x02
#define  FEATURE_DEVICE_TEST_SEO_NAK         0x03
#define  FEATURE_DEVICE_TEST_PACKET          0x04
#define  FEATURE_DEVICE_TEST_FORCE_ENABLE    0x05
   //! @}
//! @}

#define MAX_EP_PER_INTERFACE 3

//! USB Setup Data
struct usb_setup_packet
{
  U8    bmRequestType;        //!< Characteristics of the request
  U8    bRequest;             //!< Specific request
  U16   wValue;               //!< Field that varies according to request
  U16   wIndex;               //!< Field that varies according to request
  U16   wLength;              //!< Number of bytes to transfer if Data
};

#define CONTROL_GOOD          0x00
#define CONTROL_DATA_TOGGLE   0x01
#define CONTROL_DATA_PID      0x02
#define CONTROL_PID           0x04
#define CONTROL_TIMEOUT       0x08
#define CONTROL_CRC16         0x10
#define CONTROL_STALL         0x20
#define CONTROL_NO_DEVICE     0x40

//! Offsets common to all descriptor types
#define OFFSET_DESCRIPTOR_LENGTH        0
#define OFFSET_FIELD_DESCRIPTOR_TYPE    1

//! Offsets in device descriptors
#define OFFSET_FIELD_MAXPACKETSIZE      7
#define OFFSET_FIELD_VID                8
#define OFFSET_FIELD_PID               10
#define OFFSET_FIELD_NB_CONFIGURATION  17

//! Offsets in configuration descriptors
#define OFFSET_FIELD_TOTAL_LENGTH       2
#define OFFSET_FIELD_NB_INTERFACE       4
#define OFFSET_FIELD_CONFIGURATION_NB   5
#define OFFSET_FIELD_BMATTRIBUTES       7
#define REMOTE_WAKEUP_BIT     5
#define REMOTE_WAKEUP_MASK    (1 << REMOTE_WAKEUP_BIT)
#define SELF_POWERED_BIT      6
#define SELF_POWERED_MASK     (1 << SELF_POWERED_BIT)
#define OFFSET_FIELD_MAXPOWER           8

//! Offsets in interface descriptors
#define OFFSET_FIELD_INTERFACE_NB       2
#define OFFSET_FIELD_ALT                3
#define OFFSET_FIELD_NB_OF_EP           4
#define OFFSET_FIELD_CLASS              5
#define OFFSET_FIELD_SUB_CLASS          6
#define OFFSET_FIELD_PROTOCOL           7

//! Offsets in endpoint descriptors
#define OFFSET_FIELD_EP_ADDR            2
#define OFFSET_FIELD_EP_TYPE            3
#define OFFSET_FIELD_EP_SIZE            4
#define OFFSET_FIELD_EP_INTERVAL        6

#define host_clear_endpoint_feature(ep) \
(\
  usb_request.bmRequestType   = 0x02,\
  usb_request.bRequest        = CLEAR_FEATURE,\
  usb_request.wValue          = FEATURE_ENDPOINT_HALT << 8,\
  usb_request.wIndex          = (ep),\
  usb_request.wLength         = 0,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "set configuration" request
//! @param cfg_nb U8: Configuration to activate
//! @return Status
#define host_set_configuration(cfg_nb) \
(\
  usb_request.bmRequestType   = 0x00,\
  usb_request.bRequest        = SET_CONFIGURATION,\
  usb_request.wValue          = (cfg_nb),\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = 0,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "set interface" request to specify an alternate setting for an interface
//! @param interface_nb U8: Interface
//! @param alt_setting U8: Alternate setting
//! @return Status
#define host_set_interface(interface_nb, alt_setting) \
(\
  usb_request.bmRequestType   = 0x01,\
  usb_request.bRequest        = SET_INTERFACE,\
  usb_request.wValue          = (alt_setting),\
  usb_request.wIndex          = (interface_nb),\
  usb_request.wLength         = 0,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send an incomplete "get device desriptor" request
//! The descriptor received is stored in the data_stage array.
//! The received descriptors are limited to the length of the control pipe.
//! @return Status
#define host_get_device_descriptor_incomplete() \
(\
  usb_request.bmRequestType   = 0x80,\
  usb_request.bRequest        = GET_DESCRIPTOR,\
  usb_request.wValue          = DEVICE_DESCRIPTOR << 8,\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = 64,\
  usb_request.incomplete_read = TRUE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "get device desriptor" request
//! The descriptor received is stored in the data_stage array.
//! @return Status
#define host_get_device_descriptor() \
(\
  usb_request.bmRequestType   = 0x80,\
  usb_request.bRequest        = GET_DESCRIPTOR,\
  usb_request.wValue          = DEVICE_DESCRIPTOR << 8,\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = 18,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "get device configuration descriptor" request
//! The configuration descriptor received is stored in the data_stage array.
//! @param cfg_ix U8: Index of the configuration descriptor to get
//! @return Status
#define host_get_configuration_descriptor(cfg_ix) \
(\
  usb_request.bmRequestType   = 0x80,\
  usb_request.bRequest        = GET_DESCRIPTOR,\
  usb_request.wValue          = CONFIGURATION_DESCRIPTOR << 8 | (cfg_ix),\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = SIZEOF_DATA_STAGE,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "set address" request
//! @param addr U8: Address assigned to the device
//! @return Status
#define host_set_address(addr) \
(\
  usb_request.bmRequestType   = 0x00,\
  usb_request.bRequest        = SET_ADDRESS,\
  usb_request.wValue          = (addr),\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = 0,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send a "set feature" "device remote wake-up"
//! @return Status
#define host_set_feature_remote_wakeup() \
(\
  usb_request.bmRequestType   = 0x00,\
  usb_request.bRequest        = SET_FEATURE,\
  usb_request.wValue          = 1,\
  usb_request.wIndex          = 1,\
  usb_request.wLength         = 0,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

//! @brief Send the mass-storage specific request "get max LUN"
//! @return Status
#define host_ms_get_max_lun() \
(\
  usb_request.bmRequestType   = 0xA1,\
  usb_request.bRequest        = MS_GET_MAX_LUN,\
  usb_request.wValue          = 0,\
  usb_request.wIndex          = 0,\
  usb_request.wLength         = 1,\
  usb_request.incomplete_read = FALSE,\
  host_transfer_control(data_stage)\
)

#endif  // _USB_IDS_H_
