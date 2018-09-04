#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

#define mainREGION_1_SIZE	2001
#define mainREGION_2_SIZE	18005
#define mainREGION_3_SIZE	1007
static void  prvInitialiseHeap( void );
volatile uint8_t _interruptDepth=0;

void Task_Main(void* p_arg);

// -----------------------------
// global declares
// -----------------------------
#define TaskPrio_tMain                (TaskPrio_Base +0)
#define TaskStkSz_tMain               (configMINIMAL_STACK_SIZE) //96
void    Task_tMain(void* p_arg);

static void printpacket(const char* hint, pbuf* packet)
{
	int i=0;
	printf("%s packet[%d]: ", hint, packet->tot_len);
	for (; packet; packet = packet->next)
	{
		for(i=0; i<packet->len; i++)
			printf("%02x ", packet->payload[i]);
	}
	printf("\r\n");
}

// -----------------------------
// dummys that were original in BSP.c
// -----------------------------
DS18B20 ds18b20s[CHANNEL_SZ_DS18B20];
USART TTY1 = {NULL, {NULL,} , {NULL,}};
uint32_t BSP_typeId, BSP_nodeId = 0;

void BSP_Init(void)
{}

void PECU_setRelay(uint8_t id, uint8_t on)
{
}

uint16_t MotionState(void)
{
	uint16_t state =0;
	return state;
}

void htddl_OnReceived(HtNetIf* netif, const hwaddr_t src, pbuf* data)
{
	printpacket("htddl_OnReceived()", data);
}

hwaddr_t bindaddr = {0x12,0x34,0x56,0x78};

HtNetIf netifUplink;

////////////////////
void HtNode_OnObjectQueried(HtNetIf* netif, uint8_t objectId, uint8_t subIdx)
{
	vTaskSuspendAll();
	printf("HtNode_OnObjectQueried() object[%d:%d]\n", objectId, subIdx);
	xTaskResumeAll();
}

void HtNode_OnObjectChanged(HtNetIf* netif, uint8_t objectId, uint8_t subIdx, const uint8_t* newValue, uint8_t vlen)
{
	vTaskSuspendAll();
	printf("HtNode_OnObjectChanged() object[%d:%d]: ", objectId, subIdx);
	while (vlen--)
		printf("%02x ", *newValue++);
	printf("\n");
	xTaskResumeAll();
}

// ---------------------------------------------------------------------------
// dummy for HtEdge
// ---------------------------------------------------------------------------
void HtEdge_OnNodeAdvertize(HtNetIf* netif, uint8_t eNid, NodeId_t theNode, NodeId_t nidBy)
{
	// TODO: collect (orphanNodeId, srcNodeId, ttl, count++)
	// and wait for a while, then pick the path has maximal count and maximal ttl
	// overwrite the orphan->srcNodeId by calling LRUMap_set(downLinks, theParty, srcNodeId) for the picked path
	// ToUSART: ad:<netif->name>:<eNid>:<theNode>:<nidBy>
	char atmsg[40] = "\0", *p =atmsg;
	snprintf(p, sizeof(atmsg)-3, "AD+%04x:%x;%04x;%s\n", theNode, eNid, nidBy, netif->name); 
	trace("OnNodeAdvertize() %s", atmsg);
}

void HtNode_OnObjectValues(uint8_t eNid, uint8_t objectId, uint8_t subIdx, uint8_t* value, uint8_t vlen)
{
	char atmsg[40] = "\0", *p =atmsg;
	p += snprintf(p, sizeof(atmsg)-3, "OV+%x/%x+%xL%x:", eNid, objectId, subIdx, vlen); 
	while (vlen--)
		p += snprintf(p, atmsg + sizeof(atmsg)-3 -p, "%02x", *(value++));
	*p++ = '\n'; *p='\0';

	trace("OnObjectValues() %s", atmsg);
}

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
int main(void)
{
#ifdef DEBUG
    debug();
#endif

	// initialize the board
	BSP_Init();

	// create the main task.
	createTask(Main, NULL);

	// The suicide tasks must be created last as they need to know how many
	// tasks were running prior to their creation in order to ascertain whether
	// or not the correct/expected number of tasks are running at any given time.
    // vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );
	
	// run the scheduler
	runTaskScheduler();

	// Will only get here if there was not enough heap space to create the idle task. 
	return 0;
}
