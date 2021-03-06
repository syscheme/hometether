// usually this is named as sys_arch.c under lwip portal

#include "sys_arch.h"

// below is refer to FreeRTOS\Demo\lwIP_AVR32_UC3\NETWORK\lwip-port\AT32UC3A\sys_arch.c

// =====section 1. about the eth configuration for the application====================
/*
#ifndef _CONF_ETH_H_
#define _CONF_ETH_H_

// define stack size for WEB server task
#define lwipBASIC_WEB_SERVER_STACK_SIZE   256

// define stack size for TFTP server task
#define lwipBASIC_TFTP_SERVER_STACK_SIZE  1024

// define stack size for SMTP Client task
#define lwipBASIC_SMTP_CLIENT_STACK_SIZE  256

// define stack size for lwIP task
#define lwipINTERFACE_STACK_SIZE          512

// define stack size for netif task
#define netifINTERFACE_TASK_STACK_SIZE    256

// define WEB server priority
#define ethWEBSERVER_PRIORITY             ( tskIDLE_PRIORITY + 2 )

// define TFTP server priority
#define ethTFTPSERVER_PRIORITY            ( tskIDLE_PRIORITY + 3 )

// define SMTP Client priority
#define ethSMTPCLIENT_PRIORITY            ( tskIDLE_PRIORITY + 5 )

// define lwIP task priority
#define lwipINTERFACE_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )

// define netif task priority
#define netifINTERFACE_TASK_PRIORITY      ( configMAX_PRIORITIES - 1 )

// Number of threads that can be started with sys_thread_new()
#define SYS_THREAD_MAX                      6

// LED used by the ethernet task, toggled on each activation
#define webCONN_LED                         7

// Phy Address (set through strap options)
#define ETHERNET_CONF_PHY_ADDR             0x01
#define ETHERNET_CONF_PHY_ID               0x20005C90

// Number of receive buffers
#define ETHERNET_CONF_NB_RX_BUFFERS        20

// USE_RMII_INTERFACE must be defined as 1 to use an RMII interface, or 0 to use an MII interface.
#define ETHERNET_CONF_USE_RMII_INTERFACE   1

// Number of Transmit buffers
#define ETHERNET_CONF_NB_TX_BUFFERS        10

// Size of each Transmit buffer.
#define ETHERNET_CONF_TX_BUFFER_SIZE       512

// Clock definition
#define ETHERNET_CONF_SYSTEM_CLOCK         48000000

// Use Auto Negociation to get speed and duplex
#define ETHERNET_CONF_AN_ENABLE                      1

// Do not use auto cross capability
#define ETHERNET_CONF_AUTO_CROSS_ENABLE              0
// use direct cable
#define ETHERNET_CONF_CROSSED_LINK                   0


// ethernet default parameters
// MAC address definition.  The MAC address must be unique on the network.
#define ETHERNET_CONF_ETHADDR0                        0x00
#define ETHERNET_CONF_ETHADDR1                        0x04
#define ETHERNET_CONF_ETHADDR2                        0x25
#define ETHERNET_CONF_ETHADDR3                        0x40
#define ETHERNET_CONF_ETHADDR4                        0x40
#define ETHERNET_CONF_ETHADDR5                        0x40

// The IP address being used.
#define ETHERNET_CONF_IPADDR0                         192
#define ETHERNET_CONF_IPADDR1                         168
#define ETHERNET_CONF_IPADDR2                         0
#define ETHERNET_CONF_IPADDR3                         2

// The gateway address being used.
#define ETHERNET_CONF_GATEWAY_ADDR0                   192
#define ETHERNET_CONF_GATEWAY_ADDR1                   168
#define ETHERNET_CONF_GATEWAY_ADDR2                   0
#define ETHERNET_CONF_GATEWAY_ADDR3                   1

// The network mask being used.
#define ETHERNET_CONF_NET_MASK0                       255
#define ETHERNET_CONF_NET_MASK1                       255
#define ETHERNET_CONF_NET_MASK2                       255
#define ETHERNET_CONF_NET_MASK3                       0

#endif
*/

// lwIP includes.
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

// Message queue constants.
#define archMESG_QUEUE_LENGTH   ( 6 )
#define archPOST_BLOCK_TIME_MS  ( ( unsigned portLONG ) 10000 )

struct timeoutlist 
{
  struct sys_timeouts timeouts;
  xTaskHandle pid;
};

static struct timeoutlist timeoutlist[SYS_THREAD_MAX];
static u16_t nextthread = 0;
int intlevel = 0;

extern void ethernetif_input( void * pvParameters );

