#ifndef RTE_OS_H
#define RTE_OS_H


/* Prototipos de inicialización del RTE / OS */
void Rte_Start(void);

/* Aqui van los prototipos de las funciones
 de los threads (OS Tasks) */

 void OsTask_comm_rx(void * p1, void * p2, void * p3);
 void OsTask_comm_tx(void * p1, void * p2, void * p3); 
 void OsTask_Nvm(void *p1, void *p2, void *p3);

#endif /* RTE_OS_H */