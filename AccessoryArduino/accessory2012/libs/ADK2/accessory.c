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
#include "Arduino.h"
#include "string.h"
#include "conf_usb.h"
#include "usb_drv.h"
#include "usb_ids.h"

#define ADK_INTERNAL
#include "Audio.h"
#include "usbh.h"

#define ACCESSORY_TIMEOUT 1000

static int inpipe = -1;
static int outpipe = -1;
static int audiopipe = -1;
static int audiopacketsize = -1;

/* audio state */
#define AUDBUFSIZE 682
#define AUDBUFCOUNT 3
static uint16_t *audbuf = 0x20100000; // flash memory controller's unused 4K buffer
static size_t audbufpos;
static int curraudbuf = 0;
#define MAX_SAMPLE_SKEW 50
static int audsamplerate;
static int audsampleratebase;
static uint8_t audrecvbuf[256]; // receive buffer for the iso endpoint

static void audio_iso_callback(void *arg, int result, void *buf, size_t pos);
size_t process_audio(uint16_t *outbuf, uint16_t *inbuf, size_t len);

static int total_samp = 0;

#define READ8(ptr, pos) (*(((uint8_t *)ptr) + pos))
#define READ16(ptr, pos) (*(uint16_t *)(((uint8_t *)ptr) + pos))

int accessory_init(usbh_device_t *dev)
{
	TRACE_OTG("vid 0x%x, pid 0x%x, devaddr %d\n", dev->vid, dev->pid, dev->address);

	// main accessory endpoints
	uint8_t inendp = 0;
	size_t inpacketsize = 0;
	uint8_t outendp = 0;
	size_t outpacketsize = 0;

	// audio interfaces
	bool audio = false;
	uint8_t audioendp = 0;
	uint8_t audiointerface = 0;
	uint8_t audio_alternate_setting = 0;

	// find the correct endpoint
	int pos = 0;
	int total_len = READ16(dev->devconfig, OFFSET_FIELD_TOTAL_LENGTH);
	bool found_acc_interface = false;
	bool found_audio_interface = false;
	while (pos < total_len) {
		uint8_t length = READ8(dev->devconfig, pos);
		uint8_t type = READ8(dev->devconfig, pos + 1);

		TRACE_OTG("DESCRIPTOR: type %d, length %d\n", type, length);

		switch (type) {
			case DESCRIPTOR_INTERFACE: {
				uint8_t num_ep = READ8(dev->devconfig, pos + OFFSET_FIELD_NB_OF_EP);
				uint8_t class = READ8(dev->devconfig, pos + OFFSET_FIELD_CLASS);
				uint8_t sub_class = READ8(dev->devconfig, pos + OFFSET_FIELD_SUB_CLASS);
				uint8_t protocol = READ8(dev->devconfig, pos + OFFSET_FIELD_PROTOCOL);

				TRACE_OTG("\tINTERFACE: ep %d class 0x%x sub 0x%x prot 0x%x\n", num_ep, class, sub_class, protocol);

				// make sure we've cleared our corresponding endpoint search
				found_acc_interface = false;
				found_audio_interface = false;

				// look for the accessory interface
				if (num_ep == 2 &&
					class == 0xff &&
					sub_class == 0xff &&
					(inendp <= 0 || outendp <= 0)) {

					// this is probably our accessory interface
					found_acc_interface = true;
				} else if (class == 0x1 && // audio
					sub_class == 0x2 && // streaming
					num_ep > 0) { // alternate setting with actual endpoints

					audiointerface = READ8(dev->devconfig, pos + OFFSET_FIELD_INTERFACE_NB);
					audio_alternate_setting = READ8(dev->devconfig, pos + OFFSET_FIELD_ALT);

					found_audio_interface = true;
				}
				break;
			}
			case DESCRIPTOR_ENDPOINT: {
				uint8_t ep_type = READ8(dev->devconfig, pos + OFFSET_FIELD_EP_TYPE);
				uint8_t endp = READ8(dev->devconfig, pos + OFFSET_FIELD_EP_ADDR);
				size_t packetsize = READ16(dev->devconfig, pos + OFFSET_FIELD_EP_SIZE);

				TRACE_OTG("\tENDPOINT: type 0x%x addr 0x%x\n", ep_type, endp);

				if (found_acc_interface) {
					if (ep_type != 2) // bulk
						break;
					if (endp & 0x80) {
						// in
						if (inendp <= 0) {
							inendp = endp;
							inpacketsize = packetsize;
						}
					} else {
						// out
						if (outendp <= 0) {
							outendp = endp;
							outpacketsize = packetsize;
						}
					}
				}
				if (found_audio_interface) {
					if (ep_type != ((3 << 2) | (1 << 0))) // synchronous iso
						break;

					audiopacketsize = packetsize;
					audioendp = endp;
				}
				break;
			}
		}

		pos += length;
	}

	TRACE_OTG("inendp 0x%x (%d), outendp 0x%x (%d)\n", inendp, inpacketsize, outendp, outpacketsize);
	outpipe = usbh_setup_endpoint(dev->address, outendp, ENDP_TYPE_BULK, outpacketsize);
	TRACE_OTG("outpipe %d\n", outpipe);
	inpipe = usbh_setup_endpoint(dev->address, inendp, ENDP_TYPE_BULK, inpacketsize);
	TRACE_OTG("inpipe %d\n", inpipe);

	if (audioendp > 0) {
		TRACE_OTG("audio: interface %d alt setting %d endp 0x%x packet size %d\n",
			audiointerface, audio_alternate_setting, audioendp, audiopacketsize);

		audiopipe = usbh_setup_endpoint(dev->address, audioendp, ENDP_TYPE_ISO, audiopacketsize);
		TRACE_OTG("audiopipe %d\n", audiopipe);

		// enable audio
		// set configuration 1
		struct usb_setup_packet setup = (struct usb_setup_packet){
			USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE,
			SETUP_SET_INTERFACE,
			audio_alternate_setting,
			audiointerface,
			0
		};

		int len = usbh_send_setup(&setup, NULL, false);
		TRACE_OTG("send_setup returns %d\n", len);

		audsamplerate = 44100;
		audsampleratebase = 44100;
		audioOn(AUDIO_USB, audsamplerate);

		// enable SMC so we can use its 4K memory buffer
		pmc_enable_periph_clk(ID_SMC);

		usbh_queue_iso_transfer(audiopipe, audrecvbuf, sizeof(audrecvbuf), &audio_iso_callback, NULL);
	}

	return 0;
}

