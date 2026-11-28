#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "Rte_OS.h"
#include "Can_mis.h"
#include "CanIf.h"
#include "Acc_Signals.h"
#include "Nvm_Manager.h"
#include "controles.h"
#include "Sensors_Mgr.h"

/* Tamaños de stack (2048 bytes para hilos con ISO-TP/printk) */
#define STACK_SIZE_diagnostic   1024
#define STACK_SIZE_control      1536
#define STACK_SIZE_RX           2048
#define STACK_SIZE_TX           2048
#define STACK_SIZE_sensors      1024
#define STACK_SIZE_NVM          1024

/* Prioridades (menor número = mayor prioridad) */
#define PRIORIDAD_RX            2
#define PRIO_TASK_control       4
#define PRIORIDAD_TX            5
#define PRIO_TASK_diagnostic    6
#define PRIO_TASK_sensors       7
#define PRIO_TASK_NVM           8

K_THREAD_STACK_DEFINE(stack_diagnostic, STACK_SIZE_diagnostic);
K_THREAD_STACK_DEFINE(stack_control, STACK_SIZE_control);
K_THREAD_STACK_DEFINE(stack_comm_rx, STACK_SIZE_RX);
K_THREAD_STACK_DEFINE(stack_comm_tx, STACK_SIZE_TX);
K_THREAD_STACK_DEFINE(stack_sensors, STACK_SIZE_sensors);
K_THREAD_STACK_DEFINE(stack_nvm, STACK_SIZE_NVM);

static struct k_thread thread_diagnostic_data;
static struct k_thread thread_control_data;
static struct k_thread thread_comm_rx_data;
static struct k_thread thread_comm_tx_data;
static struct k_thread thread_sensors_data;
static struct k_thread thread_nvm_data;

/* Cola IPC del RTE entre Control (ASW) y TX (BSW) */
#define ACC_MSG_MAX_LEN 64
struct acc_tx_msg {
    uint8_t payload[ACC_MSG_MAX_LEN];
    size_t len;
};

K_MSGQ_DEFINE(rte_acc_tx_msgq, sizeof(struct acc_tx_msg), 4, 4);

/* Hilo 1: Diagnóstico UDS */
void OsTask_diagnostic(void *p1, void *p2, void *p3)
{
    while (1) {
        k_msleep(500);
    }
}

/* Hilo 2: Control ACC (50 ms) */
void OsTask_control(void *p1, void *p2, void *p3)
{
    static uint8_t alive_cnt = 0;
    uint8_t sim_speed = 60;
    uint8_t sim_dist  = 45;

    while (1) {
        /* 1. Lectura de entradas físicas (Botón ACC PTD1 condicionado a Ignición) */
        Controles_ProcesarBotonAcc();

        /* 2. Leer distancia fusionada real de sensores (TF-Luna + JSN-SR04T) */
        bool real_target = false;
        uint16_t fused_cm = Sensors_GetFusedDistanceCm(&real_target);
        uint8_t dist_m;

        if (g_AccToClusterSignals.acc_state == ACC_STATE_ACTIVE) {
            sim_speed = (sim_speed >= 115) ? 60 : (sim_speed + 1);
            sim_dist  = (sim_dist <= 8) ? 45 : (sim_dist - 1);

            /* Si los sensores físicos detectan objeto, usar distancia real; si no, simulación */
            dist_m = real_target ? (uint8_t)((fused_cm + 50U) / 100U) : sim_dist;

            g_AccToClusterSignals.motor_pwm_duty  = (dist_m < 15) ? 10 : 65;
            g_AccSensorTelemetry.throttle_cmd_pct = g_AccToClusterSignals.motor_pwm_duty;
            g_AccSensorTelemetry.brake_cmd_pct    = (dist_m < 12) ? 50 : 0;
        } else {
            dist_m = real_target ? (uint8_t)((fused_cm + 50U) / 100U) : sim_dist;
            g_AccToClusterSignals.motor_pwm_duty  = 0;
            g_AccSensorTelemetry.throttle_cmd_pct = 0;
            g_AccSensorTelemetry.brake_cmd_pct    = 0;
        }

        g_AccToClusterSignals.vehicle_speed_kmh = sim_speed;
        g_AccToClusterSignals.set_speed_kmh     = g_ClusterToAccCommands.set_speed_req_kmh;
        g_AccToClusterSignals.target_distance_m = dist_m;
        g_AccToClusterSignals.time_gap_level    = g_ClusterToAccCommands.gap_level_req;
        g_AccToClusterSignals.warning_flags     = (dist_m < 12) ?
                                                  (ACC_FLAG_TARGET_DETECTED | ACC_FLAG_FCW_BRAKE_ALERT) :
                                                  ACC_FLAG_TARGET_DETECTED;

        alive_cnt = (alive_cnt + 1) & 0x0F;
        g_AccToClusterSignals.alive_crc = Acc_ComputeAliveCrc(
            (const Acc_ClusterSignals_t *)&g_AccToClusterSignals, alive_cnt);

        k_msleep(50);
    }
}

