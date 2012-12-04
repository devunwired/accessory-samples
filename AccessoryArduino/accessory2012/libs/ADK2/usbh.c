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
#define ADK_INTERNAL
#include "Arduino.h"
#include "usbh.h"
#include "coop.h"
#include "conf_usb.h"
#include "usb_drv.h"
#include "usb_ids.h"

#define USE_HIGH_SPEED 0
#define AUDIO_ACCESSORY 0
#define APP_ACCESSORY 1

#define ACC_ID 10

#define PIPE_FLAG_INCOMPLETE_SETUP 0x1

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct usbh_pipe {
	volatile enum pipe_state {
		PIPE_NO_CONFIG,
		PIPE_IDLE,
		PIPE_SETUP_DTOH,
		PIPE_SETUP_DTOH_IN,
		PIPE_SETUP_DTOH_OUT,
		PIPE_SETUP_HTOD,
		PIPE_SETUP_HTOD_OUT,
		PIPE_SETUP_HTOD_IN,
		PIPE_SETUP_COMPLETE,
		PIPE_IN,
		PIPE_IN_COMPLETE,
		PIPE_OUT,
		PIPE_OUT_COMPLETE,
		PIPE_ISO_IN,
		PIPE_ISO_IN_WAIT,
	} state;
	uint32_t flags;
	uint8_t endp;
	enum usbh_endp_type type;

	/* current transfer stats */
	void *ptr;
	size_t pos;
	size_t total_len;
	int result;

	/* isochronous transfers */
	usbh_iso_callback_t iso_cb;
	void *iso_cb_arg;
} usbh_pipe_t;

typedef struct usbh_state {
	enum {
		USBH_DISABLED,
		USBH_INIT,
		USBH_DEVICE_UNATTACHED,
		USBH_WAIT_FOR_DEVICE,
		USBH_DEVICE_ATTACHED,
		USBH_DEVICE_ATTACHED_SOF_WAIT,
		USBH_DEVICE_ATTACHED_RESET,
		USBH_DEVICE_ATTACHED_RESET_WAIT,
		USBH_DEVICE_ATTACHED_POST_RESET_WAIT,
		USBH_DEVICE_ATTACHED_QUERY,
		USBH_DEVICE_TRY_ACCESSORY,
		USBH_DEVICE_ACCESSORY_INIT,
		USBH_DEVICE_ACCESSORY,
		USBH_DEVICE_IDLE,
		USBH_DEVICE_REGISTER_HID,
	} state;
	uint8_t next_address;
	uint32_t last_sof;
	uint32_t irq_count;

	usbh_device_t dev;

	usbh_pipe_t pipe[CHIP_USB_NUMENDPOINTS];

	uint64_t sleep_ts;


	// accessory strings
	const char *accessory_string_vendor;
	const char *accessory_string_name;
	const char *accessory_string_longname;
	const char *accessory_string_version;
	const char *accessory_string_url;
	const char *accessory_string_serial;
} usbh_state_t;

/* master state for the stack */
static usbh_state_t usbh;

static const char *pipe_state_to_str(enum pipe_state state)
{
#define STATE2STR(x) case x: return #x;

	switch (state) {
		STATE2STR(PIPE_NO_CONFIG)
		STATE2STR(PIPE_IDLE)
		STATE2STR(PIPE_SETUP_DTOH)
		STATE2STR(PIPE_SETUP_DTOH_IN)
		STATE2STR(PIPE_SETUP_DTOH_OUT)
		STATE2STR(PIPE_SETUP_HTOD)
		STATE2STR(PIPE_SETUP_HTOD_OUT)
		STATE2STR(PIPE_SETUP_HTOD_IN)
		STATE2STR(PIPE_SETUP_COMPLETE)
		STATE2STR(PIPE_IN)
		STATE2STR(PIPE_IN_COMPLETE)
		STATE2STR(PIPE_OUT)
		STATE2STR(PIPE_OUT_COMPLETE)
		default: return "unknown";
	}

#undef STATE2STR
}

static void dump_pipe(int pipe)
{
	const usbh_pipe_t *p = &usbh.pipe[pipe];

	TRACE_OTG("pipe %d: state %d (%s) flags 0x%x ptr 0x%x pos %u total_len %u\n",
		pipe, p->state, pipe_state_to_str(p->state), p->flags, p->ptr, p->pos, p->total_len);
}

static void usbh_start_sleep(void)
{
	usbh.sleep_ts = millis();
}

static bool usbh_sleep_expired(uint32_t ms)
{
	uint64_t now = millis();

	if (now - usbh.sleep_ts >= ms)
		return true;
	else
		return false;
}

int usbh_setup_endpoint(uint8_t addr, uint8_t endp, enum usbh_endp_type type, size_t packet_size)
{
	// find a free pipe
	int pipe;
	for (pipe = 0; pipe < CHIP_USB_NUMENDPOINTS; pipe++) {
		if (usbh.pipe[pipe].state == PIPE_NO_CONFIG)
			break;
	}
	if (pipe >= CHIP_USB_NUMENDPOINTS)
		return -1;

	usbh_pipe_t *p = &usbh.pipe[pipe];

	p->state = PIPE_IDLE;
	p->endp = endp;
	p->type = type;

	// set up hardware
	uint32_t token = (endp & USB_ENDPOINT_IN) ? TOKEN_IN : TOKEN_OUT;
	switch (type) {
		default:
		case ENDP_TYPE_CONTROL:
			Host_configure_pipe(pipe, 0, endp, TYPE_CONTROL, TOKEN_SETUP, packet_size, SINGLE_BANK);
			break;
		case ENDP_TYPE_BULK:
			Host_configure_pipe(pipe, 0, endp, TYPE_BULK, token, packet_size, SINGLE_BANK);
			break;
		case ENDP_TYPE_INT:
			Host_configure_pipe(pipe, 0, endp, TYPE_INTERRUPT, token, packet_size, SINGLE_BANK);
			break;
		case ENDP_TYPE_ISO:
			Host_configure_pipe(pipe, 0, endp, TYPE_ISOCHRONOUS, token, packet_size, SINGLE_BANK);
			break;
	}
	Host_configure_address(pipe, addr);
	Host_enable_pipe_interrupt(pipe);

	return pipe;
}

