#include "controles.h"
#include "Acc_Signals.h"
#include "Ignition_Drv.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Obtiene el nodo del Device Tree mediante el alias acc-btn (PTD1) */
#define ACC_BTN_NODE DT_ALIAS(acc_btn)

#if !DT_NODE_HAS_STATUS(ACC_BTN_NODE, okay)
#error "El alias 'acc-btn' (PTD1) no esta definido o habilitado en el overlay"
#endif

static const struct gpio_dt_spec acc_btn = GPIO_DT_SPEC_GET(ACC_BTN_NODE, gpios);
static bool acc_encendido = false;
static int prev_btn_state = 0;

int32_t Controles_Init(void)
{
    if (!gpio_is_ready_dt(&acc_btn)) {
        printk("[Controles] Error: Puerto GPIO PTD1 no listo\n");
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&acc_btn, GPIO_INPUT);
    if (ret != 0) {
        printk("[Controles] Error configurando PTD1 [%d]\n", ret);
        return ret;
    }

    acc_encendido = false;
    g_AccToClusterSignals.acc_state = ACC_STATE_OFF;

    printk("[Controles] Boton ACC listo en PTD1 (Estado inicial: OFF)\n");
    return 0;
}

void Controles_ProcesarBotonAcc(void)
{
    int curr_state = gpio_pin_get_dt(&acc_btn);
    if (curr_state < 0) {
        return;
    }

    /* 1. Interlock con Ignición: si está OFF, forzar ACC apagado e ignorar activación */
    if (!Ignition_IsOn()) {
        if (acc_encendido) {
            acc_encendido = false;
            g_AccToClusterSignals.acc_state = ACC_STATE_OFF;
            g_AccSensorTelemetry.throttle_cmd_pct = 0U;
            /* Si conservaste tu struct anterior en Acc_Signals.h, usa:
               g_AccToClusterSignals.motor_pwm_duty = 0; */
            printk("[Controles] Ignicion OFF -> ACC desactivado automaticamente\n");
        }

        if (curr_state == 1 && prev_btn_state == 0) {
            printk("[Controles] Ignicion en OFF: enciende ignicion (PTD2) antes de activar ACC\n");
        }

        prev_btn_state = curr_state;
        return;
    }

    /* 2. Ignición ON: detección de flanco en PTD1 para alternar ACC */
    if (curr_state == 1 && prev_btn_state == 0) {
        k_msleep(30); /* Antirrebote mecanico de 30 ms */

        if (gpio_pin_get_dt(&acc_btn) == 1) {
            acc_encendido = !acc_encendido;

            if (acc_encendido) {
                g_AccToClusterSignals.acc_state = ACC_STATE_ACTIVE;
                printk("[Controles] Boton PTD1 presionado -> ACC ENCENDIDO (ACTIVE)\n");
            } else {
                g_AccToClusterSignals.acc_state = ACC_STATE_OFF;
                g_AccSensorTelemetry.throttle_cmd_pct = 0U;
                printk("[Controles] Boton PTD1 presionado -> ACC APAGADO (OFF)\n");
            }
        }
    }

    prev_btn_state = curr_state;
}