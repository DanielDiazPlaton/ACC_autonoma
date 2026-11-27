#ifndef DSD_H
#define DSD_H

#include <stdint.h>

#define DSD_SID_SESSION_CONTROL 0x10U       //Identificador de un servicio de diagnostico de sesión
#define DSD_SID_READ_DATA       0x22U       //Identificador de un servicio de lectura 

int DSD_Dispatch(const uint8_t *rx,
                 uint16_t rx_len,
                 uint8_t *tx,               //Recibe la información de UDS y lo que va a mandar, además de verificar que todo este bienS
                 uint16_t tx_capacity,
                 uint16_t *tx_len);

#endif /* DSD_H */