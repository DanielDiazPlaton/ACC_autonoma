#ifndef DSL_H
#define DSL_H
#include <stdint.h>

#define DSL_DEFAULT_SESSION 0x01U //

#define DSL_RX_MAX_BYTES 8U
#define DSL_TX_MAX_BYTES 20U

int DSL_Init(void); // Inicializa la DefaultSession
int DSL_SetSession(uint8_t sesion); // Evita cambiar de sesión
uint8_t DSL_GetSession(void); //Permite consultar que sesión esta activa 
int DSL_Receive(const uint8_t *rx,
                uint16_t rx_len,
                uint8_t *tx,              //Esta función recibirá los mensajes UDS que nos entregue COMM y los enviará al DSD.
                uint16_t tx_capacity,
                uint16_t *tx_len);

#endif /* DSL_H */