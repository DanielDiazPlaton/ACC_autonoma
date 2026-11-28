#include "diag_read.h"
#include "Acc_Signals.h"
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

int DiagRead_ReadDid(uint16_t did, uint8_t *out_buf, uint16_t max_len, uint16_t *out_len)
{
    if ((out_buf == NULL) || (out_len == NULL)) {
        return -EINVAL;
    }

    *out_len = 0U;

    switch (did) {
        case DIAG_DID_VEHICLE_VIN:
            if (max_len < VEHICLE_VIN_LENGTH) {
                return -ENOSPC;
            }
            memcpy(out_buf, g_VehicleVIN, VEHICLE_VIN_LENGTH);
            *out_len = VEHICLE_VIN_LENGTH;
            printk("[ASW-DiagRead] VIN entregado desde aplicacion: %s\n", g_VehicleVIN);
            return 0;

        default:
            return -ENOENT;
    }
}

uint8_t DiagRead_GetVehicleSpeedKmh(void)
{
    return g_AccToClusterSignals.vehicle_speed_kmh;
}

uint16_t DiagRead_GetEngineRpmRaw(void)
{
    return (g_AccToClusterSignals.acc_state == ACC_STATE_ACTIVE) ? (1500U * 4U) : (750U * 4U);
}

void DiagRead_GetMilStatus(uint8_t out_status[4])
{
    if (out_status == NULL) {
        return;
    }
    /* MIL apagada, 0 DTCs almacenados */
    out_status[0] = 0x00U;
    out_status[1] = 0x07U;
    out_status[2] = 0x65U;
    out_status[3] = 0x00U;
}

uint8_t DiagRead_GetDtcCount(void)
{
    return 0U;
}