int usbh_free_endpoint(int pipe)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

    Host_disable_pipe_interrupt(pipe);
    Host_reset_pipe(pipe);
    Host_unallocate_memory(pipe);
    Host_disable_pipe(pipe);

	memset(p, 0, sizeof(usbh_pipe_t));

	return 0;
}

static void usbh_start_iso_transfer(int pipe)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	ASSERT(p->type == ENDP_TYPE_ISO);

	if (p->endp & USB_ENDPOINT_IN) {
		p->state = PIPE_ISO_IN;
		Disable_global_interrupt();
		Host_reset_pipe(pipe);
		UOTGHS->UOTGHS_HSTPIPERR[pipe] = 0;
		Host_ack_in_received(pipe); Host_enable_in_received_interrupt(pipe);
		Host_ack_pipe_error(pipe); Host_enable_pipe_error_interrupt(pipe);
		Enable_global_interrupt();
		Host_disable_continuous_in_mode(pipe);
		Host_configure_pipe_token(pipe, TOKEN_IN);
		Host_ack_in_received(pipe);
		Host_unfreeze_pipe(pipe);
	} else {
		panic("unsupported iso out\n");
	}
}

int usbh_queue_iso_transfer(int pipe, void *buf, size_t buflen, usbh_iso_callback_t cb, void *arg)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

//	TRACE_OTG("pipe %d buf %p buflen %d cb %p arg %p\n", pipe, buf, buflen, cb, arg);

	ASSERT(p->type == ENDP_TYPE_ISO);
	ASSERT(p->state == PIPE_IDLE);

	p->ptr = buf;
	p->total_len = buflen;
	p->pos = 0;

	/* iso specific bits */
	p->iso_cb = cb;
	p->iso_cb_arg = arg;

	ASSERT(p->endp & USB_ENDPOINT_IN);

	usbh_start_iso_transfer(pipe);

	return 0;
}

int usbh_set_iso_buffer(int pipe, void *buf, size_t buflen)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	ASSERT(p->type == ENDP_TYPE_ISO);

	p->ptr = buf;
	p->total_len = buflen;
	p->pos = 0;

	return 0;
}

int usbh_queue_transfer(int pipe, void *buf, size_t len)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	//TRACE_OTG("pipe %d, endp 0x%x, buf %p, len 0x%x\n", pipe, p->endp, buf, len);

	ASSERT(p->state == PIPE_IDLE);

	p->ptr = buf;
	p->total_len = len;
	p->pos = 0;

	if (p->type == ENDP_TYPE_BULK || p->type == ENDP_TYPE_ISO) {
		if (p->endp & USB_ENDPOINT_IN) {
			p->state = PIPE_IN;
			Disable_global_interrupt();
			Host_reset_pipe(pipe);
			UOTGHS->UOTGHS_HSTPIPERR[pipe] = 0;
			Host_ack_in_received(pipe); Host_enable_in_received_interrupt(pipe);
			Host_ack_nak_received(pipe); Host_enable_nak_received_interrupt(pipe);
			//Host_ack_short_packet(pipe); Host_enable_short_packet_interrupt(pipe);
			Host_ack_pipe_error(pipe); Host_enable_pipe_error_interrupt(pipe);
			Enable_global_interrupt();
			Host_disable_continuous_in_mode(pipe);
			Host_configure_pipe_token(pipe, TOKEN_IN);
			Host_ack_in_received(pipe);
			Host_unfreeze_pipe(pipe);
		} else {
			p->state = PIPE_OUT;
			Disable_global_interrupt();
			Host_reset_pipe(pipe);
			Host_configure_pipe_token(pipe, TOKEN_OUT);
			Host_ack_out_ready(pipe); Host_enable_out_ready_interrupt(pipe);
			Host_ack_pipe_error(pipe); Host_enable_pipe_error_interrupt(pipe);
			//Host_enable_ping(pipe); // XXX ONLY for usb 2.0

			Host_unfreeze_pipe(pipe);
			Enable_global_interrupt();

			Host_reset_pipe_fifo_access(pipe);

			size_t towrite = MIN(p->total_len, Host_get_pipe_size(pipe));
			uint32_t written = host_write_p_txpacket(pipe, p->ptr, towrite, NULL);
			p->pos += written;

			Host_ack_out_ready(pipe);
			Host_send_out(pipe);
		}
	} else {
		panic("unimplemented\n");
	}
#if 0
	dbgPrintf("INRQ 0x%x\n", UOTGHS->UOTGHS_HSTPIPINRQ[pipe]);
	dbgPrintf("PIPCFG 0x%x\n", UOTGHS->UOTGHS_HSTPIPCFG[pipe]);
	dbgPrintf("PIPIMR 0x%x\n", UOTGHS->UOTGHS_HSTPIPIMR[pipe]);
	dbgPrintf("PIPERR 0x%x\n", UOTGHS->UOTGHS_HSTPIPERR[pipe]);
	dbgPrintf("PIP 0x%x\n", UOTGHS->UOTGHS_HSTPIP);
#endif

	return 0;
}

