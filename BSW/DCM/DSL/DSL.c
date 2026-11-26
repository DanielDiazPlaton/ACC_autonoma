#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stddef.h>
#include "DSL.h"
#include "DSD.h"


static uint8_t sesion_actual = DSL_DEFAULT_SESSION; // Variable estática que guarda la sesión actual llamando al identificador

int DSL_Init(void)
{
    int ret = 0;

    sesion_actual = DSL_DEFAULT_SESSION;                //Inicia sesión en DefaultSession

    printk("[DSL] Inicializado en DefaultSession\n");

    return ret;
}


int DSL_SetSession(uint8_t sesion)
{
    int ret = 0;

    if (sesion == DSL_DEFAULT_SESSION)
    {
        sesion_actual = DSL_DEFAULT_SESSION;

        printk("[DSL] DefaultSession activa\n");
    }
    else                                                          //Evita cambiar de sesión
    {
        ret = -ENOTSUP;

        printk("[DSL] Sesion 0x%02X no soportada\n", sesion);
    }

    return ret;
}


uint8_t DSL_GetSession(void)
{
    uint8_t sesion = sesion_actual;             //Verifica la sesión en la que estamos

    return sesion;
}


int DSL_Receive(const uint8_t *rx,
                uint16_t rx_len,
                uint8_t *tx,                      //Recibe los mensajes de UDS a través de CAN y envía la respuesta a DSD
                uint16_t tx_capacity,
                uint16_t *tx_len)
{
    int ret = 0;

    if (tx_len != NULL)
    {
        *tx_len = 0U;
    }

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
    else if (tx_capacity < DSL_TX_MAX_BYTES)
    {
        ret = -ENOSPC;

        printk("[DSL] Error: Buffer TX insuficiente\n");
    }
    else
    {
        printk("[DSL] Solicitud UDS recibida\n");

        ret = D_Dispatch(rx,
                           rx_len,
                           tx,
                           tx_capacity,
                           tx_len);
    }

    return ret;
}