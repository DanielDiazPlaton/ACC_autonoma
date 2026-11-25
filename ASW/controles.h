#ifndef CONTROLES_H
#define CONTROLES_H

#include <stdint.h>

/* Configura el pin PTD1 como entrada GPIO para el botón de encendido del ACC */
int32_t Controles_Init(void);

/* Lee el botón PTD1 con antirrebote y alterna el estado ACC (OFF <-> ACTIVE) */
void Controles_ProcesarBotonAcc(void);

#endif /* CONTROLES_H */