void accessory_deinit(void)
{
	inpipe = -1;
	outpipe = -1;
	audiopipe = -1;
}

void accessory_work(void)
{
#if 0
	static int last_samp = -1;

	if (total_samp - last_samp >= 44100) {
		last_samp = total_samp;
		TRACE_OTG("samp %d\n", total_samp);
	}
#endif
}

/* bulk in/out */
int accessory_send(const void *buf, size_t len)
{
	int res;

	if (outpipe < 0)
		return -1;
	if (!usbh_accessory_connected())
		return -1;

//	TRACE_OTG("buf %p, len %d... ", buf, len);

	res = usbh_check_transfer(outpipe);
	if (res != PIPE_RESULT_NOT_QUEUED) {
//		TRACE_OTG_NONL("pipe with bad result before transfer %d\n", res);
		return -1;
	}
	usbh_queue_transfer(outpipe, buf, len);

	res = -1;
	int now = millis();
	for (;;) {
		coopYield();

		res = usbh_check_transfer(outpipe);
		if (res >= 0)
			break;

		if (res < 0 && res != PIPE_RESULT_NOT_READY) {
//			TRACE_OTG_NONL("error %d in transfer\n", res);
			break;
		}
		if (millis() - now >= ACCESSORY_TIMEOUT) {
//			TRACE_OTG_NONL("timeout\n");
			// XXX cancel transfer
			break;
		}
	}

//	TRACE_OTG_NONL("res %d\n", res);
	return res;
}

