#ifndef DSD_H
#define DSD_H

#include <stdint.h>

/* Servicios OBD-II (SAE J1979 / ISO 15765-4) */
#define DSD_SID_OBD_CURRENT_DATA 0x01U
#define DSD_SID_OBD_READ_DTCS    0x03U
#define DSD_SID_OBD_PENDING_DTCS 0x07U
#define DSD_SID_OBD_VEHICLE_INFO 0x09U

/* Servicios UDS (ISO 14229-1) */
#define DSD_SID_SESSION_CONTROL  0x10U
#define DSD_SID_READ_DATA        0x22U
#define DSD_SID_TESTER_PRESENT   0x3EU

int DSD_Dispatch(const uint8_t *rx,
                 uint16_t rx_len,
                 uint8_t *tx,
                 uint16_t tx_capacity,
                 uint16_t *tx_len);

#endif /* DSD_H */