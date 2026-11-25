#include "Can_mis.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

K_SEM_DEFINE(isotp_tx_sem, 1, 1);

/* Direcciones ISO-TP cruzadas para Loopback en la misma tarjeta */
static const struct isotp_msg_id rx_addr_acc = {
    .std_id = CAN_ACC_RX_ID,
};

static const struct isotp_msg_id tx_addr_acc = {
    .std_id = CAN_ACC_TX_ID,
};

/* Canal 1: Envío hacia N947 */
static const struct isotp_msg_id tx_to_n947   = { .std_id = 0x180 };
static const struct isotp_msg_id fc_from_n947 = { .std_id = 0x080 };

/* Canal 2: Recepción de respuesta desde N947 */
static const struct isotp_msg_id rx_from_n947 = { .std_id = 0x101 };
static const struct isotp_msg_id fc_to_n947   = { .std_id = 0x001 };

static const struct device *can_dev;
static struct isotp_recv_ctx recv_ctx_acc;
static struct isotp_send_ctx send_ctx_acc;

/* Parámetros de control de flujo (Block Size = 8, STmin = 0 ms) */
static const struct isotp_fc_opts fc_opts_acc = {
    .bs = 8,
    .stmin = 0
};

static void send_complete_cb(int error_nr, void *arg)
{
    ARG_UNUSED(arg);
    /* Liberar el contexto TX solo cuando ISO-TP realmente terminó */
    k_sem_give(&isotp_tx_sem);

    if (error_nr == ISOTP_N_OK) {
        printk("[CAN-ISOTP] TX completado con exito\n");
    } else {
        printk("[CAN-ISOTP] Error en callback TX [%d]\n", error_nr);
    }
}

int32_t can_init_mis(void)
{
    int32_t ret;

    /* 1. Obtener instancia del Device Tree (zephyr,canbus -> flexcan0) */
    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("[CAN] Error: Dispositivo CAN no esta listo.\n");
        return -ENODEV;
    }

#if CAN_TEST_LOOPBACK_MODE
    /* 2. Configurar modo Loopback interno antes de arrancar el periférico */
    ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
    if (ret != 0) {
        printk("[CAN] Error al configurar modo Loopback [%d]\n", ret);
        return ret;
    }
    printk("[CAN] Modo LOOPBACK habilitado\n");
#endif

    /* 3. Arrancar el controlador FlexCAN */
    can_set_bitrate(can_dev, 500000);
    can_set_mode(can_dev, CAN_MODE_NORMAL);
    ret = can_start(can_dev);
    if (ret != 0) {
        printk("[CAN] Error al iniciar controlador CAN [%d]\n", ret);
        return ret;
    }

    /* 4. Vincular contexto de recepción ISO-TP en el ID 0x080 */
    ret = isotp_bind(&recv_ctx_acc, can_dev,
                 &rx_from_n947, &fc_to_n947,
                 &fc_opts_acc, K_FOREVER);
    if (ret != ISOTP_N_OK) {
        printk("[CAN] Error en isotp_bind RX ID 0x%X [%d]\n",
               rx_addr_acc.std_id, ret);
        return ret;
    }

    printk("[CAN] Inicializado correctamente (Escuchando ID 0x%X)\n",
           rx_addr_acc.std_id);
    return 0;
}

void can_receive_data(void)
{
    int rem_len;
    int received_len = 0;
    struct net_buf *buf;
    char rx_buffer[64] = {0};

    do {
        rem_len = isotp_recv_net(&recv_ctx_acc, &buf, K_MSEC(1000));
        if (rem_len < 0) {
            if (rem_len != ISOTP_RECV_TIMEOUT) {
                printk("[CAN-RX] Error de recepcion [%d]\n", rem_len);
            }
            return;
        }

        while (buf != NULL) {
            if ((received_len + buf->len) < sizeof(rx_buffer)) {
                memcpy(&rx_buffer[received_len], buf->data, buf->len);
            }
            received_len += buf->len;
            buf = net_buf_frag_del(NULL, buf);
        }
    } while (rem_len > 0);

    if (received_len > 0) {
        printk("[CAN-RX] (%d bytes) -> %s", received_len, rx_buffer);
    }
}

int32_t can_send_data(const uint8_t *data, size_t len)
{
    int32_t ret;

    if (can_dev == NULL || data == NULL || len == 0) {
        return -EINVAL;
    }

    /* Esperar máximo 500 ms a que termine el envío anterior; si el bus está
       caído o sin ACK, aborta limpiamente sin corromper send_ctx_acc */
    if (k_sem_take(&isotp_tx_sem, K_MSEC(500)) != 0) {
        printk("[CAN-TX] Bus ocupado / sin ACK previo, paquete omitido\n");
        return -EBUSY;
    }

    ret = isotp_send(&send_ctx_acc, can_dev,
                 data, len,
                 &tx_to_n947, &fc_from_n947,
                 send_complete_cb, NULL);
    if (ret != ISOTP_N_OK) {
        /* Si falló al iniciar, devolvemos el semáforo inmediatamente */
        k_sem_give(&isotp_tx_sem);
        printk("[CAN-TX] Error al enviar datos [%d]\n", ret);
    }

    return ret;
}