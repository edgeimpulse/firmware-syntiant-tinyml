/*
 * Copyright (c) 2021 Syntiant Corp.  All rights reserved.
 * Contact at http://www.syntiant.com
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file LICENSE in the main
 * directory of this source tree, or the OpenIB.org BSD license below.  Any
 * code involving Linux software will require selection of the GNU General
 * Public License (GPL) Version 2.
 *
 * OPENIB.ORG BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef SYNTIANT_NDP10X_MICRO_H
#define SYNTIANT_NDP10X_MICRO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file syntiant_ndp10x_micro.h
 * @brief Interface to Syntiant NDP10x minimal chip interface library
 */

/** @brief version string */
extern char *syntiant_ndp10x_micro_version;

/*
 * Integration Interfaces
 */

/**
 * @brief handle/pointer to integration-specific device instance state
 *
 * An @c syntiant_ndp10x_micro_handle_t typically points to a structure
 * containing all information required to access and manipulate an NDP
 * device instance in a particular environment.  For example, a Linux user
 * mode integration might require a Linux file descriptor to access the
 * hardware, or a 'flat, embedded' integration might require a hardware
 * memory address.
 *
 * It is not used directly by the NDP ILib, but rather is passed unchanged to
 * integration interface functions.
 */
typedef void *syntiant_ndp10x_micro_handle_t;

/**
 * @brief exchange bytes with NDP device integration interface
 *
 * A provider of @c syntiant_ndp10x_micro_transfer will send @p count bytes
 * to the device and retrieve @p count bytes.
 *
 * This interface supports an underlying underlying SPI bus connection
 * to NDP where equal numbers of bytes are sent and received for cases
 * where full duplex transfer is relevant.  Simplex transfer is accomplished
 * by setting either @p out or @p in to NULL as appropriate.
 *
 * The interface provider may (should) support burst style efficiency
 * optimizations even across transfers where appropriate.  For example,
 * a multidrop SPI bus can continue to leave the interface selected
 * if subsequent transfers pick up where the previous left off and the
 * NDP device has not been deselected for other reasons.
 *
 * Transfers to the MCU space are generally expected to be simplex and
 * out == NULL -> read operation, in == NULL -> write operation.
 *
 * @param d handle to the NDP device
 * @param mcu target MCU space if true (SPI bus space if false)
 * @param addr starting address
 * @param out bytes to send, or NULL if '0' bytes should be sent
 * @param in bytes to receive, or NULL if received bytes should be ignored
 * @param count number of bytes to exchange
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
typedef int (*syntiant_ndp10x_micro_transfer_f)
(syntiant_ndp10x_micro_handle_t d, int mcu, uint32_t addr, void *out,
 void *in, unsigned int count);

/**
 * @brief device state structure
 *
 * The chip user must zero this structure and fill the ->d and ->transfer
 * members before performing any ILib calls.
 *
 */
struct syntiant_ndp10x_micro_device_s {
    /* State that must be initialized by the chip user before any calls */
    syntiant_ndp10x_micro_handle_t d; /**< device handle */
    syntiant_ndp10x_micro_transfer_f transfer; /**< data transfer function */
    /* implementation-internal state (look but don't touch) */
    uint32_t fw_state_addr;          /**< firmware state address in NDP ram */
    uint32_t mbout;                  /**< previous mbout */
    uint32_t match_ring_size;        /**< firmware match ring size */
    uint32_t match_producer;         /**< firmware match ring producer */
    uint32_t match_consumer;         /**< host match ring consumer */
    uint32_t tankptr_last;           /**< holding tank last read offset */
    uint32_t tankptr_match;          /**< holding tank match read offset */
    uint32_t log_state;              /**< log processing state */
    uint32_t log_len;                /**< log TLV length */
    uint32_t log_tag_or_addr;        /**< log TLV tag or address */
    uint32_t log_readaddr;           /**< log read MCU address */
};

/*
 * Public Interfaces
 */

