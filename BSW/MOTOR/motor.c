#include "motor.h"
#include "gpio_mcal.h"

// Inicializar el motor
int motor_init(void)
{
    int ret = 0;

    ret = gpio_mcal_init();

    if (ret == 0)
    {
        ret = motor_detener();
    }

    return ret;
}

// Hacer avanzar el motor
int motor_avanzar(void)
{
    int ret = 0;

    ret = gpio_mcal_set(GPIO_MCAL_IN2, 0);

    if (ret == 0)
    {
        ret = gpio_mcal_set(GPIO_MCAL_IN1, 1);
    }

    return ret;
}

// Hacer retroceder el motor
int motor_retroceder(void)
{
    int ret = 0;

    ret = gpio_mcal_set(GPIO_MCAL_IN1, 0);

    if (ret == 0)
    {
        ret = gpio_mcal_set(GPIO_MCAL_IN2, 1);
    }

    return ret;
}

// Detener el motor
int motor_detener(void)
{
    int ret = 0;
    int ret_in2 = 0;

    ret = gpio_mcal_set(GPIO_MCAL_IN1, 0);

    ret_in2 = gpio_mcal_set(GPIO_MCAL_IN2, 0);

    if (ret == 0)
    {
        ret = ret_in2;
    }

    return ret;
}