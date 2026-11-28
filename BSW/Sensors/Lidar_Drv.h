#ifndef LIDAR_DRV_H
#define LIDAR_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

typedef struct {
    uint16_t distance_cm;    /* Distancia al objetivo frontal [cm] */
    uint16_t strength;       /* Intensidad del retorno óptico */
    int16_t  rel_speed_cms;  /* Velocidad relativa [cm/s] (<0 acercándose) */
    bool     target_present; /* true si hay objeto dentro de rango útil */
    bool     valid;          /* true si el sensor está sano y actualizado */
} Lidar_Reading_t;

int  Lidar_Init(const struct device *uart_dev);
int  Lidar_GetReading(Lidar_Reading_t *out);

#endif /* LIDAR_DRV_H */