/**
 * @brief transfer bytes bytes to or from an NDP address
 *
 * Read or write data from an NDP device register or MCU address space.
 * Only one of @c out and @c in must be non-NULL.  This function is primarily
 * used for diagnostic purposes. It performs a call to the
 * syntiant_ndp10x_micro_transfer_f integration interface.
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 # @param mcu read from MCU address space (SPI otherwise)
 * @param address the address for the transfer
 * @param out pointer to the value to write
 * @param in pointer to the value to read
 * @param count the number of bytes to transfer
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
extern int syntiant_ndp10x_micro_transfer
(struct syntiant_ndp10x_micro_device_s *ndp, int mcu, uint32_t address,
 void *out, void *in, unsigned int count);

/**
 * @brief apply a initialization transaction log to the NDP chip
 *
 * The ndp10x micro ILib reduces all initialization activities down
 * to performing the chip configuration activities (predominantly register
 * writes) captured in a recorded log of an initialization sequence
 * performed by the normal (full-sized) ILib.  The initialization log
 * description is structured as TLV (tag, length, value)-structured
 * byte stream where all TLVs are padded to a multiple of 4-bytes in length.
 *
 * This function applies an initialization log to an NDP device allow
 * bit-by-bit loading for very memory constrained systems.
 *
 * It can be used for a big 'initialization' and for small configuration
 * adjustments (e.g. enable/disable match-per-frame)
 *
 * The loading process includes:
 *
 * 1. call @c _load_log() with len = 0 to initialize loading state and
 *    @c _MORE indicates success
 * 2. call @c _load_log() with a chunk of the load log, note the length
 *    MUST be 4-byte aligned.  @c _MORE indicates success, but more
 *    log data is expected.  @c _NONE indicates success and no more
 *    log data is expected.  Other return status codes indicate an error
 *    loading the log.
 * 3. repeat 2. until the return code is != @c _MORE
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 * @param log a chunk of NDP initialization log buffer
 * @param len the length of the
 * @return a @c SYNTIANT_NDP_ERROR_* code.
 */
extern int syntiant_ndp10x_micro_load_log
(struct syntiant_ndp10x_micro_device_s *ndp, void *log, int len);

/**
 * @brief apply a initialization transaction log to the NDP chip and report
 *        an MCU read address for subsequent use
 *
 * This function operates identically to @c syntiant_ndp10x_micro_load_log_read
 * except that it also returns an MCU address that is the starting address
 * of the last read operation performed when creating the log recording.
 *
 * This function does not perform this MCU read of its own accord.  The
 * address is returned to allow the chip user to subsequently read the
 * address with @c syntiant_ndp10x_micro_transfer calls whenver required.
 * For example, sensor data read through the NDP serial access facilitiy
 * can be retrieved by performing the access with
 * @c syntiant_ndp10x_micro_load_log_read and then retrieving the
 * status and data with @c syntiant_ndp10x_micro_transfer.
 *
 * The chip user must be familiar with the internals of the ILib used
 * to create the log file which may change somewhat depending on the
 * version to make use of the read address.
 *
 * @param ndp NDP state object
 * @param log a chunk of NDP initialization log buffer
 * @param len the length of the
 * @param readaddr the address of the final MCU read performed in the log
 *      sequence.  @c readaddr will only be set when the return status is
 *      @c SYNTIANT_NDP_ERROR_NONE, i.e. on the final chunk of a log.
  * @return a @c SYNTIANT_NDP_ERROR_* code.
 */
extern int syntiant_ndp10x_micro_load_log_read
(struct syntiant_ndp10x_micro_device_s *ndp, void *log, int len,
 uint32_t *readaddr);

/**
 * @brief notification causes
 */
enum syntiant_ndp10x_micro_notification_e {
    SYNTIANT_NDP10X_MICRO_NOTIFICATION_DNN = 0x01,
                                /**< DNN frame completion reported */
    SYNTIANT_NDP10X_MICRO_NOTIFICATION_MATCH = 0x02,
                                /**< algorithm match reported */
    SYNTIANT_NDP10X_MICRO_NOTIFICATION_ALL_M
    = ((SYNTIANT_NDP10X_MICRO_NOTIFICATION_MATCH << 1) - 1)
};

