// Aqui es donde se crearan los hilos
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "Rte_OS.h"

// Aquí estoy definiendo el tamaño del stack y la prioridad de los hilos
#define STACK_SIZE_RX 1024
#define STACK_SIZE_TX 1024
#define STACK_SIZE_NVM      1024
#define PRIORIDAD_RX 5
#define PRIORIDAD_TX 5
#define PRIO_TASK_NVM       5


K_THREAD_STACK_DEFINE (stack_comm_rx, STACK_SIZE_RX);
K_THREAD_STACK_DEFINE (stack_comm_tx, STACK_SIZE_TX);
K_THREAD_STACK_DEFINE(stack_nvm, STACK_SIZE_NVM);
static struct k_thread thread_comm_rx_data;
static struct k_thread thread_comm_tx_data;
static struct k_thread thread_nvm_data;

void OsTask_comm_rx (void * p1, void * p2, void * p3)
{
    //datos que se ejectuan una sola vez

    while (1)
    {
        const char *tname = k_thread_name_get(k_current_get());
        printk("%lld ms] Ejecutando thread: %s, Prioridad: %d\n", 
        k_uptime_get(), 
        tname ? tname : "unnamed", 
        k_thread_priority_get(k_current_get())); 
        k_msleep(20);
    }// End while
}

void OsTask_comm_tx (void * p1, void * p2, void * p3)
{
    //datos que se ejectuan una sola vez

    while (1)
    {
        const char *tname = k_thread_name_get(k_current_get());
        printk("%lld ms] Ejecutando thread: %s, Prioridad: %d\n", 
        k_uptime_get(), 
        tname ? tname : "unnamed", 
        k_thread_priority_get(k_current_get())); 
        k_msleep(20);
    }// End while
}
void OsTask_Nvm(void *p1, void *p2, void *p3)
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
    // Inicializacion del OS
    k_tid_t tid_rx = k_thread_create(
        &thread_comm_rx_data,
        stack_comm_rx,
        K_THREAD_STACK_SIZEOF(stack_comm_rx),
        OsTask_comm_rx, 
        NULL, NULL, NULL,
        PRIORIDAD_RX,
        0,
        K_NO_WAIT);

    //Nombre task
    k_thread_name_set (tid_rx, "Task_RX");
    
    k_tid_t tid_tx = k_thread_create(
        &thread_comm_tx_data,
        stack_comm_tx,
        K_THREAD_STACK_SIZEOF(stack_comm_rx),
        OsTask_comm_tx, 
        NULL, NULL, NULL,
        PRIORIDAD_TX,
        0,
        K_NO_WAIT);
  //Nombre task
    k_thread_name_set (tid_tx, "Task_TX");
  
  // Inicializacion del OS
    k_tid_t tid_nvm = k_thread_create(&thread_nvm_data, stack_nvm,
                                    K_THREAD_STACK_SIZEOF(stack_nvm),
                                    OsTask_Nvm, NULL, NULL, NULL,
                                    PRIO_TASK_NVM, 0, K_NO_WAIT);


    // Nombre task
    k_thread_name_set(tid_nvm, "Task_NVM");

    

}