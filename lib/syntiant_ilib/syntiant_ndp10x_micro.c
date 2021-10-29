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

#include <syntiant_ilib/syntiant_portability.h>
#include <syntiant_ilib/syntiant_ndp_error.h>
#include <syntiant_ilib/syntiant_ndp_ilib_version.h>
#include <syntiant_ilib/syntiant_ndp10x_micro.h>

/*
 * chip hardware register definitions
 */
#define NDP10X_SPI_INTSTS 0x02U
#define NDP10X_SPI_INTSTS_MBIN_INT_SHIFT 1
#define NDP10X_SPI_INTSTS_MBIN_INT(v) \
        ((v) << NDP10X_SPI_INTSTS_MBIN_INT_SHIFT)
#define NDP10X_SPI_INTCTL 0x03U
#define NDP10X_SPI_CTL 0x04U
#define NDP10X_SPI_CTL_RESETN_SHIFT 0
#define NDP10X_SPI_CTL_RESETN_MASK 0x01U
#define NDP10X_SPI_CTL_RESETN_MASK_INSERT(x, v) \
        (((x) & ~NDP10X_SPI_CTL_RESETN_MASK) | \
         ((v) << NDP10X_SPI_CTL_RESETN_SHIFT))
#define NDP10X_SPI_CTL_EXTCLK_SHIFT 2
#define NDP10X_SPI_CTL_EXTCLK_MASK 0x04U
#define NDP10X_SPI_CTL_EXTCLK_MASK_INSERT(x, v) \
        (((x) & ~NDP10X_SPI_CTL_EXTCLK_MASK) \
         | ((v) << NDP10X_SPI_CTL_EXTCLK_SHIFT))
#define NDP10X_SPI_MATCH_WINNER_SHIFT 0
#define NDP10X_SPI_MATCH_WINNER_MASK 0x3fU
#define NDP10X_SPI_MATCH_WINNER_EXTRACT(x) \
        (((x) & NDP10X_SPI_MATCH_WINNER_MASK) >> NDP10X_SPI_MATCH_WINNER_SHIFT)
#define NDP10X_SPI_MATCH_MATCH_MASK 0x40U
#define NDP10X_SPI_MATCH_MULT_MASK 0x80U
#define NDP10X_SPI_SAMPLE 0x20U
#define NDP10X_SPI_MBIN 0x30U
#define NDP10X_SPI_MBIN_RESP 0x31U
#define NDP10X_SPI_MADDR(i) (0x40U + ((i) << 0))

#define NDP10X_BOOTROM 0x01000000U
#define NDP10X_BOOTRAM_REMAP 0x1fffc000U
#define NDP10X_RAM 0x20000000U
#define NDP10X_RAM_SIZE 0x00018000U
#define NDP10X_CHIP_CONFIG_FLLSTS0 0x40009068U
#define NDP10X_CHIP_CONFIG_FLLSTS0_MODE_SHIFT 0
#define NDP10X_CHIP_CONFIG_FLLSTS0_MODE_MASK 0x00000007U
#define NDP10X_CHIP_CONFIG_FLLSTS0_MODE_EXTRACT(x) \
        (((x) & NDP10X_CHIP_CONFIG_FLLSTS0_MODE_MASK) \
         >> NDP10X_CHIP_CONFIG_FLLSTS0_MODE_SHIFT)
#define NDP10X_CHIP_CONFIG_FLLSTS0_MODE_LOCKED 0x5U
#define NDP10X_DSP_CONFIG_TANK 0x4000c0a8U
#define NDP10X_DSP_CONFIG_TANK_SIZE_SHIFT 4
#define NDP10X_DSP_CONFIG_TANK_SIZE_MASK 0x001ffff0U
#define NDP10X_DSP_CONFIG_TANK_SIZE_EXTRACT(x) \
        (((x) & NDP10X_DSP_CONFIG_TANK_SIZE_MASK) \
         >> NDP10X_DSP_CONFIG_TANK_SIZE_SHIFT)
#define NDP10X_DSP_CONFIG_TANKADDR 0x4000c0b0U
#define NDP10X_DNNSTATICFEATURE 0x60061000U
#define NDP10X_DNNSTATICFEATURE_SIZE 0x00001000U

#define MAGIC_VALUE 0x53bde5a1U

