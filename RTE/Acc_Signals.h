#ifndef ACC_SIGNALS_H
#define ACC_SIGNALS_H

#include <stdint.h>
#include <stdbool.h>

/* Estados operativos del Control Crucero Adaptativo */
typedef enum {
    ACC_STATE_OFF      = 0x00,
    ACC_STATE_STANDBY  = 0x01,
    ACC_STATE_ACTIVE   = 0x02,
    ACC_STATE_OVERRIDE = 0x03,
    ACC_STATE_FAULT    = 0x04
} Acc_State_t;

/* Banderas de alerta en Byte 6 */
#define ACC_FLAG_TARGET_DETECTED   (1U << 0)
#define ACC_FLAG_FCW_BRAKE_ALERT   (1U << 1)
#define ACC_FLAG_SENSOR_FAULT      (1U << 2)

/* Longitud estándar ISO 3779 para VIN */
#define VEHICLE_VIN_LENGTH 17U



/* Estructura global de señales del ACC (8 bytes exactos para CAN Classic) */
typedef struct __attribute__((packed)) {
    uint8_t acc_state;          /* Byte 0: Acc_State_t */
    uint8_t vehicle_speed_kmh;  /* Byte 1: 0..250 km/h */
    uint8_t set_speed_kmh;      /* Byte 2: 30..180 km/h */
    uint8_t target_distance_m;  /* Byte 3: 0..254 m (255 = sin objetivo) */
    uint8_t time_gap_level;     /* Byte 4: 1, 2, 3 */
    uint8_t motor_pwm_duty;     /* Byte 5: 0..100 % */
    uint8_t warning_flags;      /* Byte 6: Bitfield ACC_FLAG_* */
    uint8_t alive_crc;          /* Byte 7: [7:4] XOR Checksum | [3:0] Alive Counter */
} Acc_ClusterSignals_t;

/* Comandos recibidos desde el Clúster (Raspberry Pi -> MCXW71, ID 0x101) */
typedef struct __attribute__((packed)) {
    uint8_t acc_enable_req;     /* 0: Off, 1: On/Resume, 2: Cancel */
    uint8_t set_speed_req_kmh;  /* Velocidad deseada por el usuario */
    uint8_t gap_level_req;      /* Nivel de distancia solicitado (1..3) */
    uint8_t reserved[5];
} Cluster_To_Acc_Cmd_t;

/* Instancias globales accesibles por ASW, RTE y BSW */
extern volatile Acc_ClusterSignals_t g_AccToClusterSignals;
extern volatile Cluster_To_Acc_Cmd_t g_ClusterToAccCommands;

/* Variable global en RAM espejo de la Flash/NVM (accesible por DCM y ASW) */
extern char g_VehicleVIN[VEHICLE_VIN_LENGTH + 1U];

/* Helper para calcular el Byte 7 (Alive Counter + Checksum XOR) */
uint8_t Acc_ComputeAliveCrc(const Acc_ClusterSignals_t *sig, uint8_t counter);

/* Funciones de interfaz hacia NVM (OsTask_Nvm / Flash interna) */
int32_t Nvm_InitAndLoadVIN(void);
int32_t Nvm_SaveVIN(const uint8_t *new_vin, size_t len);

#endif /* ACC_SIGNALS_H */