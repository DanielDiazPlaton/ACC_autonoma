#include "Acc_Signals.h"
#include "Nvm_Manager.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/nvs.h>

#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define NVS_ID_VEHICLE_VIN   0xF190U

static struct nvs_fs fs;
static bool nvs_ready = false;

int32_t Nvm_InitAndLoadVIN(void)
{
    int rc;
    struct flash_pages_info info;

    fs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs.flash_device)) {
        printk("[NVM] Error: Dispositivo Flash no listo\n");
        return -ENODEV;
    }

    fs.offset = NVS_PARTITION_OFFSET;
    rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    if (rc) {
        printk("[NVM] Error obteniendo info de pagina Flash [%d]\n", rc);
        return rc;
    }

    fs.sector_size  = info.size;
    fs.sector_count = 2U; /* 2 sectores para nivelacion de desgaste NVS */

    rc = nvs_mount(&fs);
    if (rc) {
        printk("[NVM] Error montando NVS [%d]\n", rc);
        return rc;
    }

    nvs_ready = true;

    /* Intentar leer el VIN guardado previamente en Flash */
    char flash_vin[VEHICLE_VIN_LENGTH];
    ssize_t bytes_read = nvs_read(&fs, NVS_ID_VEHICLE_VIN, flash_vin, VEHICLE_VIN_LENGTH);

    if (bytes_read == VEHICLE_VIN_LENGTH) {
        memcpy(g_VehicleVIN, flash_vin, VEHICLE_VIN_LENGTH);
        g_VehicleVIN[VEHICLE_VIN_LENGTH] = '\0';
        printk("[NVM] VIN cargado desde Flash: %s\n", g_VehicleVIN);
    } else {
        /* Primera vez que arranca: guardar el VIN por defecto en Flash */
        printk("[NVM] Flash sin VIN previo. Guardando VIN default: %s\n", g_VehicleVIN);
        nvs_write(&fs, NVS_ID_VEHICLE_VIN, g_VehicleVIN, VEHICLE_VIN_LENGTH);
    }

    return 0;
}

int32_t Nvm_SaveVIN(const uint8_t *new_vin, size_t len)
{
    if (!nvs_ready || new_vin == NULL || len != VEHICLE_VIN_LENGTH) {
        return -EINVAL;
    }

    /* 1. Actualizar variable global en RAM inmediatamente */
    memcpy(g_VehicleVIN, new_vin, VEHICLE_VIN_LENGTH);
    g_VehicleVIN[VEHICLE_VIN_LENGTH] = '\0';

    /* 2. Persistir en Flash interna */
    ssize_t written = nvs_write(&fs, NVS_ID_VEHICLE_VIN, g_VehicleVIN, VEHICLE_VIN_LENGTH);
    if (written < 0) {
        printk("[NVM] Error escribiendo VIN en Flash [%d]\n", (int)written);
        return (int32_t)written;
    }

    printk("[NVM] Nuevo VIN guardado en Flash: %s\n", g_VehicleVIN);
    return 0;
}