#define TAG_HEADER 1U
#define TAG_CHECKSUM 4U
#define TAG_UILIB_EXT_CLK 28U
#define TAG_UILIB_INT_CLK 29U
#define TAG_UILIB_SPI_WRITE 30U
#define TAG_UILIB_MCU_WRITE 31U
#define TAG_UILIB_MB_NOP 74U
#define TAG_UILIB_MCU_READ 75U
#define SYNTIANT_NDP10X_MICRO_MAX_TRANSFER 2048
#define NDP10X_MB_HOST_TO_MCU_OWNER 0x08U
#define NDP10X_MB_HOST_TO_MCU_M 0x07U
#define NDP10X_MB_MCU_TO_HOST_OWNER 0x80U
#define NDP10X_MB_MCU_TO_HOST_M 0x70U
#define NDP10X_MB_MCU_TO_HOST_S 4U
#define NDP10X_MB_REQUEST_NOP 0x0U
#define NDP10X_MB_RESPONSE_SUCCESS 0x0U
#define NDP10X_FW_STATE_POINTERS_FW_STATE 0x1fffc0c0U
#define NDP10X_FW_STATE_TANKPTR_OFFSET 0
#define NDP10X_FW_STATE_MATCH_RING_SIZE_OFFSET 4
#define NDP10X_FW_STATE_MATCH_PRODUCER_OFFSET 8
#define NDP10X_FW_STATE_MATCH_RING_OFFSET 12
#define NDP10X_FW_STATE_MATCH_RING_SUMMARY_OFFSET 0
#define NDP10X_FW_STATE_MATCH_RING_TANKPTR_OFFSET 1
#define NDP10X_FW_STATE_MATCH_RING_ENTRY_SIZE \
    (NDP10X_FW_STATE_MATCH_RING_TANKPTR_OFFSET + 1)

#define SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE 0
#define SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_TAG 1
#define SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_LENGTH 2
#define SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_MCU 3
#define SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_SPI 4

#define SYNTIANT_NDP10X_MICRO_NOP_TIMEOUT 1000000

char *syntiant_ndp10x_micro_version = SYNTIANT_NDP_ILIB_VERSION;

int
syntiant_ndp10x_micro_transfer(struct syntiant_ndp10x_micro_device_s *ndp,
                               int mcu, uint32_t address, void *out,
                               void *in, unsigned int count)
{
    int s;
    unsigned int max_burst;
    unsigned int burst;

#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
    if (out && in) {
        return SYNTIANT_NDP_ERROR_ARG;
    }

    if (mcu && (count % 4 || address % 4)) {
        return SYNTIANT_NDP_ERROR_ARG;
    }
#endif

    while (count) {
        max_burst =
            SYNTIANT_NDP10X_MICRO_MAX_TRANSFER
            - (address % SYNTIANT_NDP10X_MICRO_MAX_TRANSFER);

        burst = (count > max_burst) ? max_burst : count;

        s = (ndp->transfer)(ndp->d, mcu, address, out, in, burst);
        if (s) {
            return s;
        }

        if (in || mcu || address != NDP10X_SPI_SAMPLE) {
            address += (unsigned int) burst;
        }
        out = ((uint8_t*) out) + (out ? burst : 0);
        in = ((uint8_t*) in) + (out ? 0 : burst);
        count -= burst;
    }

    return SYNTIANT_NDP_ERROR_NONE;
}

int syntiant_ndp10x_micro_ext_clk_slow_read
(struct syntiant_ndp10x_micro_device_s *ndp, uint32_t addr, uint32_t *vp);
int
syntiant_ndp10x_micro_ext_clk_slow_read
(struct syntiant_ndp10x_micro_device_s *ndp, uint32_t addr, uint32_t *v)
{
    uint32_t b[2];
    int s;

    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MADDR(0), &addr,
                                       NULL, 4);
    if (s)
        goto out;
    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MADDR(0), NULL, b,
                                       sizeof(b));
    *v = b[1];

 out:
    return s;

}
int syntiant_ndp10x_micro_ext_clk(struct syntiant_ndp10x_micro_device_s *ndp);
int
syntiant_ndp10x_micro_ext_clk(struct syntiant_ndp10x_micro_device_s *ndp)
{
    uint32_t v, v0;
    uint8_t ctl, ctls;
    int i, s;

    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_CTL, NULL, &ctl,
                                       sizeof(ctl));
    if (s) {
        return s;
    }

    ctls = NDP10X_SPI_CTL_EXTCLK_MASK_INSERT(ctl, 1);

    s = syntiant_ndp10x_micro_ext_clk_slow_read(ndp, NDP10X_BOOTROM, &v);
    if (s) {
        return s;
    }

    for (i = 0; i < 128; i++) {
        s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_CTL, &ctls, NULL,
                                           sizeof(ctls));
        if (s) {
            return s;
        }

        s = syntiant_ndp10x_micro_ext_clk_slow_read
            (ndp, NDP10X_BOOTROM + 4, &v0);
        if (s) {
            return s;
        }

        if (v != v0) {
            s = syntiant_ndp10x_micro_ext_clk_slow_read
                (ndp, NDP10X_BOOTROM, &v0);
            if (s) {
                return s;
            }
            if (v == v0) {
                return SYNTIANT_NDP_ERROR_NONE;
            }
        }

        s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_CTL, &ctl, NULL,
                                           sizeof(ctl));
        if (s) {
            return s;
        }
    }

    return SYNTIANT_NDP_ERROR_TIMEOUT;
}

