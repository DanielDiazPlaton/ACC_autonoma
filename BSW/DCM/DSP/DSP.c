#include "DSP.h"
#include "DSL.h"
#include "diag_read.h"
#include <stddef.h>
#include <errno.h>
#include <string.h>

#define DSP_VIN_LENGTH     17U
#define DSP_P2_MS          50U
#define DSP_P2_STAR_10MS   500U

/* 0: VIN provisional T09. 1: Lectura desde ASW T010 */
#ifndef DSP_USE_ASW
#define DSP_USE_ASW 0
#endif


/* Estructura de identificadores DID */
typedef struct
{
    uint16_t did;
    uint16_t length;

    int (*handler)(uint16_t did,
                   uint8_t *out_buf,
                   uint16_t max_len,
                   uint16_t *out_len);

} DSP_DidEntry;


/* Funcion provisional para la lectura del VIN */
#if !DSP_USE_ASW

static int dsp_read_vin_demo(uint16_t did,
                             uint8_t *out_buf,
                             uint16_t max_len,
                             uint16_t *out_len)
{
    static const uint8_t vin[] = "3N1TEST0000000001";

    int ret = 0;

    *out_len = 0U;

    if (did != DIAG_DID_VEHICLE_VIN)
    {
        ret = -ENOENT;
    }
    else if (max_len < DSP_VIN_LENGTH)
    {
        ret = -ENOSPC;
    }
    else
    {
        memcpy(out_buf, vin, DSP_VIN_LENGTH);

        *out_len = DSP_VIN_LENGTH;
    }

    return ret;
}

#endif


/* Tabla de identificadores DID soportados */
static const DSP_DidEntry identificadores[] =
{
#if DSP_USE_ASW

    {DIAG_DID_VEHICLE_VIN, DSP_VIN_LENGTH, DiagRead_ReadDid}

#else

    {DIAG_DID_VEHICLE_VIN, DSP_VIN_LENGTH, dsp_read_vin_demo}

#endif
};


/* Validacion de parametros */
static int dsp_validate(const uint8_t *rx,
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

    if ((rx == NULL) ||
        (tx == NULL) ||
        (tx_len == NULL) ||
        (rx_len == 0U))
    {
        ret = -EINVAL;
    }
    else if (tx_capacity < 3U)
    {
        ret = -ENOSPC;
    }

    return ret;
}


/* Construccion de una respuesta negativa UDS */
static void dsp_negative(uint8_t sid,
                         uint8_t nrc,
                         uint8_t *tx,
                         uint16_t *tx_len)
{
    tx[0] = 0x7FU;
    tx[1] = sid;
    tx[2] = nrc;

    *tx_len = 3U;
}


/* SID 0x10: Diagnostic Session Control */
int DSP_HandleSessionControl(const uint8_t *rx,
                             uint16_t rx_len,
                             uint8_t *tx,
                             uint16_t tx_capacity,
                             uint16_t *tx_len)
{
    int ret = dsp_validate(rx,
                           rx_len,
                           tx,
                           tx_capacity,
                           tx_len);

    if (ret == 0)
    {
        /* Verificar longitud */
        if (rx_len != 2U)
        {
            dsp_negative(rx[0], 0x13U, tx, tx_len);
        }

        /* Verificar sesion solicitada */
        else if ((rx[1] & 0x7FU) != DSL_DEFAULT_SESSION)
        {
            dsp_negative(rx[0], 0x12U, tx, tx_len);
        }

        /* Verificar capacidad del buffer */
        else if (((rx[1] & 0x80U) == 0U) &&
                 (tx_capacity < 6U))
        {
            dsp_negative(rx[0], 0x14U, tx, tx_len);
        }

        /* Establecer Default Session */
        else if (DSL_SetSession(DSL_DEFAULT_SESSION) != 0)
        {
            dsp_negative(rx[0], 0x22U, tx, tx_len);
        }

        /* Construir respuesta positiva */
        else if ((rx[1] & 0x80U) == 0U)
        {
            tx[0] = 0x50U;
            tx[1] = DSL_DEFAULT_SESSION;

            tx[2] = (uint8_t)(DSP_P2_MS >> 8U);
            tx[3] = (uint8_t)DSP_P2_MS;

            tx[4] = (uint8_t)(DSP_P2_STAR_10MS >> 8U);
            tx[5] = (uint8_t)DSP_P2_STAR_10MS;

            *tx_len = 6U;
        }
    }

    return ret;
}


