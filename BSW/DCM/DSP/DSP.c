#include "DSP.h"
#include "DSL.h"
#include "diag_read.h"
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

static void dsp_build_nrc(uint8_t sid, uint8_t nrc, uint8_t *tx, uint16_t *tx_len)
{
    tx[0] = 0x7FU;
    tx[1] = sid;
    tx[2] = nrc;
    *tx_len = 3U;
}

/* Servicio UDS 0x10: DiagnosticSessionControl */
int DSP_HandleSessionControl(const uint8_t *rx,
                             uint16_t rx_len,
                             uint8_t *tx,
                             uint16_t tx_capacity,
                             uint16_t *tx_len)
{
    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL)) {
        return -EINVAL;
    }

    *tx_len = 0U;

    if (tx_capacity < 3U) {
        return -ENOSPC;
    }

    if (rx_len != 2U) {
        dsp_build_nrc(rx[0], 0x13U, tx, tx_len);
        return 0;
    }

    uint8_t sub_func = rx[1] & 0x7FU;
    uint8_t suppress_pos_rsp = (rx[1] & 0x80U) ? 1U : 0U;

    if (DSL_SetSession(sub_func) != 0) {
        dsp_build_nrc(rx[0], 0x12U, tx, tx_len);
        return 0;
    }

    if (suppress_pos_rsp != 0U) {
        return 0;
    }

    if (tx_capacity < 6U) {
        return -ENOSPC;
    }

    tx[0] = 0x50U;
    tx[1] = sub_func;
    tx[2] = 0x00U; /* P2Server_max = 50 ms */
    tx[3] = 0x32U;
    tx[4] = 0x01U; /* P2*Server_max = 5000 ms */
    tx[5] = 0xF4U;
    *tx_len = 6U;

    printk("[DSP] SessionControl 0x%02X OK\n", sub_func);
    return 0;
}

/* Servicio UDS 0x22: ReadDataByIdentifier -> ASW/diag_read.c */
int DSP_HandleReadDataByIdentifier(const uint8_t *rx,
                                   uint16_t rx_len,
                                   uint8_t *tx,
                                   uint16_t tx_capacity,
                                   uint16_t *tx_len)
{
    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL)) {
        return -EINVAL;
    }

    *tx_len = 0U;

    if (tx_capacity < 3U) {
        return -ENOSPC;
    }

    if (rx_len != 3U) {
        dsp_build_nrc(rx[0], 0x13U, tx, tx_len);
        return 0;
    }

    uint16_t did = ((uint16_t)rx[1] << 8) | (uint16_t)rx[2];
    uint16_t data_len = 0U;

    int ret = DiagRead_ReadDid(did, &tx[3], (uint16_t)(tx_capacity - 3U), &data_len);
    if (ret == -ENOENT) {
        dsp_build_nrc(rx[0], 0x31U, tx, tx_len);
        return 0;
    } else if (ret != 0) {
        return ret;
    }

    tx[0] = 0x62U;
    tx[1] = rx[1];
    tx[2] = rx[2];
    *tx_len = (uint16_t)(3U + data_len);

    printk("[DSP] ReadDataByIdentifier DID=0x%04X (%u bytes)\n", did, data_len);
    return 0;
}

/* Servicio UDS 0x3E: TesterPresent (mantiene viva la sesion extendida) */
int DSP_HandleTesterPresent(const uint8_t *rx,
                            uint16_t rx_len,
                            uint8_t *tx,
                            uint16_t tx_capacity,
                            uint16_t *tx_len)
{
    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL)) {
        return -EINVAL;
    }

    *tx_len = 0U;

    if (tx_capacity < 3U) {
        return -ENOSPC;
    }

    if (rx_len != 2U) {
        dsp_build_nrc(rx[0], 0x13U, tx, tx_len);
        return 0;
    }

    uint8_t sub_func = rx[1] & 0x7FU;
    uint8_t suppress_pos_rsp = (rx[1] & 0x80U) ? 1U : 0U;

    if (sub_func != 0x00U) {
        dsp_build_nrc(rx[0], 0x12U, tx, tx_len);
        return 0;
    }

    if (suppress_pos_rsp == 0U) {
        tx[0] = 0x7EU;
        tx[1] = 0x00U;
        *tx_len = 2U;
    }

    return 0;
}