int syntiant_ndp10x_micro_int_clk(struct syntiant_ndp10x_micro_device_s *ndp);
int
syntiant_ndp10x_micro_int_clk(struct syntiant_ndp10x_micro_device_s *ndp)
{
    int i, s;
    uint32_t fllsts0;
    int mode;

    for (i = 0; i < 2500; i++) {
        s = syntiant_ndp10x_micro_transfer(ndp, 1, NDP10X_CHIP_CONFIG_FLLSTS0,
                                           NULL, &fllsts0, sizeof(fllsts0));
        if (s) {
            return s;
        }

        mode = NDP10X_CHIP_CONFIG_FLLSTS0_MODE_EXTRACT(fllsts0);

        if (mode == NDP10X_CHIP_CONFIG_FLLSTS0_MODE_LOCKED) {
            return SYNTIANT_NDP_ERROR_NONE;
        }
        /* TODO: check if mode == TRACKINITIAL? */
    }

    return SYNTIANT_NDP_ERROR_TIMEOUT;
}

int syntiant_ndp10x_micro_mb_nop(struct syntiant_ndp10x_micro_device_s *ndp);
int
syntiant_ndp10x_micro_mb_nop(struct syntiant_ndp10x_micro_device_s *ndp)
{
    int i = 0;
    int s, s0;
    uint8_t v, intctl, mbin, mbout;
    unsigned int own;

    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_INTCTL, NULL,
                                       &intctl, 1);
    if (s) {
        return s;
    }
    v = 0;
    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_INTCTL, &v,
                                       NULL, 1);
    if (s) {
        return s;
    }
    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MBIN, NULL,
                                       &mbin, 1);
    if (s) {
        goto out;
    }
    mbin = (uint8_t)
            (((mbin & ~NDP10X_MB_HOST_TO_MCU_M) | NDP10X_MB_REQUEST_NOP)
             ^ NDP10X_MB_HOST_TO_MCU_OWNER);
    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MBIN, &mbin,
                                       NULL, 1);
    if (s) {
        goto out;
    }

    own = mbin & NDP10X_MB_HOST_TO_MCU_OWNER;
    for (i = 0; i < SYNTIANT_NDP10X_MICRO_NOP_TIMEOUT; i++) {
        s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MBIN_RESP, NULL,
                                           &mbout, 1);
        if (s) {
            goto out;
        }
        if ((mbout & NDP10X_MB_HOST_TO_MCU_OWNER) == own) {
            if ((mbout & NDP10X_MB_HOST_TO_MCU_M)
                != NDP10X_MB_RESPONSE_SUCCESS) {
                s = SYNTIANT_NDP_ERROR_FAIL;
            }
            ndp->mbout =
                (ndp->mbout
                 & (NDP10X_MB_MCU_TO_HOST_OWNER | NDP10X_MB_MCU_TO_HOST_M))
                 | (mbout & (NDP10X_MB_HOST_TO_MCU_OWNER
                            | NDP10X_MB_HOST_TO_MCU_M));
            goto out;
        }
    }
    s = SYNTIANT_NDP_ERROR_TIMEOUT;

out:
    s0 = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_INTCTL, &intctl,
                                       NULL, 1);
    s = s ? s : s0;

    return s;
}

