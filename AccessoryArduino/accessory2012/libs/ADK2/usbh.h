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
#ifdef ADK_INTERNAL
#ifndef __USB_USBH_H
#define __USB_USBH_H

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct usbh_device {
	uint8_t address;
	uint8_t devdesc[64];
	uint8_t devconfig[512]; // XXX max

	uint16_t vid;
	uint16_t pid;
} usbh_device_t;

#define PIPE_RESULT_OK    (0)
#define PIPE_RESULT_NOT_READY (-1)
#define PIPE_RESULT_NOT_QUEUED (-2)
#define PIPE_RESULT_ERR   (-3)
#define PIPE_RESULT_RESET (-4)
#define PIPE_RESULT_NAK   (-5)
#define PIPE_RESULT_STALL (-6)

#define DEFAULT_ACCESSORY_STRING_VENDOR "Google, Inc."
#define DEFAULT_ACCESSORY_STRING_NAME   "DemoKit"
#define DEFAULT_ACCESSORY_STRING_LONGNAME "DemoKit ADK2012"
#define DEFAULT_ACCESSORY_STRING_VERSION  "2.0"
#define DEFAULT_ACCESSORY_STRING_URL    "http://www.android.com"
#define DEFAULT_ACCESSORY_STRING_SERIAL "0000000012345678"

void usbh_init(void);
void usbh_start(void);
void usbh_work(void);

void usbh_set_accessory_string_vendor(const char *str);
void usbh_set_accessory_string_name(const char *str);
void usbh_set_accessory_string_longname(const char *str);
void usbh_set_accessory_string_version(const char *str);
void usbh_set_accessory_string_url(const char *str);
void usbh_set_accessory_string_serial(const char *str);

enum usbh_endp_type { 
	ENDP_TYPE_CONTROL,
	ENDP_TYPE_BULK,
	ENDP_TYPE_INT,
	ENDP_TYPE_ISO
};

int usbh_accessory_connected(void);

int usbh_setup_endpoint(uint8_t addr, uint8_t endp, enum usbh_endp_type type, size_t packet_size);
int usbh_free_endpoint(int pipe);

int usbh_queue_transfer(int pipe, void *buf, size_t len);
int usbh_check_transfer(int pipe);
int usbh_send_setup(const void *setup, void *buf, int incomplete);

/* isochronous transfers */
typedef void (*usbh_iso_callback_t)(void *arg, int result, void *buf, size_t pos);
int usbh_queue_iso_transfer(int pipe, void *buf, size_t buflen, usbh_iso_callback_t cb, void *arg);
int usbh_set_iso_buffer(int pipe, void *buf, size_t buflen);

uint32_t usbh_get_frame_num(void);

int accessory_init(usbh_device_t *dev);
void accessory_deinit(void);

int accessory_send(const void *buf, size_t len);
int accessory_receive(void *buf, size_t len);

int accessory_send_hid(void *buf, size_t len);

void accessory_work(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
