#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdlib.h>

#define TAREA_A_PRIO 1
#define TAREA_A_STK_SIZE 128
#define TAREA_B_PRIO 1
#define TAREA_B_STK_SIZE 128

static int variable_compartida = 0;
static SemaphoreHandle_t mutex;

void incTarea(void *parameter)
{
	int local_var;

	while (1)
	{
		if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) // Toma el mutex: si esta ocupado, la tarea se bloquea (no busy-wait)
		{
			local_var = variable_compartida;								 // Lee el recurso compartido a una variable local
			local_var++;													 // Incrementa la copia local
			vTaskDelay(pdMS_TO_TICKS(100 + (rand() % (500 - 100 + 1))));	 // Simula trabajo dentro de la seccion critica (entre 100 y 500 ms)
			variable_compartida = local_var;								 // Escribe el resultado de vuelta al recurso compartido
			printf("%s : %d\r\n", pcTaskGetName(NULL), variable_compartida); // Reporta el nombre de la tarea y el valor actual
			xSemaphoreGive(mutex);											 // Libera el mutex para que la otra tarea pueda entrar
		}
		taskYIELD(); // Cede CPU al terminar cada iteracion
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
	printf("Start\r\n");

	srand((unsigned)xTaskGetTickCount()); // Semilla del rand
	mutex = xSemaphoreCreateMutex();

	if (mutex != NULL) // Verifica que la creacion del mutex haya sido exitosa
	{
		xTaskCreate((TaskFunction_t)incTarea, "A", TAREA_A_STK_SIZE, NULL, TAREA_A_PRIO, NULL);
		xTaskCreate((TaskFunction_t)incTarea, "B", TAREA_B_STK_SIZE, NULL, TAREA_B_PRIO, NULL);
	}

	vTaskStartScheduler();
	vTaskDelete(NULL);
	while (1)
	{
	}
}