int
syntiant_ndp10x_micro_load_log_read(struct syntiant_ndp10x_micro_device_s *ndp,
                                    void *log, int len, uint32_t *readaddr)
{
    int s;
    unsigned int st, fr;
    int mcu;
    uint32_t v, llen, wlen, toa;

#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
    if (len % 4) {
        return SYNTIANT_NDP_ERROR_ARG;
    }
#endif

    if (!len) {
        ndp->log_state = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
        ndp->log_readaddr = 0;
        return SYNTIANT_NDP_ERROR_MORE;
    }

    st = ndp->log_state;
    toa = ndp->log_tag_or_addr;
    llen = ndp->log_len;
    while (len) {
        switch (st) {
        case SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE:
            st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_TAG;
            toa = *(uint32_t *) log;
            len -= (int) sizeof(uint32_t);
            log = ((uint32_t *) log) + 1;
            continue;

        case SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_TAG:
            st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_LENGTH;
            v = *(uint32_t *) log;
            switch (toa) {
            case TAG_HEADER:
            case TAG_CHECKSUM:
            case TAG_UILIB_MCU_READ:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v != 4) {
                    /* junk following the HEADER or END */
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                break;
            case TAG_UILIB_EXT_CLK:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v) {
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                s = syntiant_ndp10x_micro_ext_clk(ndp);
                if (s) {
                    return s;
                }
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
                break;
            case TAG_UILIB_INT_CLK:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v) {
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                s = syntiant_ndp10x_micro_int_clk(ndp);
                if (s) {
                    return s;
                }
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
                break;
            case TAG_UILIB_MB_NOP:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v) {
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                s = syntiant_ndp10x_micro_mb_nop(ndp);
                if (s) {
                    return s;
                }
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
                break;
            case TAG_UILIB_SPI_WRITE:
                break;
            case TAG_UILIB_MCU_WRITE:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v % 4) {
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                break;
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
            default:
                return SYNTIANT_NDP_ERROR_PACKAGE;
#endif
            }
            len -= (int) sizeof(uint32_t);
            log = ((uint32_t *) log) + 1;
            llen = v;
            continue;

        case SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_LENGTH:
            v = *(uint32_t *) log;
            switch (toa) {
            case TAG_HEADER:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
                if (v != MAGIC_VALUE) {
                    return SYNTIANT_NDP_ERROR_PACKAGE;
                }
#endif
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
                break;
            case TAG_CHECKSUM:
                *readaddr = ndp->log_readaddr;
                return SYNTIANT_NDP_ERROR_NONE;
            case TAG_UILIB_SPI_WRITE:
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_SPI;
                break;
            case TAG_UILIB_MCU_WRITE:
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_MCU;
                break;
            case TAG_UILIB_MCU_READ:
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
                ndp->log_readaddr = v;
                break;
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
            default:
                return SYNTIANT_NDP_ERROR_PACKAGE;
#endif
            }
            len -= (int) sizeof(uint32_t);
            llen -= (int) sizeof(uint32_t);
            log = ((uint32_t *) log) + 1;
            toa = v;
            continue;

        default:
            mcu = st == SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_MCU;
            wlen = ((unsigned int) len) < llen ? ((unsigned int) len) : llen;
            s = syntiant_ndp10x_micro_transfer(ndp, mcu, toa, log, NULL, wlen);
            if (s) {
                return s;
            }
            llen -= wlen;
            if (!llen) {
                fr = wlen & (sizeof(uint32_t) - 1);
                if (fr) {
                    wlen += 4 - fr;
                }
                st = SYNTIANT_NDP10X_MICRO_LOAD_LOG_STATE_IDLE;
            }
            toa += wlen;
            len -= (int) wlen;
            log = ((uint8_t *) log) + wlen;
        }
    }
    ndp->log_state = st;
    ndp->log_tag_or_addr = toa;
    ndp->log_len = llen;

    return SYNTIANT_NDP_ERROR_MORE;
}

int
syntiant_ndp10x_micro_load_log(struct syntiant_ndp10x_micro_device_s *ndp,
                               void *log, int len)
{
    uint32_t v;

    return syntiant_ndp10x_micro_load_log_read(ndp, log, len, &v);
}

int syntiant_ndp10x_micro_read_fw_state(struct syntiant_ndp10x_micro_device_s
                                        *ndp);
int
syntiant_ndp10x_micro_read_fw_state(struct syntiant_ndp10x_micro_device_s *ndp)
{
    uint32_t addr, v;
    int s;

    if (!ndp->fw_state_addr) {
        addr = NDP10X_FW_STATE_POINTERS_FW_STATE;
        s = syntiant_ndp10x_micro_transfer(ndp, 1, addr, NULL, &v, sizeof(v));
        if (s) {
            return s;
        }
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
        if (v < NDP10X_BOOTRAM_REMAP || NDP10X_RAM + NDP10X_RAM_SIZE <= v) {
            return SYNTIANT_NDP_ERROR_UNINIT;
        }
#endif
        ndp->fw_state_addr = v;
    }

    return SYNTIANT_NDP_ERROR_NONE;
}

