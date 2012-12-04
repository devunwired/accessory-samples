/* This source file is part of the AVR Software Framework 2.0.0 release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Low-level driver for AVR32 USBB.
 *
 * This file contains the USBB low-level driver routines.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USBB module can be used.
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

//_____ I N C L U D E S ____________________________________________________

#include "usb_drv.h"


//_____ M A C R O S ________________________________________________________
#define min(x,y)  ((x)>(y)?(y):(x))


//_____ D E C L A R A T I O N S ____________________________________________

#if USB_DEVICE_FEATURE == DISABLED && USB_HOST_FEATURE == DISABLED
  #error At least one of USB_DEVICE_FEATURE and USB_HOST_FEATURE must be enabled
#endif

void Uotghs_trace(void)
{
    TRACE_OTG("CTRL: 0x%X\n\r",UOTGHS->UOTGHS_CTRL);
    TRACE_OTG("SR: 0x%X\n\r",UOTGHS->UOTGHS_SR);
    TRACE_OTG("FSM: 0x%X\n\r",UOTGHS->UOTGHS_FSM);
    TRACE_OTG("DEVCTRL: 0x%X\n\r",UOTGHS->UOTGHS_DEVCTRL);
    TRACE_OTG("DEVISR: 0x%X\n\r",UOTGHS->UOTGHS_DEVISR);
    TRACE_OTG("DEVIMR: 0x%X\n\r",UOTGHS->UOTGHS_DEVIMR);
    TRACE_OTG("DEVEPTCFG0: 0x%X\n\r",UOTGHS->UOTGHS_DEVEPTCFG[0]);
    TRACE_OTG("DEVEPTCFG1: 0x%X\n\r",UOTGHS->UOTGHS_DEVEPTCFG[1]);
    TRACE_OTG("DEVEPTCFG2: 0x%X\n\r",UOTGHS->UOTGHS_DEVEPTCFG[2]);
    TRACE_OTG("DEVEPTISR: 0x%X\n\r",UOTGHS->UOTGHS_DEVEPTISR[0]);
    TRACE_OTG("HSTCTRL: 0x%X\n\r",UOTGHS->UOTGHS_HSTCTRL);
    TRACE_OTG("HSTISR: 0x%X\n\r",UOTGHS->UOTGHS_HSTISR);
    TRACE_OTG("HSTIMR: 0x%X\n\r",UOTGHS->UOTGHS_HSTIMR);
    TRACE_OTG("HSTPIP: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIP);
    TRACE_OTG("HSTFNUM: 0x%X\n\r",UOTGHS->UOTGHS_HSTFNUM);
    TRACE_OTG("HSTADDR1: 0x%X\n\r",UOTGHS->UOTGHS_HSTADDR1);
    TRACE_OTG("HSTADDR2: 0x%X\n\r",UOTGHS->UOTGHS_HSTADDR2);
    TRACE_OTG("HSTADDR3: 0x%X\n\r",UOTGHS->UOTGHS_HSTADDR3);
    TRACE_OTG("HSTPIPCFG: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPCFG[0]);
    TRACE_OTG("HSTPIPCFG1: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPCFG[1]);
    TRACE_OTG("HSTPIPCFG2: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPCFG[2]);
    TRACE_OTG("HSTPIPINRQ: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPINRQ[0]);
    TRACE_OTG("HSTPIPERR0: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPERR[0]);
    TRACE_OTG("HSTPIPISR0: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPISR[0]);
    TRACE_OTG("HSTPIPIMR0: 0x%X\n\r",UOTGHS->UOTGHS_HSTPIPIMR[0]);

    /* Test ID */
    if( UOTGHS->UOTGHS_SR & UOTGHS_SR_ID)
    {
        TRACE_OTG("ID=1: PERIPHERAL\n\r");
    }
    else
    {
        TRACE_OTG("ID=0: HOST\n\r");
    }

    /* Test VBUS */
    if( 0 != (UOTGHS->UOTGHS_SR & UOTGHS_SR_VBUS) ) {
        TRACE_OTG("VBUS = 1\n\r");
    }
    else {
        TRACE_OTG("VBUS = 0\n\r");
    }

    /* Test SPEED */
    TRACE_OTG("Speed = 0x%X\n\r", UOTGHS->UOTGHS_SR & UOTGHS_SR_SPEED_Msk);

    /* FSM State:
     *    0   A_IDLESTATE         This is the start state for A-devices (when the ID pin is 0)
     *    1   A_WAIT_VRISE        In this state, the A-device waits for the voltage on VBus to rise above the A-device VBus
     *                            Valid threshold (4.4 V).
     *    2   A_WAIT_BCON         In this state, the A-device waits for the B-device to signal a connection.
     *    3   A_HOST              In this state, the A-device that operates in Host mode is operational.
     *    4   A_SUSPEND           The A-device operating as a host is in the suspend mode.
     *    5   A_PERIPHERAL        The A-device operates as a peripheral.
     *    6   A_WAIT_VFALL        In this state, the A-device waits for the voltage on VBus to drop below the A-device
     *                            Session Valid threshold (1.4 V).
     *    7   A_VBUS_ERR          In this state, the A-device waits for recovery of the over-current condition that caused it
     *                            to enter this state.
     *    8   A_WAIT_DISCHARGE    In this state, the A-device waits for the data USB line to discharge (100 us).
     *    9   B_IDLE              This is the start state for B-device (when the ID pin is 1).
     *    10  B_PERIPHERAL        In this state, the B-device acts as the peripheral.
     *    11  B_WAIT_BEGIN_HNP    In this state, the B-device is in suspend mode and waits until 3 ms before initiating the
     *                            HNP protocol if requested.
     *    12  B_WAIT_DISCHARGE    In this state, the B-device waits for the data USB line to discharge (100 us) before
     *                            becoming Host.
     *    13  B_WAIT_ACON         In this state, the B-device waits for the A-device to signal a connect before becoming B-
     *    Host.
     *    14  B_HOST              In this state, the B-device acts as the Host.
     *    15  B_SRP_INIT          In this state, the B-device attempts to start a session using the SRP protocol. */
    TRACE_OTG("FSM = 0x%X\n\r", UOTGHS->UOTGHS_FSM);
}

