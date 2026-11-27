#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stddef.h>
#include "DSL.h"
#include "DSD.h"

static uint8_t sesion_actual = DSL_DEFAULT_SESSION;
static int64_t ultima_actividad_ms = 0;

static void dsl_check_s3_timeout(void)
{
    if (sesion_actual != DSL_DEFAULT_SESSION)
    {
        int64_t ahora = k_uptime_get();

        if ((ahora - ultima_actividad_ms) >= DSL_S3_TIMEOUT_MS)
        {
            sesion_actual = DSL_DEFAULT_SESSION;
            printk("[DSL] S3 timeout: regreso automatico a DefaultSession\n");
        }
    }
}

int DSL_Init(void)
{
    int ret = 0;

    sesion_actual = DSL_DEFAULT_SESSION;
    ultima_actividad_ms = k_uptime_get();

    printk("[DSL] Inicializado en DefaultSession\n");

    return ret;
}

int DSL_SetSession(uint8_t sesion)
{
    int ret = 0;

    if ((sesion == DSL_DEFAULT_SESSION) || (sesion == DSL_EXTENDED_SESSION))
    {
        sesion_actual = sesion;
        ultima_actividad_ms = k_uptime_get();

        printk("[DSL] Sesion activa: 0x%02X\n", sesion_actual);
    }
    else
    {
        ret = -ENOTSUP;

        printk("[DSL] Sesion 0x%02X no soportada\n", sesion);
    }

    return ret;
}

uint8_t DSL_GetSession(void)
{
    dsl_check_s3_timeout();

    return sesion_actual;
}

int DSL_Receive(const uint8_t *rx,
                uint16_t rx_len,
                uint8_t *tx,
                uint16_t tx_capacity,
                uint16_t *tx_len)
{
    int ret = 0;

    if (tx_len != NULL)
    {
        *tx_len = 0U;
    }

    dsl_check_s3_timeout();

    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL))
    {
        ret = -EINVAL;

        printk("[DSL] Error: Puntero invalido\n");
    }
    else if (rx_len == 0U)
    {
        ret = -EINVAL;

        printk("[DSL] Error: Mensaje vacio\n");
    }
    else if (rx_len > DSL_RX_MAX_BYTES)
    {
        ret = -EMSGSIZE;

        printk("[DSL] Error: Mensaje demasiado grande\n");
    }
    else if (tx_capacity < 3U)
    {
        ret = -ENOSPC;

        printk("[DSL] Error: Buffer TX insuficiente\n");
    }
    else
    {
        uint16_t cap_efectiva = (tx_capacity > DSL_TX_MAX_BYTES) ? DSL_TX_MAX_BYTES : tx_capacity;

        ultima_actividad_ms = k_uptime_get();

        printk("[DSL] Solicitud diagnostica recibida (SID=0x%02X)\n", rx[0]);

        ret = DSD_Dispatch(rx,
                           rx_len,
                           tx,
                           cap_efectiva,
                           tx_len);
    }

    return ret;
}