int usbh_check_transfer(int pipe)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	// poll the pipe to wait for it to get to complete
	int result;
	switch (p->state) {
		case PIPE_IN_COMPLETE:
		case PIPE_OUT_COMPLETE:
			p->state = PIPE_IDLE;
			if (p->result < 0) {
				result = p->result;
			} else {
				result = p->pos;
			}
			p->result = PIPE_RESULT_OK;
			break;
		case PIPE_IN:
		case PIPE_OUT:
			result = PIPE_RESULT_NOT_READY;
			break;
		default:
		case PIPE_NO_CONFIG:
		case PIPE_IDLE:
			result = PIPE_RESULT_NOT_QUEUED;
			break;
	}

	return result;
}

uint32_t usbh_get_frame_num(void)
{
	return (UOTGHS->UOTGHS_HSTFNUM >> 3) & 0x7ff;
}

static void usbh_host_reset_pipe(int pipe)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	switch (p->state) {
		case PIPE_SETUP_DTOH:
		case PIPE_SETUP_DTOH_IN:
		case PIPE_SETUP_DTOH_OUT:
		case PIPE_SETUP_HTOD:
		case PIPE_SETUP_HTOD_OUT:
		case PIPE_SETUP_HTOD_IN:
		case PIPE_SETUP_COMPLETE:
			p->state = PIPE_SETUP_COMPLETE;
			p->result = PIPE_RESULT_RESET;
			break;
		default:
			p->state = PIPE_IDLE;
			;
	}
}

static void usbh_reset_all_pipes(void)
{
	int i;
	for (i = 0; i < CHIP_USB_NUMENDPOINTS; i++) {
		usbh_host_reset_pipe(i);
	}
}

static void usbh_clear_all_pipes(void)
{
	int i;
	for (i = 0; i < CHIP_USB_NUMENDPOINTS; i++) {
		usbh_free_endpoint(i);
	}
}