/* SID 0x22: Read Data By Identifier */
int DSP_HandleReadDataByIdentifier(const uint8_t *rx,
                                   uint16_t rx_len,
                                   uint8_t *tx,
                                   uint16_t tx_capacity,
                                   uint16_t *tx_len)
{
    int ret = dsp_validate(rx,
                           rx_len,
                           tx,
                           tx_capacity,
                           tx_len);

    if (ret == 0)
    {
        /* Verificar sesion actual */
        if (DSL_GetSession() != DSL_DEFAULT_SESSION)
        {
            dsp_negative(rx[0], 0x7FU, tx, tx_len);
        }

        /* Verificar longitud de solicitud */
        else if (rx_len != 3U)
        {
            dsp_negative(rx[0], 0x13U, tx, tx_len);
        }
        else
        {
            /* Obtener DID de 16 bits */
            uint16_t did = ((uint16_t)rx[1] << 8U) | rx[2];

            uint8_t sid = rx[0];
            uint8_t nrc = 0x31U;

            /* Obtener cantidad de identificadores */
            size_t cantidad =
                sizeof(identificadores) /
                sizeof(identificadores[0]);

            /* Buscar DID en la tabla */
            for (size_t i = 0U; i < cantidad; i++)
            {
                if (did == identificadores[i].did)
                {
                    uint16_t data_len = 0U;

                    /* Verificar capacidad del buffer */
                    if (tx_capacity <
                        (3U + identificadores[i].length))
                    {
                        nrc = 0x14U;
                    }
                    else
                    {
                        /* Ejecutar funcion asociada al DID */
                        int read_ret = identificadores[i].handler(
                            did,
                            &tx[3],
                            tx_capacity - 3U,
                            &data_len
                        );

                        /* Verificar resultado de lectura */
                        if (read_ret == -ENOENT)
                        {
                            nrc = 0x31U;
                        }
                        else if (read_ret == -ENOSPC)
                        {
                            nrc = 0x14U;
                        }
                        else if ((read_ret != 0) ||
                                 (data_len != identificadores[i].length))
                        {
                            nrc = 0x22U;
                        }
                        else
                        {
                            /* Construir respuesta positiva */
                            tx[0] = 0x62U;
                            tx[1] = (uint8_t)(did >> 8U);
                            tx[2] = (uint8_t)did;

                            *tx_len = 3U + data_len;

                            nrc = 0U;
                        }
                    }

                    break;
                }
            }

            /* Construir respuesta negativa si existe error */
            if (nrc != 0U)
            {
                dsp_negative(sid, nrc, tx, tx_len);
            }
        }
    }

    return ret;
}


/* SID 0x3E: Tester Present */
int DSP_HandleTesterPresent(const uint8_t *rx,
                            uint16_t rx_len,
                            uint8_t *tx,
                            uint16_t tx_capacity,
                            uint16_t *tx_len)
{
    int ret = dsp_validate(rx,
                           rx_len,
                           tx,
                           tx_capacity,
                           tx_len);

    if (ret == 0)
    {
        /* Verificar longitud */
        if (rx_len != 2U)
        {
            dsp_negative(rx[0], 0x13U, tx, tx_len);
        }

        /* Verificar subfuncion */
        else if ((rx[1] & 0x7FU) != 0U)
        {
            dsp_negative(rx[0], 0x12U, tx, tx_len);
        }

        /* Construir respuesta positiva */
        else if ((rx[1] & 0x80U) == 0U)
        {
            tx[0] = 0x7EU;
            tx[1] = 0x00U;

            *tx_len = 2U;
        }
    }

    return ret;
}


/* Servicios OBD-II fuera del alcance de la DEMO */
int DSP_HandleObdRequest(const uint8_t *rx,
                         uint16_t rx_len,
                         uint8_t *tx,
                         uint16_t tx_capacity,
                         uint16_t *tx_len)
{
    int ret = dsp_validate(rx,
                           rx_len,
                           tx,
                           tx_capacity,
                           tx_len);

    return ret;
}