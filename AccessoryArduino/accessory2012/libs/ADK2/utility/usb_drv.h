/* This header file is part of the AVR Software Framework 2.0.0 release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Low-level driver for AVR32 USBB.
 *
 * This file contains the USBB low-level driver definitions.
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

#ifndef _USB_DRV_H_
#define _USB_DRV_H_


//_____ I N C L U D E S ____________________________________________________

#include "compiler.h"
//#include "preprocessor.h"
//#include "usbb.h"
#include "conf_usb.h"
#include "chip.h"

//! @defgroup USBB_low_level_driver USBB low-level driver module
//! USBB low-level driver module
//! @warning Bit-masks are used instead of bit-fields because PB registers
//! require 32-bit write accesses while AVR32-GCC 4.0.2 builds 8-bit
//! accesses even when volatile unsigned int bit-fields are specified.
//! @{

//_____ M A C R O S ________________________________________________________
//#ifndef UOTGHS_RAM_ADDR
//#define UOTGHS_RAM_ADDR (0x20180000u) /**< USB On-The-Go Interface RAM base address */
//#endif

//! Maximal number of USBB pipes/endpoints.
//! This is the same value as the one produced by Usb_get_pipe_endpoint_max_nbr().
//! As it is constant and known for a given target, there is no need to decrease
//! performance and to complexify program structure by using a value in memory.
//! The use of MAX_PEP_NB is hence preferred here to the use of a variable
//! initialized from Usb_get_pipe_endpoint_max_nbr().
#define MAX_PEP_NB            CHIP_USB_NUMENDPOINTS

//! @defgroup USBB_endpoints USBB endpoints
//! Identifiers of USBB endpoints (USBB device operating mode).
//! @{
#define EP_CONTROL            0
#define EP_0                  0
#define EP_1                  1
#define EP_2                  2
#define EP_3                  3
#define EP_4                  4
#define EP_5                  5
#define EP_6                  6
//! @}

//! @defgroup USBB_pipes USBB pipes
//! Identifiers of USBB pipes (USBB host operating mode).
//! @{
#define P_CONTROL             0
#define P_0                   0
#define P_1                   1
#define P_2                   2
#define P_3                   3
#define P_4                   4
#define P_5                   5
#define P_6                   6
//! @}

//! @defgroup USBB_types USBB standard types
//! List of the standard types used in USBB.
//! @{
#define IP_NAME_PART_1           1
#define IP_NAME_PART_2           2

#define DMA_BUFFER_SIZE_16_BITS  AVR32_USBB_UFEATURES_DMA_BUFFER_SIZE_16_BITS
#define DMA_BUFFER_SIZE_24_BITS  AVR32_USBB_UFEATURES_DMA_BUFFER_SIZE_24_BITS

#define AWAITVRISE_TIMER         AVR32_USBB_USBCON_TIMPAGE_A_WAIT_VRISE
  #define AWAITVRISE_TMOUT_20_MS   AVR32_USBB_USBCON_TIMVALUE_A_WAIT_VRISE_20_MS
  #define AWAITVRISE_TMOUT_50_MS   AVR32_USBB_USBCON_TIMVALUE_A_WAIT_VRISE_50_MS
  #define AWAITVRISE_TMOUT_70_MS   AVR32_USBB_USBCON_TIMVALUE_A_WAIT_VRISE_70_MS
  #define AWAITVRISE_TMOUT_100_MS  AVR32_USBB_USBCON_TIMVALUE_A_WAIT_VRISE_100_MS
#define VBBUSPULSING_TIMER       AVR32_USBB_USBCON_TIMPAGE_VB_BUS_PULSING
  #define VBBUSPULSING_TMOUT_15_MS AVR32_USBB_USBCON_TIMVALUE_VB_BUS_PULSING_15_MS
  #define VBBUSPULSING_TMOUT_23_MS AVR32_USBB_USBCON_TIMVALUE_VB_BUS_PULSING_23_MS
  #define VBBUSPULSING_TMOUT_31_MS AVR32_USBB_USBCON_TIMVALUE_VB_BUS_PULSING_31_MS
  #define VBBUSPULSING_TMOUT_40_MS AVR32_USBB_USBCON_TIMVALUE_VB_BUS_PULSING_40_MS
#define PDTMOUTCNT_TIMER         AVR32_USBB_USBCON_TIMPAGE_PD_TMOUT_CNT
  #define PDTMOUTCNT_TMOUT_93_MS   AVR32_USBB_USBCON_TIMVALUE_PD_TMOUT_CNT_93_MS
  #define PDTMOUTCNT_TMOUT_105_MS  AVR32_USBB_USBCON_TIMVALUE_PD_TMOUT_CNT_105_MS
  #define PDTMOUTCNT_TMOUT_118_MS  AVR32_USBB_USBCON_TIMVALUE_PD_TMOUT_CNT_118_MS
  #define PDTMOUTCNT_TMOUT_131_MS  AVR32_USBB_USBCON_TIMVALUE_PD_TMOUT_CNT_131_MS
#define SRPDETTMOUT_TIMER        AVR32_USBB_USBCON_TIMPAGE_SRP_DET_TMOUT
  #define SRPDETTMOUT_TMOUT_10_US  AVR32_USBB_USBCON_TIMVALUE_SRP_DET_TMOUT_10_US
  #define SRPDETTMOUT_TMOUT_100_US AVR32_USBB_USBCON_TIMVALUE_SRP_DET_TMOUT_100_US
  #define SRPDETTMOUT_TMOUT_1_MS   AVR32_USBB_USBCON_TIMVALUE_SRP_DET_TMOUT_1_MS
  #define SRPDETTMOUT_TMOUT_11_MS  AVR32_USBB_USBCON_TIMVALUE_SRP_DET_TMOUT_11_MS

#define TYPE_CONTROL             (UOTGHS_HSTPIPCFG_PTYPE_CTRL   >> UOTGHS_HSTPIPCFG_PTYPE_Pos)
#define TYPE_ISOCHRONOUS         (UOTGHS_HSTPIPCFG_PTYPE_ISO    >> UOTGHS_HSTPIPCFG_PTYPE_Pos)
#define TYPE_BULK                (UOTGHS_HSTPIPCFG_PTYPE_BLK    >> UOTGHS_HSTPIPCFG_PTYPE_Pos)
#define TYPE_INTERRUPT           (UOTGHS_HSTPIPCFG_PTYPE_INTRPT >> UOTGHS_HSTPIPCFG_PTYPE_Pos)

#define TRANSFER_TYPE_MASK          0x03
#define SYNCHRONIZATION_TYPE_MASK   0x0c
#define USAGE_TYPE_MASK             0x30

#define DIRECTION_OUT            0  // OUT
#define DIRECTION_IN             UOTGHS_DEVEPTCFG_EPDIR  // IN

#define TOKEN_SETUP              (UOTGHS_HSTPIPCFG_PTOKEN_SETUP >> UOTGHS_HSTPIPCFG_PTOKEN_Pos)
#define TOKEN_IN                 (UOTGHS_HSTPIPCFG_PTOKEN_IN    >> UOTGHS_HSTPIPCFG_PTOKEN_Pos)
#define TOKEN_OUT                (UOTGHS_HSTPIPCFG_PTOKEN_OUT   >> UOTGHS_HSTPIPCFG_PTOKEN_Pos)

#define SINGLE_BANK              (UOTGHS_HSTPIPCFG_PBK_1_BANK >> UOTGHS_HSTPIPCFG_PBK_Pos)
#define DOUBLE_BANK              (UOTGHS_HSTPIPCFG_PBK_2_BANK >> UOTGHS_HSTPIPCFG_PBK_Pos)
#define TRIPLE_BANK              (UOTGHS_HSTPIPCFG_PBK_3_BANK >> UOTGHS_HSTPIPCFG_PBK_Pos)

#define BANK_PID_DATA0           0
#define BANK_PID_DATA1           1
//! @}


//! Post-increment operations associated with 64-, 32-, 16- and 8-bit accesses to
//! the FIFO data registers of pipes/endpoints
//! @note 64- and 32-bit accesses to FIFO data registers do not require pointer
//! post-increment while 16- and 8-bit ones do.
//! @note Only for internal use.
//! @{
#define Pep_fifo_access_64_post_inc()
#define Pep_fifo_access_32_post_inc()
#define Pep_fifo_access_16_post_inc()   ++
#define Pep_fifo_access_8_post_inc()    ++
//! @}


//! @defgroup USBB_properties USBB IP properties
//! These macros give access to IP properties
//! @{
  //! Get IP name part 1 or 2
#define Usb_get_ip_name(part)           (AVR32_USBB_unamex(part))
  //! Get IP version
#define Usb_get_ip_version()            (Rd_bitfield(AVR32_USBB_uvers, AVR32_USBB_UVERS_VERSION_NUM_MASK))
  //! Get number of metal fixes
#define Usb_get_metal_fix_nbr()         (Rd_bitfield(AVR32_USBB_uvers, AVR32_USBB_UVERS_METAL_FIX_NUM_MASK))
  //! Get maximal number of pipes/endpoints (number of hardware-implemented pipes/endpoints)
#define Usb_get_pipe_endpoint_max_nbr() (((Rd_bitfield(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_EPT_NBR_MAX_MASK) - 1) & ((1 << AVR32_USBB_UFEATURES_EPT_NBR_MAX_SIZE) - 1)) + 1)
  //! Get number of hardware-implemented DMA channels
#define Usb_get_dma_channel_nbr()       (Rd_bitfield(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_DMA_CHANNEL_NBR_MASK))
  //! Get DMA buffer size
#define Usb_get_dma_buffer_size()       (Rd_bitfield(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_DMA_BUFFER_SIZE_MASK))
  //! Get DMA FIFO depth in words
#define Usb_get_dma_fifo_word_depth()   (((Rd_bitfield(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_DMA_FIFO_WORD_DEPTH_MASK) - 1) & ((1 << AVR32_USBB_UFEATURES_DMA_FIFO_WORD_DEPTH_SIZE) - 1)) + 1)
  //! Get DPRAM size (FIFO maximal size) in bytes
#define Usb_get_dpram_size()            (128 << Rd_bitfield(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_FIFO_MAX_SIZE_MASK))
  //! Test if DPRAM is natively byte write capable
#define Is_usb_dpram_byte_write_capable() (Tst_bits(AVR32_USBB_ufeatures, AVR32_USBB_UFEATURES_BYTE_WRITE_DPRAM_MASK))
  //! Get size of USBB PB address space
#define Usb_get_ip_paddress_size()      (AVR32_USBB_uaddrsize)
//! @}


//! @defgroup USBB_general USBB common management drivers
//! These macros manage the USBB controller
//! @{
  //! Configure time-out of specified OTG timer
#define Usb_configure_timeout(timer, timeout) (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UNLOCK),\
                                               Wr_bitfield(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_TIMPAGE_Msk, timer, UOTGHS_CTRL_TIMPAGE_Pos),\
                                               Wr_bitfield(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_TIMVALUE_Msk, timeout, UOTGHS_CTRL_TIMVALUE_Pos),\
                                               Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UNLOCK))
  //! Get configured time-out of specified OTG timer
#define Usb_get_timeout(timer)                (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UNLOCK),\
                                               Wr_bitfield(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_TIMPAGE_Msk, timer, UOTGHS_CTRL_TIMPAGE_Pos),\
                                               Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UNLOCK),\
                                               Rd_bitfield(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_TIMVALUE_Msk, UOTGHS_CTRL_TIMVALUE_Pos))

  //! Enable external USB_ID pin (listened to by USB)
#define Usb_enable_id_pin()             (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIDE))
  //! Disable external USB_ID pin (ignored by USB)
#define Usb_disable_id_pin()            (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIDE))
  //! Test if external USB_ID pin enabled (listened to by USB)
#define Is_usb_id_pin_enabled()         (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIDE))
  //! Disable external USB_ID pin and force device mode
#define Usb_force_device_mode()         (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIMOD), Usb_disable_id_pin())
  //! Test if device mode is forced
