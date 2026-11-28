#include "CanIf.h"
#include "DSL.h"
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>

static uint8_t dcm_tx_buf[DSL_TX_MAX_BYTES];
static bool dcm_dsl_initialized = false;

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

void Dcm_Init(void)
{
    if (!dcm_dsl_initialized) {
        DSL_Init();
        dcm_dsl_initialized = true;
    }
}

void Dcm_RxIndication(const PduInfoType *PduInfoPtr)
{
    if ((PduInfoPtr == NULL) || (PduInfoPtr->SduDataPtr == NULL) || (PduInfoPtr->SduLength == 0U)) {
        return;
    }

    if (!dcm_dsl_initialized) {
        Dcm_Init();
    }

    uint16_t tx_len = 0U;
    int ret = DSL_Receive(PduInfoPtr->SduDataPtr,
                          PduInfoPtr->SduLength,
                          dcm_tx_buf,
                          sizeof(dcm_tx_buf),
                          &tx_len);

    if ((ret == 0) && (tx_len > 0U)) {
        dcm_send_response(dcm_tx_buf, tx_len);
    } else if (ret != 0) {
        printk("[DCM] DSL_Receive retorno error [%d]\n", ret);
    }
}