#ifndef DSP_H
#define DSP_H

#include <stdint.h>

int DSP_HandleSessionControl(const uint8_t *rx,
                             uint16_t rx_len,
                             uint8_t *tx,
                             uint16_t tx_capacity,
                             uint16_t *tx_len);

int DSP_HandleReadDataByIdentifier(const uint8_t *rx,
                                   uint16_t rx_len,
                                   uint8_t *tx,
                                   uint16_t tx_capacity,
                                   uint16_t *tx_len);

int DSP_HandleTesterPresent(const uint8_t *rx,
                            uint16_t rx_len,
                            uint8_t *tx,
                            uint16_t tx_capacity,
                            uint16_t *tx_len);

int DSP_HandleObdRequest(const uint8_t *rx,
                         uint16_t rx_len,
                         uint8_t *tx,
                         uint16_t tx_capacity,
                         uint16_t *tx_len);

#endif /* DSP_H */