#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
// #include <stdlib.h>

#define TAREA_A_PRIO 1
#define TAREA_A_STK_SIZE 128
#define TAREA_B_PRIO 1
#define TAREA_B_STK_SIZE 128

volatile uint32_t contador = 0;

void tarea_A(void *parameter)
{
	while (1)
	{
		for (uint8_t i = 0; i < 1000; i++)
		{
			contador++;
			vTaskDelay(pdMS_TO_TICKS(100 + (rand() % (500 - 100 + 1))));
		}
		printf("t1: ");
		printf("%u", contador);
		vTaskDelete(NULL);
	}
}

void tarea_B(void *parameter)
{
	while (1)
	{
		for (uint8_t i = 0; i < 1000; i++)
		{
			contador++;

			vTaskDelay(pdMS_TO_TICKS(100 + (rand() % (500 - 100 + 1))));
		}
		printf("t2: ");
		printf("%u", contador);
		vTaskDelete(NULL);
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
	printf("Start\n");

	xTaskCreate((TaskFunction_t)tarea_A, "A", TAREA_A_STK_SIZE, NULL, TAREA_A_PRIO, NULL);
	xTaskCreate((TaskFunction_t)tarea_B, "B", TAREA_B_STK_SIZE, NULL, TAREA_B_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
	{
	}
}