/* Hilo 3: Comunicación CAN RX */
void OsTask_comm_rx(void *p1, void *p2, void *p3)
{
    while (1) {
        /* Escucha tramas entrantes ISO-TP / OBD-II en el bus CAN */
        CanIf_PollUdsRx();
    }
}

/* Hilo 4: Comunicación CAN TX */
void OsTask_comm_tx(void *p1, void *p2, void *p3)
{
    uint8_t tx_snapshot[8];
    PduInfoType acc_pdu = {
        .SduDataPtr = tx_snapshot,
        .SduLength  = 8
    };
    uint8_t telem_div = 0U;

    while (1) {
        /* 0x180 cada 50 ms */
        memcpy(tx_snapshot, (const void *)&g_AccToClusterSignals, 8);
        CanIf_Transmit(PDU_ID_TX_ACC_CLUSTER_STATUS, &acc_pdu);

        /* 0x181 cada 100 ms */
        if ((telem_div++ % 2U) == 0U) {
            uint8_t telem_buf[sizeof(Acc_Sensor_Telemetry_Msg_t)];
            memcpy(telem_buf, (const void *)&g_AccSensorTelemetry, sizeof(telem_buf));

            PduInfoType telem_pdu = {
                .SduDataPtr = telem_buf,
                .SduLength  = sizeof(telem_buf)
            };
            CanIf_Transmit(PDU_ID_TX_ACC_SENSOR_TELEMETRY, &telem_pdu);
        }

        k_msleep(50);
    }
}

/* Hilo 5: Lectura y fusión de sensores (TF-Luna + JSN-SR04T cada 50 ms) */
void OsTask_sensors(void *p1, void *p2, void *p3)
{
    while (1) {
        Sensors_UpdateFusion_50ms();
        k_msleep(50);
    }
}

/* Hilo 6: Memoria No Volátil (NVM) */
void OsTask_Nvm(void *p1, void *p2, void *p3)
{
    while (1) {
        k_msleep(500);
    }
}

void Rte_Start(void)
{
    /* Inicializar Flash/NVS y cargar el VIN en RAM antes de arrancar tareas */
    if (Nvm_InitAndLoadVIN() != 0) {
        printk("[RTE] Advertencia: No se pudo montar NVS, usando VIN default en RAM\n");
    }

    /* Inicializar driver CAN antes de arrancar los hilos de comunicación */
    if (CanIf_Init() != 0) {
        printk("[RTE] Fallo critico al inicializar CanIf\n");
        return;
    }

    /* Inicializar Ignición (PTD2/PTD3), JSN-SR04T (PTB5/PTB4) y TF-Luna (LPUART0) */
    Sensors_And_Ignition_Init();

    Controles_Init();

    k_tid_t tid_diagnostic = k_thread_create(&thread_diagnostic_data, stack_diagnostic,
                                             K_THREAD_STACK_SIZEOF(stack_diagnostic),
                                             OsTask_diagnostic, NULL, NULL, NULL,
                                             PRIO_TASK_diagnostic, 0, K_NO_WAIT);
    k_thread_name_set(tid_diagnostic, "Task_diagnostic");

    k_tid_t tid_control = k_thread_create(&thread_control_data, stack_control,
                                          K_THREAD_STACK_SIZEOF(stack_control),
                                          OsTask_control, NULL, NULL, NULL,
                                          PRIO_TASK_control, 0, K_NO_WAIT);
    k_thread_name_set(tid_control, "Task_control");

    k_tid_t tid_rx = k_thread_create(&thread_comm_rx_data, stack_comm_rx,
                                     K_THREAD_STACK_SIZEOF(stack_comm_rx),
                                     OsTask_comm_rx, NULL, NULL, NULL,
                                     PRIORIDAD_RX, 0, K_NO_WAIT);
    k_thread_name_set(tid_rx, "Task_RX");

    k_tid_t tid_tx = k_thread_create(&thread_comm_tx_data, stack_comm_tx,
                                     K_THREAD_STACK_SIZEOF(stack_comm_tx),
                                     OsTask_comm_tx, NULL, NULL, NULL,
                                     PRIORIDAD_TX, 0, K_NO_WAIT);
    k_thread_name_set(tid_tx, "Task_TX");

    k_tid_t tid_sensors = k_thread_create(&thread_sensors_data, stack_sensors,
                                          K_THREAD_STACK_SIZEOF(stack_sensors),
                                          OsTask_sensors, NULL, NULL, NULL,
                                          PRIO_TASK_sensors, 0, K_NO_WAIT);
    k_thread_name_set(tid_sensors, "Task_sensors");

    k_tid_t tid_nvm = k_thread_create(&thread_nvm_data, stack_nvm,
                                      K_THREAD_STACK_SIZEOF(stack_nvm),
                                      OsTask_Nvm, NULL, NULL, NULL,
                                      PRIO_TASK_NVM, 0, K_NO_WAIT);
    k_thread_name_set(tid_nvm, "Task_NVM");
}