#define Is_usb_device_mode_forced()     (!Is_usb_id_pin_enabled() && Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIMOD))
  //! Disable external USB_ID pin and force host mode
#define Usb_force_host_mode()           (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIMOD), Usb_disable_id_pin())
  //! Test if host mode is forced
#define Is_usb_host_mode_forced()       (!Is_usb_id_pin_enabled() && !Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_UIMOD))

#if USB_HOST_FEATURE == ENABLED
    //! Multiplexed pin used for USB_VBOF: AVR32_USBB_USB_VBOF_x_x.
    //! To be selected according to the AVR32_USBB_USB_VBOF_x_x_PIN and
    //! AVR32_USBB_USB_VBOF_x_x_FUNCTION definitions from <avr32/uc3cxxxx.h>.
//    #if (defined AVR32_USBB)
//    #  define USB_VBOF                           AVR32_USBB_VBOF_0_0
//    #else
    #define USB_VBOF                           AVR32_USBC_VBOF
//    #endif

  //! Check that multiplexed pin used for USB_VBOF is defined
  #ifndef USB_VBOF
    #error YOU MUST define in your board header file the multiplexed pin used for USB_VBOF as AVR32_USBB_USB_VBOF_x_x
  #endif
  //! Pin and function for USB_VBOF according to configuration from USB_VBOF
#define USB_VBOF_PIN            ATPASTE2(USB_VBOF, _PIN)
#define USB_VBOF_FUNCTION       ATPASTE2(USB_VBOF, _FUNCTION)
  //! Output USB_VBOF onto its pin
#define Usb_output_vbof_pin()   {const Pin pUOTGHSPinVBOF[] = {PIN_UOTGHS_VBOF};\
                                PIO_PinConfigure( pUOTGHSPinVBOF, PIO_LISTSIZE( pUOTGHSPinVBOF ));}




#endif  // USB_HOST_FEATURE == ENABLED
  //! Set USB_VBOF output pin polarity
#define Usb_set_vbof_active_high()      (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSPO))
#define Usb_set_vbof_active_low()       (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSPO))
  //! Get USB_VBOF output pin polarity
#define Is_usb_vbof_active_high()       (!Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSPO))
#define Is_usb_vbof_active_low()        (!Is_usb_vbof_active_high())

  //! Use device full speed mode (default)
#define Usb_use_full_speed_mode()       (Clr_bits(UOTGHS->UOTGHS_DEVCTRL, (1<<16) /*AVR32_USBB_UDCON_LS_MASK*/))
  //! Test if device full speed mode is used
#define Is_usb_full_speed_mode_used()   (!Is_usb_low_speed_mode_forced())
#ifdef AVR32_USBB_UDCON_SPDCONF
  //! Force device full speed mode (i.e. disable high speed)
//#define Usb_force_full_speed_mode()	(Wr_bitfield(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_SPDCONF_Msk, 3))
#define Usb_force_full_speed_mode()	(Clr_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_SPDCONF_Msk)),\
                                    (Set_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_SPDCONF_FULL_SPEED))
//! Enable dual speed mode (full speed and high speed; default)
#define Usb_use_dual_speed_mode()	(Wr_bitfield(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_SPDCONF_Msk, 0, UOTGHS_DEVCTRL_SPDCONF_Pos))
#else
#define Usb_force_full_speed_mode()	do { } while (0)
#define Usb_use_dual_speed_mode()	do { } while (0)
#endif
  //! Force device low-speed mode
#define Usb_force_low_speed_mode()      (Set_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_LS))
  //! Test if device low-speed mode is forced
#define Is_usb_low_speed_mode_forced()  (Tst_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_LS))
  //! Test if controller is in full speed mode
#define Is_usb_full_speed_mode()        (Rd_bitfield(UOTGHS->UOTGHS_SR, UOTGHS_SR_SPEED_Msk, UOTGHS_SR_SPEED_Pos) == 0x00)
  //! Test if controller is in low-speed mode
//#define Is_usb_low_speed_mode()         (Rd_bitfield(UOTGHS->UOTGHS_SR, UOTGHS_SR_SPEED_Msk) == AVR32_USBB_USBSTA_SPEED_LOW)
#define Is_usb_low_speed_mode()         (Rd_bitfield(UOTGHS->UOTGHS_SR, UOTGHS_SR_SPEED_Msk, UOTGHS_SR_SPEED_Pos) == 0x02)

  //! Enable USB macro
#define Usb_enable()                  (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_USBE))
  //! Disable USB macro
#define Usb_disable()                 (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_USBE))
#define Is_usb_enabled()              (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_USBE))

  //! Enable OTG pad
#define Usb_enable_otg_pad()          (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_OTGPADE))
  //! Disable OTG pad
#define Usb_disable_otg_pad()         (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_OTGPADE))
#define Is_usb_otg_pad_enabled()      (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_OTGPADE))

  //! Stop (freeze) internal USB clock
#define Usb_freeze_clock()            (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_FRZCLK))
#define Usb_unfreeze_clock()          (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_FRZCLK))
#define Is_usb_clock_frozen()         (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_FRZCLK))

  //! Get the dual-role device state of the internal USB finite state machine of the USBB controller
#define Usb_get_fsm_drd_state()       (Rd_bitfield(UOTGHS->UOTGHS_FSM, UOTGHS_FSM_DRDSTATE_Msk))

#define Usb_enable_id_interrupt()     (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_IDTE))
#define Usb_disable_id_interrupt()    (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_IDTE))
#define Is_usb_id_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_IDTE))
#define Is_usb_id_device()            (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_IDTI))
#define Usb_ack_id_transition()       (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_IDTIC)
#define Usb_raise_id_transition()     (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_IDTIS)
#define Is_usb_id_transition()        (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_IDTI))

#define Usb_enable_vbus_interrupt()   (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSTE))
#define Usb_disable_vbus_interrupt()  (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSTE))
#define Is_usb_vbus_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSTE))
#define Is_usb_vbus_high()            (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_VBUS))
#define Is_usb_vbus_low()             (!Is_usb_vbus_high())
#define Usb_ack_vbus_transition()     (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_VBUSTIC)
#define Usb_raise_vbus_transition()   (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_VBUSTIS)
#define Is_usb_vbus_transition()      (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_VBUSTI))

  //! enables hardware control over the USB_VBOF output pin
#define Usb_enable_vbus_hw_control()  (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSHWC))
  //! disables hardware control over the USB_VBOF output pin
#define Usb_disable_vbus_hw_control() (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSHWC))
#define Is_usb_vbus_hw_control_enabled() (!Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBUSHWC))
  //! requests VBus activation
#define Usb_enable_vbus()             (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_VBUSRQS)
  //! requests VBus deactivation
#define Usb_disable_vbus()            (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_VBUSRQC)
  //! tests if VBus activation has been requested
#define Is_usb_vbus_enabled()         (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_VBUSRQ))

  //! initiates a Host Negociation Protocol
#define Usb_device_initiate_hnp()     (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPREQ))
  //! accepts a Host Negociation Protocol
#define Usb_host_accept_hnp()         (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPREQ))
  //! rejects a Host Negociation Protocol
#define Usb_host_reject_hnp()         (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPREQ))
  //! initiates a Session Request Protocol
#define Usb_device_initiate_srp()     (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPREQ))
  //! selects VBus as SRP method
#define Usb_select_vbus_srp_method()  (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPSEL))
#define Is_usb_vbus_srp_method_selected() (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPSEL))
  //! selects data line as SRP method
#define Usb_select_data_srp_method()  (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPSEL))
#define Is_usb_data_srp_method_selected() (!Is_usb_vbus_srp_method_selected())
  //! tests if a HNP occurs
#define Is_usb_hnp()                  (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPREQ))
  //! tests if a SRP from device occurs
#define Is_usb_device_srp()           (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPREQ))

  //! enables suspend time out interrupt
#define Usb_enable_suspend_time_out_interrupt()   (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_STOE))
  //! disables suspend time out interrupt
#define Usb_disable_suspend_time_out_interrupt()  (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_STOE))
#define Is_usb_suspend_time_out_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_STOE))
  //! acks suspend time out interrupt
#define Usb_ack_suspend_time_out_interrupt()      (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_STOIC)
  //! raises suspend time out interrupt
#define Usb_raise_suspend_time_out_interrupt()    (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_STOIS)
  //! tests if a suspend time out occurs
#define Is_usb_suspend_time_out_interrupt()       (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_STOI))

  //! enables HNP error interrupt
#define Usb_enable_hnp_error_interrupt()          (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPERRE))
  //! disables HNP error interrupt
#define Usb_disable_hnp_error_interrupt()         (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPERRE))
#define Is_usb_hnp_error_interrupt_enabled()      (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_HNPERRE))
  //! acks HNP error interrupt
#define Usb_ack_hnp_error_interrupt()             (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_HNPERRIC)
  //! raises HNP error interrupt
#define Usb_raise_hnp_error_interrupt()           (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_HNPERRIS)
  //! tests if a HNP error occurs
#define Is_usb_hnp_error_interrupt()              (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_HNPERRI))

  //! enables role exchange interrupt
#define Usb_enable_role_exchange_interrupt()      (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_ROLEEXE))
  //! disables role exchange interrupt
#define Usb_disable_role_exchange_interrupt()     (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_ROLEEXE))
#define Is_usb_role_exchange_interrupt_enabled()  (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_ROLEEXE))
  //! acks role exchange interrupt
#define Usb_ack_role_exchange_interrupt()         (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_ROLEEXIC)
  //! raises role exchange interrupt
#define Usb_raise_role_exchange_interrupt()       (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_ROLEEXIS)
  //! tests if a role exchange occurs
#define Is_usb_role_exchange_interrupt()          (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_ROLEEXI))

  //! enables B-device connection error interrupt
#define Usb_enable_bconnection_error_interrupt()  (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_BCERRE))
  //! disables B-device connection error interrupt
#define Usb_disable_bconnection_error_interrupt() (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_BCERRE))
#define Is_usb_bconnection_error_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_BCERRE))
  //! acks B-device connection error interrupt
#define Usb_ack_bconnection_error_interrupt()     (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_BCERRIC)
  //! raises B-device connection error interrupt
#define Usb_raise_bconnection_error_interrupt()   (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_BCERRIS)
  //! tests if a B-device connection error occurs
#define Is_usb_bconnection_error_interrupt()      (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_BCERRI))

  //! enables VBus error interrupt
#define Usb_enable_vbus_error_interrupt()         (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBERRE))
  //! disables VBus error interrupt
#define Usb_disable_vbus_error_interrupt()        (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBERRE))
#define Is_usb_vbus_error_interrupt_enabled()     (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_VBERRE))
  //! acks VBus error interrupt
#define Usb_ack_vbus_error_interrupt()            (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_VBERRIC)
  //! raises VBus error interrupt
#define Usb_raise_vbus_error_interrupt()          (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_VBERRIS)
  //! tests if a VBus error occurs
#define Is_usb_vbus_error_interrupt()             (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_VBERRI))

  //! enables SRP interrupt
#define Usb_enable_srp_interrupt()                (Set_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPE))
  //! disables SRP interrupt
#define Usb_disable_srp_interrupt()               (Clr_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPE))
#define Is_usb_srp_interrupt_enabled()            (Tst_bits(UOTGHS->UOTGHS_CTRL, UOTGHS_CTRL_SRPE))
  //! acks SRP interrupt
#define Usb_ack_srp_interrupt()                   (UOTGHS->UOTGHS_SCR = UOTGHS_SCR_SRPIC)
  //! raises SRP interrupt
#define Usb_raise_srp_interrupt()                 (UOTGHS->UOTGHS_SFR = UOTGHS_SFR_SRPIS)
  //! tests if a SRP occurs
#define Is_usb_srp_interrupt()                    (Tst_bits(UOTGHS->UOTGHS_SR, UOTGHS_SR_SRPI))
//! @}


//! @defgroup USBB_device_driver USBB device controller drivers
//! These macros manage the USBB Device controller.
//! @{
  //! initiates a remote wake-up
