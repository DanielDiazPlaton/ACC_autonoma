#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "../MCAL/pwm_mcal.h"
#include "Direccion.h"

#define ANGULO_CENTRO_IZQ     90
#define ANGULO_CENTRO_DER     90
#define ANGULO_IZQUIERDA_IZQ  60
#define ANGULO_IZQUIERDA_DER  60
#define ANGULO_DERECHA_IZQ   120
#define ANGULO_DERECHA_DER   120


int Direccion_Init(void)
{
    int ret = 0;

    ret = Pwm_Mcal_Init();

    if (ret == 0)
    {
        ret = Direccion_Centro();
    }
    else
    {
        printk("Error al inicializar direccion\n");
    }

    return ret;
}

static int Direccion_Mover(uint8_t angulo_izq, uint8_t angulo_der)
{
    int ret = 0;

    ret = Pwm_Mcal_Servo_Izquierdo(angulo_izq);

    if (ret == 0)
    {
        ret = Pwm_Mcal_Servo_Derecho(angulo_der);
    }
    else
    {
        printk("Error al mover servo izquierdo\n");
    }

    return ret;
}

int Direccion_Centro(void)
{
    int ret = 0;

    ret = Direccion_Mover(ANGULO_CENTRO_IZQ,
                          ANGULO_CENTRO_DER);

    return ret;
}

int Direccion_Izquierda(void)
{
    int ret = 0;

    ret = Direccion_Mover(ANGULO_IZQUIERDA_IZQ,
                          ANGULO_IZQUIERDA_DER);

    return ret;
}

int Direccion_Derecha(void)
{
    int ret = 0;

    ret = Direccion_Mover(ANGULO_DERECHA_IZQ,
                          ANGULO_DERECHA_DER);

    return ret;
}