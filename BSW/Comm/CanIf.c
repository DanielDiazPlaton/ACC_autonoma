#include "CanIf.h"
#include "Acc_Signals.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/can.h>
#include <zephyr/canbus/isotp.h>

#define CAN_ID_ACC_STATUS_TX     0x180U
#define CAN_ID_CLUSTER_CMD_RX    0x101U
#define CAN_ID_UDS_PHYS_REQ_RX   0x7E0U
#define CAN_ID_UDS_FUNC_REQ_RX   0x7DFU
#define CAN_ID_UDS_PHYS_RESP_TX  0x7E8U

static const struct device *can_dev;

static const struct isotp_msg_id uds_rx_addr = { .std_id = CAN_ID_UDS_PHYS_REQ_RX };
static const struct isotp_msg_id uds_tx_addr = { .std_id = CAN_ID_UDS_PHYS_RESP_TX };
static struct isotp_send_ctx uds_send_ctx;

typedef struct {
    uint32_t can_id;
    uint8_t  is_ext;
    uint8_t  dlc;
    uint8_t  data[8];
} Sniff_RxMsg_t;

K_MSGQ_DEFINE(diag_rx_msgq, sizeof(Sniff_RxMsg_t), 8, 4);

static volatile uint32_t g_n947_rx_count = 0;

/* ISR 100% libre de printk: solo copia datos y encola */
static void canif_rx_cb(const struct device *dev, struct can_frame *frame, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    uint8_t is_ext = (frame->flags & CAN_FRAME_IDE) ? 1U : 0U;

    /* Trama cíclica de la N947 (0x101) */
    if (!is_ext && frame->id == CAN_ID_CLUSTER_CMD_RX) {
        CanIf_RxIndication(frame->id, frame->data, frame->dlc);
        return;
    }

    /* Cualquier otra trama en el bus (0x7DF, 0x7E0, 29-bit, etc.) -> cola segura */
    Sniff_RxMsg_t msg = {
        .can_id = frame->id,
        .is_ext = is_ext,
        .dlc    = frame->dlc
    };
    memset(msg.data, 0, sizeof(msg.data));
    memcpy(msg.data, frame->data, (frame->dlc <= 8U) ? frame->dlc : 8U);
    k_msgq_put(&diag_rx_msgq, &msg, K_NO_WAIT);
}

int32_t CanIf_Init(void)
{
    int32_t ret;

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("[CanIf] Error: Dispositivo CAN no listo\n");
        return -ENODEV;
    }

    uint32_t core_clk = 0;
    struct can_timing timing = {0};

    can_get_core_clock(can_dev, &core_clk);
    ret = can_calc_timing(can_dev, &timing, 500000, 800); /* 80.0% sample point */
    if (ret >= 0) {
        /* Ampliar SJW para tolerar deriva de reloj con el escáner */
        timing.sjw = (timing.phase_seg2 >= 3U) ? 3U : timing.phase_seg2;
        int t_ret = can_set_timing(can_dev, &timing);

        uint32_t tq = 1U + timing.prop_seg + timing.phase_seg1 + timing.phase_seg2;
        uint32_t real_bps = (timing.prescaler > 0U && tq > 0U) ? (core_clk / (timing.prescaler * tq)) : 0U;

        printk("[CAN-CLK] Core=%u Hz | Presc=%u Prop=%u PS1=%u PS2=%u SJW=%u -> Real=%u bps (set=%d)\n",
               core_clk, timing.prescaler, timing.prop_seg,
               timing.phase_seg1, timing.phase_seg2, timing.sjw, real_bps, t_ret);
    } else {
        printk("[CAN-CLK] can_calc_timing fallo [%d], usando can_set_bitrate\n", ret);
        can_set_bitrate(can_dev, 500000);
    }

    // can_set_bitrate(can_dev, 500000);
    can_set_mode(can_dev, CAN_MODE_NORMAL);

    ret = can_start(can_dev);
    if (ret != 0) {
        printk("[CanIf] Error al iniciar FlexCAN [%d]\n", ret);
        return ret;
    }

    /* Filtro 1: Comandos del Clúster N947 (0x101 exacto) */
    const struct can_filter cmd_filter = {
        .id = CAN_ID_CLUSTER_CMD_RX,
        .mask = CAN_STD_ID_MASK,
        .flags = 0U
    };
    ret = can_add_rx_filter(can_dev, canif_rx_cb, NULL, &cmd_filter);
    if (ret < 0) {
        printk("[CanIf] Error filtro 0x101 [%d]\n", ret);
        return ret;
    }

    /* Filtro 2: Peticiones OBD-II del escáner V529 (0x7DF exacto; deja 0x7E0 libre para isotp_send) */
    const struct can_filter obd_filter = {
        .id = CAN_ID_UDS_FUNC_REQ_RX,
        .mask = CAN_STD_ID_MASK,
        .flags = 0U
    };
    ret = can_add_rx_filter(can_dev, canif_rx_cb, NULL, &obd_filter);
    if (ret < 0) {
        printk("[CanIf] Error filtro 0x7DF [%d]\n", ret);
        return ret;
    }

    printk("[CanIf] Sniffer activo (11b + 29b): TX=0x180, escuchando TODO el bus\n");
    return 0;
}

