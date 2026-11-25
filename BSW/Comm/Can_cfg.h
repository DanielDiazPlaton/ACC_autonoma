#ifndef CAN_CONFIG_H
#define CAN_CONFIG_H

/* Habilitar modo Loopback interno para prueba en una sola tarjeta */
#define CAN_TEST_LOOPBACK_MODE  0

/* IDs estándar para prueba ISO-TP */
#define CAN_ACC_RX_ID           0x080
#define CAN_ACC_TX_ID           0x180

#define CAN_CTRL_RX_ID          0x001
#define CAN_CTRL_TX_ID          0x101

#endif /* CAN_CONFIG_H */