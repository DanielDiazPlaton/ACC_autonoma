#ifndef DSL_H
#define DSL_H

#include <stdint.h>

#define DSL_DEFAULT_SESSION  0x01U
#define DSL_EXTENDED_SESSION 0x03U
#define DSL_S3_TIMEOUT_MS    5000LL

#define DSL_RX_MAX_BYTES 64U
#define DSL_TX_MAX_BYTES 64U

int DSL_Init(void);
int DSL_SetSession(uint8_t sesion);
uint8_t DSL_GetSession(void);
int DSL_Receive(const uint8_t *rx,
                uint16_t rx_len,
                uint8_t *tx,
                uint16_t tx_capacity,
                uint16_t *tx_len);

#endif /* DSL_H */