#define Usb_initiate_remote_wake_up()             (Set_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_RMWKUP))
  //! detaches from USB bus
#define Usb_detach()                              (Set_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_DETACH))
  //! attaches to USB bus
#define Usb_attach()                              (Clr_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_DETACH))
  //! test if remote wake-up still running
#define Is_usb_pending_remote_wake_up()           (Tst_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_RMWKUP))
  //! test if the device is detached
#define Is_usb_detached()                         (Tst_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_DETACH))

  //! Test device compliance
#define Usb_dev_forceHighSpeed()   Wr_bitfield(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_SPDCONF_Msk, 2, UOTGHS_DEVCTRL_SPDCONF_Pos)

  //! enables remote wake-up interrupt
#define Usb_enable_remote_wake_up_interrupt()     (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_UPRSMES)
  //! disables remote wake-up interrupt
#define Usb_disable_remote_wake_up_interrupt()    (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_UPRSMEC)

#define Is_usb_remote_wake_up_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_UPRSME))
  //! acks remote wake-up
#define Usb_ack_remote_wake_up_start()            (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_UPRSMC)
  //! raises remote wake-up
#define Usb_raise_remote_wake_up_start()          (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_UPRSMS)
  //! tests if remote wake-up still running
#define Is_usb_remote_wake_up_start()             (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_UPRSM))

  //! enables resume interrupt
#define Usb_enable_resume_interrupt()             (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_EORSMES)
  //! disables resume interrupt
#define Usb_disable_resume_interrupt()            (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_EORSMEC)
#define Is_usb_resume_interrupt_enabled()         (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_EORSME))
  //! acks resume
#define Usb_ack_resume()                          (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_EORSMC)
  //! raises resume
#define Usb_raise_resume()                        (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_EORSMS)
  //! tests if resume occurs
#define Is_usb_resume()                           (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_EORSM))

  //! enables wake-up interrupt
#define Usb_enable_wake_up_interrupt()            (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_WAKEUPES)
  //! disables wake-up interrupt
#define Usb_disable_wake_up_interrupt()           (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_WAKEUPEC)
#define Is_usb_wake_up_interrupt_enabled()        (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_WAKEUPE))
  //! acks wake-up
#define Usb_ack_wake_up()                         (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_WAKEUPC)
  //! raises wake-up
#define Usb_raise_wake_up()                       (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_WAKEUPS)
  //! tests if wake-up occurs
#define Is_usb_wake_up()                          (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_WAKEUP))

  //! enables USB reset interrupt
#define Usb_enable_reset_interrupt()              (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_EORSTES)
  //! disables USB reset interrupt
#define Usb_disable_reset_interrupt()             (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_EORSTEC)
#define Is_usb_reset_interrupt_enabled()          (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_EORSTE))
  //! acks USB reset
#define Usb_ack_reset()                           (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_EORSTC)
  //! raises USB reset
#define Usb_raise_reset()                         (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_EORSTS)
  //! tests if USB reset occurs
#define Is_usb_reset()                            (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_EORST))

//! disables Micro Start of Frame Interrupt
#define Usb_disable_msof_interrupt()               (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_MSOFEC)
//! enables Start-of-Frame Interrupt
#define Usb_enable_sof_interrupt()                (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_SOFES)
  //! disables Start-of-Frame Interrupt
#define Usb_disable_sof_interrupt()               (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_SOFEC)
#define Is_usb_sof_interrupt_enabled()            (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVICR_SOFC))
  //! acks Start-of-Frame
#define Usb_ack_sof()                             (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_SOFC)
  //! raises Start-of-Frame
#define Usb_raise_sof()                           (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_SOFS)
  //! tests if Start-of-Frame occurs
#define Is_usb_sof()                              (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_SOF))

  //! enables suspend state interrupt
#define Usb_enable_suspend_interrupt()            (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_SUSPES)
  //! disables suspend state interrupt
#define Usb_disable_suspend_interrupt()           (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_SUSPEC)
#define Is_usb_suspend_interrupt_enabled()        (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_SUSPE))
  //! acks Suspend
#define Usb_ack_suspend()                         (UOTGHS->UOTGHS_DEVICR = UOTGHS_DEVICR_SUSPC)

  //! raises Suspend
#define Usb_raise_suspend()                       (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_SUSPS)
  //! tests if Suspend state detected
#define Is_usb_suspend()                          (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_SUSP))

  //! enables USB device address
#define Usb_enable_address()                      (Set_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_ADDEN))
  //! disables USB device address
#define Usb_disable_address()                     (Clr_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_ADDEN))
#define Is_usb_address_enabled()                  (Tst_bits(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_ADDEN))
  //! configures the USB device address
//#define Usb_configure_address(addr)               (Wr_bitfield(UOTGHS->UOTGHS_DEVCTRL, AVR32_USBB_UDCON_UADD_MASK, addr))
#define Usb_configure_address(addr)                 UOTGHS->UOTGHS_DEVCTRL &= ~UOTGHS_DEVCTRL_UADD_Msk;\
                                                    UOTGHS->UOTGHS_DEVCTRL |= UOTGHS_DEVCTRL_UADD(addr)
  //! gets the currently configured USB device address
#define Usb_get_configured_address()              (Rd_bitfield(UOTGHS->UOTGHS_DEVCTRL, UOTGHS_DEVCTRL_UADD_Msk))

  //! returns the current frame number
#define Usb_frame_number()                        (Rd_bitfield(UOTGHS->UOTGHS_DEVFNUM, UOTGHS_DEVFNUM_MFNUM_Msk))
  //! tests if a crc error occurs in frame number
#define Is_usb_frame_number_crc_error()           (Tst_bits(UOTGHS->UOTGHS_DEVFNUM, UOTGHS_DEVFNUM_FNCERR))
//! @}


//! @defgroup USBB_general_endpoint USBB endpoint drivers
//! These macros manage the common features of the endpoints.
//! @{
  //! resets the selected endpoint
#define Usb_reset_endpoint(ep)                    (Set_bits(UOTGHS->UOTGHS_DEVEPT, UOTGHS_DEVEPT_EPRST0 << (ep)),\
                                                   Clr_bits(UOTGHS->UOTGHS_DEVEPT, UOTGHS_DEVEPT_EPRST0 << (ep)))
  //! tests if the selected endpoint is being reset
#define Is_usb_resetting_endpoint(ep)             (Tst_bits(UOTGHS->UOTGHS_DEVEPT, UOTGHS_DEVEPT_EPRST0 << (ep)))

  //! enables the selected endpoint
#define Usb_enable_endpoint(ep)                   (Set_bits(UOTGHS->UOTGHS_DEVEPT, 1 << (ep)))
  //! enables the STALL handshake
#define Usb_enable_stall_handshake(ep)            (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_STALLRQS)
  //! Sends a STALL handshake for the next host request. A STALL handshake will
  //! be sent for each following request until a SETUP or a Clear Halt Feature
  //! occurs for this endpoint.
#define Usb_halt_endpoint(ep)                     (Usb_enable_stall_handshake(ep))
  //! resets the data toggle sequence
#define Usb_reset_data_toggle(ep)                 (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_RSTDTS)
  //! disables the selected endpoint
#define Usb_disable_endpoint(ep)                  (Clr_bits(UOTGHS->UOTGHS_DEVEPT, 1 << (ep)))
  //! disables the STALL handshake
#define Usb_disable_stall_handshake(ep)           (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_STALLRQC)
  //! tests if the selected endpoint is enabled
#define Is_usb_endpoint_enabled(ep)               (Tst_bits(UOTGHS->UOTGHS_DEVEPT, 1 << (ep)))
  //! tests if STALL handshake request is running
#define Is_usb_endpoint_stall_requested(ep)       (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_STALLRQ))
  //! tests if the data toggle sequence is being reset
#define Is_usb_data_toggle_reset(ep)              (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_RSTDT))

  //! tests if an interrupt is triggered by the selected endpoint
#define Is_usb_endpoint_interrupt(ep)             (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_PEP_0 << (ep)))
  //! enables the selected endpoint interrupt
#define Usb_enable_endpoint_interrupt(ep)         (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_PEP_0 << (ep))
  //! disables the selected endpoint interrupt
#define Usb_disable_endpoint_interrupt(ep)        (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_PEP_0 << (ep))
  //! tests if the selected endpoint interrupt is enabled
#define Is_usb_endpoint_interrupt_enabled(ep)     (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_PEP_0 << (ep)))
  //! returns the lowest endpoint number generating an endpoint interrupt or MAX_PEP_NB if none
#define Usb_get_interrupt_endpoint_number()       (ctz(((UOTGHS->UOTGHS_DEVISR >> 12 /*AVR32_USBB_UDINT_EP0INT_OFFSET*/) &\
                                                        (UOTGHS->UOTGHS_DEVIMR >> 12 /*AVR32_USBB_UDINTE_EP0INTE_OFFSET*/)) |\
                                                       (1 << MAX_PEP_NB)))

  //! configures the selected endpoint type
#define Usb_configure_endpoint_type(ep, type)     (Wr_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPTYPE_Msk, type, UOTGHS_DEVEPTCFG_EPTYPE_Pos))
  //! gets the configured selected endpoint type
#define Usb_get_endpoint_type(ep)                 (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPTYPE_Msk))
  //! enables the bank autoswitch for the selected endpoint
#define Usb_enable_endpoint_bank_autoswitch(ep)   (Set_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_AUTOSW))
  //! disables the bank autoswitch for the selected endpoint
#define Usb_disable_endpoint_bank_autoswitch(ep)   (Clr_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_AUTOSW))
#define Is_usb_endpoint_bank_autoswitch_enabled(ep) (Tst_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_AUTOSW))
  //! configures the selected endpoint direction
#define Usb_configure_endpoint_direction(ep, dir) (Wr_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPDIR, dir, 8))
  //! gets the configured selected endpoint direction
#define Usb_get_endpoint_direction(ep)            (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPDIR, 8))
  //! Bounds given integer size to allowed range and rounds it up to the nearest
  //! available greater size, then applies register format of USBB controller
  //! for endpoint size bit-field.
//#define Usb_format_endpoint_size(size)            (32 - clz(((U32)min(max(size, 8), 1024) << 1) - 1) - 1 - 3)
#define Usb_format_endpoint_size(size)          ((size <= 8  ) ? 0:\
                                                (size <= 16 ) ? 1:\
                                                (size <= 32 ) ? 2:\
                                                (size <= 64 ) ? 3:\
                                                (size <= 128) ? 4:\
                                                (size <= 256) ? 5:\
                                                (size <= 512) ? 6:7)

  //! configures the selected endpoint size
#define Usb_configure_endpoint_size(ep, size)     (Wr_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPSIZE_Msk, Usb_format_endpoint_size(size), UOTGHS_DEVEPTCFG_EPSIZE_Pos))
  //! gets the configured selected endpoint size
#define Usb_get_endpoint_size(ep)                 (8 << Rd_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPSIZE_Msk, UOTGHS_DEVEPTCFG_EPSIZE_Pos))
  //! configures the selected endpoint number of banks
#define Usb_configure_endpoint_bank(ep, bank)     (Wr_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPBK_Msk, bank, UOTGHS_DEVEPTCFG_EPBK_Pos))
  //! gets the configured selected endpoint number of banks
#define Usb_get_endpoint_bank(ep)                 (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPBK_Msk, UOTGHS_DEVEPTCFG_EPBK_Pos))
  //! allocates the configuration x in DPRAM memory
#define Usb_allocate_memory(ep)                   (Set_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_ALLOC))
  //! un-allocates the configuration x in DPRAM memory
#define Usb_unallocate_memory(ep)                 (Clr_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_ALLOC))
#define Is_usb_memory_allocated(ep)               (Tst_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_ALLOC))

  //! configures selected endpoint in one step
