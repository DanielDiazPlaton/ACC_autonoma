#include "gpio_mcal.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <errno.h>

// Pines definidos en el devicetree
#define MOTOR_IN1_NODE DT_ALIAS(motor_in1)
#define MOTOR_IN2_NODE DT_ALIAS(motor_in2)

// Configuracion de los GPIO
static const struct gpio_dt_spec motor_in1 =
    GPIO_DT_SPEC_GET(MOTOR_IN1_NODE, gpios);

static const struct gpio_dt_spec motor_in2 =
    GPIO_DT_SPEC_GET(MOTOR_IN2_NODE, gpios);

// Inicializar los GPIO
int gpio_mcal_init(void)
{
    int ret = 0;

    if (!gpio_is_ready_dt(&motor_in1))
    {
        ret = -ENODEV;
    }
    else if (!gpio_is_ready_dt(&motor_in2))
    {
        ret = -ENODEV;
    }
    else
    {
        ret = gpio_pin_configure_dt(&motor_in1,
                                    GPIO_OUTPUT_INACTIVE);

        if (ret == 0)
        {
            ret = gpio_pin_configure_dt(&motor_in2,
                                        GPIO_OUTPUT_INACTIVE);
        }
    }

    return ret;
}

// Cambiar el estado de un GPIO
int gpio_mcal_set(gpio_mcal_pin_t pin, uint8_t estado)
{
    int ret = 0;

    if (estado > 1)
    {
        ret = -EINVAL;
    }
    else if (pin == GPIO_MCAL_IN1)
    {
        ret = gpio_pin_set_dt(&motor_in1, estado);
    }
    else if (pin == GPIO_MCAL_IN2)
    {
        ret = gpio_pin_set_dt(&motor_in2, estado);
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}