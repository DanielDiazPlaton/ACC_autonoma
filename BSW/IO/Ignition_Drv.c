#include "Ignition_Drv.h"
#include "Acc_Signals.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#define ZEPHYR_USER_NODE   DT_PATH(zephyr_user)
#define IGN_DEBOUNCE_MS    250LL

static const struct gpio_dt_spec ign_btn_gpio =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, ign_btn_gpios);

static const struct gpio_dt_spec obd_relay_gpio =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, obd_relay_gpios);

static struct gpio_callback ign_btn_cb_data;
static struct k_work        ign_toggle_work;

static volatile bool   g_ignition_on = false;
static volatile int64_t last_press_ms = 0;

static void apply_ignition_state(bool on)
{
    g_ignition_on = on;

    /* gpio_pin_set_dt respeta automáticamente ACTIVE_HIGH / ACTIVE_LOW del overlay */
    gpio_pin_set_dt(&obd_relay_gpio, on ? 1 : 0);

    /* Interlock de seguridad: si se apaga la ignición, apagar ACC y actuadores */
    if (!on) {
        g_AccToClusterSignals.acc_state = ACC_STATE_OFF;
        g_AccSensorTelemetry.throttle_cmd_pct = 0U;
        g_AccToClusterSignals.motor_pwm_duty = 0;
        g_AccSensorTelemetry.brake_cmd_pct    = 0U;
    }

    printk("[IGN] Ignicion %s -> Relevador 12V OBD-II %s\n",
           on ? "ON" : "OFF",
           on ? "ENERGIZADO" : "CORTADO");
}

static void ign_toggle_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    apply_ignition_state(!g_ignition_on);
}

static void ign_btn_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    int64_t now = k_uptime_get();
    if ((now - last_press_ms) < IGN_DEBOUNCE_MS) {
        return;
    }
    last_press_ms = now;

    k_work_submit(&ign_toggle_work);
}

int Ignition_Init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&ign_btn_gpio) || !gpio_is_ready_dt(&obd_relay_gpio)) {
        printk("[IGN] Error: GPIOs de ignicion/rele no listos\n");
        return -ENODEV;
    }

    /* Configurar salida del relevador inicialmente APAGADA (INACTIVE) */
    ret = gpio_pin_configure_dt(&obd_relay_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("[IGN] Error configurando relevador PTD3 [%d]\n", ret);
        return ret;
    }

    /* Configurar botón de ignición en PTD2 como entrada con interrupción */
    ret = gpio_pin_configure_dt(&ign_btn_gpio, GPIO_INPUT);
    if (ret != 0) {
        printk("[IGN] Error configurando boton PTD2 [%d]\n", ret);
        return ret;
    }

    k_work_init(&ign_toggle_work, ign_toggle_work_handler);

    gpio_init_callback(&ign_btn_cb_data, ign_btn_isr, BIT(ign_btn_gpio.pin));
    ret = gpio_add_callback(ign_btn_gpio.port, &ign_btn_cb_data);
    if (ret != 0) {
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&ign_btn_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("[IGN] Error habilitando interrupcion en PTD2 [%d]\n", ret);
        return ret;
    }

    g_ignition_on = false;
    printk("[IGN] Listo: Boton PTD2 + Relevador PTD3 (Estado inicial: OFF)\n");
    return 0;
}

bool Ignition_IsOn(void)
{
    return g_ignition_on;
}

void Ignition_SetState(bool on)
{
    apply_ignition_state(on);
}