#define Usb_configure_endpoint(ep, type, dir, size, bank) \
(\
  Usb_enable_endpoint(ep),\
  Clr_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], 0xFFFF),\
  Wr_bits(UOTGHS->UOTGHS_DEVEPTCFG[ep], UOTGHS_DEVEPTCFG_EPTYPE_Msk |\
                                 UOTGHS_DEVEPTCFG_EPDIR  |\
                                 UOTGHS_DEVEPTCFG_EPSIZE_Msk |\
                                 UOTGHS_DEVEPTCFG_EPBK_Msk,   \
          ( (U32)(type) << UOTGHS_DEVEPTCFG_EPTYPE_Pos ) |\
          ( (U32)((dir) << 8 ) & UOTGHS_DEVEPTCFG_EPDIR ) |\
          ( (U32)(Usb_format_endpoint_size(size)) << UOTGHS_DEVEPTCFG_EPSIZE_Pos ) |\
          ( (U32)((bank) << UOTGHS_DEVEPTCFG_EPBK_Pos ) )),\
  Usb_allocate_memory(ep),\
\
  Is_usb_endpoint_configured(ep)\
)

  //! acks endpoint overflow interrupt
#define Usb_ack_overflow_interrupt(ep)            (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_OVERFIC)
  //! raises endpoint overflow interrupt
#define Usb_raise_overflow_interrupt(ep)          (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_OVERFIS)
  //! acks endpoint underflow interrupt
#define Usb_ack_underflow_interrupt(ep)           (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_RXSTPIC)
  //! raises endpoint underflow interrupt
#define Usb_raise_underflow_interrupt(ep)         (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_RXSTPIS)
  //! returns data toggle
#define Usb_data_toggle(ep)                       (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_DTSEQ_Msk))
  //! returns the number of busy banks
#define Usb_nb_busy_bank(ep)                      (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_NBUSYBK_Msk))
  //! tests if current endpoint is configured
#define Is_usb_endpoint_configured(ep)            (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_CFGOK))
  //! tests if an overflow occurs
#define Is_usb_overflow(ep)                       (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_OVERFI))
  //! tests if an underflow occurs
#define Is_usb_underflow(ep)                      (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_RXSTPI))

  //! returns the byte count
#define Usb_byte_count(ep)                        (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_BYCT_Msk, UOTGHS_DEVEPTISR_BYCT_Pos))
  //! returns the control direction
#define Usb_control_direction()                   (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTISR(EP_CONTROL), UOTGHS_DEVEPTISR_CTRLDIR, 17))
  //! returns the number of the current bank
#define Usb_current_bank(ep)                      (Rd_bitfield(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_CURRBK_Msk, UOTGHS_DEVEPTISR_CURRBK_Pos))

  //! kills last bank
#define Usb_kill_last_in_bank(ep)                 (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_KILLBKS)
  //! acks SHORT PACKET received
#define Usb_ack_short_packet(ep)                  (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_SHORTPACKETC)
  //! raises SHORT PACKET received
#define Usb_raise_short_packet(ep)                (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_SHORTPACKETS)
  //! acks STALL sent
#define Usb_ack_stall(ep)                         (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_STALLEDIC)
  //! raises STALL sent
#define Usb_raise_stall(ep)                       (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_SHORTPACKETS)
  //! acks CRC ERROR ISO OUT detected
#define Usb_ack_crc_error(ep)                     (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_STALLEDIC)
  //! raises CRC ERROR ISO OUT detected
#define Usb_raise_crc_error(ep)                   (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_STALLEDIS)
  //! acks NAK IN received
#define Usb_ack_nak_in(ep)                        (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_NAKINIC)
  //! raises NAK IN received
#define Usb_raise_nak_in(ep)                      (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_NAKINIS)
  //! acks NAK OUT received
#define Usb_ack_nak_out(ep)                       (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_NAKOUTIC)
  //! raises NAK OUT received
#define Usb_raise_nak_out(ep)                     (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_NAKOUTIS)

  //! tests if last bank killed
#define Is_usb_last_in_bank_killed(ep)            (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_KILLBK))
  //! tests if endpoint read allowed
#define Is_usb_read_enabled(ep)                   (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_RWALL))
  //! tests if endpoint write allowed
#define Is_usb_write_enabled(ep)                  (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_RWALL))
  //! tests if SHORT PACKET received
#define Is_usb_short_packet(ep)                   (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_SHORTPACKET))
  //! tests if STALL sent
#define Is_usb_stall(ep)                          (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_STALLEDI))
  //! tests if CRC ERROR ISO OUT detected
#define Is_usb_crc_error(ep)                      (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_STALLEDI))
  //! tests if NAK IN received
#define Is_usb_nak_in(ep)                         (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_NAKINI))
  //! tests if NAK OUT received
#define Is_usb_nak_out(ep)                        (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_NAKOUTI))

  //! clears FIFOCON bit
#define Usb_ack_fifocon(ep)                       (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_FIFOCONC)

  //! acks SETUP received
#define Usb_ack_setup_received_free()             (UOTGHS->UOTGHS_DEVEPTICR[EP_CONTROL] = UOTGHS_DEVEPTICR_RXSTPIC)
  //! raises SETUP received
#define Usb_raise_setup_received()                (UOTGHS->UOTGHS_DEVEPTIFR[EP_CONTROL] = UOTGHS_DEVEPTIFR_RXSTPIS)
  //! acks OUT received
#define Usb_ack_out_received(ep)                  (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_RXOUTIC)
  //! raises OUT received
#define Usb_raise_out_received(ep)                (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_RXOUTIS)
  //! frees current bank for OUT endpoint
#define Usb_free_out(ep)                          (Usb_ack_fifocon(ep))
  //! acks OUT received and frees current bank
#define Usb_ack_out_received_free(ep)             (Usb_ack_out_received(ep), Usb_free_out(ep))
  //! acks OUT received on control endpoint and frees current bank
#define Usb_ack_control_out_received_free()       (UOTGHS->UOTGHS_DEVEPTICR[EP_CONTROL] = UOTGHS_DEVEPTICR_RXOUTIC)
  //! raises OUT received on control endpoint
#define Usb_raise_control_out_received()          (UOTGHS->UOTGHS_DEVEPTIFR[EP_CONTROL] = UOTGHS_DEVEPTIFR_RXOUTIS)

  //! acks IN ready
#define Usb_ack_in_ready(ep)                      (UOTGHS->UOTGHS_DEVEPTICR[ep] = UOTGHS_DEVEPTICR_TXINIC)
  //! raises IN ready
#define Usb_raise_in_ready(ep)                    (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_TXINIS)
  //! sends current bank for IN endpoint
#define Usb_send_in(ep)                           (Usb_ack_fifocon(ep))
  //! acks IN ready and sends current bank
#define Usb_ack_in_ready_send(ep)                 (Usb_ack_in_ready(ep), Usb_send_in(ep))
  //! acks IN ready on control endpoint and sends current bank
#define Usb_ack_control_in_ready_send()           (UOTGHS->UOTGHS_DEVEPTICR[EP_CONTROL] = UOTGHS_DEVEPTICR_TXINIC)
  //! raises IN ready on control endpoint
#define Usb_raise_control_in_ready()              (UOTGHS->UOTGHS_DEVEPTIFR[EP_CONTROL] = UOTGHS_DEVEPTIFR_TXINIS)

  //! tests if FIFOCON bit set
#define Is_usb_fifocon(ep)                        (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_FIFOCON))

  //! tests if SETUP received
#define Is_usb_setup_received()                   (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[EP_CONTROL], UOTGHS_DEVEPTISR_RXSTPI))
  //! tests if OUT received
#define Is_usb_out_received(ep)                   (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_RXOUTI))
  //! tests if current bank filled for OUT endpoint
#define Is_usb_out_filled(ep)                     (Is_usb_fifocon(ep))
  //! tests if OUT received on control endpoint
#define Is_usb_control_out_received()             (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[EP_CONTROL], UOTGHS_DEVEPTISR_RXOUTI))

  //! tests if IN ready
#define Is_usb_in_ready(ep)                       (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[ep], UOTGHS_DEVEPTISR_TXINI))
  //! tests if current bank sent for IN endpoint
#define Is_usb_in_sent(ep)                        (Is_usb_fifocon(ep))
  //! tests if IN ready on control endpoint
#define Is_usb_control_in_ready()                 (Tst_bits(UOTGHS->UOTGHS_DEVEPTISR[EP_CONTROL], UOTGHS_DEVEPTISR_TXINI))

  //! forces all banks full (OUT) or free (IN) interrupt
#define Usb_force_bank_interrupt(ep)              (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_NBUSYBKS)
  //! unforces all banks full (OUT) or free (IN) interrupt
#define Usb_unforce_bank_interrupt(ep)            (UOTGHS->UOTGHS_DEVEPTIFR[ep] = UOTGHS_DEVEPTIFR_NBUSYBKS)
  //! enables all banks full (OUT) or free (IN) interrupt
#define Usb_enable_bank_interrupt(ep)             (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_NBUSYBKES)
  //! enables SHORT PACKET received interrupt
#define Usb_enable_short_packet_interrupt(ep)     (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_SHORTPACKETES)
  //! enables STALL sent interrupt
#define Usb_enable_stall_interrupt(ep)            (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_STALLRQS)
  //! enables CRC ERROR ISO OUT detected interrupt
#define Usb_enable_crc_error_interrupt(ep)        (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_STALLRQS)
  //! enables overflow interrupt
#define Usb_enable_overflow_interrupt(ep)         (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_OVERFES)
  //! enables NAK IN interrupt
#define Usb_enable_nak_in_interrupt(ep)           (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_NAKINES)
  //! enables NAK OUT interrupt
#define Usb_enable_nak_out_interrupt(ep)          (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_NAKOUTES)
  //! enables SETUP received interrupt
#define Usb_enable_setup_received_interrupt()     (UOTGHS->UOTGHS_DEVEPTIER(EP_CONTROL) = UOTGHS_DEVEPTIER_RXSTPES)
  //! enables underflow interrupt
#define Usb_enable_underflow_interrupt(ep)        (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_UNDERFES)
  //! enables OUT received interrupt
#define Usb_enable_out_received_interrupt(ep)     (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_RXOUTES)
  //! enables OUT received on control endpoint interrupt
#define Usb_enable_control_out_received_interrupt() (UOTGHS->UOTGHS_DEVEPTIER(EP_CONTROL) = UOTGHS_DEVEPTIER_RXOUTES)
  //! enables IN ready interrupt
#define Usb_enable_in_ready_interrupt(ep)         (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_TXINES)
  //! enables IN ready on control endpoint interrupt
#define Usb_enable_control_in_ready_interrupt()   (UOTGHS->UOTGHS_DEVEPTIER(EP_CONTROL) = UOTGHS_DEVEPTIER_TXINES)
  //! disables all banks full (OUT) or free (IN) interrupt
#define Usb_disable_bank_interrupt(ep)            (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_NBUSYBKEC)
  //! disables SHORT PACKET received interrupt
#define Usb_disable_short_packet_interrupt(ep)    (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_SHORTPACKETEC)
  //! disables STALL sent interrupt
#define Usb_disable_stall_interrupt(ep)           (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_STALLEDEC)
  //! disables CRC ERROR ISO OUT detected interrupt
#define Usb_disable_crc_error_interrupt(ep)       (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_STALLEDEC)
  //! disables overflow interrupt
#define Usb_disable_overflow_interrupt(ep)        (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_OVERFEC)
  //! disables NAK IN interrupt
#define Usb_disable_nak_in_interrupt(ep)          (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_NAKINEC)
  //! disables NAK OUT interrupt
#define Usb_disable_nak_out_interrupt(ep)         (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_NAKOUTEC)
  //! disables SETUP received interrupt
#define Usb_disable_setup_received_interrupt()    (UOTGHS->UOTGHS_DEVEPTIDR[EP_CONTROL] = UOTGHS_DEVEPTIDR_RXSTPEC)
  //! disables underflow interrupt
#define Usb_disable_underflow_interrupt(ep)       (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_RXSTPEC)
  //! disables OUT received interrupt