static void usbh_host_pipe_irq(int pipe)
{
	usbh_pipe_t *p = &usbh.pipe[pipe];

	uint32_t pipe_isr = UOTGHS->UOTGHS_HSTPIPISR[pipe];
//	TRACE_OTG("pipe isr 0x%x\n", pipe_isr);
	pipe_isr &= UOTGHS->UOTGHS_HSTPIPIMR[pipe];
//	TRACE_OTG("pipe %d isr 0x%x (post mask)\n", pipe, pipe_isr);
//	dump_pipe(pipe);

	if (pipe_isr & UOTGHS_HSTPIPISR_TXSTPI) { // transmitted setup
		Host_ack_setup_ready();
		Host_disable_setup_ready_interrupt();
		if (p->state == PIPE_SETUP_DTOH) {
			if (p->total_len > 0) {
				// deal with the IN token
				Host_disable_continuous_in_mode(pipe);
				Host_configure_pipe_token(pipe, TOKEN_IN);
				Host_ack_in_received_free(pipe); Host_enable_in_received_interrupt(pipe);
				Host_ack_stall(pipe); Host_enable_stall_interrupt(pipe);
				Host_unfreeze_pipe(pipe);

				p->state = PIPE_SETUP_DTOH_IN;
			} else {
				panic("unhandled situation\n");
				p->state = PIPE_SETUP_DTOH_OUT;
			}
		} else if (p->state == PIPE_SETUP_HTOD) {
			if (p->total_len > 0) {
				// out token phase
				Host_configure_pipe_token(pipe, TOKEN_OUT);
				Host_ack_out_ready(pipe); Host_enable_out_ready_interrupt(pipe);
				Host_ack_stall(pipe); Host_enable_stall_interrupt(pipe);

				Host_unfreeze_pipe(pipe);

				Host_reset_pipe_fifo_access(pipe);
				uint32_t written = host_write_p_txpacket(pipe, p->ptr, p->total_len, NULL);
				//TRACE_OTG("%d bytes written to packet\n", written);
				p->pos += written;

				Host_send_out(pipe);

//				panic("unhandled situation: out phase with bytes\n");
				p->state = PIPE_SETUP_HTOD_OUT;
			} else {
				// deal with the IN token
				Host_disable_continuous_in_mode(pipe);
				Host_configure_pipe_token(pipe, TOKEN_IN);
				Host_ack_in_received_free(pipe); Host_enable_in_received_interrupt(pipe);
				Host_ack_stall(pipe); Host_enable_stall_interrupt(pipe);
				Host_unfreeze_pipe(pipe);

				p->state = PIPE_SETUP_HTOD_IN;
			}
		} else {
			panic("bad state %d (%s) with TXSTPI\n", p->state, pipe_state_to_str(p->state));
		}
	}
	if (pipe_isr & UOTGHS_HSTPIPISR_RXINI) { // in packet
		if (p->state == PIPE_SETUP_DTOH_IN) {
			//TRACE_OTG("got in, ");

			Host_reset_pipe_fifo_access(pipe);

			uint32_t read = host_read_p_rxpacket(pipe, (uint8_t *)p->ptr + p->pos, p->total_len - p->pos, NULL);
			p->pos += read;

			//TRACE_OTG_NONL("read %d\n", read);

			Host_freeze_pipe(pipe);

			if ((p->flags & PIPE_FLAG_INCOMPLETE_SETUP) || read < Host_get_pipe_size(pipe) || p->pos == p->total_len) {
				// deal with OUT token
				//TRACE_OTG("done, moving to OUT phase\n");
				Host_ack_in_received(pipe);
				Host_disable_in_received_interrupt(pipe);
				Host_configure_pipe_token(pipe, TOKEN_OUT);
				Host_ack_out_ready_send(pipe); Host_enable_out_ready_interrupt(pipe);
				Host_ack_stall(pipe); Host_enable_stall_interrupt(pipe);
				Host_unfreeze_pipe(pipe);
				p->state = PIPE_SETUP_DTOH_OUT;
			} else {
				Host_ack_in_received_free(pipe);
				Host_unfreeze_pipe(pipe);
			}
		} else if (p->state == PIPE_SETUP_HTOD_IN) {
			Host_free_in(pipe);
			Host_disable_in_received_interrupt(pipe);
			p->state = PIPE_SETUP_COMPLETE;
			//TRACE_OTG("end of in, done\n");
		} else if (p->state == PIPE_IN) {
			// regular pipe in in mode
			Host_ack_in_received(pipe);

			//TRACE_OTG("got in, ");

			Host_reset_pipe_fifo_access(pipe);
			uint32_t read = host_read_p_rxpacket(pipe, (uint8_t *)p->ptr + p->pos, p->total_len - p->pos, NULL);
			p->pos += read;

			//TRACE_OTG_NONL("read %d\n", read);

			Host_freeze_pipe(pipe);

			if (read < Host_get_pipe_size(pipe) || p->pos == p->total_len) {
				Host_reset_pipe(pipe);
				p->state = PIPE_IN_COMPLETE;
				p->result = PIPE_RESULT_OK;
			} else {
				// multi in transfer
				//Host_configure_pipe_token(pipe, TOKEN_IN);
				Host_ack_in_received_free(pipe);
				Host_unfreeze_pipe(pipe);
			}
		} else if (p->state == PIPE_ISO_IN) {
			// isochronous pipe in in mode
			Host_ack_in_received(pipe);

			//TRACE_OTG("ISO got in, ");

			Host_reset_pipe_fifo_access(pipe);
			uint32_t read = host_read_p_rxpacket(pipe, (uint8_t *)p->ptr + p->pos, p->total_len - p->pos, NULL);
			p->pos += read;

			//TRACE_OTG_NONL("read %d\n", read);

			Host_freeze_pipe(pipe);

			p->state = PIPE_ISO_IN_WAIT;
			p->result = PIPE_RESULT_OK;

			Host_ack_in_received(pipe);

			// schedule for a new sof interrupt
			Host_enable_sof_interrupt();
			usbh.last_sof = (UOTGHS->UOTGHS_HSTFNUM >> 3) & 0x7ff;

			// do callback
			p->iso_cb(p->iso_cb_arg, p->result, p->ptr, p->pos);
		} else {
			panic("bad state %d (%s) with RXINIT\n", p->state, pipe_state_to_str(p->state));
		}
	}
	if (pipe_isr & UOTGHS_HSTPIPISR_TXOUTI) { // out packet complete
		if (p->state == PIPE_SETUP_DTOH_OUT) {
			Host_disable_out_ready_interrupt(pipe);
			Host_ack_out_ready(pipe);
			p->state = PIPE_SETUP_COMPLETE;
			p->result = PIPE_RESULT_OK;
			//TRACE_OTG("end of out, done\n");
		} else if (p->state == PIPE_SETUP_HTOD_OUT) {
			if (p->pos == p->total_len) {
				// deal with the IN token
				Host_disable_out_ready_interrupt(pipe);
				Host_ack_out_ready(pipe);
				Host_disable_continuous_in_mode(pipe);
				Host_configure_pipe_token(pipe, TOKEN_IN);
				Host_ack_in_received_free(pipe); Host_enable_in_received_interrupt(pipe);
				Host_ack_stall(pipe); Host_enable_stall_interrupt(pipe);
				Host_unfreeze_pipe(pipe);
				p->state = PIPE_SETUP_HTOD_IN;
			} else {
				panic("unhandled situation, multi out control transfer\n");
				Host_ack_out_ready(pipe);
				Host_unfreeze_pipe(pipe);
			}
		} else if (p->state == PIPE_OUT) {
			// regular pipe in out mode
			Host_ack_out_ready(pipe);
			if (p->pos == p->total_len) {
				//TRACE_OTG("OUT complete\n");
				Host_reset_pipe(pipe);
				p->state = PIPE_OUT_COMPLETE;
				p->result = PIPE_RESULT_OK;
			} else {
				//Host_reset_pipe(pipe);
				Host_configure_pipe_token(pipe, TOKEN_OUT);
				Host_ack_out_ready(pipe); Host_enable_out_ready_interrupt(pipe);
				Host_ack_pipe_error(pipe); Host_enable_pipe_error_interrupt(pipe);

				Host_unfreeze_pipe(pipe);

				Host_reset_pipe_fifo_access(pipe);
				size_t towrite = MIN(p->total_len - p->pos, Host_get_pipe_size(pipe));
				//TRACE_OTG("second part of out: towrite %d pos %d\n", towrite, p->pos);
				uint32_t written = host_write_p_txpacket(pipe, p->ptr + p->pos, towrite, NULL);
				p->pos += written;

				Host_ack_out_ready(pipe);
				Host_send_out(pipe);
			}
		} else {
			panic("bad state %d (%s) with TXOUTI\n", p->state, pipe_state_to_str(p->state));
		}
	}
	if (pipe_isr & UOTGHS_HSTPIPISR_NAKEDI) { // nak received
		//TRACE_OTG("pipe %d got nak\n", pipe);
		Host_ack_nak_received(pipe);
		Host_disable_nak_received_interrupt(pipe);
		if (p->state == PIPE_IN) {
			Host_reset_pipe(pipe);
			p->state = PIPE_IN_COMPLETE;
			p->result = PIPE_RESULT_NAK;
		} if (p->state == PIPE_IN_COMPLETE) {
			; // do nothing
		} else {
			panic("bad state %d (%s) with NAKEDI\n", p->state, pipe_state_to_str(p->state));
		}
	}
	if (pipe_isr & UOTGHS_HSTPIPISR_RXSTALLDI) { // stall received
		Host_disable_stall_interrupt(pipe);
		Host_ack_stall(pipe);
		if (p->state == PIPE_SETUP_DTOH_IN) {
			Host_disable_in_received_interrupt(pipe);
			p->state = PIPE_SETUP_COMPLETE;
			p->result = PIPE_RESULT_STALL;
		} else if (p->state == PIPE_SETUP_HTOD_IN) {
			Host_disable_in_received_interrupt(pipe);
			p->state = PIPE_SETUP_COMPLETE;
			p->result = PIPE_RESULT_STALL;
		} else {
			panic("bad state %d (%s) with RXSTALLDI\n", p->state, pipe_state_to_str(p->state));
		}
	}
	if (pipe_isr & UOTGHS_HSTPIPISR_PERRI) { // pipe error
		uint32_t perr = UOTGHS->UOTGHS_HSTPIPERR[pipe];
		TRACE_OTG("pipe %d got perr 0x%x\n", pipe, perr);

		Host_ack_pipe_error(pipe);

		if (p->state == PIPE_IN && perr & UOTGHS_HSTPIPERR_PID) {
			UOTGHS->UOTGHS_HSTPIPERR[pipe] = ~0x64;

			panic("got PID error\n");

			// XXX why do we get pid error?
			Host_unfreeze_pipe(pipe);
		}
	}
}

