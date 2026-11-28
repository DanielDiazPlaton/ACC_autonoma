#ifndef ULTRASONIC_DRV_H
#define ULTRASONIC_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>

typedef struct {
    uint16_t distance_cm;
    bool     valid;
    uint8_t  fault_code; /* 0=OK, 1=Timeout/Sin eco, 2=Fuera de rango */
} Ultrasonic_Reading_t;

int  Ultrasonic_Init(const struct gpio_dt_spec *trig, const struct gpio_dt_spec *echo);
int  Ultrasonic_Read(Ultrasonic_Reading_t *out);

#endif /* ULTRASONIC_DRV_H */