#define Usb_disable_out_received_interrupt(ep)    (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_RXOUTEC)
  //! disables OUT received on control endpoint interrupt
#define Usb_disable_control_out_received_interrupt() (UOTGHS->UOTGHS_DEVEPTIDR[EP_CONTROL] = UOTGHS_DEVEPTIDR_RXOUTEC)
  //! disables IN ready interrupt
#define Usb_disable_in_ready_interrupt(ep)        (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_TXINEC)
  //! disables IN ready on control endpoint interrupt
#define Usb_disable_control_in_ready_interrupt()  (UOTGHS->UOTGHS_DEVEPTIDR[EP_CONTROL] = UOTGHS_DEVEPTIDR_TXINEC)
  //! tests if all banks full (OUT) or free (IN) interrupt enabled
#define Is_usb_bank_interrupt_enabled(ep)         (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_NBUSYBKE))
  //! tests if SHORT PACKET received interrupt is enabled
#define Is_usb_short_packet_interrupt_enabled(ep) (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_SHORTPACKETE))
  //! tests if STALL sent interrupt is enabled
#define Is_usb_stall_interrupt_enabled(ep)        (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_STALLEDE))
  //! tests if CRC ERROR ISO OUT detected interrupt is enabled
#define Is_usb_crc_error_interrupt_enabled(ep)    (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_STALLEDE))
  //! tests if overflow interrupt is enabled
#define Is_usb_overflow_interrupt_enabled(ep)     (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_OVERFE))
  //! tests if NAK IN interrupt is enabled
#define Is_usb_nak_in_interrupt_enabled(ep)       (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_NAKINE))
  //! tests if NAK OUT interrupt is enabled
#define Is_usb_nak_out_interrupt_enabled(ep)      (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_NAKOUTE))
  //! tests if SETUP received interrupt is enabled
#define Is_usb_setup_received_interrupt_enabled() (Tst_bits(Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[EP_CONTROL], UOTGHS_DEVEPTIMR_RXSTPE))
  //! tests if underflow interrupt is enabled
#define Is_usb_underflow_interrupt_enabled(ep)    (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_RXSTPE))
  //! tests if OUT received interrupt is enabled
#define Is_usb_out_received_interrupt_enabled(ep) (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_RXOUTE))
  //! tests if OUT received on control endpoint interrupt is enabled
#define Is_usb_control_out_received_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[EP_CONTROL], UOTGHS_DEVEPTIMR_RXOUTE))
  //! tests if IN ready interrupt is enabled
#define Is_usb_in_ready_interrupt_enabled(ep)     (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_TXINE))
  //! tests if IN ready on control endpoint interrupt is enabled
#define Is_usb_control_in_ready_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[EP_CONTROL], UOTGHS_DEVEPTIMR_TXINE))


#define AVR32_USBB_SLAVE_ADDRESS           UOTGHS_RAM_ADDR
#define AVR32_USBB_SLAVE_SIZE              0x00800000
#define AVR32_USBB_SLAVE                   ((unsigned char *)AVR32_USBB_SLAVE_ADDRESS)
//#define USB_FIFO  (((volatile uint32_t *)UOTGHS_RAM_ADDR))
//#define USB_FIFO_CHAR  (((volatile uint8_t *)UOTGHS_RAM_ADDR))
#define TPASTE2( a, b)                            a##b
#define TPASTE3( a, b, c)                         a##b##c

//! Access point to the FIFO data registers of pipes/endpoints
  //! @param x      Pipe/endpoint of which to access FIFO data register
  //! @param scale  Data index scale in bits: 64, 32, 16 or 8
  //! @return       Volatile 64-, 32-, 16- or 8-bit data pointer to FIFO data register
#define AVR32_USBB_FIFOX_DATA(x, scale) \
          (((volatile TPASTE2(U, scale) (*)[0x8000 / ((scale) / 8)])AVR32_USBB_SLAVE)[(x)])


//! Get 64-, 32-, 16- or 8-bit access to FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @return       Volatile 64-, 32-, 16- or 8-bit data pointer to FIFO data register
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
#define Usb_get_endpoint_fifo_access(ep, scale) \
          (AVR32_USBB_FIFOX_DATA(ep, scale))

  //! Reset known position inside FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to reset known position
  //! @warning Always call this macro before any read/write macro/function
  //! when at FIFO beginning.
#define Usb_reset_endpoint_fifo_access(ep) \
          (pep_fifo[(ep)].u64ptr = Usb_get_endpoint_fifo_access(ep, 64))

  //! Read 64-, 32-, 16- or 8-bit data from FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @return       64-, 32-, 16- or 8-bit data read
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @note This macro assures that used HSB addresses are identical to the
  //! DPRAM internal pointer modulo 32 bits.
  //! @warning Always call Usb_reset_endpoint_fifo_access before this macro when
  //! at FIFO beginning.
  //! @warning Do not mix calls to this macro with calls to indexed macros below.
#define Usb_read_endpoint_data(ep, scale) \
          (*pep_fifo[(ep)].TPASTE3(u, scale, ptr)\
           TPASTE3(Pep_fifo_access_, scale, _post_inc)())

  //! Write 64-, 32-, 16- or 8-bit data to FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param data   64-, 32-, 16- or 8-bit data to write
  //! @return       64-, 32-, 16- or 8-bit data written
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @note This macro assures that used HSB addresses are identical to the
  //! DPRAM internal pointer modulo 32 bits.
  //! @warning Always call Usb_reset_endpoint_fifo_access before this macro when
  //! at FIFO beginning.
  //! @warning Do not mix calls to this macro with calls to indexed macros below.
#define Usb_write_endpoint_data(ep, scale, data) \
          (*pep_fifo[(ep)].TPASTE3(u, scale, ptr)\
           TPASTE3(Pep_fifo_access_, scale, _post_inc)() = (data))

  //! Read 64-, 32-, 16- or 8-bit indexed data from FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param index  Index of scaled data array to access
  //! @return       64-, 32-, 16- or 8-bit data read
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
  //! @warning Do not mix calls to this macro with calls to non-indexed macros above.
#define Usb_read_endpoint_indexed_data(ep, scale, index) \
          (AVR32_USBB_FIFOX_DATA(ep, scale)[(index)])

  //! Write 64-, 32-, 16- or 8-bit indexed data to FIFO data register of selected endpoint.
  //! @param ep     Endpoint of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param index  Index of scaled data array to access
  //! @param data   64-, 32-, 16- or 8-bit data to write
  //! @return       64-, 32-, 16- or 8-bit data written
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
  //! @warning Do not mix calls to this macro with calls to non-indexed macros above.
#define Usb_write_endpoint_indexed_data(ep, scale, index, data) \
          (AVR32_USBB_FIFOX_DATA(ep, scale)[(index)] = (data))
//! @}


//! @defgroup USBB_general_endpoint_dma USBB endpoint DMA drivers
//! These macros manage the common features of the endpoint DMA channels.
//! @{
  //! enables the disabling of HDMA requests by endpoint interrupts
#define Usb_enable_endpoint_int_dis_hdma_req(ep)      (UOTGHS->UOTGHS_DEVEPTIER[ep] = UOTGHS_DEVEPTIER_EPDISHDMAS)
  //! disables the disabling of HDMA requests by endpoint interrupts
#define Usb_disable_endpoint_int_dis_hdma_req(ep)     (UOTGHS->UOTGHS_DEVEPTIDR[ep] = UOTGHS_DEVEPTIDR_EPDISHDMAC)
  //! tests if the disabling of HDMA requests by endpoint interrupts is enabled
#define Is_usb_endpoint_int_dis_hdma_req_enabled(ep)  (Tst_bits(UOTGHS->UOTGHS_DEVEPTIMR[ep], UOTGHS_DEVEPTIMR_EPDISHDMA))

  //! raises the selected endpoint DMA channel interrupt
#define Usb_raise_endpoint_dma_interrupt(epdma)       (UOTGHS->UOTGHS_DEVIFR = UOTGHS_DEVIFR_DMA_1 << ((epdma) - 1))
  //! tests if an interrupt is triggered by the selected endpoint DMA channel
#define Is_usb_endpoint_dma_interrupt(epdma)          (Tst_bits(UOTGHS->UOTGHS_DEVISR, UOTGHS_DEVISR_DMA_1 << ((epdma) - 1)))
  //! enables the selected endpoint DMA channel interrupt
#define Usb_enable_endpoint_dma_interrupt(epdma)      (UOTGHS->UOTGHS_DEVIER = UOTGHS_DEVIER_DMA_1 << ((epdma) - 1))
  //! disables the selected endpoint DMA channel interrupt
#define Usb_disable_endpoint_dma_interrupt(epdma)     (UOTGHS->UOTGHS_DEVIDR = UOTGHS_DEVIDR_DMA_1 << ((epdma) - 1))
  //! tests if the selected endpoint DMA channel interrupt is enabled
#define Is_usb_endpoint_dma_interrupt_enabled(epdma)  (Tst_bits(UOTGHS->UOTGHS_DEVIMR, UOTGHS_DEVIMR_DMA_1 << ((epdma) - 1)))
//! @todo Implement macros for endpoint DMA registers and descriptors
#if 0
#define Usb_set_endpoint_dma_nxt_desc_addr(epdma, nxt_desc_addr) (AVR32_USBB_UDDMAX_NEXTDESC(epdma).nxt_desc_addr = (U32)(nxt_desc_addr))
#define Usb_get_endpoint_dma_nxt_desc_addr(epdma)                ((avr32_usbb_uxdmax_t *)AVR32_USBB_UDDMAX_NEXTDESC(epdma).nxt_desc_addr)
#define (epdma) (AVR32_USBB_UDDMAX_addr(epdma))
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).ch_byte_length)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).burst_lock_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).desc_ld_irq_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).eobuff_irq_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).eot_irq_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).dmaend_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).buff_close_in_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).ld_nxt_ch_desc_en)
#define (epdma) (AVR32_USBB_UDDMAX_CONTROL(epdma).ch_en)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).ch_byte_cnt)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).desc_ld_sta)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).eoch_buff_sta)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).eot_sta)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).ch_active)
#define (epdma) (AVR32_USBB_UDDMAX_STATUS(epdma).ch_en)
#endif
//! @}


//! @defgroup USBB_host_driver USBB host controller drivers
//! These macros manage the USBB Host controller.
//! @{
  //! enables SOF generation
#define Host_enable_sof()                      (Set_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SOFE))
  //! disables SOF generation
#define Host_disable_sof()                     (Clr_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SOFE))
  //! tests if SOF generation enabled
#define Is_host_sof_enabled()                  (Tst_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SOFE))
  //! sends a USB Reset to the device
#define Host_send_reset()                      (Set_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_RESET))
  //! stops sending a USB Reset to the device
#define Host_stop_sending_reset()              (Clr_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_RESET))
  //! tests if USB Reset running
#define Is_host_sending_reset()                (Tst_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_RESET))
  //! sends a USB Resume to the device
#define Host_send_resume()                     (Set_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_RESUME))
  //! tests if USB Resume running
#define Is_host_sending_resume()               (Tst_bits(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_RESUME))

#ifdef AVR32_USBB_UHCON_SPDCONF
  //! Force device full speed mode (i.e. disable high speed)
#define Host_force_full_speed_mode()	        (Wr_bitfield(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SPDCONF_Msk, 3, UOTGHS_HSTCTRL_SPDCONF_Pos))
  //! Enable hihgh speed mode
#define Host_enable_high_speed_mode()	        (Wr_bitfield(UOTGHS->UOTGHS_HSTCTRL, UOTGHS_HSTCTRL_SPDCONF_Msk, 0, UOTGHS_HSTCTRL_SPDCONF_Pos))
#endif

  //! enables host Start-of-Frame interrupt
#define Host_enable_sof_interrupt()            (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_HSOFIES)
  //! enables host Start-of-Frame interrupt
#define Host_disable_sof_interrupt()           (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_HSOFIEC)
#define Is_host_sof_interrupt_enabled()        (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_HSOFIE))
  //! acks SOF detection
