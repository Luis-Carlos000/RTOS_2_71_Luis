#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

#define TAREA1_PRIO 2
#define TAREA1_STK 1024
#define TAREA2_PRIO 2
#define TAREA2_STK 1024

static const uint8_t queue_len = 5;
static QueueHandle_t q_datos;
static SemaphoreHandle_t uart_mutex;

void TAREA1(void *pvParameters)
{
	int contador = 0;
	while (1)
	{
		if (xQueueSend(q_datos, (void *)&contador, portMAX_DELAY) == pdTRUE) // Envia el contador a la cola
		{
			xSemaphoreTake(uart_mutex, portMAX_DELAY);
			printf("Productor ----> Enviando: %d\r\n", contador);
			xSemaphoreGive(uart_mutex);
			contador++;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void TAREA2(void *pvParameters)
{
	int recibido;
	while (1)
	{
		if (xQueueReceive(q_datos, (void *)&recibido, portMAX_DELAY) == pdTRUE) // Recibe el dato de la cola
		{
			xSemaphoreTake(uart_mutex, portMAX_DELAY);
			printf("Consumidor ----> Recibe: %d\r\n", recibido);
			xSemaphoreGive(uart_mutex);
		}
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
	USART_Printf_Init(115200);
	Delay_Ms(3000);

	printf("SystemClk: %u Hz\r\n", (unsigned)SystemCoreClock);
	printf("FreeRTOS Kernel Version: %s\r\n", tskKERNEL_VERSION_NUMBER);

	q_datos = xQueueCreate(queue_len, sizeof(int));
	uart_mutex = xSemaphoreCreateMutex();

	xTaskCreate(TAREA1, "TAREA1", TAREA1_STK, NULL, TAREA1_PRIO, NULL);
	xTaskCreate(TAREA2, "TAREA2", TAREA2_STK, NULL, TAREA2_PRIO, NULL);

	vTaskStartScheduler();
	vTaskDelete(NULL);
	while (1)
		;
}