int
syntiant_ndp10x_micro_poll(struct syntiant_ndp10x_micro_device_s *ndp,
                           uint32_t *causes, int clear)
{
    int s;
    uint8_t intsts, mbout, mbin;
    uint32_t addr;
    uint32_t v[2];
    uint32_t c = 0;

    s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_INTSTS, NULL, &intsts,
                                       1);
    if (s) {
        return s;
    }
    if (clear) {
        s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_INTSTS, &intsts,
                                           NULL, 1);
        if (s) {
            return s;
        }
    }

    if (intsts & NDP10X_SPI_INTSTS_MBIN_INT(1)) {
        s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MBIN_RESP, NULL,
                                           &mbout, 1);
        if (s) {
            return s;
        }
        if ((mbout ^ ndp->mbout) & NDP10X_MB_MCU_TO_HOST_OWNER) {
            c = SYNTIANT_NDP10X_MICRO_NOTIFICATION_DNN;
            s = syntiant_ndp10x_micro_read_fw_state(ndp);
            if (s) {
                return s;
            }
            addr = ndp->fw_state_addr + NDP10X_FW_STATE_MATCH_RING_SIZE_OFFSET;
            s = syntiant_ndp10x_micro_transfer(ndp, 1, addr, NULL, v,
                                                sizeof(v));
            if (s) {
                return s;
            }
            ndp->match_ring_size = v[(NDP10X_FW_STATE_MATCH_RING_SIZE_OFFSET
                                      - NDP10X_FW_STATE_MATCH_RING_SIZE_OFFSET)
                                     / sizeof(uint32_t)];
            ndp->match_producer = v[(NDP10X_FW_STATE_MATCH_PRODUCER_OFFSET
                                     - NDP10X_FW_STATE_MATCH_RING_SIZE_OFFSET)
                                    / sizeof(uint32_t)];
            mbin = (mbout & NDP10X_MB_MCU_TO_HOST_OWNER)
                | (NDP10X_MB_RESPONSE_SUCCESS << NDP10X_MB_MCU_TO_HOST_S);
            s = syntiant_ndp10x_micro_transfer(ndp, 0, NDP10X_SPI_MBIN, &mbin,
                                               NULL, 1);
            if (s) {
                return s;
            }
        }
        ndp->mbout = mbout;
    }

    if (ndp->match_producer != ndp->match_consumer) {
        c |= SYNTIANT_NDP10X_MICRO_NOTIFICATION_MATCH;
    }

    *causes = c;

    return SYNTIANT_NDP_ERROR_NONE;
}


int
syntiant_ndp10x_micro_send_data(struct syntiant_ndp10x_micro_device_s *ndp,
                                uint8_t *data, unsigned int len, int type,
                                uint32_t address)
{
    int s;
    int mcu = type == SYNTIANT_NDP10X_MICRO_SEND_DATA_TYPE_FEATURE_STATIC;
    uint32_t a;

#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
    if (mcu
        && (NDP10X_DNNSTATICFEATURE_SIZE < address + len || (len & 0x3))) {
        return SYNTIANT_NDP_ERROR_ARG;
    }
    if (!mcu && address) {
        return SYNTIANT_NDP_ERROR_ARG;
    }
#endif

    a = mcu ? NDP10X_DNNSTATICFEATURE + address : NDP10X_SPI_SAMPLE;
    s = syntiant_ndp10x_micro_transfer(ndp, mcu, a, data, NULL, len);

    return s;
}

enum tank_regs_e {
    TANK_REGS_TANK = 0,
    TANK_REGS_TANKADDR =
    (NDP10X_DSP_CONFIG_TANKADDR - NDP10X_DSP_CONFIG_TANK) / sizeof(uint32_t),
    TANK_REGS_WORDS = TANK_REGS_TANKADDR + 1
};

