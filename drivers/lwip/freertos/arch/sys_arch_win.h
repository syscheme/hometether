// this is usually named as sys_arch.h for lwip portal

#ifndef __ARCH_SYS_ARCH_WIN_H__
#define __ARCH_SYS_ARCH_WIN_H__

#define _WINSOCKAPI_ // to exclude Windows .h definitions

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef __cplusplus
}
#endif

#ifndef NULL
#define NULL 0
#endif // NULL

#define SYS_MBOX_NULL ((xQueueHandle)NULL)
#define SYS_SEM_NULL  ((xSemaphoreHandle)NULL)

typedef xSemaphoreHandle sys_sem_t;
typedef xSemaphoreHandle sys_mutex_t;
typedef xQueueHandle sys_mbox_t;
typedef xTaskHandle sys_thread_t;

#define sys_mbox_valid(sem)        ( ( (*sem) == NULL) ? pdFALSE : pdTRUE )
#define sys_mbox_set_invalid(sem)  ( (*sem) = NULL )
#define sys_sem_valid(sem)         ( ( (*sem) == NULL) ? pdFALSE : pdTRUE )
#define sys_sem_set_invalid(sem)   ( (*sem) = NULL )

// let sys.h use binary semaphores for mutexes
#define LWIP_COMPAT_MUTEX 0

#ifndef MAX_QUEUE_ENTRIES
#define MAX_QUEUE_ENTRIES 100
#endif

#endif // __ARCH_SYS_ARCH_WIN_H__

