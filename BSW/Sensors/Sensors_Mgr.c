#include "Sensors_Mgr.h"
#include "Ignition_Drv.h"
#include "Ultrasonic_Drv.h"
#include "Lidar_Drv.h"
#include "Acc_Signals.h"
#include <zephyr/drivers/gpio.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec us_trig_spec =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, us_trig_gpios);
static const struct gpio_dt_spec us_echo_spec =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, us_echo_gpios);
static const struct device *lidar_uart_dev =
    DEVICE_DT_GET(DT_ALIAS(lidar_uart));

static volatile uint16_t g_fused_distance_cm = 0xFFFFU;
static volatile bool     g_target_detected   = false;

void Sensors_And_Ignition_Init(void)
{
    Ignition_Init();
    Ultrasonic_Init(&us_trig_spec, &us_echo_spec);
    Lidar_Init(lidar_uart_dev);
}

void Sensors_UpdateFusion_50ms(void)
{
    Lidar_Reading_t      lidar = {0};
    Ultrasonic_Reading_t us    = {0};

    Lidar_GetReading(&lidar);
    Ultrasonic_Read(&us);

    /* Actualiza la estructura de telemetría cruda que sale por CAN 0x181 */
    g_AccSensorTelemetry.lidar_dist_cm      = lidar.valid ? lidar.distance_cm : 0xFFFFU;
    g_AccSensorTelemetry.ultrasonic_dist_cm = us.valid    ? us.distance_cm    : 0xFFFFU;
    g_AccSensorTelemetry.rel_speed_cms      = lidar.valid ? lidar.rel_speed_cms : 0;

    /* Fusión conservadora: usa el mínimo si ambos ven objetivo */
    bool detected = false;
    uint16_t fused_cm = 0xFFFFU;

    if (lidar.target_present && us.valid) {
        fused_cm = (lidar.distance_cm < us.distance_cm) ? lidar.distance_cm : us.distance_cm;
        detected = true;
    } else if (lidar.target_present) {
        fused_cm = lidar.distance_cm;
        detected = true;
    } else if (us.valid) {
        fused_cm = us.distance_cm;
        detected = true;
    }

    g_fused_distance_cm = fused_cm;
    g_target_detected   = detected;
}

uint16_t Sensors_GetFusedDistanceCm(bool *target_detected)
{
    if (target_detected != NULL) {
        *target_detected = g_target_detected;
    }
    return g_fused_distance_cm;
}