static void usbh_host_irq(uint32_t isr)
{
	int i;

	if (isr & UOTGHS_HSTISR_DCONNI) { // device connected
		dbgPrintf("USB: connect\n");
	}
	if (isr & UOTGHS_HSTISR_DDISCI) { // device disconnected
		dbgPrintf("USB: disconnect\n");
		usbh.state = USBH_DEVICE_UNATTACHED;
		host_disable_all_pipes();
		usbh_reset_all_pipes();
		goto done;
	}
	if (isr & UOTGHS_HSTISR_HSOFI) { // start of frame
		Host_ack_sof();

		//uint32_t f = (UOTGHS->UOTGHS_HSTFNUM >> 3) & 0x7ff;
		//if (usbh.last_sof != f) {
			for (i = 0; i < CHIP_USB_NUMENDPOINTS; i++) {
				if (usbh.pipe[i].state == PIPE_ISO_IN_WAIT) {
					//dbgPrintf("SOF: iso in wait pipe %d\n", i);
					//dbgPrintf("last 0x%x f 0x%x\n", last_sof, f);
					Host_disable_sof_interrupt();

					usbh_start_iso_transfer(i);
				}
			}
		//}
	}

	for (i = 0; i < CHIP_USB_NUMENDPOINTS; i++) {
		if (isr & (1 << (8 + i)))
			usbh_host_pipe_irq(i);
	}

done:
	UOTGHS->UOTGHS_HSTICR = isr;
}

void UOTGHS_Handler(void)
{
	usbh.irq_count++;

#if 0
	if ((usbh.irq_count % 1024) == 0)
		TRACE_OTG("%d usb irqs\n", usbh.irq_count);
#endif

	uint32_t gen_sr = UOTGHS->UOTGHS_SR;
	uint32_t host_isr = UOTGHS->UOTGHS_HSTISR;
	host_isr &= UOTGHS->UOTGHS_HSTIMR;

	if (host_isr) {
		usbh_host_irq(host_isr);
	}
}

int usbh_send_setup(const void *_setup, void *buf, int incomplete)
{
	const struct usb_setup_packet *setup = _setup;

//	TRACE_OTG("setup %p, buf %p, incomplete %d\n", setup, buf, incomplete);
	ASSERT(usbh.pipe[P_CONTROL].state == PIPE_IDLE);

	Disable_global_interrupt();
	Host_configure_pipe_token(P_CONTROL, TOKEN_SETUP);
	Host_ack_setup_ready();
	Host_unfreeze_pipe(P_CONTROL);
	Host_enable_setup_ready_interrupt();

	Host_reset_pipe_fifo_access(P_CONTROL);
	host_write_p_txpacket(P_CONTROL, setup, sizeof(*setup), NULL);
	Host_send_setup();

	/* set up pipe state */
	usbh_pipe_t *p = &usbh.pipe[P_CONTROL];
	p->state = (setup->bmRequestType & USB_SETUP_DIR_DEVICE_TO_HOST) ? PIPE_SETUP_DTOH : PIPE_SETUP_HTOD;
	p->flags = incomplete ? PIPE_FLAG_INCOMPLETE_SETUP : 0;
	p->ptr = buf;
	p->pos = 0;
	p->total_len = setup->wLength;

	Enable_global_interrupt();

	// XXX timeout
	uint64_t t = micros();
	int result = 0;
	for (;;) {
		coopYield();

		ASSERT(p->state != PIPE_NO_CONFIG);
		ASSERT(p->state != PIPE_IDLE);

		// poll the pipe to wait for it to get to complete
		if (p->state == PIPE_SETUP_COMPLETE) {
			p->state = PIPE_IDLE;
			if (p->result < 0) {
				result = p->result;
			} else {
				result = p->pos;
			}
			p->result = PIPE_RESULT_OK;
			break;
		}
	}
	//TRACE_OTG("transfer complete, took %lld usecs\n", micros());

//	TRACE_OTG("transfer complete, result %d\n", result);
	return result;
}