//! Pointers to the FIFO data registers of pipes/endpoints
//! Use aggregated pointers to have several alignments available for a same address
UnionVPtr pep_fifo[MAX_PEP_NB];

//! ---------------------------------------------------------
//! ------------------ HOST ---------------------------------
//! ---------------------------------------------------------

#if USB_HOST_FEATURE == ENABLED

void Host_configure_address( uint8_t pipe, uint8_t addr)
{
    if( pipe == 0 ) {
        UOTGHS->UOTGHS_HSTADDR1 &= ~UOTGHS_HSTADDR1_HSTADDRP0_Msk;
        UOTGHS->UOTGHS_HSTADDR1 |= UOTGHS_HSTADDR1_HSTADDRP0(addr);
    }
    else if( pipe == 1 ) {
        UOTGHS->UOTGHS_HSTADDR1 &= ~UOTGHS_HSTADDR1_HSTADDRP1_Msk;
        UOTGHS->UOTGHS_HSTADDR1 |= UOTGHS_HSTADDR1_HSTADDRP1(addr);
    }
    else if( pipe == 2 ) {
        UOTGHS->UOTGHS_HSTADDR1 &= ~UOTGHS_HSTADDR1_HSTADDRP2_Msk;
        UOTGHS->UOTGHS_HSTADDR1 |= UOTGHS_HSTADDR1_HSTADDRP2(addr);
    }
    else if( pipe == 3 ) {
        UOTGHS->UOTGHS_HSTADDR1 &= ~UOTGHS_HSTADDR1_HSTADDRP3_Msk;
        UOTGHS->UOTGHS_HSTADDR1 |= UOTGHS_HSTADDR1_HSTADDRP3(addr);
    }
    else if( pipe == 4 ) {
        UOTGHS->UOTGHS_HSTADDR2 &= ~(0x7F<<0);
        UOTGHS->UOTGHS_HSTADDR2 |= addr;
    }
    else if( pipe == 5 ) {
        UOTGHS->UOTGHS_HSTADDR2 &= ~(0x7F<<8);
        UOTGHS->UOTGHS_HSTADDR2 |= (addr<<8);
    }
    else if( pipe == 6 ) {
        UOTGHS->UOTGHS_HSTADDR2 &= ~(0x7F<<16);
        UOTGHS->UOTGHS_HSTADDR2 |= (addr<<16);
    }
    else if( pipe == 7 ) {
        UOTGHS->UOTGHS_HSTADDR2 &= ~(0x7F<<24);
        UOTGHS->UOTGHS_HSTADDR2 |= (addr<<24);
    }
    else if( pipe == 8 ) {
        UOTGHS->UOTGHS_HSTADDR3 &= ~UOTGHS_HSTADDR3_HSTADDRP8_Msk;
        UOTGHS->UOTGHS_HSTADDR3 |= UOTGHS_HSTADDR3_HSTADDRP8(addr);
    }
    else if( pipe == 9 ) {
        UOTGHS->UOTGHS_HSTADDR3 &=  ~(0x7F<<8);
        UOTGHS->UOTGHS_HSTADDR3 |= (addr<<8);
    }
}



