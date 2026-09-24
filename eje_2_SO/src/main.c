#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED1_PRIO 2
#define LED1_STK_SIZE 256
#define LED2_PRIO 1
#define LED2_STK_SIZE 256
#define BOTON_PRIO 1
#define BOTON_STK_SIZE 256

#define LED_PORT GPIOA
#define LED1_PIN GPIO_Pin_0
#define LED2_PIN GPIO_Pin_1

#define BOTON_PORT GPIOA
#define BOTON_PIN GPIO_Pin_4

#define T_1 500
#define T_2 500

void GPIO_Init_All(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BOTON_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(BOTON_PORT, &GPIO_InitStructure);
}

void toggle_led01(void *parameter)
{
	while (1)
	{
		GPIO_SetBits(LED_PORT, LED1_PIN);	// Enciende el LED1
		printf("Led 1 encendido\r\n");		// Reporta por serie el encendido del LED1
		vTaskDelay(pdMS_TO_TICKS(T_1));		// Mantiene LED1 encendido 500 ms
		GPIO_ResetBits(LED_PORT, LED1_PIN); // Apaga el LED1
		vTaskDelay(pdMS_TO_TICKS(T_1));		// Mantiene LED1 apagado 500 ms
	}
}

void toggle_led02(void *parameter)
{
	while (1)
	{
		GPIO_SetBits(LED_PORT, LED2_PIN);	// Enciende el LED2
		vTaskDelay(pdMS_TO_TICKS(T_2));		// Mantiene LED2 encendido 500 ms antes de reportar
		printf("Led 2 encendido\r\n");		// Reporta por serie el encendido del LED2
		GPIO_ResetBits(LED_PORT, LED2_PIN); // Apaga el LED2
		vTaskDelay(pdMS_TO_TICKS(T_2));		// Mantiene LED2 apagado 500 ms
	}
}

void toggle_boton01(void *parameter)
{
	while (1)
	{
		uint8_t estado_boton = GPIO_ReadInputDataBit(BOTON_PORT, BOTON_PIN); // Lee el estado del boton (IPU: 0 = presionado)
		if (estado_boton == 0)												 // Si el boton esta presionado
		{
			printf("boton presionado\r\n"); // Reporta por serie la presion del boton
		}
		vTaskDelay(pdMS_TO_TICKS(10));
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

	GPIO_Init_All();

	xTaskCreate((TaskFunction_t)toggle_led01, "led01", LED1_STK_SIZE, NULL, LED1_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_led02, "led02", LED2_STK_SIZE, NULL, LED2_PRIO, NULL);
	xTaskCreate((TaskFunction_t)toggle_boton01, "boton01", BOTON_STK_SIZE, NULL, BOTON_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
	{
	}
}