#ifndef LIDAR_SIM_DRV_H
#define LIDAR_SIM_DRV_H

#include "Lidar_Drv.h"

typedef enum {
    LIDAR_SIM_FREE_ROAD      = 0U, /* Sin vehículo al frente */
    LIDAR_SIM_STEADY_LEAD    = 1U, /* Auto al frente a 60 km/h constante */
    LIDAR_SIM_HARD_BRAKE     = 2U, /* Auto al frente frena fuerte hasta 0 km/h */
    LIDAR_SIM_CUT_IN         = 3U, /* Un auto se mete a 15 m y luego acelera */
    LIDAR_SIM_SENSOR_FAULT   = 4U  /* Simula pérdida/falla de sensor */
} Lidar_Sim_Scenario_t;

void LidarSim_Init(Lidar_Sim_Scenario_t scenario);
void LidarSim_SetScenario(Lidar_Sim_Scenario_t scenario);

/**
 * @brief Avanza la física del vehículo frontal. Llamar cada ciclo de control (ej. 50 ms).
 * @param dt_ms           Paso de tiempo en milisegundos.
 * @param ego_speed_kmh   Velocidad actual de nuestro vehículo (Ego).
 */
void LidarSim_Step(uint32_t dt_ms, float ego_speed_kmh);

int  LidarSim_GetReading(Lidar_Reading_t *out);

#endif /* LIDAR_SIM_DRV_H */