#ifndef SENSORS_MGR_H
#define SENSORS_MGR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa Ignición (PTD2/PTD3), JSN-SR04T (PTB5/PTB4) y TF-Luna (LPUART0).
 */
void Sensors_And_Ignition_Init(void);

/**
 * @brief Lee TF-Luna y JSN-SR04T, calcula la distancia fusionada y actualiza g_AccSensorTelemetry.
 */
void Sensors_UpdateFusion_50ms(void);

/**
 * @brief Devuelve la última distancia fusionada en cm (0xFFFF si no hay objetivo).
 */
uint16_t Sensors_GetFusedDistanceCm(bool *target_detected);

#endif /* SENSORS_MGR_H */