#define Host_ack_sof()                         (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_HSOFIC)
  //! raises SOF detection
#define Host_raise_sof()                       (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_HSOFIS)
  //! tests if SOF detected
#define Is_host_sof()                          (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_HSOFI))

  //! enables host wake-up interrupt detection
#define Host_enable_hwup_interrupt()            (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_HWUPIES)
  //! disables host wake-up interrupt detection
#define Host_disable_hwup_interrupt()           (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_HWUPIEC)
#define Is_host_hwup_interrupt_enabled()        (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_HWUPIE))
  //! acks host wake-up detection
#define Host_ack_hwup()                         (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_HWUPIC)
  //! raises host wake-up detection
#define Host_raise_hwup()                       (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_HWUPIS)
  //! tests if host wake-up detected
#define Is_host_hwup()                          (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_HWUPI))

  //! enables host down stream rsm sent interrupt detection
#define Host_enable_down_stream_resume_interrupt()            (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_RSMEDIES)
  //! disables host down stream rsm sent interrupt detection
#define Host_disable_down_stream_resume_interrupt()           (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_RSMEDIEC)
#define Is_host_down_stream_resume_interrupt_enabled()        (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_RSMEDIE))
  //! acks host down stream resume sent
#define Host_ack_down_stream_resume()                         (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_RSMEDIC)
  //! raises host down stream resume sent
#define Host_raise_down_stream_resume()                       (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_RSMEDIS)
#define Is_host_down_stream_resume()                          (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_RSMEDI))

  //! enables host remote wake-up interrupt detection
#define Host_enable_remote_wakeup_interrupt()         (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_RXRSMIES)
  //! disables host remote wake-up interrupt detection
#define Host_disable_remote_wakeup_interrupt()        (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_RXRSMIEC)
#define Is_host_remote_wakeup_interrupt_enabled()     (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_RXRSMIE))
  //! acks host remote wake-up detection
#define Host_ack_remote_wakeup()                      (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_RXRSMIC)
  //! raises host remote wake-up detection
#define Host_raise_remote_wakeup()                    (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_RXRSMIS)
  //! tests if host remote wake-up detected
#define Is_host_remote_wakeup()                       (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_RXRSMI))

  //! enables host device connection interrupt
#define Host_enable_device_connection_interrupt()     (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_DCONNIES)
  //! disables USB device connection interrupt
#define Host_disable_device_connection_interrupt()    (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_DCONNIEC)
#define Is_host_device_connection_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_DCONNIE))
  //! acks device connection
#define Host_ack_device_connection()           (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_DCONNIC)
  //! raises device connection
#define Host_raise_device_connection()         (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_DCONNIS)
  //! tests if a USB device has been detected
#define Is_host_device_connection()            (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_DCONNI))

  //! enables host device disconnection interrupt
#define Host_enable_device_disconnection_interrupt()     (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_DDISCIES)
  //! disables USB device connection interrupt
#define Host_disable_device_disconnection_interrupt()    (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_DDISCIEC)
#define Is_host_device_disconnection_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_DDISCIE))
  //! acks device disconnection
#define Host_ack_device_disconnection()        (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_DDISCIC)
  //! raises device disconnection
#define Host_raise_device_disconnection()      (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_DDISCIS)
  //! tests if a USB device has been removed
#define Is_host_device_disconnection()         (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_DDISCI))

  //! enables host USB reset sent interrupt
#define Host_enable_reset_sent_interrupt()     (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_RSTIES)
  //! disables host USB reset sent interrupt
#define Host_disable_reset_sent_interrupt()    (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_RSTIEC)
#define Is_host_reset_sent_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_RSTIE))
  //! acks host USB reset sent
#define Host_ack_reset_sent()                  (UOTGHS->UOTGHS_HSTICR = UOTGHS_HSTICR_RSTIC)
  //! raises host USB reset sent
#define Host_raise_reset_sent()                (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_RSTIS)
  //! tests if host USB reset sent
#define Is_host_reset_sent()                   (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_RSTI))

  //! sets the current frame number
#define Host_set_frame_number(fnum)            (Wr_bitfield(UOTGHS->UOTGHS_HSTFNUM, UOTGHS_HSTFNUM_MFNUM_Msk, fnum, UOTGHS_HSTFNUM_MFNUM_Pos))
  //! returns the current frame number
#define Host_frame_number()                    (Rd_bitfield(UOTGHS->UOTGHS_HSTFNUM, UOTGHS_HSTFNUM_MFNUM_Msk, UOTGHS_HSTFNUM_MFNUM_Pos))
  //! returns the current frame length
#define Host_frame_length()                    (Rd_bitfield(UOTGHS->UOTGHS_HSTFNUM, UOTGHS_HSTFNUM_FLENHIGH_Msk, UOTGHS_HSTFNUM_FLENHIGH_Pos))

  //! configures the USB device address associated with the selected pipe
//#define Host_configure_address(p, addr)      (Wr_bitfield(AVR32_USBB_uhaddrx(1 + ((p) >> 2)), AVR32_USBB_UHADDR1_UHADDR_P0_MASK << (((p) & 0x03) << 3), addr))
//#define Host_configure_address(p, addr)        (Wr_bitfield(UOTGHS->UOTGHS_HSTADDR1[1 + ((p) >> 2)], UOTGHS_HSTADDR1_HSTADDRP0_Msk << (((p) & 0x03) << 3), addr, UOTGHS_HSTADDR1_HSTADDRP0_Pos))
  //! gets the currently configured USB device address associated with the selected pipe
//#define Host_get_configured_address(p)       (Rd_bitfield(AVR32_USBB_uhaddrx(1 + ((p) >> 2)), AVR32_USBB_UHADDR1_UHADDR_P0_MASK << (((p) & 0x03) << 3)))
#define Host_get_configured_address(p)         (Rd_bitfield(UOTGHS->UOTGHS_HSTADDR1[1 + ((p) >> 2)], UOTGHS_HSTADDR1_HSTADDRP0_Msk << (((p) & 0x03) << 3), UOTGHS_HSTADDR1_HSTADDRP0_Pos))
//! @}


//! @defgroup USBB_general_pipe USBB pipe drivers
//! These macros manage the common features of the pipes.
//! @{
  //! enables the selected pipe
#define Host_enable_pipe(p)                    (Set_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PEN0 << (p)))
  //! disables the selected pipe
#define Host_disable_pipe(p)                   (Clr_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PEN0 << (p)))
  //! tests if the selected pipe is enabled
#define Is_host_pipe_enabled(p)                (Tst_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PEN0 << (p)))

  //! tests if an interrupt is triggered by the selected pipe
#define Is_host_pipe_interrupt(p)              (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_PEP_0 << (p)))
  //! enables the selected pipe interrupt
#define Host_enable_pipe_interrupt(p)          (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_PEP_0 << (p))
  //! disables the selected pipe interrupt
#define Host_disable_pipe_interrupt(p)         (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_PEP_0 << (p))
  //! tests if the selected pipe interrupt is enabled
#define Is_host_pipe_interrupt_enabled(p)      (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_PEP_0 << (p)))
  //! returns the lowest pipe number generating a pipe interrupt or MAX_PEP_NB if none
#define Host_get_interrupt_pipe_number(p)      ((UOTGHS->UOTGHS_HSTISR >> 8 >> p) &\
                                                (UOTGHS->UOTGHS_HSTIMR >> 8 >> p))

  //! configures the interrupt pipe request frequency (period in ms) for the selected pipe
#define Host_configure_pipe_int_req_freq(p, freq) (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_INTFRQ_Msk, freq, UOTGHS_HSTPIPCFG_INTFRQ_Pos))
  //! gets the configured interrupt pipe request frequency (period in ms) for the selected pipe
#define Host_get_pipe_int_req_freq(p)          (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_INTFRQ_Msk, UOTGHS_HSTPIPCFG_INTFRQ_Pos))
  //! configures the selected pipe endpoint number
#define Host_configure_pipe_endpoint_number(p, ep_num) (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PEPNUM_Msk, ep_num, UOTGHS_HSTPIPCFG_PEPNUM_Pos))
  //! gets the configured selected pipe endpoint number
#define Host_get_pipe_endpoint_number(p)       (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PEPNUM_Msk, UOTGHS_HSTPIPCFG_PEPNUM_Pos))
  //! configures the selected pipe type
#define Host_configure_pipe_type(p, type)      (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PTYPE_Msk, type, UOTGHS_HSTPIPCFG_PTYPE_Pos))
  //! gets the configured selected pipe type
#define Host_get_pipe_type(p)                  (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PTYPE_Msk, UOTGHS_HSTPIPCFG_PTYPE_Pos))
  //! enables the bank autoswitch for the selected pipe
#define Host_enable_pipe_bank_autoswitch(p)    (Set_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_AUTOSW))
  //! disables the bank autoswitch for the selected pipe
#define Host_disable_pipe_bank_autoswitch(p)   (Clr_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_AUTOSW))
#define Is_host_pipe_bank_autoswitch_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_AUTOSW))
  //! configures the selected pipe token
#define Host_configure_pipe_token(p, token)    (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PTOKEN_Msk, token, UOTGHS_HSTPIPCFG_PTOKEN_Pos))
  //! gets the configured selected pipe token
#define Host_get_pipe_token(p)                 (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PTOKEN_Msk, UOTGHS_HSTPIPCFG_PTOKEN_Pos))
  //! Bounds given integer size to allowed range and rounds it up to the nearest
  //! available greater size, then applies register format of USBB controller
  //! for pipe size bit-field.
//#define Host_format_pipe_size(size)            (32 - clz(((U32)min(max(size, 8), 1024) << 1) - 1) - 1 - 3)
#define Host_format_pipe_size(size)             (size <= 8  ) ? (  8):\
                                                (size <= 16 ) ? ( 16):\
                                                (size <= 32 ) ? ( 32):\
                                                (size <= 64 ) ? ( 64):\
                                                (size <= 128) ? (128):\
                                                (size <= 256) ? (256):\
                                                (size <= 512) ? (512):(1024)
  //! configures the selected pipe size
#define Host_configure_pipe_size(p, size)      (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PSIZE_Msk, Host_format_pipe_size(size), UOTGHS_HSTPIPCFG_PSIZE_Pos))
  //! gets the configured selected pipe size
#define Host_get_pipe_size(p)                  (8U << Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PSIZE_Msk, UOTGHS_HSTPIPCFG_PSIZE_Pos))
  //! configures the selected pipe number of banks
#define Host_configure_pipe_bank(p, bank)      (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PBK_Msk, bank, UOTGHS_HSTPIPCFG_PBK_Pos))
  //! gets the configured selected pipe number of banks
#define Host_get_pipe_bank(p)                  (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PBK_Msk, UOTGHS_HSTPIPCFG_PBK_Pos))
  //! allocates the configuration x in DPRAM memory
#define Host_allocate_memory(p)                (Set_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_ALLOC))
  //! un-allocates the configuration x in DPRAM memory
#define Host_unallocate_memory(p)              (Clr_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_ALLOC))
#define Is_host_memory_allocated(p)            (Tst_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_ALLOC))
  //! Enable PING management for the endpoint p
#define Host_enable_ping(p)                    (Set_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_PINGEN))

  //! configures selected pipe in one step
#define Host_configure_pipe(p, freq, ep_num, type, token, size, bank) \
(\
  Host_enable_pipe(p),\
  Clr_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], 0xFFFFFFFF),\
  Wr_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_INTFRQ_Msk |\
                                UOTGHS_HSTPIPCFG_PEPNUM_Msk |\
                                UOTGHS_HSTPIPCFG_PTYPE_Msk  |\
                                UOTGHS_HSTPIPCFG_PTOKEN_Msk |\
                                UOTGHS_HSTPIPCFG_PSIZE_Msk  |\
                                UOTGHS_HSTPIPCFG_PBK_Msk,    \
          (UOTGHS_HSTPIPCFG_INTFRQ((U32)(freq  ))) |\
          (UOTGHS_HSTPIPCFG_PEPNUM((U32)(ep_num))) |\
          ( (U32)(type  ) << UOTGHS_HSTPIPCFG_PTYPE_Pos ) |\
          ( (U32)(token ) << UOTGHS_HSTPIPCFG_PTOKEN_Pos) |\
          ( (U32)Usb_format_endpoint_size(size) << UOTGHS_HSTPIPCFG_PSIZE_Pos ) |\
          ( (U32)(bank ) << UOTGHS_HSTPIPCFG_PBK_Pos ) ),\
  Host_allocate_memory(p),\
