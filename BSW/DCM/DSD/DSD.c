#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include "DSD.h"
#include "DSP.h"

typedef struct
{
    uint8_t sid;

    int (*handler)(const uint8_t *rx,
                   uint16_t rx_len,
                   uint8_t *tx,
                   uint16_t tx_capacity,
                   uint16_t *tx_len);

} DSD_ServiceEntry;


static const DSD_ServiceEntry servicios[] =
{
    {DSD_SID_SESSION_CONTROL, DSP_HandleSessionControl},
    {DSD_SID_READ_DATA, DSP_HandleReadDataByIdentifier}
};


int DSD_Dispatch(const uint8_t *rx,
                 uint16_t rx_len,
                 uint8_t *tx,
                 uint16_t tx_capacity,
                 uint16_t *tx_len)
{
    int ret = 0;
    bool encontrado = false;

    if (tx_len != NULL)
    {
        *tx_len = 0U;
    }

    if ((rx == NULL) || (tx == NULL) || (tx_len == NULL))
    {
        ret = -EINVAL;
    }
    else if (rx_len == 0U)
    {
        ret = -EINVAL;
    }
    else if (tx_capacity < 3U)
    {
        ret = -ENOSPC;
    }
    else
    {
        size_t cantidad_servicios =
            sizeof(servicios) / sizeof(servicios[0]);

        for (size_t i = 0; i < cantidad_servicios; i++)
        {
            if (rx[0] == servicios[i].sid)
            {
                encontrado = true;

                ret = servicios[i].handler(rx,
                                          rx_len,
                                          tx,
                                          tx_capacity,
                                          tx_len);

                break;
            }
        }

        if (encontrado == false)
        {
            tx[0] = 0x7FU;
            tx[1] = rx[0];
            tx[2] = 0x11U;

            *tx_len = 3U;
        }
    }

    return ret;
}