int
syntiant_ndp10x_micro_extract_data(struct syntiant_ndp10x_micro_device_s *ndp,
                                   int from, uint8_t *data, unsigned int *lenp)
{
    unsigned int len = (unsigned int) *lenp;
    uint32_t tank_regs[TANK_REGS_WORDS];
    uint32_t v, base, addr;
    unsigned int start = 0;
    uint32_t currptr, rcurrptr;
    unsigned int bufsize, used, rlen, offset;
    int s;

#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
    if (from < 0 || SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_LAST < from
        || len % 4) {
        return SYNTIANT_NDP_ERROR_ARG;
    }
#endif

    s = syntiant_ndp10x_micro_transfer(ndp, 1, NDP10X_DSP_CONFIG_TANK,
                                       NULL, tank_regs, sizeof(tank_regs));
    if (s) {
        return s;
    }

    base = tank_regs[TANK_REGS_TANKADDR];
    v = tank_regs[TANK_REGS_TANK];
    bufsize = NDP10X_DSP_CONFIG_TANK_SIZE_EXTRACT(v);

    s = syntiant_ndp10x_micro_read_fw_state(ndp);
    if (s) {
        return s;
    }

    addr = ndp->fw_state_addr + NDP10X_FW_STATE_TANKPTR_OFFSET;
    s = syntiant_ndp10x_micro_transfer(ndp, 1, addr, NULL, &currptr,
                                       sizeof(currptr));
    rcurrptr = currptr & ~0x3U;
    currptr = (currptr + 3U) & ~0x3U;

    switch (from) {
    case SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_MATCH:
        start = ndp->tankptr_match;
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
        if (start + (start < currptr ? bufsize : 0) < currptr + len) {
            return SYNTIANT_NDP_ERROR_ARG;
        }
#endif
        start = start + bufsize - len;
        break;
    case SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_UNREAD:
        start = ndp->tankptr_last;
        break;
    case SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_OLDEST:
        start = (currptr + 3U) & ~0x3U;
        break;
    case SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_NEWEST:
#ifdef SYNTIANT_NDP10X_MICRO_ARGS_CHECKS
        if (bufsize - (currptr - rcurrptr) < len) {
            return SYNTIANT_NDP_ERROR_ARG;
        }
#endif
        start = rcurrptr + bufsize - len;
    }

    if (bufsize <= start) {
        start -= bufsize;
    }

    used = bufsize ? rcurrptr + bufsize - start : 0;
    if (bufsize < used
        || (from == SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_UNREAD
            && bufsize <= used)) {
        used -= bufsize;
    }

    if (!data &&
        (from == SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_MATCH
         || from == SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_NEWEST)) {
        len = 0;
    }

    if (used < len) {
        len = used;
    }

    if (len && data) {
        offset = 0;
        if (bufsize < start + len) {
            rlen = bufsize - start;
            s = syntiant_ndp10x_micro_transfer(ndp, 1, base + start, NULL,
                                               data, rlen);
            if (s) {
                return s;
            }
            len -= rlen;
            offset = rlen;
            start = 0;
        }

        s = syntiant_ndp10x_micro_transfer(ndp, 1, base + start, NULL,
                                           data + offset, len);
        if (s) {
            return s;
        }
    }

    start = start + len;
    ndp->tankptr_last = start < bufsize ? start : start - bufsize;

    *lenp = used;

    return SYNTIANT_NDP_ERROR_NONE;
}

int
syntiant_ndp10x_micro_get_match(struct syntiant_ndp10x_micro_device_s *ndp,
                                int *match)
{
    int s;
    uint32_t addr, cons, v;
    uint32_t ms[NDP10X_FW_STATE_MATCH_RING_ENTRY_SIZE];
    int m = -1;

    if (ndp->match_producer != ndp->match_consumer) {
        s = syntiant_ndp10x_micro_read_fw_state(ndp);
        if (s) {
            return s;
        }

        cons = ndp->match_consumer;
        addr = ndp->fw_state_addr + NDP10X_FW_STATE_MATCH_RING_OFFSET;
        addr += cons * (uint32_t) sizeof(ms);

        s = syntiant_ndp10x_micro_transfer(ndp, 1, addr, NULL, ms, sizeof(ms));
        if (s) {
            return s;
        }

        v = ms[NDP10X_FW_STATE_MATCH_RING_SUMMARY_OFFSET];
        if (v & NDP10X_SPI_MATCH_MATCH_MASK
            && !(v & NDP10X_SPI_MATCH_MULT_MASK)) {
            m = NDP10X_SPI_MATCH_WINNER_EXTRACT(v);
            ndp->tankptr_match = ms[NDP10X_FW_STATE_MATCH_RING_TANKPTR_OFFSET]
                & ~0x3U;
        }
        cons++;
        cons = cons == ndp->match_ring_size ? 0 : cons;
        ndp->match_consumer = cons;
    }

    *match = m;
    return SYNTIANT_NDP_ERROR_NONE;
}