\
  Is_host_pipe_configured(p)\
)


//static void Host_configure_pipe(uint8_t p, uint32_t freq, uint32_t ep_num, uint32_t type, uint32_t token, uint32_t size, uint32_t bank)
//{
////  Host_enable_pipe(p);
////  Wr_bits(UOTGHS->UOTGHS_HSTPIPCFG[p], UOTGHS_HSTPIPCFG_INTFRQ_Msk |
////                                UOTGHS_HSTPIPCFG_PEPNUM_Msk |
////                                UOTGHS_HSTPIPCFG_PTYPE_Msk  |
////                                UOTGHS_HSTPIPCFG_PTOKEN_Msk |
////                                UOTGHS_HSTPIPCFG_PSIZE_Msk  |
////                                UOTGHS_HSTPIPCFG_PBK_Msk,
////          (UOTGHS_HSTPIPCFG_INTFRQ((U32)(freq  ))) |
////          (UOTGHS_HSTPIPCFG_PEPNUM((U32)(ep_num))) |
////          (((U32)(type  ) << UOTGHS_HSTPIPCFG_PTYPE_Pos ) & UOTGHS_HSTPIPCFG_PTYPE_Msk ) |
////          (((U32)(token ) << UOTGHS_HSTPIPCFG_PTOKEN_Pos) & UOTGHS_HSTPIPCFG_PTOKEN_Msk) |
////          ( (U32)Host_format_pipe_size(size) << UOTGHS_HSTPIPCFG_PSIZE_Pos               ) |
////          (((U32)(bank  ) << UOTGHS_HSTPIPCFG_PBK_Pos   ) & UOTGHS_HSTPIPCFG_PBK_Msk   ));
////  Host_allocate_memory(p);
////  Is_host_pipe_configured(p);
//}

  //! resets the selected pipe
#define Host_reset_pipe(p)                     (Set_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PRST0 << (p)),\
                                                Clr_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PRST0 << (p)))
  //! tests if the selected pipe is being reset
#define Is_host_resetting_pipe(p)              (Tst_bits(UOTGHS->UOTGHS_HSTPIP, UOTGHS_HSTPIP_PRST0 << (p)))

  //! freezes the pipe
#define Host_freeze_pipe(p)                    (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_PFREEZES)
  //! unfreezees the pipe
#define Host_unfreeze_pipe(p)                  (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_PFREEZEC)
  //! tests if the current pipe is frozen
#define Is_host_pipe_frozen(p)                 (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_PFREEZE))

  //! resets the data toggle sequence
#define Host_reset_data_toggle(p)              (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_RSTDTS)
  //! tests if the data toggle sequence is being reset
#define Is_host_data_toggle_reset(p)           (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_RSTDT))

  //! acks pipe overflow interrupt
#define Host_ack_overflow_interrupt(p)         (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_OVERFIC)
  //! raises pipe overflow interrupt
#define Host_raise_overflow_interrupt(p)       (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_OVERFIS)
  //! acks pipe underflow interrupt
#define Host_ack_underflow_interrupt(p)        (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_TXSTPIC)
  //! raises pipe underflow interrupt
#define Host_raise_underflow_interrupt(p)      (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_TXSTPIS)
  //! tests if an overflow occurs
#define Is_host_overflow(p)                    (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_OVERFI))
  //! tests if an underflow occurs
#define Is_host_underflow(p)                   (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_TXSTPI))

  //! returns data toggle
#define Host_data_toggle(p)                    (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_DTSEQ_Msk, UOTGHS_HSTPIPISR_DTSEQ_Pos))
  //! returns the number of busy banks
#define Host_nb_busy_bank(p)                   (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_NBUSYBK_Msk, UOTGHS_HSTPIPISR_NBUSYBK_Pos))
  //! returns the number of the current bank
#define Host_current_bank(p)                   (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_CURRBK_Msk, UOTGHS_HSTPIPISR_CURRBK_Pos))
  //! tests if current pipe is configured
#define Is_host_pipe_configured(p)             (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_CFGOK))
  //! returns the byte count
#define Host_byte_count(p)                     (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_PBYCT_Msk, UOTGHS_HSTPIPISR_PBYCT_Pos))

  //! tests if a STALL has been received
#define Is_host_stall(p)                       (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_RXSTALLDI))
  //! tests if CRC ERROR ISO IN detected
#define Is_host_crc_error(p)                   (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_RXSTALLDI))
  //! tests if an error occurs on current pipe
#define Is_host_pipe_error(p)                  (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_PERRI))
  //! tests if SHORT PACKET received
#define Is_host_short_packet(p)                (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_SHORTPACKETI))

  //! clears FIFOCON bit
#define Host_ack_fifocon(p)                    (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_FIFOCONC)

  //! acks setup
#define Host_ack_setup_ready()                 (UOTGHS->UOTGHS_HSTPIPICR[P_CONTROL] = UOTGHS_HSTPIPICR_TXSTPIC)
  //! raises setup
#define Host_raise_setup_ready()               (UOTGHS->UOTGHS_HSTPIPIFR[P_CONTROL] = UOTGHS_HSTPIPIFR_TXSTPIS)
  //! sends current bank for SETUP pipe
#define Host_send_setup()                      (Host_ack_fifocon(P_CONTROL))
  //! acks setup and sends current bank
#define Host_ack_setup_ready_send()            (Host_ack_setup_ready(), Host_send_setup())
  //! acks OUT sent
#define Host_ack_out_ready(p)                  (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_TXOUTIC)
  //! raises OUT sent
#define Host_raise_out_ready(p)                (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_TXOUTIS)
  //! sends current bank for OUT pipe
#define Host_send_out(p)                       (Host_ack_fifocon(p))
  //! acks OUT sent and sends current bank
#define Host_ack_out_ready_send(p)             (Host_ack_out_ready(p), Host_send_out(p))

  //! acks IN reception
#define Host_ack_in_received(p)                (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_RXINIC)
  //! raises IN reception
#define Host_raise_in_received(p)              (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_RXINIS)
  //! frees current bank for IN pipe
#define Host_free_in(p)                        (Host_ack_fifocon(p))
  //! acks IN reception and frees current bank
#define Host_ack_in_received_free(p)           (Host_ack_in_received(p), Host_free_in(p))

  //! tests if FIFOCON bit set
#define Is_host_fifocon(p)                     (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_FIFOCON))

  //! tests if SETUP has been sent
#define Is_host_setup_ready()                  (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[P_CONTROL], UOTGHS_HSTPIPISR_TXSTPI))
  //! tests if current bank sent for SETUP pipe
#define Is_host_setup_sent()                   (Is_host_fifocon(P_CONTROL))
  //! tests if OUT has been sent
#define Is_host_out_ready(p)                   (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_TXOUTI))
  //! tests if current bank sent for OUT pipe
#define Is_host_out_sent(p)                    (Is_host_fifocon(p))

  //! tests if IN received
#define Is_host_in_received(p)                 (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_RXINI))
  //! tests if IN received in current bank
#define Is_host_in_filled(p)                   (Is_host_fifocon(p))

  //! acks STALL reception
#define Host_ack_stall(p)                      (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_RXSTALLDIC)
  //! raises STALL reception
#define Host_raise_stall(p)                    (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_RXSTALLDIS)
  //! acks CRC ERROR ISO IN detected
#define Host_ack_crc_error(p)                  (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_RXSTALLDIC)
  //! raises CRC ERROR ISO IN detected
#define Host_raise_crc_error(p)                (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_RXSTALLDIS)
  //! acks pipe error
#define Host_ack_pipe_error(p)                 (UOTGHS->UOTGHS_HSTPIPIFR[p] = (Is_host_pipe_error(p)) ? UOTGHS_HSTPIPIFR_PERRIS : 0)
  //! raises pipe error
#define Host_raise_pipe_error(p)               (UOTGHS->UOTGHS_HSTPIPIFR[p] = (Is_host_pipe_error(p)) ? 0 : UOTGHS_HSTPIPIFR_PERRIS)
  //! acks SHORT PACKET received
#define Host_ack_short_packet(p)               (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_SHORTPACKETIC)
  //! raises SHORT PACKET received
#define Host_raise_short_packet(p)             (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_SHORTPACKETIS)

  //! tests if NAK handshake has been received
#define Is_host_nak_received(p)                (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_NAKEDI))
  //! acks NAK received
#define Host_ack_nak_received(p)               (UOTGHS->UOTGHS_HSTPIPICR[p] = UOTGHS_HSTPIPICR_NAKEDIC)
  //! raises NAK received
#define Host_raise_nak_received(p)             (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_NAKEDIS)

  //! tests if pipe read allowed
#define Is_host_read_enabled(p)                (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_RWALL))
  //! tests if pipe write allowed
#define Is_host_write_enabled(p)               (Tst_bits(UOTGHS->UOTGHS_HSTPIPISR[p], UOTGHS_HSTPIPISR_RWALL))

  //! enables continuous IN mode
#define Host_enable_continuous_in_mode(p)      (Set_bits(UOTGHS->UOTGHS_HSTPIPINRQ[p], UOTGHS_HSTPIPINRQ_INMODE))
  //! disables continuous IN mode
#define Host_disable_continuous_in_mode(p)     (Clr_bits(UOTGHS->UOTGHS_HSTPIPINRQ[p], UOTGHS_HSTPIPINRQ_INMODE))
  //! tests if continuous IN mode is enabled
#define Is_host_continuous_in_mode_enabled(p)  (Tst_bits(UOTGHS->UOTGHS_HSTPIPINRQ[p], UOTGHS_HSTPIPINRQ_INMODE))

  //! sets number of IN requests to perform before freeze
#define Host_in_request_number(p, in_num)      (Wr_bitfield(UOTGHS->UOTGHS_HSTPIPINRQ[p], UOTGHS_HSTPIPINRQ_INRQ_Msk, (in_num) - 1, UOTGHS_HSTPIPINRQ_INRQ_Pos))
  //! returns number of remaining IN requests
#define Host_get_in_request_number(p)          (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPINRQ[p], UOTGHS_HSTPIPINRQ_INRQ_Msk) + 1, UOTGHS_HSTPIPINRQ_INRQ_Pos)

  //! acks all pipe error
#define Host_ack_all_errors(p)                 (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATATGL |\
                                                                               UOTGHS_HSTPIPERR_DATAPID |\
                                                                               UOTGHS_HSTPIPERR_PID     |\
                                                                               UOTGHS_HSTPIPERR_TIMEOUT |\
                                                                               UOTGHS_HSTPIPERR_CRC16   |\
                                                                               UOTGHS_HSTPIPERR_COUNTER_Msk))
  //! tests if error occurs on pipe
#define Host_error_status(p)                   (Rd_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATATGL |\
                                                                              UOTGHS_HSTPIPERR_DATAPID |\
                                                                              UOTGHS_HSTPIPERR_PID     |\
                                                                              UOTGHS_HSTPIPERR_TIMEOUT |\
                                                                              UOTGHS_HSTPIPERR_CRC16))

  //! acks bad data toggle
#define Host_ack_bad_data_toggle(p)            (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATATGL))
#define Is_host_bad_data_toggle(p)             (Tst_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATATGL))
  //! acks data PID error
#define Host_ack_data_pid_error(p)             (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATAPID))
#define Is_host_data_pid_error(p)              (Tst_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_DATAPID))
  //! acks PID error
