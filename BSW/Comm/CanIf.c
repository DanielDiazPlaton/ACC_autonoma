#include "CanIf.h"
#include "Acc_Signals.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/can.h>
#include <zephyr/canbus/isotp.h>

/* Mapa de IDs físicos en el bus CAN */
#define CAN_ID_ACC_STATUS_TX     0x180U
#define CAN_ID_CLUSTER_CMD_RX    0x101U
#define CAN_ID_UDS_PHYS_REQ_RX   0x7E0U
#define CAN_ID_UDS_FUNC_REQ_RX   0x7DFU
#define CAN_ID_UDS_PHYS_RESP_TX  0x7E8U

static const struct device *can_dev;

/* Contextos ISO-TP exclusivos para Diagnóstico UDS (DCM) */
static const struct isotp_msg_id uds_rx_addr = { .std_id = CAN_ID_UDS_PHYS_REQ_RX };
static const struct isotp_msg_id uds_tx_addr = { .std_id = CAN_ID_UDS_PHYS_RESP_TX };
static const struct isotp_fc_opts uds_fc_opts = { .bs = 8, .stmin = 0 };

static struct isotp_recv_ctx uds_recv_ctx;
static struct isotp_send_ctx uds_send_ctx;

/* Buffer lineal ensamblado que se entrega al módulo DCM */
#define DCM_MAX_UDS_BUF_SIZE 64
static uint8_t dcm_rx_linear_buf[DCM_MAX_UDS_BUF_SIZE];

/* Callback de hardware de Zephyr para tramas CAN clásicas (No ISO-TP) */
static void canif_classic_rx_cb(const struct device *dev, struct can_frame *frame, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    CanIf_RxIndication(frame->id, frame->data, frame->dlc);
}

int32_t CanIf_Init(void)
{
    int32_t ret;

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("[CanIf] Error: Dispositivo CAN no listo\n");
        return -ENODEV;
    }

    can_set_bitrate(can_dev, 500000);
    can_set_mode(can_dev, CAN_MODE_NORMAL);

    ret = can_start(can_dev);
    if (ret != 0) {
        printk("[CanIf] Error al iniciar FlexCAN [%d]\n", ret);
        return ret;
    }

    /* 1. Filtro hardware para comandos del Clúster (0x101) */
    const struct can_filter cmd_filter = {
        .id = CAN_ID_CLUSTER_CMD_RX,
        .mask = CAN_STD_ID_MASK,
        .flags = 0
    };
    ret = can_add_rx_filter(can_dev, canif_classic_rx_cb, NULL, &cmd_filter);
    if (ret < 0) {
        printk("[CanIf] Error al registrar filtro 0x101 [%d]\n", ret);
        return ret;
    }

    /* 2. Vincular canal ISO-TP para peticiones UDS del DCM (0x7E0 -> 0x7E8) */
    ret = isotp_bind(&uds_recv_ctx, can_dev,
                     &uds_rx_addr, &uds_tx_addr,
                     &uds_fc_opts, K_FOREVER);
    if (ret != ISOTP_N_OK) {
        printk("[CanIf] Error en isotp_bind UDS 0x7E0 [%d]\n", ret);
        return ret;
    }

    printk("[CanIf] Inicializado: ACC TX=0x180, CMD RX=0x101, UDS=0x7E0/0x7E8\n");
    return 0;
}

/* Transmisión desde capas superiores (PduR -> CanIf -> Driver) */
int32_t CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    if (can_dev == NULL || PduInfoPtr == NULL || PduInfoPtr->SduDataPtr == NULL) {
        return -EINVAL;
    }

    if (TxPduId == PDU_ID_TX_ACC_CLUSTER_STATUS) {
        /* Trama CAN clásica de 8 bytes para señales cíclicas del ACC */
        struct can_frame frame = {
            .id = CAN_ID_ACC_STATUS_TX,
            .dlc = 8,
            .flags = 0
        };
        memcpy(frame.data, PduInfoPtr->SduDataPtr, 8);
        return can_send(can_dev, &frame, K_MSEC(50), NULL, NULL);
    }
    else if (TxPduId == PDU_ID_TX_UDS_DIAG_RESP) {
        /* Respuesta UDS multi-frame segmentada por ISO-TP hacia el DCM tester */
        return isotp_send(&uds_send_ctx, can_dev,
                          PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength,
                          &uds_tx_addr, &uds_rx_addr,
                          NULL, NULL);
    }

    return -EINVAL;
}

