#include "Lidar_Sim_Drv.h"
#include <errno.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>

#define SIM_MAX_RANGE_M  120.0f

static Lidar_Sim_Scenario_t sim_scenario = LIDAR_SIM_FREE_ROAD;
static float sim_dist_m      = SIM_MAX_RANGE_M;
static float sim_lead_mps    = 0.0f;
static float sim_rel_mps     = 0.0f;
static bool  sim_target_on   = false;
static bool  sim_valid       = true;
static uint32_t sim_time_ms  = 0U;

void LidarSim_SetScenario(Lidar_Sim_Scenario_t scenario)
{
    sim_scenario = scenario;
    sim_time_ms  = 0U;
    sim_valid    = (scenario != LIDAR_SIM_SENSOR_FAULT);

    switch (scenario) {
        case LIDAR_SIM_FREE_ROAD:
            sim_dist_m    = SIM_MAX_RANGE_M;
            sim_lead_mps  = 0.0f;
            sim_target_on = false;
            break;

        case LIDAR_SIM_STEADY_LEAD:
            sim_dist_m    = 45.0f;
            sim_lead_mps  = 60.0f / 3.6f; /* 16.67 m/s */
            sim_target_on = true;
            break;

        case LIDAR_SIM_HARD_BRAKE:
            sim_dist_m    = 35.0f;
            sim_lead_mps  = 70.0f / 3.6f; /* 19.44 m/s */
            sim_target_on = true;
            break;

        case LIDAR_SIM_CUT_IN:
            sim_dist_m    = 15.0f;
            sim_lead_mps  = 45.0f / 3.6f; /* 12.50 m/s */
            sim_target_on = true;
            break;

        case LIDAR_SIM_SENSOR_FAULT:
        default:
            sim_dist_m    = 0.0f;
            sim_target_on = false;
            sim_valid     = false;
            break;
    }

    printk("[LIDAR-SIM] Escenario activo: %u\n", (unsigned int)scenario);
}

void LidarSim_Init(Lidar_Sim_Scenario_t scenario)
{
    LidarSim_SetScenario(scenario);
}

void LidarSim_Step(uint32_t dt_ms, float ego_speed_kmh)
{
    if (!sim_valid || !sim_target_on) {
        sim_rel_mps = 0.0f;
        return;
    }

    float dt_s = (float)dt_ms * 0.001f;
    float ego_mps = ego_speed_kmh / 3.6f;
    sim_time_ms += dt_ms;

    /* Perfil de aceleración del vehículo objetivo según escenario */
    if (sim_scenario == LIDAR_SIM_HARD_BRAKE) {
        /* Frena a -5.0 m/s^2 tras 2 segundos hasta detenerse */
        if (sim_time_ms >= 2000U && sim_lead_mps > 0.0f) {
            sim_lead_mps -= 5.0f * dt_s;
            if (sim_lead_mps < 0.0f) sim_lead_mps = 0.0f;
        }
    } else if (sim_scenario == LIDAR_SIM_CUT_IN) {
        /* Después de 4 segundos acelera a +2.0 m/s^2 para alejarse */
        if (sim_time_ms >= 4000U && sim_lead_mps < (100.0f / 3.6f)) {
            sim_lead_mps += 2.0f * dt_s;
        }
    }

    sim_rel_mps = sim_lead_mps - ego_mps;
    sim_dist_m += sim_rel_mps * dt_s;

    if (sim_dist_m < 0.5f) {
        sim_dist_m = 0.5f; /* Límite físico de colisión en simulación */
    } else if (sim_dist_m >= SIM_MAX_RANGE_M) {
        sim_dist_m = SIM_MAX_RANGE_M;
        sim_target_on = false; /* El vehículo frontal salió del rango */
    }
}

int LidarSim_GetReading(Lidar_Reading_t *out)
{
    if (out == NULL) return -EINVAL;

    out->valid          = sim_valid;
    out->target_present = sim_target_on;
    out->distance_cm    = sim_target_on ? (uint16_t)(sim_dist_m * 100.0f) : 0xFFFFU;
    out->strength       = sim_valid ? 1500U : 0U;
    out->rel_speed_cms  = (int16_t)(sim_rel_mps * 100.0f);

    return sim_valid ? 0 : -EIO;
}