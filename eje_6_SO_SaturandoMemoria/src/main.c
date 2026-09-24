#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

#define TASK_P0_PRIO 1

#define TASK_P0_STK_SIZE 1500

#define BUTTON_GPIO_PORT GPIOA
#define BUTTON_GPIO_PIN GPIO_Pin_4

TaskHandle_t TaskP0_Handler;

volatile uint8_t flag_p0 = 1;

void task_p0(void *pvParameters)
{
	while (1)
	{
		int a = 1;
		int b[1000];
		for (int i = 0; i < 1000; i++)
		{
			b[i] = a + 1;
		}
		printf("%d\n", b[0]);
		printf("-----Stack-----");
		int Watermarkrtosejemplo = uxTaskGetStackHighWaterMark(NULL);
		printf("%d\n", Watermarkrtosejemplo);
		printf("-----Heap-----");
		printf("%d\n", xPortGetFreeHeapSize());
		int *ptr = (int *)pvPortMalloc(1024 * sizeof(int));
		if (ptr == NULL)
		{
			printf("Sin espacio en memoria\n");
			vPortFree(NULL);
		}
		else
		{
			for (int i = 0; i < 1024; i++)
			{
				ptr[i] = 3;
			}
		}
		printf("Espacio libre despues de malloc (bytes): %d\n", xPortGetFreeHeapSize());
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
#ifdef NVIC_PriorityGroup_2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
#elif defined(NVIC_PriorityGroup_1)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
#endif
	SystemCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(9600);
	Delay_Ms(3000);

#if defined(CH32V30X)
	printf("ChipID: %08x\r\n", (unsigned)DBGMCU_GetCHIPID());
#else
	printf("DeviceID: %08x\r\n", (unsigned)DBGMCU_GetDEVID());
#endif

	xTaskCreate((TaskFunction_t)task_p0,
				(const char *)"p0",
				(uint16_t)TASK_P0_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P0_PRIO,
				(TaskHandle_t *)&TaskP0_Handler);

	vTaskStartScheduler();

	vTaskDelete(NULL);
	while (1)
	{
		printf("shouldn't run at here!!\n");
	}
}