/* Clasificador de recepción CanIf -> PduR */
void CanIf_RxIndication(uint32_t CanId, const uint8_t *CanSduPtr, uint8_t CanDlc)
{
    PduInfoType pdu_info = {
        .SduDataPtr = (uint8_t *)CanSduPtr,
        .SduLength  = CanDlc
    };

    if (CanId == CAN_ID_CLUSTER_CMD_RX) {
        /* Ruta: CanIf -> PduR -> Com (Actualiza comandos del ACC) */
        PduR_CanIfRxIndication(PDU_ID_RX_CLUSTER_ACC_CMD, &pdu_info);
    }
    else if (CanId == CAN_ID_UDS_PHYS_REQ_RX || CanId == CAN_ID_UDS_FUNC_REQ_RX) {
        /* Ruta: CanIf/CanTp -> PduR -> Dcm */
        PduR_CanIfRxIndication(PDU_ID_RX_UDS_DIAG_REQ_PHYS, &pdu_info);
    }
}

/* PDU Router (PduR): Enruta hacia el consumidor final (COM o DCM) */
void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (RxPduId == PDU_ID_RX_CLUSTER_ACC_CMD && PduInfoPtr->SduLength >= sizeof(Cluster_To_Acc_Cmd_t)) {
        /* Actualización atómica de las variables globales de comando */
        memcpy((void *)&g_ClusterToAccCommands, PduInfoPtr->SduDataPtr, sizeof(Cluster_To_Acc_Cmd_t));
        printk("[W71-RX] Comando Cluster recibido -> Enable:%u | SetSpeed:%u | Gap:%u\n",
           g_ClusterToAccCommands.acc_enable_req,
           g_ClusterToAccCommands.set_speed_req_kmh,
           g_ClusterToAccCommands.gap_level_req);
    }
    else if (RxPduId == PDU_ID_RX_UDS_DIAG_REQ_PHYS) {
        /* Entrega el buffer lineal UDS al módulo DCM */
        Dcm_RxIndication(PduInfoPtr);
    }
}

/* Ruta de transmisión desde el DCM: Dcm -> PduR -> CanIf */
int32_t PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    return CanIf_Transmit(TxPduId, PduInfoPtr);
}

/* Tarea de sondeo ISO-TP para reensamblar peticiones UDS hacia el DCM (Llamar en OsTask_comm_rx) */
void CanIf_PollUdsRx(void)
{
    int rem_len;
    uint16_t total_len = 0;
    struct net_buf *buf;

    do {
        rem_len = isotp_recv_net(&uds_recv_ctx, &buf, K_MSEC(100));
        if (rem_len < 0) {
            return;
        }

        while (buf != NULL) {
            if ((total_len + buf->len) <= DCM_MAX_UDS_BUF_SIZE) {
                memcpy(&dcm_rx_linear_buf[total_len], buf->data, buf->len);
                total_len += buf->len;
            }
            buf = net_buf_frag_del(NULL, buf);
        }
    } while (rem_len > 0);

    if (total_len > 0) {
        CanIf_RxIndication(CAN_ID_UDS_PHYS_REQ_RX, dcm_rx_linear_buf, (uint8_t)total_len);
    }
}

/* Stub débil (weak) del DCM para compilar antes de integrar Dcm.c */
__attribute__((weak)) void Dcm_RxIndication(const PduInfoType *PduInfoPtr)
{
    printk("[DCM-RX] Peticion UDS recibida (%u bytes): SID=0x%02X\n",
           PduInfoPtr->SduLength, PduInfoPtr->SduDataPtr[0]);
}