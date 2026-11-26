#ifndef CANIF_H
#define CANIF_H

#include <stdint.h>
#include <stddef.h>

/* Identificadores de PDU lógicos (PduIdType estilo AUTOSAR) */
typedef enum {
    PDU_ID_TX_ACC_CLUSTER_STATUS = 0,  /* MCXW71 -> RPi (0x180) */
    PDU_ID_RX_CLUSTER_ACC_CMD    = 1,  /* RPi -> MCXW71 (0x101) */
    PDU_ID_RX_UDS_DIAG_REQ_PHYS  = 2,  /* RPi -> MCXW71 (0x7E0) */
    PDU_ID_RX_UDS_DIAG_REQ_FUNC  = 3,  /* Broadcast -> MCXW71 (0x7DF) */
    PDU_ID_TX_UDS_DIAG_RESP      = 4   /* MCXW71 -> RPi (0x7E8) */
} PduIdType;

/* Estructura estándar AUTOSAR PduInfoType */
typedef struct {
    uint8_t *SduDataPtr;
    uint16_t SduLength;
} PduInfoType;

/* API de CanIf */
int32_t CanIf_Init(void);
int32_t CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
void    CanIf_RxIndication(uint32_t CanId, const uint8_t *CanSduPtr, uint8_t CanDlc);

/* Puntos de enlace hacia PduR (PDU Router) */
void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
int32_t PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

/* Hook hacia el módulo DCM (Diagnostic Communication Manager) */
void Dcm_RxIndication(const PduInfoType *PduInfoPtr);

void CanIf_PollUdsRx(void);

#endif /* CANIF_H */