#define Host_ack_pid_error(p)                  (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_PID))
#define Is_host_pid_error(p)                   (Tst_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_PID))
  //! acks time-out error
#define Host_ack_timeout_error(p)              (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_TIMEOUT))
#define Is_host_timeout_error(p)               (Tst_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_TIMEOUT))
  //! acks CRC16 error
#define Host_ack_crc16_error(p)                (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_CRC16))
#define Is_host_crc16_error(p)                 (Tst_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_CRC16))
  //! clears the error counter
#define Host_clear_error_counter(p)            (Clr_bits(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_COUNTER_Msk))
#define Host_get_error_counter(p)              (Rd_bitfield(UOTGHS->UOTGHS_HSTPIPERR[p], UOTGHS_HSTPIPERR_COUNTER_Msk, UOTGHS_HSTPIPERR_COUNTER_Pos))

  //! enables overflow interrupt
#define Host_enable_overflow_interrupt(p)      (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_OVERFIES)
  //! disables overflow interrupt
#define Host_disable_overflow_interrupt(p)     (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_OVERFIEC)
  //! tests if overflow interrupt is enabled
#define Is_host_overflow_interrupt_enabled(p)  (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_OVERFIE))

  //! enables underflow interrupt
#define Host_enable_underflow_interrupt(p)     (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_TXSTPES)
  //! disables underflow interrupt
#define Host_disable_underflow_interrupt(p)    (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_TXSTPEC)
  //! tests if underflow interrupt is enabled
#define Is_host_underflow_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_TXSTPE))

  //! forces all banks full (OUT) or free (IN) interrupt
#define Host_force_bank_interrupt(p)           (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_NBUSYBKS)
  //! unforces all banks full (OUT) or free (IN) interrupt
#define Host_unforce_bank_interrupt(p)         (UOTGHS->UOTGHS_HSTPIPIFR[p] = UOTGHS_HSTPIPIFR_NBUSYBKS)
  //! enables all banks full (IN) or free (OUT) interrupt
#define Host_enable_bank_interrupt(p)          (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_NBUSYBKES)
  //! disables all banks full (IN) or free (OUT) interrupt
#define Host_disable_bank_interrupt(p)         (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_NBUSYBKEC)
  //! tests if all banks full (IN) or free (OUT) interrupt is enabled
#define Is_host_bank_interrupt_enabled(p)      (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_NBUSYBKE))

  //! enables SHORT PACKET received interrupt
#define Host_enable_short_packet_interrupt(p)  (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_SHORTPACKETIES)
  //! disables SHORT PACKET received interrupt
#define Host_disable_short_packet_interrupt(p) (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_SHORTPACKETIEC)
  //! tests if SHORT PACKET received interrupt is enabled
#define Is_host_short_packet_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_SHORTPACKETIE))

  //! enables STALL received interrupt
#define Host_enable_stall_interrupt(p)         (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_RXSTALLDES)
  //! disables STALL received interrupt
#define Host_disable_stall_interrupt(p)        (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_RXSTALLDEC)
  //! tests if STALL received interrupt is enabled
#define Is_host_stall_interrupt_enabled(p)     (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_RXSTALLDE))

  //! enables CRC ERROR ISO IN detected interrupt
#define Host_enable_crc_error_interrupt(p)     (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_RXSTALLDES)
  //! disables CRC ERROR ISO IN detected interrupt
#define Host_disable_crc_error_interrupt(p)    (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_RXSTALLDEC)
  //! tests if CRC ERROR ISO IN detected interrupt is enabled
#define Is_host_crc_error_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_RXSTALLDE))

  //! enables NAK received interrupt
#define Host_enable_nak_received_interrupt(p)  (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_NAKEDES)
  //! disables NAK received interrupt
#define Host_disable_nak_received_interrupt(p) (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_NAKEDEC)
  //! tests if NAK received interrupt is enabled
#define Is_host_nak_received_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_NAKEDE))

  //! enables pipe error interrupt
#define Host_enable_pipe_error_interrupt(p)    (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_PERRES)
  //! disables pipe error interrupt
#define Host_disable_pipe_error_interrupt(p)   (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_PERREC)
  //! tests if pipe error interrupt is enabled
#define Is_host_pipe_error_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_PERRE))

  //! enables SETUP pipe ready interrupt
#define Host_enable_setup_ready_interrupt()    (UOTGHS->UOTGHS_HSTPIPIER[P_CONTROL] = UOTGHS_HSTPIPIER_TXSTPES)
  //! disables SETUP pipe ready interrupt
#define Host_disable_setup_ready_interrupt()   (UOTGHS->UOTGHS_HSTPIPIDR[P_CONTROL] = UOTGHS_HSTPIPIDR_TXSTPEC)
  //! tests if SETUP pipe ready interrupt is enabled
#define Is_host_setup_ready_interrupt_enabled() (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[P_CONTROL], UOTGHS_HSTPIPIMR_TXSTPE))

  //! enables OUT pipe ready interrupt
#define Host_enable_out_ready_interrupt(p)     (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_TXOUTES)
  //! disables OUT pipe ready interrupt
#define Host_disable_out_ready_interrupt(p)    (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_TXOUTEC)
  //! tests if OUT pipe ready interrupt is enabled
#define Is_host_out_ready_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_TXOUTE))

  //! enables IN pipe reception interrupt
#define Host_enable_in_received_interrupt(p)   (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_RXINES)
  //! disables IN pipe reception interrupt
#define Host_disable_in_received_interrupt(p)  (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_RXINEC)
  //! tests if IN pipe reception interrupt is enabled
#define Is_host_in_received_interrupt_enabled(p) (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_RXINE))

  //! Get 64-, 32-, 16- or 8-bit access to FIFO data register of selected pipe.
  //! @param p      Pipe of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @return       Volatile 64-, 32-, 16- or 8-bit data pointer to FIFO data register
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
#define Host_get_pipe_fifo_access(p, scale) \
          (AVR32_USBB_FIFOX_DATA(p, scale))

  //! Reset known position inside FIFO data register of selected pipe.
  //! @param p      Pipe of which to reset known position
  //! @warning Always call this macro before any read/write macro/function
  //! when at FIFO beginning.
#define Host_reset_pipe_fifo_access(p) \
          (pep_fifo[(p)].u64ptr = Host_get_pipe_fifo_access(p, 64))

  //! Read 64-, 32-, 16- or 8-bit data from FIFO data register of selected pipe.
  //! @param p      Pipe of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @return       64-, 32-, 16- or 8-bit data read
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @note This macro assures that used HSB addresses are identical to the
  //! DPRAM internal pointer modulo 32 bits.
  //! @warning Always call Host_reset_pipe_fifo_access before this macro when
  //! at FIFO beginning.
  //! @warning Do not mix calls to this macro with calls to indexed macros below.
#define Host_read_pipe_data(p, scale) \
          (*pep_fifo[(p)].TPASTE3(u, scale, ptr)\
           TPASTE3(Pep_fifo_access_, scale, _post_inc)())

  //! Write 64-, 32-, 16- or 8-bit data to FIFO data register of selected pipe.
  //! @param p      Pipe of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param data   64-, 32-, 16- or 8-bit data to write
  //! @return       64-, 32-, 16- or 8-bit data written
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @note This macro assures that used HSB addresses are identical to the
  //! DPRAM internal pointer modulo 32 bits.
  //! @warning Always call Host_reset_pipe_fifo_access before this macro when
  //! at FIFO beginning.
  //! @warning Do not mix calls to this macro with calls to indexed macros below.
#define Host_write_pipe_data(p, scale, data) \
          (*pep_fifo[(p)].TPASTE3(u, scale, ptr)\
           TPASTE3(Pep_fifo_access_, scale, _post_inc)() = (data))

  //! Read 64-, 32-, 16- or 8-bit indexed data from FIFO data register of selected pipe.
  //! @param p      Pipe of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param index  Index of scaled data array to access
  //! @return       64-, 32-, 16- or 8-bit data read
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
  //! @warning Do not mix calls to this macro with calls to non-indexed macros above.
#define Host_read_pipe_indexed_data(p, scale, index) \
          (AVR32_USBB_FIFOX_DATA(p, scale)[(index)])

  //! Write 64-, 32-, 16- or 8-bit indexed data to FIFO data register of selected pipe.
  //! @param p      Pipe of which to access FIFO data register
  //! @param scale  Data scale in bits: 64, 32, 16 or 8
  //! @param index  Index of scaled data array to access
  //! @param data   64-, 32-, 16- or 8-bit data to write
  //! @return       64-, 32-, 16- or 8-bit data written
  //! @warning It is up to the user of this macro to make sure that all accesses
  //! are aligned with their natural boundaries except 64-bit accesses which
  //! require only 32-bit alignment.
  //! @warning It is up to the user of this macro to make sure that used HSB
  //! addresses are identical to the DPRAM internal pointer modulo 32 bits.
  //! @warning Do not mix calls to this macro with calls to non-indexed macros above.
#define Host_write_pipe_indexed_data(p, scale, index, data) \
          (AVR32_USBB_FIFOX_DATA(p, scale)[(index)] = (data))
//! @}


//! @defgroup USBB_general_pipe_dma USBB pipe DMA drivers
//! These macros manage the common features of the pipe DMA channels.
//! @{
  //! enables the disabling of HDMA requests by pipe interrupts
#define Host_enable_pipe_int_dis_hdma_req(p)      (UOTGHS->UOTGHS_HSTPIPIER[p] = UOTGHS_HSTPIPIER_PDISHDMAS)
  //! disables the disabling of HDMA requests by pipe interrupts
#define Host_disable_pipe_int_dis_hdma_req(p)     (UOTGHS->UOTGHS_HSTPIPIDR[p] = UOTGHS_HSTPIPIDR_PDISHDMAC)
  //! tests if the disabling of HDMA requests by pipe interrupts si enabled
#define Is_host_pipe_int_dis_hdma_req_enabled(p)  (Tst_bits(UOTGHS->UOTGHS_HSTPIPIMR[p], UOTGHS_HSTPIPIMR_PDISHDMA))

  //! raises the selected pipe DMA channel interrupt
#define Host_raise_pipe_dma_interrupt(pdma)       (UOTGHS->UOTGHS_HSTIFR = UOTGHS_HSTIFR_DMA_1 << ((pdma) - 1))
  //! tests if an interrupt is triggered by the selected pipe DMA channel
#define Is_host_pipe_dma_interrupt(pdma)          (Tst_bits(UOTGHS->UOTGHS_HSTISR, UOTGHS_HSTISR_DMA_1 << ((pdma) - 1)))
  //! enables the selected pipe DMA channel interrupt
#define Host_enable_pipe_dma_interrupt(pdma)      (UOTGHS->UOTGHS_HSTIER = UOTGHS_HSTIER_DMA_1 << ((pdma) - 1))
  //! disables the selected pipe DMA channel interrupt
#define Host_disable_pipe_dma_interrupt(pdma)     (UOTGHS->UOTGHS_HSTIDR = UOTGHS_HSTIDR_DMA_1 << ((pdma) - 1))
  //! tests if the selected pipe DMA channel interrupt is enabled
#define Is_host_pipe_dma_interrupt_enabled(pdma)  (Tst_bits(UOTGHS->UOTGHS_HSTIMR, UOTGHS_HSTIMR_DMA_1 << ((pdma) - 1)))
//! @todo Implement macros for pipe DMA registers and descriptors
//! @}

//! @}


//_____ D E C L A R A T I O N S ____________________________________________

extern  UnionVPtr       pep_fifo[MAX_PEP_NB];

#if USB_HOST_FEATURE == ENABLED
extern  void            host_disable_all_pipes  (                             void   );
extern  U32             host_set_p_txpacket     (U8,         U8  , U32               );
extern  U32             host_write_p_txpacket   (U8, const void *, U32, const void **);
extern  U32             host_read_p_rxpacket    (U8,       void *, U32,       void **);
#endif

extern void Host_configure_address( uint8_t pipe, uint8_t addr);
extern void Uotghs_trace(void);

#endif  // _USB_DRV_H_
