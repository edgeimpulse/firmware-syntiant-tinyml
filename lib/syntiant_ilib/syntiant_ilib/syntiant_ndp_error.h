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
#ifndef SYNTIANT_NDP_ERROR_H
#define SYNTIANT_NDP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief error codes
 */
enum syntiant_ndp_errors_e {
    SYNTIANT_NDP_ERROR_NONE = 0,    /**< operation successful */
    SYNTIANT_NDP_ERROR_FAIL = 1,    /**< general failure */
    SYNTIANT_NDP_ERROR_ARG = 2,     /**< invalid argument error */
    SYNTIANT_NDP_ERROR_UNINIT = 3,  /**< device unintialized or no fw loaded */
    SYNTIANT_NDP_ERROR_PACKAGE = 4, /**< package format error */
    SYNTIANT_NDP_ERROR_UNSUP = 5,   /**< operation not supported */
    SYNTIANT_NDP_ERROR_NOMEM = 6,   /**< out of memory */
    SYNTIANT_NDP_ERROR_BUSY = 7,    /**< operation in progress */
    SYNTIANT_NDP_ERROR_TIMEOUT = 8, /**< operation timeout */
    SYNTIANT_NDP_ERROR_MORE = 9,    /**< more data is expected */ 
    SYNTIANT_NDP_ERROR_CONFIG = 10, /**< config error */
    SYNTIANT_NDP_ERROR_CRC = 11,    /**< CRC mismatch */
    SYNTIANT_NDP_ERROR_INVALID_NETWORK = 12, /**< invalid network id */
    SYNTIANT_NDP_ERROR_DATA_REREAD = 13, /**<data has already been read before */
    SYNTIANT_NDP_ERROR_LAST = SYNTIANT_NDP_ERROR_DATA_REREAD
};

#define SYNTIANT_NDP_ERROR_NAMES                                               \
    {                                                                          \
        "none", "fail", "arg", "uninit", "package", "unsup", "nomem", "busy",  \
            "timeout", "more", "config", "crc", "inv_net", "reread"            \
    }
    
#define SYNTIANT_NDP_ERROR_NAME(e)                                             \
    (((e) < SYNTIANT_NDP_ERROR_NONE || SYNTIANT_NDP_ERROR_LAST < (e))          \
            ? "*unknown*"                                                      \
            : syntiant_ndp_error_names[e])
     
extern char *syntiant_ndp_error_names[];


/**
 * @brief return a string naming the error code
 *
 * @param e the error code
 * @return a pointer to a string naming the error
 */
extern const char *syntiant_ndp_error_name(int e);

#ifdef __cplusplus
}
#endif

#endif
