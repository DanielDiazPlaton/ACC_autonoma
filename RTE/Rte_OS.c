// Aqui es donde se crearan los hilos
//librerias Necesarias
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "Rte_OS.h"

// Esto define el stack y su prioridad
//hilo 1 diagnostico
#define STACK_SIZE_diagnostic      1024
#define PRIO_TASK_diagnostic       5
//hilo 2 control
#define STACK_SIZE_control      1024
#define PRIO_TASK_control       5

//Preparar la memoria de ambos hilos
K_THREAD_STACK_DEFINE(stack_diagnostic, STACK_SIZE_diagnostic);
K_THREAD_STACK_DEFINE(stack_control, STACK_SIZE_control);

static struct k_thread thread_diagnostic_data;
static struct k_thread thread_control_data;

//hilo 1 diagnostic
void OsTask_diagnostic(void *p1, void *p2, void *p3)
{
    // datos que se ejecutan una sola vez


    while(1)
    {
        const char *tname = k_thread_name_get(k_current_get());

        printk("[%lld ms] Ejecutando thread: %s, Prioridad: %d\n",
        k_uptime_get(),
        tname ? tname : "unnamed",
        k_thread_priority_get(k_current_get()));

        k_msleep(20);
    }  // End while
}

//hilo 2 control
void OsTask_control(void *p1, void *p2, void *p3)
{
    // datos que se ejecutan una sola vez


    while(1)
    {
        const char *tname = k_thread_name_get(k_current_get());

        printk("[%lld ms] Ejecutando thread: %s, Prioridad: %d\n",
        k_uptime_get(),
        tname ? tname : "unnamed",
        k_thread_priority_get(k_current_get()));

        k_msleep(20);
    }  // End while
}

void Rte_Start(void)
{
    // Inicializacion del OS diagnostic
    k_tid_t tid_diagnostic = k_thread_create(&thread_diagnostic_data, stack_diagnostic,
                                    K_THREAD_STACK_SIZEOF(stack_diagnostic),
                                    OsTask_diagnostic, NULL, NULL, NULL,
                                    PRIO_TASK_diagnostic, 0, K_NO_WAIT);


    // Nombre task
    k_thread_name_set(tid_diagnostic, "Task_diagnostic");

        // Inicializacion del OS control
    k_tid_t tid_control = k_thread_create(&thread_control_data, stack_control,
                                    K_THREAD_STACK_SIZEOF(stack_control),
                                    OsTask_control, NULL, NULL, NULL,
                                    PRIO_TASK_control, 0, K_NO_WAIT);


    // Nombre task
    k_thread_name_set(tid_control, "Task_control");
}