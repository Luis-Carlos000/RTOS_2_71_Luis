#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define TAREA01_PRIO 1
#define TAREA01_STK_SIZE 256
#define TAREA02_PRIO 2
#define TAREA02_STK_SIZE 256
#define CONTROL_PRIO 1
#define CONTROL_STK_SIZE 256

const char msg[] = "hola mundo UPIIZ";

static TaskHandle_t tarea_1 = NULL;
static TaskHandle_t tarea_2 = NULL;
static TaskHandle_t tarea_control = NULL;

void tarea01(void *parameter)
{
	int msg_len = strlen(msg);

	while (1)
	{
		printf("\r\n");
		for (int i = 0; i < msg_len; i++)
		{
			printf("%c\r\n", msg[i]);	  // Imprime el caracter actual
			vTaskDelay(pdMS_TO_TICKS(1)); // Espera 1 ms entre caracteres
		}
		printf("\r\n");					 // Salto de linea despues del mensaje
		vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1 s antes de repetir
	}
}

void tarea02(void *parameter)
{
	while (1)
	{
		printf("+\r\n");				// Imprime el caracter '+'
		vTaskDelay(pdMS_TO_TICKS(100)); // Espera 100 ms
	}
}

void tarea_control_fn(void *parameter)
{
	for (int i = 0; i < 3; i++) // Repite 3 veces el ciclo suspender/reanudar
	{
		vTaskSuspend(tarea_2);			 // Suspende tarea02 (deja de imprimir '+')
		vTaskDelay(pdMS_TO_TICKS(2000)); // Espera 2 s con tarea02 suspendida
		vTaskResume(tarea_2);			 // Reanuda tarea02
		vTaskDelay(pdMS_TO_TICKS(2000)); // Espera 2 s con tarea02 activa
	}
	if (tarea_1 != NULL)
	{
		vTaskDelete(tarea_1); // Elimina tarea01 definitivamente
		tarea_1 = NULL;		  // Limpia el handler para evitar uso colgado
	}
	vTaskDelete(NULL); // La tarea de control se elimina a si misma al terminar
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
	Delay_Ms(1000);

	printf("\r\n--- Mensaje de inicio ---\r\n");

	xTaskCreate((TaskFunction_t)tarea01,
				"tarea01",
				TAREA01_STK_SIZE,
				NULL,
				TAREA01_PRIO,
				&tarea_1);

	xTaskCreate((TaskFunction_t)tarea02,
				"tarea02",
				TAREA02_STK_SIZE,
				NULL,
				TAREA02_PRIO,
				&tarea_2);

	xTaskCreate((TaskFunction_t)tarea_control_fn,
				"control",
				CONTROL_STK_SIZE,
				NULL,
				CONTROL_PRIO,
				&tarea_control);

	vTaskStartScheduler();

	while (1)
	{
	}
}