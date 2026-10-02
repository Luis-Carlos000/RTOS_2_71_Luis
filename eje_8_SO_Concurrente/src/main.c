#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"

#define TAREA_A_PRIO 1
#define TAREA_A_STK_SIZE 256
#define TAREA_B_PRIO 1
#define TAREA_B_STK_SIZE 256

char vectorNumeros[100];

volatile uint32_t contador = 0; // Recurso compartido protegido por el algoritmo de Dekker

void tarea_A(void *parameter)
{
	while (1)
	{
		for (uint8_t i = 0; i < 20; i++)
		{
			vectorNumeros[i] = (i / 2) + '0';
			i++;
			vectorNumeros[i] = 'a';
			contador++;
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}
}

void tarea_B(void *parameter)
{
	while (1)
	{
		// printf("%u ", contador);
		for (uint8_t i = 0; i < 20; i++)
		{
			vectorNumeros[i] = (i / 2) + '0';
			i++;
			vectorNumeros[i] = 'b';
			contador++;
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		if (contador >= 20)
		{
			for (contador = 0; contador < 20; contador++)
			{
				printf("%c", vectorNumeros[contador]);
				contador++;
				printf("%c ", vectorNumeros[contador]);
			}
			printf("\n");
			contador = 0;
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
	USART_Printf_Init(9600);
	// Delay_Ms(3000);
	// printf("Start\n");

	xTaskCreate((TaskFunction_t)tarea_A, "A", TAREA_A_STK_SIZE, NULL, TAREA_A_PRIO, NULL);
	xTaskCreate((TaskFunction_t)tarea_B, "B", TAREA_B_STK_SIZE, NULL, TAREA_B_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
	{
	}
}