#include "tcp_server_task.h"
#include "cmsis_os.h"
#include "main.h"
#include "mprintf.h"
#include "socket_examples.h"

osThreadId tcpServerTaskHandle = NULL;
void StartTcpServerTask(void);
void TerminateTcpServerTask(void);
static void TcpServerFun(void const * argument);

extern void tcpserv(void *arg);
extern void threadSvr(void * arg);

void StartTcpServerTask(void) {
	if(tcpServerTaskHandle==NULL) {
		osThreadDef(tcpServerTask, threadSvr, osPriorityBelowNormal, 0, 1024+512);
		tcpServerTaskHandle = osThreadCreate(osThread(tcpServerTask),NULL);
	}
}

void TerminateTcpServerTask(void) {
	if(tcpServerTaskHandle!=NULL) {
		osStatus status = osThreadTerminate(tcpServerTaskHandle);
		if(status!=osOK) {
			mprintf("failed to terminate tcp server.");
		}
		tcpServerTaskHandle = NULL;
	}
}

void TcpServerFun(void const * argument) {

	tcpserv(NULL);
	for(;;) {
		HAL_GPIO_TogglePin(GPIOB, LED1_Pin|LED3_Pin|LED2_Pin);

		osDelay(1000);
	}
}
