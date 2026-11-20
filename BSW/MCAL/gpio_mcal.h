#ifndef GPIO_MCAL_H
#define GPIO_MCAL_H

#include <stdint.h>

typedef enum
{
    GPIO_MCAL_IN1 = 0,
    GPIO_MCAL_IN2

} gpio_mcal_pin_t;

int gpio_mcal_init(void);

int gpio_mcal_set(gpio_mcal_pin_t pin, uint8_t estado);

#endif