void usbh_work(void)
{
	switch (usbh.state) {
		case USBH_DISABLED:
			break;
		case USBH_INIT:
			Usb_unfreeze_clock();

			Usb_force_host_mode();
			//Wr_bitfield(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SPDCONF_Msk, 1, UOTGHS_HSTCTRL_SPDCONF_Pos); // force full/low speed
			Usb_disable_id_pin();

			Disable_global_interrupt();
			Usb_disable();
			(void)Is_usb_enabled();
			Enable_global_interrupt();
			Usb_enable_otg_pad();
			Usb_set_vbof_active_high();
			Usb_enable_vbus_hw_control();
			Usb_enable();
			Host_enable_device_disconnection_interrupt();
#if !USE_HIGH_SPEED
			Wr_bitfield(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SPDCONF_Msk, 3, UOTGHS_HSTCTRL_SPDCONF_Pos); // force full speed mode
#endif
			usbh.state = USBH_DEVICE_UNATTACHED;

			// clear all ints
			UOTGHS->UOTGHS_SCR = 0xf;
			UOTGHS->UOTGHS_HSTICR = 0xf;
			break;

		case USBH_DEVICE_UNATTACHED:
			TRACE_OTG("DEVICE_UNATTACHED\n");
			// clear all ints
			UOTGHS->UOTGHS_SCR = 0xf;
			UOTGHS->UOTGHS_HSTICR = 0xf;
			Host_disable_sof();
			Host_ack_sof();
			Usb_enable_vbus();

			// clear all the pipes
			host_disable_all_pipes();
			usbh_clear_all_pipes();

			// wipe any accessory state
			accessory_deinit();

			usbh.state = USBH_WAIT_FOR_DEVICE;
			TRACE_OTG("USBH_WAIT_FOR_DEVICE\n");
			break;
		case USBH_WAIT_FOR_DEVICE:
			Usb_enable_vbus();
			if (Is_usb_vbus_high()) {
				if (Is_host_device_connection()) {
					TRACE_OTG("vbus high and saw device connection, moving to USBH_DEVICE_ATTACHED\n");
					Usb_ack_bconnection_error_interrupt();
					Usb_ack_vbus_error_interrupt();
					Host_ack_device_connection();
					Host_ack_hwup();
					usbh.state = USBH_DEVICE_ATTACHED;
				}
			}
			break;
		case USBH_DEVICE_ATTACHED:
			TRACE_OTG("USBH_DEVICE_ATTACHED\n");
			TRACE_OTG("starting SOF\n");
			Host_enable_sof();

			// wait 100ms
			usbh.state = USBH_DEVICE_ATTACHED_SOF_WAIT;
			usbh_start_sleep();
			break;
		case USBH_DEVICE_ATTACHED_SOF_WAIT:
			if (usbh_sleep_expired(100))
				usbh.state = USBH_DEVICE_ATTACHED_RESET;
			break;
		case USBH_DEVICE_ATTACHED_RESET:
			// reset the bus
			TRACE_OTG("USBH_DEVICE_RESET\n");
			TRACE_OTG("resetting bus\n");
			Host_send_reset();
			usbh.state = USBH_DEVICE_ATTACHED_RESET_WAIT;
			break;
		case USBH_DEVICE_ATTACHED_RESET_WAIT:
			if (Is_host_reset_sent()) {
				Host_ack_reset_sent();

				TRACE_OTG("done with reset\n");

				// wait 100ms
				usbh.state = USBH_DEVICE_ATTACHED_POST_RESET_WAIT;
				usbh_start_sleep();
			}
			break;
		case USBH_DEVICE_ATTACHED_POST_RESET_WAIT:
			if (usbh_sleep_expired(100))
				usbh.state = USBH_DEVICE_ATTACHED_QUERY;
			break;
		case USBH_DEVICE_ATTACHED_QUERY: {
			TRACE_OTG("USBH_DEVICE_ATTACHED_QUERY\n");
			TRACE_OTG("configuring ep0 pipe\n");
			int pip = usbh_setup_endpoint(0, 0, ENDP_TYPE_CONTROL, 8);
			ASSERT(pip == P_CONTROL);

			// build get device descriptor setup packet, send short version first (to find maxpacketsize)
			struct usb_setup_packet setup = {
				USB_SETUP_DIR_DEVICE_TO_HOST,
				SETUP_GET_DESCRIPTOR,
				DESCRIPTOR_DEVICE << 8,
				0,
				8
			};

			uint8_t buf[64];
			int len = usbh_send_setup(&setup, buf, true);
			TRACE_OTG("usbh_send_setup returns %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			// configure the pipe for the maxpacket size the device requests
			uint32_t maxpacketsize = buf[OFFSET_FIELD_MAXPACKETSIZE];
			TRACE_OTG("got initial descriptor: len %d, maxpacketsize %d\n", buf[OFFSET_DESCRIPTOR_LENGTH], buf[OFFSET_FIELD_MAXPACKETSIZE]);

			Host_configure_pipe(P_CONTROL, 0, EP_CONTROL, TYPE_CONTROL, TOKEN_SETUP, maxpacketsize, SINGLE_BANK);

			// assign the device an address
			usbh.dev.address = usbh.next_address++;
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_HOST_TO_DEVICE,
				SETUP_SET_ADDRESS,
				usbh.dev.address,
				0,
				0
			};

			len = usbh_send_setup(&setup, buf, false);
			TRACE_OTG("usbh_send_setup returns %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			Host_configure_address(P_CONTROL, usbh.dev.address);

			TRACE_OTG("USBH_DEVICE_ADDRESSED, address %d\n", usbh.dev.address);

			// read the final copy of the device descriptor
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_DEVICE_TO_HOST,
				SETUP_GET_DESCRIPTOR,
				DESCRIPTOR_DEVICE << 8,
				0,
				0x40
			};

			usbh.dev.devdesc;
			len = usbh_send_setup(&setup, usbh.dev.devdesc, false);
			TRACE_OTG("usbh_send_setup returns %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			// pick out the vid/pid
			usbh.dev.vid = *(uint16_t *)(usbh.dev.devdesc + OFFSET_FIELD_VID);
			usbh.dev.pid = *(uint16_t *)(usbh.dev.devdesc + OFFSET_FIELD_PID);

			dbgPrintf("USB: found device vid/pid 0x%x/0x%x\n", usbh.dev.vid, usbh.dev.pid);

			// read a copy of the config descriptor
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_DEVICE_TO_HOST,
				SETUP_GET_DESCRIPTOR,
				DESCRIPTOR_CONFIGURATION << 8,
				0,
				9
			};
			len = usbh_send_setup(&setup, buf, false);
			TRACE_OTG("usbh_send_setup returns %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			uint16_t total_config_length = *(uint16_t *)(buf + OFFSET_FIELD_TOTAL_LENGTH);

			TRACE_OTG("config descriptor length %d\n", total_config_length);

			ASSERT(total_config_length < usbh.dev.devconfig);

			// get the final copy of the config descriptor
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_DEVICE_TO_HOST,
				SETUP_GET_DESCRIPTOR,
				DESCRIPTOR_CONFIGURATION << 8,
				0,
				total_config_length
			};

			len = usbh_send_setup(&setup, usbh.dev.devconfig, false);
			TRACE_OTG("usbh_send_setup returns %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			// see if we're connected to an accessory
			switch ((usbh.dev.vid << 16) | usbh.dev.pid) {
				case 0x18d12d00: // accessory
				case 0x18d12d01: // accessory + adb
				case 0x18d12d02: // audio
				case 0x18d12d03: // audio + adb
				case 0x18d12d04: // accessory + audio
				case 0x18d12d05: // accessory + audio + adb
					dbgPrintf("USB: found accessory 0x%x:0x%x\n", usbh.dev.vid, usbh.dev.pid);
					usbh.state = USBH_DEVICE_ACCESSORY_INIT;
					break;
				default:
					dbgPrintf("USB: didn't find accessory, trying to change its mode\n");
					usbh.state = USBH_DEVICE_TRY_ACCESSORY;
					break;
			}

			break;
		}
		case USBH_DEVICE_TRY_ACCESSORY: {
			// read protocol version
			struct usb_setup_packet setup = (struct usb_setup_packet){
				USB_SETUP_DIR_DEVICE_TO_HOST | USB_SETUP_TYPE_VENDOR,
				0x33,
				0,
				0,
				2
			};

			uint16_t version;
			int len = usbh_send_setup(&setup, &version, false);
			TRACE_OTG("version len %d\n", len);
			if (len == PIPE_RESULT_RESET)
				break;

			if (len <= 0) {
				dbgPrintf("USB: no version returned, not accessory\n");
				usbh.state = USBH_DEVICE_IDLE;
				break;
			}
			TRACE_OTG("accessory returned version %d\n", version);
			if (version < 1 || version > 2) {
				dbgPrintf("USB: bad protocol version %d, not accessory\n", version);
				usbh.state = USBH_DEVICE_IDLE;
				break;
			}

			// send google strings
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
				0x34,
				0,
				0,
				0
			};
#if APP_ACCESSORY
			setup.wIndex = 0;
			setup.wLength = strlen(usbh.accessory_string_vendor) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_vendor, false);
			if (len == PIPE_RESULT_RESET)
				break;

			setup.wIndex = 1;
			setup.wLength = strlen(usbh.accessory_string_name) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_name, false);
			if (len == PIPE_RESULT_RESET)
				break;
#endif
			setup.wIndex = 2;
			setup.wLength = strlen(usbh.accessory_string_longname) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_longname, false);
			if (len == PIPE_RESULT_RESET)
				break;

			setup.wIndex = 3;
			setup.wLength = strlen(usbh.accessory_string_version) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_version, false);
			if (len == PIPE_RESULT_RESET)
				break;

			setup.wIndex = 4;
			setup.wLength = strlen(usbh.accessory_string_url) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_url, false);
			if (len == PIPE_RESULT_RESET)
				break;

			setup.wIndex = 5;
			setup.wLength = strlen(usbh.accessory_string_serial) + 1;
			len = usbh_send_setup(&setup, (void *)usbh.accessory_string_serial, false);
			if (len == PIPE_RESULT_RESET)
				break;

