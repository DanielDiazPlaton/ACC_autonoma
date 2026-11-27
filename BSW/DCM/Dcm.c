#include "CanIf.h"
#include "Acc_Signals.h"
#include <string.h>
#include <zephyr/sys/printk.h>

static uint8_t dcm_tx_buf[32];

static void dcm_send_response(const uint8_t *payload, uint16_t len)
{
    PduInfoType resp_pdu = {
        .SduDataPtr = (uint8_t *)payload,
        .SduLength  = len
    };

    int32_t ret = PduR_DcmTransmit(PDU_ID_TX_UDS_DIAG_RESP, &resp_pdu);
    if (ret != 0) {
        printk("[DCM-TX] Error enviando 0x7E8 [%d]\n", ret);
    }
}

void Dcm_RxIndication(const PduInfoType *PduInfoPtr)
{
    if (PduInfoPtr == NULL || PduInfoPtr->SduDataPtr == NULL || PduInfoPtr->SduLength == 0) {
        return;
    }

    const uint8_t *req = PduInfoPtr->SduDataPtr;
    uint16_t len = PduInfoPtr->SduLength;
    uint8_t sid = req[0];

    switch (sid) {
        /* OBD-II Modo 0x01: Handshake inicial + Live Data */
        case 0x01: {
            if (len < 2) {
                return;
            }
            uint8_t pid = req[1];

            if (pid == 0x00U) {
                /* Bitmask 0x80 0x18 0x00 0x00 -> Soportamos PID 01 (MIL), 0C (RPM) y 0D (Velocidad) */
                dcm_tx_buf[0] = 0x41;
                dcm_tx_buf[1] = 0x00;
                dcm_tx_buf[2] = 0x80;
                dcm_tx_buf[3] = 0x18;
                dcm_tx_buf[4] = 0x00;
                dcm_tx_buf[5] = 0x00;
                dcm_send_response(dcm_tx_buf, 6);
                printk("[DCM-TX] Respuesta OBD 01 00 enviada\n");
            } else if (pid == 0x01U) {
                /* MIL apagada, 0 DTCs almacenados */
                dcm_tx_buf[0] = 0x41;
                dcm_tx_buf[1] = 0x01;
                dcm_tx_buf[2] = 0x00;
                dcm_tx_buf[3] = 0x07;
                dcm_tx_buf[4] = 0x65;
                dcm_tx_buf[5] = 0x00;
                dcm_send_response(dcm_tx_buf, 6);
            } else if (pid == 0x0CU) {
                /* RPM simuladas (1500 RPM si ACC activo, 750 RPM en reposo) */
                uint16_t rpm_raw = (g_AccToClusterSignals.acc_state == ACC_STATE_ACTIVE) ? (1500U * 4U) : (750U * 4U);
                dcm_tx_buf[0] = 0x41;
                dcm_tx_buf[1] = 0x0C;
                dcm_tx_buf[2] = (uint8_t)(rpm_raw >> 8);
                dcm_tx_buf[3] = (uint8_t)(rpm_raw & 0xFF);
                dcm_send_response(dcm_tx_buf, 4);
            } else if (pid == 0x0DU) {
                /* Velocidad real en vivo del ACC (km/h) */
                dcm_tx_buf[0] = 0x41;
                dcm_tx_buf[1] = 0x0D;
                dcm_tx_buf[2] = g_AccToClusterSignals.vehicle_speed_kmh;
                dcm_send_response(dcm_tx_buf, 3);
            }
            /* Otros PIDs en 0x7DF: silencio según ISO 15765-4 */
            break;
        }

        /* OBD-II Modo 0x03 / 0x07: Lectura de DTCs (0 códigos) */
        case 0x03:
        case 0x07: {
            dcm_tx_buf[0] = (uint8_t)(sid + 0x40U);
            dcm_tx_buf[1] = 0x00;
            dcm_send_response(dcm_tx_buf, 2);
            break;
        }

        /* OBD-II Modo 0x09: Vehicle Info (VIN) */
        case 0x09: {
            if (len < 2) {
                return;
            }
            if (req[1] == 0x00U) {
                /* Bitmask 0x40 -> Soporta PID 0x02 (VIN) */
                dcm_tx_buf[0] = 0x49;
                dcm_tx_buf[1] = 0x00;
                dcm_tx_buf[2] = 0x40;
                dcm_tx_buf[3] = 0x00;
                dcm_tx_buf[4] = 0x00;
                dcm_tx_buf[5] = 0x00;
                dcm_send_response(dcm_tx_buf, 6);
            } else if (req[1] == 0x02U) {
                /* 3 bytes cabecera + 17 bytes VIN = 20 bytes exactos (3 tramas de 8 bytes por ISO-TP) */
                dcm_tx_buf[0] = 0x49;
                dcm_tx_buf[1] = 0x02;
                dcm_tx_buf[2] = 0x01;
                memcpy(&dcm_tx_buf[3], g_VehicleVIN, VEHICLE_VIN_LENGTH);
                dcm_send_response(dcm_tx_buf, 3 + VEHICLE_VIN_LENGTH);
                printk("[DCM-TX] VIN enviado al V529: %s\n", g_VehicleVIN);
            }
            break;
        }

        /* UDS 0x22 F1 90 (por si después usas un tester UDS físico) */
        case 0x22: {
            if (len >= 3 && req[1] == 0xF1U && req[2] == 0x90U) {
                dcm_tx_buf[0] = 0x62;
                dcm_tx_buf[1] = 0xF1;
                dcm_tx_buf[2] = 0x90;
                memcpy(&dcm_tx_buf[3], g_VehicleVIN, VEHICLE_VIN_LENGTH);
                dcm_send_response(dcm_tx_buf, 3 + VEHICLE_VIN_LENGTH);
            }
            break;
        }

        default:
            break;
    }
}