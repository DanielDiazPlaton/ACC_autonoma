#include "Lidar_Drv.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#define LIDAR_HEADER_BYTE     0x59U
#define LIDAR_FRAME_LEN       9U
#define LIDAR_MIN_STRENGTH    100U
#define LIDAR_MAX_DIST_CM     1200U /* 12 m (ajustable según modelo) */
#define LIDAR_MIN_DIST_CM     20U
#define LIDAR_STALE_MS        250LL

static uint8_t  rx_frame[LIDAR_FRAME_LEN];
static uint8_t  rx_idx = 0U;

static struct k_spinlock lidar_lock;
static Lidar_Reading_t   latest_reading = {0};
static uint16_t          prev_dist_cm = 0U;
static int64_t           last_update_ms = 0;

static void lidar_uart_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        uint8_t byte;
        if (uart_fifo_read(dev, &byte, 1) != 1) {
            break;
        }

        if (rx_idx == 0U && byte != LIDAR_HEADER_BYTE) {
            continue;
        }
        if (rx_idx == 1U && byte != LIDAR_HEADER_BYTE) {
            rx_idx = 0U;
            continue;
        }

        rx_frame[rx_idx++] = byte;

        if (rx_idx == LIDAR_FRAME_LEN) {
            rx_idx = 0U;

            uint8_t sum = 0U;
            for (uint8_t i = 0U; i < 8U; i++) {
                sum += rx_frame[i];
            }
            if (sum != rx_frame[8]) {
                continue; /* Checksum inválido */
            }

            uint16_t dist = (uint16_t)rx_frame[2] | ((uint16_t)rx_frame[3] << 8);
            uint16_t str  = (uint16_t)rx_frame[4] | ((uint16_t)rx_frame[5] << 8);
            int64_t  now  = k_uptime_get();

            k_spinlock_key_t key = k_spin_lock(&lidar_lock);
            int64_t dt_ms = now - last_update_ms;

            if (last_update_ms > 0 && dt_ms > 0 && dt_ms < 500) {
                int32_t delta_cm = (int32_t)dist - (int32_t)prev_dist_cm;
                latest_reading.rel_speed_cms = (int16_t)((delta_cm * 1000) / dt_ms);
            } else {
                latest_reading.rel_speed_cms = 0;
            }

            prev_dist_cm = dist;
            last_update_ms = now;

            latest_reading.distance_cm    = dist;
            latest_reading.strength       = str;
            latest_reading.valid          = (str >= LIDAR_MIN_STRENGTH && str != 0xFFFFU);
            latest_reading.target_present = (latest_reading.valid &&
                                            dist >= LIDAR_MIN_DIST_CM &&
                                            dist <= LIDAR_MAX_DIST_CM);
            k_spin_unlock(&lidar_lock, key);
        }
    }
}

int Lidar_Init(const struct device *uart_dev)
{
    if (uart_dev == NULL || !device_is_ready(uart_dev)) {
        printk("[LIDAR-DRV] Error: UART no lista\n");
        return -ENODEV;
    }

    uart_irq_callback_user_data_set(uart_dev, lidar_uart_isr, NULL);
    uart_irq_rx_enable(uart_dev);
    printk("[LIDAR-DRV] Inicializado OK (UART 9-byte protocol)\n");
    return 0;
}

int Lidar_GetReading(Lidar_Reading_t *out)
{
    if (out == NULL) return -EINVAL;

    k_spinlock_key_t key = k_spin_lock(&lidar_lock);
    *out = latest_reading;
    int64_t age = k_uptime_get() - last_update_ms;
    k_spin_unlock(&lidar_lock, key);

    if (last_update_ms == 0 || age > LIDAR_STALE_MS) {
        out->valid = false;
        out->target_present = false;
        return -ETIMEDOUT;
    }

    return 0;
}