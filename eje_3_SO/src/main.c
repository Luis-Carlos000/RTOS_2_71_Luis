#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED1_PRIO 1
#define LED1_STK_SIZE 256
#define LED2_PRIO 1
#define LED2_STK_SIZE 256
#define LED3_PRIO 1
#define LED3_STK_SIZE 256
#define SEM_PRIO 1
#define SEM_STK_SIZE 256

#define LED_PORT GPIOA
#define LED_VERDE GPIO_Pin_0
#define LED_AMARILLO GPIO_Pin_1
#define LED_ROJO GPIO_Pin_2

#define T_1 2000
#define T_2 200

volatile int aux = 0;

void GPIO_Leds_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED_VERDE | LED_AMARILLO | LED_ROJO;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);
}

void toggle_led01(void *parameter)
{
	while (1)
	{
		if (aux == 0)							 // Si el semaforo esta en el estado 0
			GPIO_SetBits(LED_PORT, LED_VERDE);	 // Enciende el LED verde
		else									 // En cualquier otro estado
			GPIO_ResetBits(LED_PORT, LED_VERDE); // Apaga el LED verde
		vTaskDelay(pdMS_TO_TICKS(T_2));			 // Espera 200 ms antes de reevaluar
	}
}

void toggle_led02(void *parameter)
{
	while (1)
	{
		if (aux == 1) // Si el semaforo esta en el estado 1
		{
			GPIO_SetBits(LED_PORT, LED_AMARILLO);	// Enciende el LED amarillo
			vTaskDelay(pdMS_TO_TICKS(T_2));			// Espera 200 ms encendido
			GPIO_ResetBits(LED_PORT, LED_AMARILLO); // Apaga el LED amarillo
			vTaskDelay(pdMS_TO_TICKS(T_2));			// Espera 200 ms apagado (parpadeo)
		}
		else // En cualquier otro estado
		{
			GPIO_ResetBits(LED_PORT, LED_AMARILLO); // Mantiene el LED amarillo apagado
			vTaskDelay(pdMS_TO_TICKS(T_2));			// Espera 200 ms antes de reevaluar
		}
	}
}

void toggle_led03(void *parameter)
{
	while (1)
	{
		if (aux == 2)							// Si el semaforo esta en el estado 2
			GPIO_SetBits(LED_PORT, LED_ROJO);	// Enciende el LED rojo
		else									// En cualquier otro estado
			GPIO_ResetBits(LED_PORT, LED_ROJO); // Apaga el LED rojo
		vTaskDelay(pdMS_TO_TICKS(T_2));			// Espera 200 ms antes de reevaluar
	}
}

void toggle_semaforo(void *parameter)
{
	while (1)
	{
		aux = 0;						// Estado 0: solo LED verde encendido
		vTaskDelay(pdMS_TO_TICKS(T_1)); // Mantiene el estado 0 durante 2 s
		aux = 1;						// Estado 1: LED amarillo parpadeando
		vTaskDelay(pdMS_TO_TICKS(T_1)); // Mantiene el estado 1 durante 2 s
		aux = 2;						// Estado 2: solo LED rojo encendido
		vTaskDelay(pdMS_TO_TICKS(T_1)); // Mantiene el estado 2 durante 2 s
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

	GPIO_Leds_Init();

	xTaskCreate((TaskFunction_t)toggle_led01, "led01", LED1_STK_SIZE, NULL, LED1_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_led02, "led02", LED2_STK_SIZE, NULL, LED2_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_led03, "led03", LED3_STK_SIZE, NULL, LED3_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_semaforo, "sem", SEM_STK_SIZE, NULL, SEM_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
	{
	}
}