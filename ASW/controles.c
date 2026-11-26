#include "controles.h"
#include "Acc_Signals.h"
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

    /* Arranca apagado por seguridad */
    acc_encendido = false;
    g_AccToClusterSignals.acc_state = ACC_STATE_OFF;

    printk("[Controles] Boton ACC listo en PTD1 (Estado inicial: OFF)\n");
    return 0;
}

void Controles_ProcesarBotonAcc(void)
{
    /* gpio_pin_get_dt devuelve 1 cuando el pin esta presionado (logica activa en bajo) */
    int curr_state = gpio_pin_get_dt(&acc_btn);

    if (curr_state < 0) {
        return;
    }

    /* Deteccion de flanco ascendente logico (momento exacto en que se presiona) */
    if (curr_state == 1 && prev_btn_state == 0) {
        k_msleep(30); /* Antirrebote mecanico de 30 ms */

        if (gpio_pin_get_dt(&acc_btn) == 1) {
            acc_encendido = !acc_encendido;

            if (acc_encendido) {
                g_AccToClusterSignals.acc_state = ACC_STATE_ACTIVE;
                printk("[Controles] Boton PTD1 presionado -> ACC ENCENDIDO (ACTIVE)\n");
            } else {
                g_AccToClusterSignals.acc_state = ACC_STATE_OFF;
                g_AccToClusterSignals.motor_pwm_duty = 0;
                printk("[Controles] Boton PTD1 presionado -> ACC APAGADO (OFF)\n");
            }
        }
    }

    prev_btn_state = curr_state;
}