// -----------------------------------------------------------------------------------
//  Creates an empty mailbox.
sys_mbox_t sys_mbox_new(void)
{
	xQueueHandle mbox;
	mbox = xQueueCreate(archMESG_QUEUE_LENGTH, sizeof(void *));
	return mbox;
}

// -----------------------------------------------------------------------------------
// Deallocates a mailbox. If there are messages still present in the mailbox when the
// mailbox is deallocated, it is an indication of a programming error in lwIP and the
// developer should be notified.
void sys_mbox_free(sys_mbox_t mbox)
{
	if (uxQueueMessagesWaiting(mbox))
	{
		// Line for breakpoint.  Should never break here!
		__asm__ __volatile__("nop");
	}

	vQueueDelete(mbox);
}

// -----------------------------------------------------------------------------------
// Posts the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t mbox, void *data)
{   
  xQueueSend( mbox, &data, ( portTickType ) ( archPOST_BLOCK_TIME_MS / portTICK_RATE_MS ) );
}

// -----------------------------------------------------------------------------------
// Blocks the thread until a message arrives in the mailbox, but does not block the
// thread longer than "timeout" milliseconds (similar to the sys_arch_sem_wait()
// function). The "msg" argument is a result parameter that is set by the function
// (i.e., by doing "*msg = ptr"). The "msg" parameter maybe NULL to indicate that the
// message should be dropped.
//
// The return values are the same as for the sys_arch_sem_wait() function:
// Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a timeout.
//
// Note that a function with a similar name, sys_mbox_fetch(), is implemented by lwIP. 
u32_t sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)
{
	void *dummyptr;
	portTickType StartTime, EndTime, Elapsed;

	StartTime = xTaskGetTickCount();

	if (msg == NULL)
		msg = &dummyptr;

	if (timeout != 0)
	{
		if (pdTRUE == xQueueReceive(mbox, &(*msg), timeout))
		{
			EndTime = xTaskGetTickCount();
			Elapsed = EndTime - StartTime;
			if (Elapsed == 0)
				Elapsed = 1;
			return (Elapsed);
		}

		// timed out blocking for message
		*msg = NULL;
		return SYS_ARCH_TIMEOUT;
	}
	
	// block forever for a message.
	while (pdTRUE != xQueueReceive(mbox, &(*msg), 10000)); // time is arbitrary

	EndTime = xTaskGetTickCount();
	Elapsed = EndTime - StartTime;
	if (Elapsed == 0)
		Elapsed = 1;

	return (Elapsed); // return time blocked TBD test
}

// -----------------------------------------------------------------------------------
// Creates and returns a new semaphore. The "count" argument specifies
// the initial state of the semaphore. TBD finish and test
sys_sem_t sys_sem_new(u8_t count)
{
	xSemaphoreHandle  xSemaphore = NULL;

	portENTER_CRITICAL();
	vSemaphoreCreateBinary(xSemaphore);
	if (xSemaphore == NULL)
		return NULL;  // TBD need assert

	if (count == 0)  // Means we want the sem to be unavailable at init state.
		xSemaphoreTake(xSemaphore, 1);

	portEXIT_CRITICAL();

	return xSemaphore;
}

// -----------------------------------------------------------------------------------
// Blocks the thread while waiting for the semaphore to be signaled. If the "timeout"
// argument is non-zero, the thread should only be blocked for the specified time (
// measured in milliseconds).
//
// If the timeout argument is non-zero, the return value is the number of milliseconds
// spent waiting for the semaphore to be signaled. If the semaphore wasn't signaled
// within the specified time, the return value is SYS_ARCH_TIMEOUT. If the thread
// didn't have to wait for the semaphore (i.e., it was already signaled), the function
// may return zero.
//
// Notice that lwIP implements a function with a similar name, sys_sem_wait(), that
// uses the sys_arch_sem_wait() function.
u32_t sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
	portTickType StartTime, EndTime, Elapsed;

	StartTime = xTaskGetTickCount();

	if (timeout != 0)
	{
		if (xSemaphoreTake(sem, timeout) != pdTRUE)
			return SYS_ARCH_TIMEOUT;

		EndTime = xTaskGetTickCount();
		Elapsed = EndTime - StartTime;
		if (Elapsed == 0)
			Elapsed = 1;

		return (Elapsed); // return time blocked TBD test
	}
	
	// must block without a timeout
	while (xSemaphoreTake(sem, 10000) != pdTRUE);
	EndTime = xTaskGetTickCount();
	Elapsed = EndTime - StartTime;
	if (Elapsed == 0)
		Elapsed = 1;

	return (Elapsed); // return time blocked
}

// -----------------------------------------------------------------------------------
// Signals a semaphore
void sys_sem_signal(sys_sem_t sem)
{
	xSemaphoreGive(sem);
}

