#ifndef IGNITION_DRV_H
#define IGNITION_DRV_H

#include <stdbool.h>

/**
 * @brief Inicializa el botón de ignición (PTD2) y la salida del relevador (PTD3).
 *        Arranca siempre con Ignición OFF (relevador apagado).
 */
int Ignition_Init(void);

/**
 * @brief Consulta el estado actual de la ignición.
 * @return true si está en ON (escáner energizado), false si está en OFF.
 */
bool Ignition_IsOn(void);

/**
 * @brief Permite forzar el estado por software (ej. diagnóstico o falla crítica).
 */
void Ignition_SetState(bool on);

#endif /* IGNITION_DRV_H */