int accessory_receive(void *buf, size_t len)
{
	int res;

	if (inpipe < 0)
		return -1;

	if (!usbh_accessory_connected())
		return -1;

//	TRACE_OTG("buf %p, len %d... ", buf, len);

	res = usbh_check_transfer(inpipe);
	if (res != PIPE_RESULT_NOT_QUEUED) {
//		TRACE_OTG_NONL("pipe with bad result before transfer %d\n", res);
		return -1;
	}
	usbh_queue_transfer(inpipe, buf, len);

	res = -1;
	int now = millis();
	for (;;) {
		coopYield();

		res = usbh_check_transfer(inpipe);
		if (res >= 0)
			break;

		if (res < 0 && res != PIPE_RESULT_NOT_READY) {
//			TRACE_OTG_NONL("error %d in transfer\n", res);
			break;
		}
		if (millis() - now >= ACCESSORY_TIMEOUT) {
//			TRACE_OTG_NONL("timeout\n");
			// XXX cancel transfer
			break;
		}
	}

//	TRACE_OTG_NONL("res %d\n", res);
	return res;
}

/* audio */
static void skew_sample_rate(int skew)
{
	audsamplerate += skew;
	if (audsamplerate - audsampleratebase > MAX_SAMPLE_SKEW)
		audsamplerate = audsampleratebase + MAX_SAMPLE_SKEW;
	if (audsampleratebase - audsamplerate > MAX_SAMPLE_SKEW)
		audsamplerate = audsampleratebase - MAX_SAMPLE_SKEW;

	audioSetSample(AUDIO_USB, audsamplerate);
}

static void audio_iso_callback(void *arg, int result, void *buf, size_t pos)
{
	if (pos == 0)
		return;

//	TRACE_OTG("arg %p result %d buf %p pos %u\n", arg, result, buf, pos);

	// set the next audio buffer
	usbh_set_iso_buffer(audiopipe, audrecvbuf, sizeof(audrecvbuf));

	// see if we can fit the processed one into the last buffer
	if (AUDBUFSIZE - audbufpos < pos / 4) {
		// we can't, kick this buffer and move onto the next
		int res = audioTryAddBuffer(AUDIO_USB, audbuf + curraudbuf * AUDBUFSIZE, audbufpos);
		if (!res) {
			//dbgPrintf("%d\n", a);
			//dbgPrintf("USB audio: Failed to queue audio buffer\n");
			//dbgPrintf("over\n");
			return;
		}

		static int lastres[2];
		if (res > (AUDBUFSIZE / 2)) {
			// we're overrunning the dac, increase its sample rate
			static int a = 0;
			if (lastres[1] < lastres[0] && lastres[0] < res) {
				a++;
				if (a > 5) {
					int adj = (res / (AUDBUFSIZE / 20));
					skew_sample_rate(adj);
					//dbgPrintf("o%d +%d %d\n", res, adj, audsamplerate);
					a = 0;
				}
			}
		} else {
			// we're underunning the dac, decrease its sample rate
			static int a = 0;

			if (res == 1 || lastres[1] > lastres[0] && lastres[0] > res) {
				a++;
				if (a > 5) {
					int adj = ((AUDBUFSIZE - res) / (AUDBUFSIZE / 20));
					skew_sample_rate(-adj);
					//dbgPrintf("u%d -%d %d\n", res, adj, audsamplerate);
					a = 0;
				}
			}
		}
		lastres[1] = lastres[0];
		lastres[0] = res;

		curraudbuf++;
		if (curraudbuf >= AUDBUFCOUNT)
			curraudbuf = 0;
		audbufpos = 0;
	}

	// process the one we just got
	size_t processed_samples = process_audio(audbuf + curraudbuf * AUDBUFSIZE + audbufpos, buf, pos);
	audbufpos += processed_samples;

	total_samp += processed_samples;
}

/* take 16 bit stereo signed samples and downconvert in place to mono 12 bit unsigned */
size_t process_audio(uint16_t *outbuf, uint16_t *inbuf, size_t len)
{
	len /= 2; // number of words

	// loop through the samples, combining left and right and decimating to 12 bits
	size_t i;
	for (i = 0; i < len/2; i++)  {
		int32_t temp = (int16_t)inbuf[i*2];
		int32_t temp2 = (int16_t)inbuf[i*2 + 1];

		temp = temp + temp2;

		int32_t vol = getVolume();
		temp *= vol;

		temp = temp >> (5 + 8);
		temp += 2048;
		temp &= 0xfff; // XXX dither the sample
		outbuf[i] = temp;
	}

	return i;
}


