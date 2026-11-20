#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include "pwm_mcal.h"

#define SERVO_PERIODO   PWM_MSEC(20)
#define SERVO_PULSO_MIN PWM_USEC(1000)
#define SERVO_PULSO_MAX PWM_USEC(2000)

static const struct pwm_dt_spec servo_izquierdo =
    PWM_DT_SPEC_GET(DT_ALIAS(servo_izquierdo));

static const struct pwm_dt_spec servo_derecho =
    PWM_DT_SPEC_GET(DT_ALIAS(servo_derecho));


int Pwm_Mcal_Init(void)
{
    int ret = 0;

    if (!pwm_is_ready_dt(&servo_izquierdo))
    {
        printk("Error: PWM izquierdo no disponible\n");
        ret = -ENODEV;
    }
    else if (!pwm_is_ready_dt(&servo_derecho))
    {
        printk("Error: PWM derecho no disponible\n");
        ret = -ENODEV;
    }
    else
    {
        printk("PWM disponible correctamente\n");
    }

    return ret;
}

static uint32_t Pwm_Mcal_Calcular_Pulso(uint8_t angulo)
{
    uint32_t pulso = 0;

    pulso = SERVO_PULSO_MIN + ((SERVO_PULSO_MAX - SERVO_PULSO_MIN) * angulo / PWM_SERVO_MAX_ANGULO);

    return pulso;
}

int Pwm_Mcal_Servo_Izquierdo(uint8_t angulo)
{
    int ret = 0;

    if (angulo > PWM_SERVO_MAX_ANGULO)
    {
        ret = -EINVAL;
    }
    else
    {
        uint32_t pulso = Pwm_Mcal_Calcular_Pulso(angulo);

        ret = pwm_set_dt(&servo_izquierdo,
                         SERVO_PERIODO,
                         pulso);
    }

    return ret;
}

int Pwm_Mcal_Servo_Derecho(uint8_t angulo)
{
    int ret = 0;

    if (angulo > PWM_SERVO_MAX_ANGULO)
    {
        ret = -EINVAL;
    }
    else
    {
        uint32_t pulso = Pwm_Mcal_Calcular_Pulso(angulo);

        ret = pwm_set_dt(&servo_derecho,
                         SERVO_PERIODO,
                         pulso);
    }

    return ret;
}