/* Servicios OBD-II (0x01, 0x03, 0x07, 0x09) -> ASW/diag_read.c */
int DSP_HandleObdRequest(const uint8_t *rx,
                         uint16_t rx_len,
                         uint8_t *tx,
                         uint16_t tx_capacity,
                         uint16_t *tx_len)
{
    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL) || (rx_len == 0U)) {
        return -EINVAL;
    }

    *tx_len = 0U;

    uint8_t sid = rx[0];

    switch (sid) {
        case 0x01U: {
            if (rx_len < 2U) {
                return 0;
            }
            uint8_t pid = rx[1];

            if (pid == 0x00U) {
                if (tx_capacity < 6U) {
                    return -ENOSPC;
                }
                tx[0] = 0x41U;
                tx[1] = 0x00U;
                tx[2] = 0x80U;
                tx[3] = 0x18U;
                tx[4] = 0x00U;
                tx[5] = 0x00U;
                *tx_len = 6U;
                printk("[DSP-OBD] Respuesta 01 00 lista\n");
            } else if (pid == 0x01U) {
                if (tx_capacity < 6U) {
                    return -ENOSPC;
                }
                tx[0] = 0x41U;
                tx[1] = 0x01U;
                DiagRead_GetMilStatus(&tx[2]);
                *tx_len = 6U;
            } else if (pid == 0x0CU) {
                if (tx_capacity < 4U) {
                    return -ENOSPC;
                }
                uint16_t rpm_raw = DiagRead_GetEngineRpmRaw();
                tx[0] = 0x41U;
                tx[1] = 0x0CU;
                tx[2] = (uint8_t)(rpm_raw >> 8);
                tx[3] = (uint8_t)(rpm_raw & 0xFFU);
                *tx_len = 4U;
            } else if (pid == 0x0DU) {
                if (tx_capacity < 3U) {
                    return -ENOSPC;
                }
                tx[0] = 0x41U;
                tx[1] = 0x0DU;
                tx[2] = DiagRead_GetVehicleSpeedKmh();
                *tx_len = 3U;
            }
            break;
        }

        case 0x03U:
        case 0x07U: {
            if (tx_capacity < 2U) {
                return -ENOSPC;
            }
            tx[0] = (uint8_t)(sid + 0x40U);
            tx[1] = DiagRead_GetDtcCount();
            *tx_len = 2U;
            break;
        }

        case 0x09U: {
            if (rx_len < 2U) {
                return 0;
            }
            if (rx[1] == 0x00U) {
                if (tx_capacity < 6U) {
                    return -ENOSPC;
                }
                tx[0] = 0x49U;
                tx[1] = 0x00U;
                tx[2] = 0x40U;
                tx[3] = 0x00U;
                tx[4] = 0x00U;
                tx[5] = 0x00U;
                *tx_len = 6U;
            } else if (rx[1] == 0x02U) {
                if (tx_capacity < 3U) {
                    return -ENOSPC;
                }
                uint16_t data_len = 0U;
                int ret = DiagRead_ReadDid(DIAG_DID_VEHICLE_VIN,
                                           &tx[3],
                                           (uint16_t)(tx_capacity - 3U),
                                           &data_len);
                if (ret == 0) {
                    tx[0] = 0x49U;
                    tx[1] = 0x02U;
                    tx[2] = 0x01U;
                    *tx_len = (uint16_t)(3U + data_len);
                    printk("[DSP-OBD] VIN listo via ASW (%u bytes)\n", *tx_len);
                }
                return ret;
            }
            break;
        }

        default:
            break;
    }

    return 0;
}