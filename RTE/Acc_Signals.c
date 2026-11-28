#include "Acc_Signals.h"


/* VIN por defecto de 17 caracteres exactos si la Flash está virgen */
char g_VehicleVIN[VEHICLE_VIN_LENGTH + 1U] = "3N1UAG2026MCXW710";

volatile Acc_ClusterSignals_t g_AccToClusterSignals = {
    .acc_state         = ACC_STATE_STANDBY,
    .vehicle_speed_kmh = 0,
    .set_speed_kmh     = 80,
    .target_distance_m = 255,
    .time_gap_level    = 2,
    .motor_pwm_duty    = 0,
    .warning_flags     = 0,
    .alive_crc         = 0
};

volatile Cluster_To_Acc_Cmd_t g_ClusterToAccCommands = {
    .acc_enable_req    = 0,
    .set_speed_req_kmh = 80,
    .gap_level_req     = 2,
    .reserved          = {0}
};

volatile Acc_Sensor_Telemetry_Msg_t g_AccSensorTelemetry = {0};

uint8_t Acc_ComputeAliveCrc(const Acc_ClusterSignals_t *sig, uint8_t counter)
{
    const uint8_t *raw = (const uint8_t *)sig;
    uint8_t xor_sum = 0;

    for (int i = 0; i < 7; i++) {
        xor_sum ^= raw[i];
    }

    /* Nibble alto = XOR reducido a 4 bits; Nibble bajo = contador 0..15 */
    uint8_t crc_nibble = ((xor_sum >> 4) ^ (xor_sum & 0x0F)) & 0x0F;
    return (uint8_t)((crc_nibble << 4) | (counter & 0x0F));
}