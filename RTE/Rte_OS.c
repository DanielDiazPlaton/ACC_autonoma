#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "Rte_OS.h"
#include "Can_mis.h"

/* Tamaños de stack (2048 bytes para hilos con ISO-TP/printk) */
#define STACK_SIZE_diagnostic   1024
#define STACK_SIZE_control      1536
#define STACK_SIZE_RX           2048
#define STACK_SIZE_TX           2048
#define STACK_SIZE_NVM          1024

/* Prioridades (menor número = mayor prioridad) */
#define PRIORIDAD_RX            2
#define PRIO_TASK_control       4
#define PRIORIDAD_TX            5
#define PRIO_TASK_diagnostic    6
#define PRIO_TASK_NVM           8

K_THREAD_STACK_DEFINE(stack_diagnostic, STACK_SIZE_diagnostic);
K_THREAD_STACK_DEFINE(stack_control, STACK_SIZE_control);
K_THREAD_STACK_DEFINE(stack_comm_rx, STACK_SIZE_RX);
K_THREAD_STACK_DEFINE(stack_comm_tx, STACK_SIZE_TX);
K_THREAD_STACK_DEFINE(stack_nvm, STACK_SIZE_NVM);

static struct k_thread thread_diagnostic_data;
static struct k_thread thread_control_data;
static struct k_thread thread_comm_rx_data;
static struct k_thread thread_comm_tx_data;
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

/* Hilo 2: Control ACC (Genera mensajes de telemetría cada 1 segundo) */
void OsTask_control(void *p1, void *p2, void *p3)
{
    uint32_t cycle_count = 0;
    uint8_t sim_speed_kmh = 60;
    uint8_t sim_distance_m = 25;
    struct acc_tx_msg msg;

    while (1) {
        cycle_count++;
        sim_speed_kmh = (sim_speed_kmh >= 120) ? 60 : (sim_speed_kmh + 5);
        sim_distance_m = (sim_distance_m <= 10) ? 35 : (sim_distance_m - 2);

        /* Formatear trama de telemetría simulada del ACC */
        int written = snprintk((char *)msg.payload, ACC_MSG_MAX_LEN,
                               "ACC_CTRL #%u | Vel:%u km/h | Dist:%u m\n",
                               cycle_count, sim_speed_kmh, sim_distance_m);

        if (written > 0) {
            msg.len = (size_t)written;
            printk("[Task_control] Enviando a RTE TX -> %s", msg.payload);

            /* Pasar mensaje al buffer del RTE sin bloquear el lazo de control */
            if (k_msgq_put(&rte_acc_tx_msgq, &msg, K_NO_WAIT) != 0) {
                printk("[Task_control] Cola TX llena, mensaje descartado\n");
            }
        }

        k_msleep(1000);
    }
}

/* Hilo 3: Comunicación CAN RX */
void OsTask_comm_rx(void *p1, void *p2, void *p3)
{
    while (1) {
        /* Escucha tramas entrantes ISO-TP en el bus CAN */
        can_receive_data();
    }
}

/* Hilo 4: Comunicación CAN TX */
void OsTask_comm_tx(void *p1, void *p2, void *p3)
{
    struct acc_tx_msg tx_item;

    while (1) {
        /* Espera a que Task_control deposite un mensaje en la cola del RTE */
        if (k_msgq_get(&rte_acc_tx_msgq, &tx_item, K_FOREVER) == 0) {
            can_send_data(tx_item.payload, tx_item.len);
        }
    }
}

/* Hilo 5: Memoria No Volátil (NVM) */
void OsTask_Nvm(void *p1, void *p2, void *p3)
{
    while (1) {
        k_msleep(1000);
    }
}

void Rte_Start(void)
{
    /* Inicializar driver CAN antes de arrancar los hilos de comunicación */
    if (can_init_mis() != 0) {
        printk("[RTE] Fallo critico al inicializar CAN\n");
    }

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

    k_tid_t tid_nvm = k_thread_create(&thread_nvm_data, stack_nvm,
                                      K_THREAD_STACK_SIZEOF(stack_nvm),
                                      OsTask_Nvm, NULL, NULL, NULL,
                                      PRIO_TASK_NVM, 0, K_NO_WAIT);
    k_thread_name_set(tid_nvm, "Task_NVM");
}