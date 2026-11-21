//Evitan que el contenido del archivo.h se incluya varias veces durantre la compilación
#ifndef PWM_MCAL_H
#define PWM_MCAL_H

// Permite utilizar tipos de datos que representan un número entero sin signo
#include <stdint.h>

//Aquí estoy definiendo los límites de referencia para los servos
#define PWM_SERVO_MIN_ANGULO 0
#define PWM_SERVO_MAX_ANGULO 180
#define PWM_SERVO_CENTRO 90

//Prototipos de las funciones
int Pwm_Mcal_Init (void); //Esta función comprueba e inicializa el control pwm, devolviendo 0 si todo sale bien y un valor sino es así
int Pwm_Mcal_Servo_Izquierdo (uint8_t angulo); //Configura el pwm del servo izquierdo
int Pwm_Mcal_Servo_Derecho (uint8_t angulo); //COnfigura el pwm del servo derecho 

#endif /* PWM_MCAL_H */