#if AUDIO_ACCESSORY
			// we want audio
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
				0x3a,
				1,
				0,
				0
			};
			len = usbh_send_setup(&setup, NULL, false);
			if (len == PIPE_RESULT_RESET)
				break;
#endif

			// send accessory start
			setup = (struct usb_setup_packet){
				USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
				0x35,
				0,
				0,
				0
			};
			len = usbh_send_setup(&setup, NULL, false);
			if (len == PIPE_RESULT_RESET)
				break;

			// go to idle mode, wait for the device to reset out from underneath us
			usbh.state = USBH_DEVICE_IDLE;
			break;
		}
		case USBH_DEVICE_ACCESSORY_INIT: {
			// set configuration 1
			struct usb_setup_packet setup = (struct usb_setup_packet){
				USB_SETUP_DIR_HOST_TO_DEVICE,
				SETUP_SET_CONFIGURATION,
				1,
				0,
				0
			};

			int len = usbh_send_setup(&setup, NULL, false);
			if (len == PIPE_RESULT_RESET)
				break;

			int ret = accessory_init(&usbh.dev);
			if (ret < 0) {
				usbh.state = USBH_DEVICE_IDLE;
				break;
			}

			usbh.state = USBH_DEVICE_REGISTER_HID;
			break;
		}
		//Added to use new HID support
		case USBH_DEVICE_REGISTER_HID: {

			uint8_t reportDesc[] =
			{
			  0x05, 0x0C,	// Usage Page (Consumer Devices)
			  0x09, 0x01,	// Usage (Consumer Control)
			  0xA1, 0x01,	// Collection (Application)
			  0x15, 0x00,	//   Logical Minimum (0)
			  0x25, 0x01,	//   Logical Maximum (1)
			  0x09, 0xB5,	//   Usage (Scan Next)
			  0x09, 0xB6,	//   Usage (Scan Prev)
			  0x09, 0xB7,	//   Usage (Stop)
			  0x09, 0xCD,	//   Usage (Play/Pause)
			  0x09, 0xE2,	//   Usage (Mute)
			  0x09, 0xE9,	//   Usage (Volume Up)
			  0x09, 0xEA,	//   Usage (Volume Down)
			  0x75, 0x01,	//   Report Size (1)
			  0x95, 0x07,	//   Report Count (7)
			  0x81, 0x02,	//   Input (Data, Variable, Absolute)
			  0x95, 0x01,   //   Report Count (1)
			  0x81, 0x01,	//   Input (Constant)
			  0xC0			// End Collection
			};
		
		    //Register HID
		    TRACE_OTG("Send Register HID Request\n");
		    struct usb_setup_packet setup = {
		      USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
		      0x36,
		      ACC_ID,
		      sizeof(reportDesc),
		      0
		    };
		    int len = usbh_send_setup(&setup, NULL, false);
		    TRACE_OTG("usbh_send_setup returns %d\n", len);
    
		    //Send Report Descriptor
   		    TRACE_OTG("Send Report Descriptor\n");
		    setup = (struct usb_setup_packet){
		      USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
		      0x38,
		      ACC_ID,
		      0,
		      0
		    };
		    setup.wLength = sizeof(reportDesc);
		    len = usbh_send_setup(&setup, &reportDesc, false);
		    TRACE_OTG("usbh_send_setup returns %d\n", len);
		    
		    usbh.state = USBH_DEVICE_ACCESSORY;
		    break;
		}
		case USBH_DEVICE_ACCESSORY:
		case USBH_DEVICE_IDLE:
			//TRACE_OTG("USBH_DEVICE_WAIT\n");
			break;
	}
}