// -----------------------------------------------------------------------------------
// Deallocates a semaphore
void sys_sem_free(sys_sem_t sem)
{
	vQueueDelete(sem);
}

// -----------------------------------------------------------------------------------
// Initialize sys arch
void sys_init(void)
{

	int i;

	// Initialize the the per-thread sys_timeouts structures make sure there are no
	// valid pids in the list
	for (i = 0; i < SYS_THREAD_MAX; i++)
	{
		timeoutlist[i].pid = 0;
		timeoutlist[i].timeouts.next = NULL;
	}

	// keep track of how many threads have been created
	nextthread = 0;
}

// -----------------------------------------------------------------------------------
// Returns a pointer to the per-thread sys_timeouts structure. In lwIP, each thread
// has a list of timeouts which is represented as a linked list of sys_timeout
// structures. The sys_timeouts structure holds a pointer to a linked list of
// timeouts. This function is called by the lwIP timeout scheduler and must not
// return a NULL value. 
//
// In a single threaded sys_arch implementation, this function will simply return a
// pointer to a global sys_timeouts variable stored in the sys_arch module.
struct sys_timeouts* sys_arch_timeouts(void)
{
	int i;
	xTaskHandle pid;
	struct timeoutlist *tl;

	pid = xTaskGetCurrentTaskHandle();

	for (i = 0; i < nextthread; i++)
	{
		tl = &(timeoutlist[i]);
		if (tl->pid == pid)
			return &(tl->timeouts);
	}


	// If we're here, this means the scheduler gave the focus to the task as it was
	// being created(because of a higher priority). Since timeoutlist[] update is
	// done just after the task creation, the array is not up-to-date.
	// => the last array entry must be the one of the current task.
	return(&(timeoutlist[nextthread].timeouts));
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// TBD 
// -----------------------------------------------------------------------------------
// Starts a new thread with priority "prio" that will begin its execution in the
// function "thread()". The "arg" argument will be passed as an argument to the
// thread() function. The id of the new thread is returned. Both the id and
// the priority are system dependent.
sys_thread_t sys_thread_new(void(*thread)(void *arg), void *arg, int prio)
{
	xTaskHandle CreatedTask;
	int result = pdFAIL;
	static int iCall = 0;

	if (thread == ethernetif_input)
		result = xTaskCreate(thread, (signed portCHAR *) "ETHINT", netifINTERFACE_TASK_STACK_SIZE, arg, prio, &CreatedTask);
	else if (iCall == 0)
	{
		/* The first time this is called we are creating the lwIP handler. */
		result = xTaskCreate(thread, (signed portCHAR *) "lwIP", lwipINTERFACE_STACK_SIZE, arg, prio, &CreatedTask);
		iCall++;
	}
#if (HTTP_USED == 1)
	else if (thread == vBasicWEBServer)
	{
		result = xTaskCreate(thread, (signed portCHAR *) "WEB", lwipBASIC_WEB_SERVER_STACK_SIZE, arg, prio, &CreatedTask);
	}
#endif
#if (TFTP_USED == 1)
	else if (thread == vBasicTFTPServer)
	{
		result = xTaskCreate(thread, (signed portCHAR *) "TFTP", lwipBASIC_TFTP_SERVER_STACK_SIZE, arg, prio, &CreatedTask);
	}
#endif
#if (SMTP_USED == 1)
	else if (thread == vBasicSMTPClient)
	{
		result = xTaskCreate(thread, (signed portCHAR *) "SMTP", lwipBASIC_SMTP_CLIENT_STACK_SIZE, arg, prio, &CreatedTask);
	}
#endif

	// For each task created, store the task handle (pid) in the timers array.
	// This scheme doesn't allow for threads to be deleted
	timeoutlist[nextthread++].pid = CreatedTask;

	if (result == pdPASS)
		return CreatedTask;

	return NULL;
}

// This optional function does a "fast" critical region protection and returns the previous
// protection level. This function is only called during very short critical regions. An
// embedded system which supports ISR-based drivers might want to implement this function
// by disabling interrupts. Task-based systems might want to implement this by using a
// mutex or disabling tasking. This function should support recursive calls from the same
// task or interrupt. In other words, sys_arch_protect() could be called while already
// protected. In that case the return value indicates that it is already protected.
//
//  sys_arch_protect() is only required if your port is supporting an operating system.
sys_prot_t sys_arch_protect(void)
{
	vPortEnterCritical();
	return 1;
}

// This optional function does a "fast" set of critical region protection to the value
// specified by pval. See the documentation for sys_arch_protect() for more information.
// This function is only required if your port is supporting an operating system.
void sys_arch_unprotect(sys_prot_t pval)
{
	(void)pval;
	vPortExitCritical();
}

