#include "Ultrasonic_Drv.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#define US_MIN_CM              25U
#define US_MAX_CM              450U
#define US_ECHO_TIMEOUT_US     30000U /* Cubre hasta ~517 cm (450 cm = ~26.1 ms) */

static struct gpio_dt_spec us_trig;
static struct gpio_dt_spec us_echo;
static bool us_initialized = false;
static uint16_t us_hist[3] = { US_MAX_CM, US_MAX_CM, US_MAX_CM };
static uint8_t us_hist_idx = 0U;

static uint16_t median3(uint16_t a, uint16_t b, uint16_t c)
{
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
}

int Ultrasonic_Init(const struct gpio_dt_spec *trig, const struct gpio_dt_spec *echo)
{
    if (trig == NULL || echo == NULL) {
        return -EINVAL;
    }
    if (!gpio_is_ready_dt(trig) || !gpio_is_ready_dt(echo)) {
        printk("[US-DRV] Error: GPIOs no listos\n");
        return -ENODEV;
    }

    us_trig = *trig;
    us_echo = *echo;

    int ret = gpio_pin_configure_dt(&us_trig, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) return ret;

    ret = gpio_pin_configure_dt(&us_echo, GPIO_INPUT);
    if (ret != 0) return ret;

    us_initialized = true;
    printk("[US-DRV] Inicializado OK\n");
    return 0;
}

int Ultrasonic_Read(Ultrasonic_Reading_t *out)
{
    if (out == NULL) return -EINVAL;
    if (!us_initialized) return -ENODEV;

    /* Pulso de disparo de 10 us */
    gpio_pin_set_dt(&us_trig, 0);
    k_busy_wait(5);
    gpio_pin_set_dt(&us_trig, 1);
    k_busy_wait(20);
    gpio_pin_set_dt(&us_trig, 0);

    /* Esperar flanco de subida en ECHO */
    uint32_t t0 = k_cycle_get_32();
    while (gpio_pin_get_dt(&us_echo) == 0) {
        if (k_cyc_to_us_floor32(k_cycle_get_32() - t0) > US_ECHO_TIMEOUT_US) {
            out->distance_cm = US_MAX_CM;
            out->valid = false;
            out->fault_code = 1U;
            return -ETIMEDOUT;
        }
    }

    /* Medir duración del pulso alto */
    uint32_t t_rise = k_cycle_get_32();
    while (gpio_pin_get_dt(&us_echo) == 1) {
        if (k_cyc_to_us_floor32(k_cycle_get_32() - t_rise) > US_ECHO_TIMEOUT_US) {
            out->distance_cm = US_MAX_CM;
            out->valid = false;
            out->fault_code = 2U; /* Sin obstáculo en rango */
            return 0;
        }
    }
    uint32_t pulse_us = k_cyc_to_us_floor32(k_cycle_get_32() - t_rise);

    /* d(cm) = pulse_us / 58 (velocidad del sonido ~343 m/s ida y vuelta) */
    uint16_t raw_cm = (uint16_t)(pulse_us / 58U);
    if (raw_cm < US_MIN_CM || raw_cm > US_MAX_CM) {
        out->distance_cm = US_MAX_CM;
        out->valid = false;
        out->fault_code = 2U;
        return 0;
    }

    us_hist[us_hist_idx] = raw_cm;
    us_hist_idx = (uint8_t)((us_hist_idx + 1U) % 3U);

    out->distance_cm = median3(us_hist[0], us_hist[1], us_hist[2]);
    out->valid = true;
    out->fault_code = 0U;
    return 0;
}