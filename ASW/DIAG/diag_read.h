#ifndef ASW_DIAG_READ_H
#define ASW_DIAG_READ_H

#include <stdint.h>

/* Identificadores de datos (DID) soportados por la aplicación */
#define DIAG_DID_VEHICLE_VIN 0xF190U

/**
 * @brief Lee un DID de aplicación (ej. 0xF190 para VIN).
 * @return 0 si se leyó correctamente, -ENOENT si el DID no existe, -ENOSPC si no cabe.
 */
int DiagRead_ReadDid(uint16_t did, uint8_t *out_buf, uint16_t max_len, uint16_t *out_len);

/* Lecturas de telemetría/estado del ACC para servicios OBD-II */
uint8_t  DiagRead_GetVehicleSpeedKmh(void);
uint16_t DiagRead_GetEngineRpmRaw(void);
void     DiagRead_GetMilStatus(uint8_t out_status[4]);
uint8_t  DiagRead_GetDtcCount(void);

#endif /* ASW_DIAG_READ_H */