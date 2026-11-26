#ifndef NVM_MANAGER_H
#define NVM_MANAGER_H

#include <stdint.h>
#include <stddef.h>

/* Inicializa el sistema de archivos NVS en Flash y carga el VIN en g_VehicleVIN */
int32_t Nvm_InitAndLoadVIN(void);

/* Actualiza el VIN en RAM (g_VehicleVIN) y lo persiste en Flash interna */
int32_t Nvm_SaveVIN(const uint8_t *new_vin, size_t len);

#endif /* NVM_MANAGER_H */