//! host_disable_all_pipes
//!
//!  This function disables all pipes for the host controller.
//!  Useful to execute upon disconnection.
//!
//! @return Void
//!
void host_disable_all_pipes(void)
{
#if USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE
  Bool sav_glob_int_en;
#endif
  U8 p;

#if USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE
  // Disable global interrupts
  if ((sav_glob_int_en = Is_global_interrupt_enabled())) Disable_global_interrupt();
#endif
  for (p = 0; p < MAX_PEP_NB; p++)
  { // Disable the pipe <p> (disable interrupt, free memory, reset pipe, ...)
    Host_disable_pipe_interrupt(p);
    Host_reset_pipe(p);
    Host_unallocate_memory(p);
    Host_disable_pipe(p);
  }
#if USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE
  (void)Is_host_pipe_enabled(MAX_PEP_NB - 1);
  // Restore the global interrupts to the initial state
  if (sav_glob_int_en) Enable_global_interrupt();
#endif
}

//! host_set_p_txpacket
//!
//!  This function fills the selected pipe FIFO with a constant byte, using
//!  as few accesses as possible.
//!
//! @param p            Number of the addressed pipe
//! @param txbyte       Byte to fill the pipe with
//! @param data_length  Number of bytes to write
//!
//! @return             Number of non-written bytes
//!
//! @note The selected pipe FIFO may be filled in several steps by calling
//! host_set_p_txpacket several times.
//!
//! @warning Invoke Host_reset_pipe_fifo_access before this function when at
//! FIFO beginning whether or not the FIFO is to be filled in several steps.
//!
//! @warning Do not mix calls to this function with calls to indexed macros.
//!
U32 host_set_p_txpacket(U8 p, U8 txbyte, U32 data_length)
{
  // Use aggregated pointers to have several alignments available for a same address
  UnionVPtr   p_fifo_cur;
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  StructCVPtr p_fifo_end;
  Union64     txval;
#else
  UnionCVPtr  p_fifo_end;
  union
  {
    U8 u8[1];
  } txval;
#endif  // !__OPTIMIZE_SIZE__

  // Initialize pointers for write loops and limit the number of bytes to write
  p_fifo_cur.u8ptr = pep_fifo[p].u8ptr;
  p_fifo_end.u8ptr = p_fifo_cur.u8ptr +
                     min(data_length, Host_get_pipe_size(p) - Host_byte_count(p));
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  p_fifo_end.u16ptr = (U16 *)Align_down((U32)p_fifo_end.u8ptr, sizeof(U16));
  p_fifo_end.u32ptr = (U32 *)Align_down((U32)p_fifo_end.u16ptr, sizeof(U32));
  p_fifo_end.u64ptr = (U64 *)Align_down((U32)p_fifo_end.u32ptr, sizeof(U64));
#endif  // !__OPTIMIZE_SIZE__
  txval.u8[0] = txbyte;
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  txval.u8[1] = txval.u8[0];
  txval.u16[1] = txval.u16[0];
  txval.u32[1] = txval.u32[0];

  // If pointer to FIFO data register is not 16-bit aligned
  if (!Test_align((U32)p_fifo_cur.u8ptr, sizeof(U16)))
  {
    // Write 8-bit data to reach 16-bit alignment
    if (p_fifo_cur.u8ptr < p_fifo_end.u8ptr)
    {
      *p_fifo_cur.u8ptr++ = txval.u8[0];
    }
  }

  // If pointer to FIFO data register is not 32-bit aligned
  if (!Test_align((U32)p_fifo_cur.u16ptr, sizeof(U32)))
  {
    // Write 16-bit data to reach 32-bit alignment
    if (p_fifo_cur.u16ptr < p_fifo_end.u16ptr)
    {
      *p_fifo_cur.u16ptr++ = txval.u16[0];
    }
  }

  // If pointer to FIFO data register is not 64-bit aligned
  if (!Test_align((U32)p_fifo_cur.u32ptr, sizeof(U64)))
  {
    // Write 32-bit data to reach 64-bit alignment
    if (p_fifo_cur.u32ptr < p_fifo_end.u32ptr)
    {
      *p_fifo_cur.u32ptr++ = txval.u32[0];
    }
  }

  // Write 64-bit-aligned data
  while (p_fifo_cur.u64ptr < p_fifo_end.u64ptr)
  {
    *p_fifo_cur.u64ptr++ = txval.u64;
  }

  // Write remaining 32-bit data if some
  if (p_fifo_cur.u32ptr < p_fifo_end.u32ptr)
  {
    *p_fifo_cur.u32ptr++ = txval.u32[0];
  }

  // Write remaining 16-bit data if some
  if (p_fifo_cur.u16ptr < p_fifo_end.u16ptr)
  {
    *p_fifo_cur.u16ptr++ = txval.u16[0];
  }

  // Write remaining 8-bit data if some
  if (p_fifo_cur.u8ptr < p_fifo_end.u8ptr)
  {
    *p_fifo_cur.u8ptr++ = txval.u8[0];
  }

#else

  // Write remaining 8-bit data if some
  while (p_fifo_cur.u8ptr < p_fifo_end.u8ptr)
  {
    *p_fifo_cur.u8ptr++ = txval.u8[0];
  }

#endif  // !__OPTIMIZE_SIZE__

  // Compute the number of non-written bytes
  data_length -= p_fifo_cur.u8ptr - pep_fifo[p].u8ptr;

  // Save current position in FIFO data register
  pep_fifo[p].u8ptr = p_fifo_cur.u8ptr;

  // Return the number of non-written bytes
  return data_length;
}