/* Added to send button events as HID to Android device */
int accessory_send_hid(void *buf, size_t len)
{
    uint8_t *data = (uint8_t *)buf;
	dbgPrintf("Send HID Event %d\n", data[0]);
	struct usb_setup_packet setup = {
	  USB_SETUP_DIR_HOST_TO_DEVICE | USB_SETUP_TYPE_VENDOR,
	  0x39,
	  ACC_ID,
	  0,
	  len
	};
	dbgPrintf("Length = %d\n", setup.wLength);
	int length = usbh_send_setup(&setup, &buf, false);
	TRACE_OTG("usbh_send_setup returns %d\n", length);
	
	return length;
}

/* for the coop threading system */
static void usbhTask(void* ptr)
{
	for (;;) {
		coopYield();
		usbh_work();
		// let the accessory state machine run
		accessory_work();
	}
}

void usbh_init(void)
{
	usbh.next_address = 1;
	usbh.state = USBH_DISABLED;

	/* default accessory strings */
	usbh.accessory_string_vendor = DEFAULT_ACCESSORY_STRING_VENDOR;
	usbh.accessory_string_name = DEFAULT_ACCESSORY_STRING_NAME;
	usbh.accessory_string_longname = DEFAULT_ACCESSORY_STRING_LONGNAME;
	usbh.accessory_string_version = DEFAULT_ACCESSORY_STRING_VERSION;
	usbh.accessory_string_url = DEFAULT_ACCESSORY_STRING_URL;
	usbh.accessory_string_serial = DEFAULT_ACCESSORY_STRING_SERIAL;

    /* Enable peripheral clock for UOTGHS */
    pmc_enable_periph_clk(ID_UOTGHS);

#if 1
    /* Enable UPLL 480 MHz */
    PMC->CKGR_UCKR = CKGR_UCKR_UPLLEN | CKGR_UCKR_UPLLCOUNT(7);
    /* Wait that UPLL is considered locked by the PMC */
    while( !(PMC->PMC_SR & PMC_SR_LOCKU) )
		;

    /* USB clock register: USB Clock Input is UTMI PLL */
    PMC->PMC_USB = PMC_USB_USBS | PMC_USB_USBDIV(0);

    PMC->PMC_SCER = PMC_SCER_UOTGCLK;
#else
    PMC->CKGR_UCKR = 0;
    PMC->PMC_USB = PMC_USB_USBDIV(0);
    PMC->PMC_SCER = PMC_SCER_UOTGCK;
#endif

    NVIC_SetPriority(UOTGHS_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
	NVIC_EnableIRQ(UOTGHS_IRQn);

	coopSpawn(&usbhTask, NULL, 2048);

	Usb_freeze_clock();
}

void usbh_set_accessory_string_vendor(const char *str)
{
	usbh.accessory_string_vendor = str;
}

void usbh_set_accessory_string_name(const char *str)
{
	usbh.accessory_string_name = str;
}

void usbh_set_accessory_string_longname(const char *str)
{
	usbh.accessory_string_longname = str;
}

void usbh_set_accessory_string_version(const char *str)
{
	usbh.accessory_string_version = str;
}

void usbh_set_accessory_string_url(const char *str)
{
	usbh.accessory_string_url = str;
}

void usbh_set_accessory_string_serial(const char *str)
{
	usbh.accessory_string_serial = str;
}

void usbh_start(void)
{
	if (usbh.state == USBH_DISABLED)
		usbh.state = USBH_INIT;
}

int usbh_accessory_connected(void)
{
	return usbh.state == USBH_DEVICE_ACCESSORY;
}

