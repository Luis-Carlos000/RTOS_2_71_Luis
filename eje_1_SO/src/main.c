#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED1_PRIO 1
#define LED1_STK_SIZE 256
#define LED2_PRIO 1
#define LED2_STK_SIZE 256

#define LED_PORT GPIOA
#define LED1_PIN GPIO_Pin_0
#define LED2_PIN GPIO_Pin_1

#define T_1 550
#define T_2 330

void GPIO_Leds_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);
}

void toggle_led01(void *parameter)
{
	while (1)
	{
		GPIO_SetBits(LED_PORT, LED1_PIN);	// Enciende el LED1
		vTaskDelay(pdMS_TO_TICKS(T_1));		// Mantiene el LED1 encendido 550 ms
		GPIO_ResetBits(LED_PORT, LED1_PIN); // Apaga el LED1
		vTaskDelay(pdMS_TO_TICKS(T_1));		// Mantiene el LED1 apagado 550 ms
	}
}

void toggle_led02(void *parameter)
{
	while (1)
	{
		GPIO_SetBits(LED_PORT, LED2_PIN);	// Enciende el LED2
		vTaskDelay(pdMS_TO_TICKS(T_2));		// Mantiene el LED2 encendido 330 ms
		GPIO_ResetBits(LED_PORT, LED2_PIN); // Apaga el LED2
		vTaskDelay(pdMS_TO_TICKS(T_2));		// Mantiene el LED2 apagado 330 ms
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

	GPIO_Leds_Init();

	xTaskCreate((TaskFunction_t)toggle_led01, "led01", LED1_STK_SIZE, NULL, LED1_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_led02, "led02", LED2_STK_SIZE, NULL, LED2_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
	{
	}
}