//! host_write_p_txpacket
//!
//!  This function writes the buffer pointed to by txbuf to the selected
//!  pipe FIFO, using as few accesses as possible.
//!
//! @param p            Number of the addressed pipe
//! @param txbuf        Address of buffer to read
//! @param data_length  Number of bytes to write
//! @param ptxbuf       NULL or pointer to the buffer address to update
//!
//! @return             Number of written bytes
//!
//! @note The selected pipe FIFO may be written in several steps by calling
//! host_write_p_txpacket several times.
//!
//! @warning Invoke Host_reset_pipe_fifo_access before this function when at
//! FIFO beginning whether or not the FIFO is to be written in several steps.
//!
//! @warning Do not mix calls to this function with calls to indexed macros.
//!
U32 host_write_p_txpacket(U8 p, const void *txbuf, U32 data_length, const void **ptxbuf)
{
  // Use aggregated pointers to have several alignments available for a same address
  UnionVPtr   p_fifo;
  UnionCPtr   txbuf_cur;
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  StructCPtr  txbuf_end;
#else
  UnionCPtr   txbuf_end;
#endif  // !__OPTIMIZE_SIZE__

  // Initialize pointers for copy loops and limit the number of bytes to copy
  p_fifo.u8ptr = pep_fifo[p].u8ptr;
  txbuf_cur.u8ptr = txbuf;
  txbuf_end.u8ptr = txbuf_cur.u8ptr +
                    min(data_length, Host_get_pipe_size(p) - Host_byte_count(p));
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  txbuf_end.u16ptr = (U16 *)Align_down((U32)txbuf_end.u8ptr, sizeof(U16));
  txbuf_end.u32ptr = (U32 *)Align_down((U32)txbuf_end.u16ptr, sizeof(U32));
  txbuf_end.u64ptr = (U64 *)Align_down((U32)txbuf_end.u32ptr, sizeof(U64));

  // If all addresses are aligned the same way with respect to 16-bit boundaries
  if (Get_align((U32)txbuf_cur.u8ptr, sizeof(U16)) == Get_align((U32)p_fifo.u8ptr, sizeof(U16)))
  {
    // If pointer to transmission buffer is not 16-bit aligned
    if (!Test_align((U32)txbuf_cur.u8ptr, sizeof(U16)))
    {
      // Copy 8-bit data to reach 16-bit alignment
      if (txbuf_cur.u8ptr < txbuf_end.u8ptr)
      {
        // 8-bit accesses to FIFO data registers do require pointer post-increment
        *p_fifo.u8ptr++ = *txbuf_cur.u8ptr++;
      }
    }

    // If all addresses are aligned the same way with respect to 32-bit boundaries
    if (Get_align((U32)txbuf_cur.u16ptr, sizeof(U32)) == Get_align((U32)p_fifo.u16ptr, sizeof(U32)))
    {
      // If pointer to transmission buffer is not 32-bit aligned
      if (!Test_align((U32)txbuf_cur.u16ptr, sizeof(U32)))
      {
        // Copy 16-bit data to reach 32-bit alignment
        if (txbuf_cur.u16ptr < txbuf_end.u16ptr)
        {
          // 16-bit accesses to FIFO data registers do require pointer post-increment
          *p_fifo.u16ptr++ = *txbuf_cur.u16ptr++;
        }
      }

      // If pointer to transmission buffer is not 64-bit aligned
      if (!Test_align((U32)txbuf_cur.u32ptr, sizeof(U64)))
      {
        // Copy 32-bit data to reach 64-bit alignment
        if (txbuf_cur.u32ptr < txbuf_end.u32ptr)
        {
          // 32-bit accesses to FIFO data registers do not require pointer post-increment
          *p_fifo.u32ptr = *txbuf_cur.u32ptr++;
        }
      }

      // Copy 64-bit-aligned data
      while (txbuf_cur.u64ptr < txbuf_end.u64ptr)
      {
        // 64-bit accesses to FIFO data registers do not require pointer post-increment
        *p_fifo.u64ptr = *txbuf_cur.u64ptr++;
      }

      // Copy 32-bit-aligned data
      if (txbuf_cur.u32ptr < txbuf_end.u32ptr)
      {
        // 32-bit accesses to FIFO data registers do not require pointer post-increment
        *p_fifo.u32ptr = *txbuf_cur.u32ptr++;
      }
    }

    // Copy remaining 16-bit data if some
    while (txbuf_cur.u16ptr < txbuf_end.u16ptr)
    {
      // 16-bit accesses to FIFO data registers do require pointer post-increment
      *p_fifo.u16ptr++ = *txbuf_cur.u16ptr++;
    }
  }

#endif  // !__OPTIMIZE_SIZE__

  // Copy remaining 8-bit data if some
  while (txbuf_cur.u8ptr < txbuf_end.u8ptr)
  {
    // 8-bit accesses to FIFO data registers do require pointer post-increment
    *p_fifo.u8ptr++ = *txbuf_cur.u8ptr++;
  }

  // Save current position in FIFO data register
  pep_fifo[p].u8ptr = p_fifo.u8ptr;

  // Return the updated buffer address and the number of non-copied bytes
  if (ptxbuf) *ptxbuf = txbuf_cur.u8ptr;
  return txbuf_cur.u8ptr - (U8 *)txbuf;
}