int32_t CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    if (can_dev == NULL || PduInfoPtr == NULL || PduInfoPtr->SduDataPtr == NULL) {
        return -EINVAL;
    }

    if (TxPduId == PDU_ID_TX_ACC_CLUSTER_STATUS) {
        struct can_frame frame = {
            .id = CAN_ID_ACC_STATUS_TX,
            .dlc = 8,
            .flags = 0
        };
        memcpy(frame.data, PduInfoPtr->SduDataPtr, 8);
        return can_send(can_dev, &frame, K_MSEC(50), NULL, NULL);
    }
    else if (TxPduId == PDU_ID_TX_UDS_DIAG_RESP) {
        if (PduInfoPtr->SduLength <= 7) {
            struct can_frame resp = {
                .id = CAN_ID_UDS_PHYS_RESP_TX,
                .dlc = 8,
                .flags = 0
            };
            memset(resp.data, 0x00, 8);
            resp.data[0] = (uint8_t)(PduInfoPtr->SduLength & 0x0F);
            memcpy(&resp.data[1], PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
            return can_send(can_dev, &resp, K_MSEC(50), NULL, NULL);
        }

        return isotp_send(&uds_send_ctx, can_dev,
                          PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength,
                          &uds_tx_addr, &uds_rx_addr,
                          NULL, NULL);
    }

    return -EINVAL;
}

void CanIf_RxIndication(uint32_t CanId, const uint8_t *CanSduPtr, uint8_t CanDlc)
{
    PduInfoType pdu_info = {
        .SduDataPtr = (uint8_t *)CanSduPtr,
        .SduLength  = CanDlc
    };

    if (CanId == CAN_ID_CLUSTER_CMD_RX) {
        PduR_CanIfRxIndication(PDU_ID_RX_CLUSTER_ACC_CMD, &pdu_info);
    }
    else if (CanId == CAN_ID_UDS_PHYS_REQ_RX || CanId == CAN_ID_UDS_FUNC_REQ_RX) {
        PduR_CanIfRxIndication(PDU_ID_RX_UDS_DIAG_REQ_PHYS, &pdu_info);
    }
}

void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (RxPduId == PDU_ID_RX_CLUSTER_ACC_CMD && PduInfoPtr->SduLength >= sizeof(Cluster_To_Acc_Cmd_t)) {
        memcpy((void *)&g_ClusterToAccCommands, PduInfoPtr->SduDataPtr, sizeof(Cluster_To_Acc_Cmd_t));
        g_n947_rx_count++;
    }
    else if (RxPduId == PDU_ID_RX_UDS_DIAG_REQ_PHYS) {
        Dcm_RxIndication(PduInfoPtr);
    }
}

int32_t PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    return CanIf_Transmit(TxPduId, PduInfoPtr);
}

/* Llamar desde el while(1) de OsTask_comm_rx */
void CanIf_PollUdsRx(void)
{
    static uint8_t last_rec = 0;
    static uint8_t last_tec = 0;
    static enum can_state last_state = CAN_STATE_ERROR_ACTIVE;
    static int64_t last_hb_ms = 0;

    Sniff_RxMsg_t msg;

    /* 1. Imprime y procesa cualquier trama que NO sea 0x101 */
    while (k_msgq_get(&diag_rx_msgq, &msg, K_MSEC(50)) == 0) {
        printk("[BUS-SNIFF] ID=0x%X (%s) DLC=%u Data=%02X %02X %02X %02X %02X %02X %02X %02X\n",
               msg.can_id, msg.is_ext ? "29b" : "11b", msg.dlc,
               msg.data[0], msg.data[1], msg.data[2], msg.data[3],
               msg.data[4], msg.data[5], msg.data[6], msg.data[7]);

        if (!msg.is_ext && (msg.can_id == CAN_ID_UDS_FUNC_REQ_RX || msg.can_id == CAN_ID_UDS_PHYS_REQ_RX)) {
            if (msg.dlc >= 2) {
                uint8_t pci_type = (msg.data[0] >> 4) & 0x0F;
                uint8_t sf_len   = msg.data[0] & 0x0F;
                if (pci_type == 0x00 && sf_len > 0 && sf_len <= (msg.dlc - 1)) {
                    CanIf_RxIndication(msg.can_id, &msg.data[1], sf_len);
                }
            }
        }
    }

    /* 2. Monitor de contadores de error de FlexCAN (detecta desfase de bitrate/timing) */
    enum can_state st;
    struct can_bus_err_cnt err;
    if (can_dev != NULL && can_get_state(can_dev, &st, &err) == 0) {
        if (err.rx_err_cnt != last_rec || err.tx_err_cnt != last_tec || st != last_state) {
            printk("[CAN-ERR] State=%d REC=%u TEC=%u\n", (int)st, err.rx_err_cnt, err.tx_err_cnt);
            last_rec = err.rx_err_cnt;
            last_tec = err.tx_err_cnt;
            last_state = st;
        }
    }

    /* 3. Latido cada 2 s confirmando que la N947 sigue viva sin inundar la consola */
    int64_t now = k_uptime_get();
    if ((now - last_hb_ms) >= 2000) {
        last_hb_ms = now;
        printk("[HB] N947 RX_0x101=%u | SetSpeed=%u\n",
               g_n947_rx_count, g_ClusterToAccCommands.set_speed_req_kmh);
    }
}

__attribute__((weak)) void Dcm_RxIndication(const PduInfoType *PduInfoPtr)
{
    printk("[DCM-WEAK] Falta compilar Dcm.c! SID=0x%02X\n", PduInfoPtr->SduDataPtr[0]);
}