/**
 * @brief poll and check NDP interrupts
 *
 * Call @c syntiant_ndp10x_micro_poll either periodically or when an interrupt
 * has been reported.  @c syntiant_ndp_poll will progress any outstanding action
 * or communication with the NDP device as well as decoding and reporting
 * notification information.  Set @c clear to clear the notifications.
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 * @param notifications ored @c SYNTIANT_NDP_NOTIFICATION_*s
 # @param clear clear notifications if true
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
extern int syntiant_ndp10x_micro_poll
(struct syntiant_ndp10x_micro_device_s *ndp, uint32_t *causes, int clear);

/**
 * @brief send input to the neural network
 */
enum syntiant_ndp10x_micro_send_data_type_e {
    SYNTIANT_NDP10X_MICRO_SEND_DATA_TYPE_STREAMING       = 0x00,
    /**< streaming vector input features */
    SYNTIANT_NDP10X_MICRO_SEND_DATA_TYPE_FEATURE_STATIC  = 0x01,
    /**< send to static features */
    SYNTIANT_NDP10X_MICRO_SEND_DATA_TYPE_LAST
    = SYNTIANT_NDP10X_MICRO_SEND_DATA_TYPE_FEATURE_STATIC
};

/**
 * @brief send input data to the neural network
 *
 * The format of the data depends on the configuration of the NDP device
 * datapath which is controlled by the contents of the neural network parameters
 * package.  For example, the data might be a stream of bytes, or a stream
 * of 12-bit quantities stored zero-padded in pairs of bytes.
 *
 * Whether an NDP device is configured to process data input from the
 * processor or other inputs is determined by the initialization of the device.
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 * @param data array of data values
 * @param len @p data length
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
extern int syntiant_ndp10x_micro_send_data
(struct syntiant_ndp10x_micro_device_s *ndp, uint8_t *data, unsigned int len,
 int type, uint32_t address);

/**
 * @brief data extraction ranges
 */
enum syntiant_ndp10x_micro_extract_from_e {
    SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_MATCH = 0x00,
    /**< from 'length' older than a match.  The matching extraction position
       is only updated when @c syntiant_ndp10x_micro_get_match() is called
       and reports a match has been detected */
    SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_UNREAD = 0x01,
    /**< from the oldest unread */
    SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_OLDEST = 0x02,
    /**< from the oldest recorded */
    SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_NEWEST = 0x03,
    /**< from the newest recorded */
    SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_LAST
    = SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_NEWEST
};

/**
 * @brief extract stored data from the NDP device
 *
 * The data is typically 16-bit PCM audio, but may differ depending on
 * device configuration.
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 * @param from point at which to start extracting
 * @param data array of data values.  NULL will 'discard' @len data
 *        if @c from = @c _UNREAD and @c _OLDEST and set the next
 *        pointer start of the requested data if @c from = @c _MATCH
 * @param len @p supplies length of the data array and returns the amount
 *        of data that would have been extracted if len were unconstrained
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
extern int syntiant_ndp10x_micro_extract_data
(struct syntiant_ndp10x_micro_device_s *ndp, int from, uint8_t *data,
 unsigned int *len);

/**
 * @brief retrieve NDP match summary information
 *
 * The only recovery from an error is to reinitialize the NDP by reinitializing
 * the @c syntiant_ndp10x_micro_device_s structure and running a 'fresh'
 * @c syntiant_ndp10x_micro_load_log sequence(s).
 *
 * @param ndp NDP state object
 * @param match matched class, or < 0 if no match
 * @return a @c SYNTIANT_NDP_ERROR_* code
 */
extern int syntiant_ndp10x_micro_get_match
(struct syntiant_ndp10x_micro_device_s *ndp, int *match);

#ifdef __cplusplus
}
#endif

#endif