//! host_read_p_rxpacket
//!
//!  This function reads the selected pipe FIFO to the buffer pointed to by
//!  rxbuf, using as few accesses as possible.
//!
//! @param p            Number of the addressed pipe
//! @param rxbuf        Address of buffer to write
//! @param data_length  Number of bytes to read
//! @param prxbuf       NULL or pointer to the buffer address to update
//!
//! @return             Number of read bytes
//!
//! @note The selected pipe FIFO may be read in several steps by calling
//! host_read_p_rxpacket several times.
//!
//! @warning Invoke Host_reset_pipe_fifo_access before this function when at
//! FIFO beginning whether or not the FIFO is to be read in several steps.
//!
//! @warning Do not mix calls to this function with calls to indexed macros.
//!
U32 host_read_p_rxpacket(U8 p, void *rxbuf, U32 data_length, void **prxbuf)
{
  // Use aggregated pointers to have several alignments available for a same address
  UnionCVPtr  p_fifo;
  UnionPtr    rxbuf_cur;
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  StructCPtr  rxbuf_end;
#else
  UnionCPtr   rxbuf_end;
#endif  // !__OPTIMIZE_SIZE__

  // Initialize pointers for copy loops and limit the number of bytes to copy
  p_fifo.u8ptr = pep_fifo[p].u8ptr;
  rxbuf_cur.u8ptr = rxbuf;
  rxbuf_end.u8ptr = rxbuf_cur.u8ptr + min(data_length, Host_byte_count(p));
#if (!defined __OPTIMIZE_SIZE__) || !__OPTIMIZE_SIZE__  // Auto-generated when GCC's -Os command option is used
  rxbuf_end.u16ptr = (U16 *)Align_down((U32)rxbuf_end.u8ptr, sizeof(U16));
  rxbuf_end.u32ptr = (U32 *)Align_down((U32)rxbuf_end.u16ptr, sizeof(U32));
  rxbuf_end.u64ptr = (U64 *)Align_down((U32)rxbuf_end.u32ptr, sizeof(U64));

  // If all addresses are aligned the same way with respect to 16-bit boundaries
  if (Get_align((U32)rxbuf_cur.u8ptr, sizeof(U16)) == Get_align((U32)p_fifo.u8ptr, sizeof(U16)))
  {
    // If pointer to reception buffer is not 16-bit aligned
    if (!Test_align((U32)rxbuf_cur.u8ptr, sizeof(U16)))
    {
      // Copy 8-bit data to reach 16-bit alignment
      if (rxbuf_cur.u8ptr < rxbuf_end.u8ptr)
      {
        // 8-bit accesses to FIFO data registers do require pointer post-increment
        *rxbuf_cur.u8ptr++ = *p_fifo.u8ptr++;
      }
    }

    // If all addresses are aligned the same way with respect to 32-bit boundaries
    if (Get_align((U32)rxbuf_cur.u16ptr, sizeof(U32)) == Get_align((U32)p_fifo.u16ptr, sizeof(U32)))
    {
      // If pointer to reception buffer is not 32-bit aligned
      if (!Test_align((U32)rxbuf_cur.u16ptr, sizeof(U32)))
      {
        // Copy 16-bit data to reach 32-bit alignment
        if (rxbuf_cur.u16ptr < rxbuf_end.u16ptr)
        {
          // 16-bit accesses to FIFO data registers do require pointer post-increment
          *rxbuf_cur.u16ptr++ = *p_fifo.u16ptr++;
        }
      }

      // If pointer to reception buffer is not 64-bit aligned
      if (!Test_align((U32)rxbuf_cur.u32ptr, sizeof(U64)))
      {
        // Copy 32-bit data to reach 64-bit alignment
        if (rxbuf_cur.u32ptr < rxbuf_end.u32ptr)
        {
          // 32-bit accesses to FIFO data registers do not require pointer post-increment
          *rxbuf_cur.u32ptr++ = *p_fifo.u32ptr;
        }
      }

      // Copy 64-bit-aligned data
      while (rxbuf_cur.u64ptr < rxbuf_end.u64ptr)
      {
        // 64-bit accesses to FIFO data registers do not require pointer post-increment
        *rxbuf_cur.u64ptr++ = *p_fifo.u64ptr;
      }

      // Copy 32-bit-aligned data
      if (rxbuf_cur.u32ptr < rxbuf_end.u32ptr)
      {
        // 32-bit accesses to FIFO data registers do not require pointer post-increment
        *rxbuf_cur.u32ptr++ = *p_fifo.u32ptr;
      }
    }

    // Copy remaining 16-bit data if some
    while (rxbuf_cur.u16ptr < rxbuf_end.u16ptr)
    {
      // 16-bit accesses to FIFO data registers do require pointer post-increment
      *rxbuf_cur.u16ptr++ = *p_fifo.u16ptr++;
    }
  }

#endif  // !__OPTIMIZE_SIZE__

  // Copy remaining 8-bit data if some
  while (rxbuf_cur.u8ptr < rxbuf_end.u8ptr)
  {
    // 8-bit accesses to FIFO data registers do require pointer post-increment
    *rxbuf_cur.u8ptr++ = *p_fifo.u8ptr++;
  }

  // Save current position in FIFO data register
  pep_fifo[p].u8ptr = (volatile U8 *)p_fifo.u8ptr;

  // Return the updated buffer address and the number of copied bytes
  if (prxbuf) *prxbuf = rxbuf_cur.u8ptr;
  return (rxbuf_cur.u8ptr - (U8 *)rxbuf);
}

#endif  // USB_HOST_FEATURE == ENABLED
