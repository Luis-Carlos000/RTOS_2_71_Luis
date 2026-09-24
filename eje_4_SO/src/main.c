#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

#define TASK_P0_PRIO 1
#define TASK_P1_PRIO 1
#define TASK_P2_PRIO 1
#define TASK_P3_PRIO 1
#define TASK_P4_PRIO 1
#define TASK_P5_PRIO 1
#define TASK_P6_PRIO 1

#define TASK_P0_STK_SIZE 256
#define TASK_P1_STK_SIZE 256
#define TASK_P2_STK_SIZE 256
#define TASK_P3_STK_SIZE 256
#define TASK_P4_STK_SIZE 256
#define TASK_P5_STK_SIZE 256
#define TASK_P6_STK_SIZE 256

#define BUTTON_GPIO_PORT GPIOA
#define BUTTON_GPIO_PIN GPIO_Pin_4

TaskHandle_t TaskP0_Handler;
TaskHandle_t TaskP1_Handler;
TaskHandle_t TaskP2_Handler;
TaskHandle_t TaskP3_Handler;
TaskHandle_t TaskP4_Handler;
TaskHandle_t TaskP5_Handler;
TaskHandle_t TaskP6_Handler;

volatile uint8_t flag_p0 = 1;
volatile uint8_t flag_p1 = 0;
volatile uint8_t flag_p2 = 0;
volatile uint8_t flag_p3 = 0;
volatile uint8_t flag_p4 = 0;
volatile uint8_t flag_p5 = 0;
volatile uint8_t flag_p6 = 0;

void GPIO_Button_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = BUTTON_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStructure);
}

void task_p0(void *pvParameters)
{
	while (1)
	{
		if (flag_p0 && (!(flag_p1 || flag_p2 || flag_p3 || flag_p4 || flag_p5 || flag_p6)))
		{
			if (GPIO_ReadInputDataBit(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
			{
				flag_p0 = 0;
				flag_p1 = 1;
				vTaskDelay(pdMS_TO_TICKS(300));
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p1(void *pvParameters)
{
	while (1)
	{
		if (flag_p1)
		{
			flag_p1 = 0;
			printf("Bajando taladro rapidamente hasta a1\r\n");
			vTaskDelay(pdMS_TO_TICKS(500));
			flag_p2 = 1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p2(void *pvParameters)
{
	while (1)
	{
		if (flag_p2)
		{
			flag_p2 = 0;
			printf("Girando taladro y bajando lentamente\r\n");
			vTaskDelay(pdMS_TO_TICKS(1000));
			if (rand() & 1)
			{
				printf("Pieza alta\r\n");
				flag_p5 = 1;
			}
			else
			{
				printf("Pieza baja\r\n");
				flag_p3 = 1;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p3(void *pvParameters)
{
	while (1)
	{
		if (flag_p3)
		{
			flag_p3 = 0;
			printf("Subiendo taladro lentamente y girando hasta a2\r\n");
			vTaskDelay(pdMS_TO_TICKS(800));
			flag_p4 = 1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p4(void *pvParameters)
{
	while (1)
	{
		if (flag_p4)
		{
			flag_p4 = 0;
			printf("Subiendo taladro rapidamente hasta a0\r\n");
			vTaskDelay(pdMS_TO_TICKS(500));
			flag_p0 = 1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p5(void *pvParameters)
{
	while (1)
	{
		if (flag_p5)
		{
			flag_p5 = 0;
			printf("Subiendo taladro lentamente y girando hasta a1\r\n");
			vTaskDelay(pdMS_TO_TICKS(800));
			flag_p6 = 1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void task_p6(void *pvParameters)
{
	while (1)
	{
		if (flag_p6)
		{
			flag_p6 = 0;
			printf("Bajando taladro lentamente y girando hasta a3\r\n");
			vTaskDelay(pdMS_TO_TICKS(800));
			flag_p3 = 1;
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

#if defined(CH32V30X)
	printf("ChipID: %08x\r\n", (unsigned)DBGMCU_GetCHIPID());
#else
	printf("DeviceID: %08x\r\n", (unsigned)DBGMCU_GetDEVID());
#endif

	GPIO_Button_Init();
	srand((unsigned)xTaskGetTickCount());

	xTaskCreate((TaskFunction_t)task_p0,
				(const char *)"p0",
				(uint16_t)TASK_P0_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P0_PRIO,
				(TaskHandle_t *)&TaskP0_Handler);

	xTaskCreate((TaskFunction_t)task_p1,
				(const char *)"p1",
				(uint16_t)TASK_P1_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P1_PRIO,
				(TaskHandle_t *)&TaskP1_Handler);

	xTaskCreate((TaskFunction_t)task_p2,
				(const char *)"p2",
				(uint16_t)TASK_P2_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P2_PRIO,
				(TaskHandle_t *)&TaskP2_Handler);

	xTaskCreate((TaskFunction_t)task_p3,
				(const char *)"p3",
				(uint16_t)TASK_P3_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P3_PRIO,
				(TaskHandle_t *)&TaskP3_Handler);

	xTaskCreate((TaskFunction_t)task_p4,
				(const char *)"p4",
				(uint16_t)TASK_P4_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P4_PRIO,
				(TaskHandle_t *)&TaskP4_Handler);

	xTaskCreate((TaskFunction_t)task_p5,
				(const char *)"p5",
				(uint16_t)TASK_P5_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P5_PRIO,
				(TaskHandle_t *)&TaskP5_Handler);

	xTaskCreate((TaskFunction_t)task_p6,
				(const char *)"p6",
				(uint16_t)TASK_P6_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_P6_PRIO,
				(TaskHandle_t *)&TaskP6_Handler);

	vTaskStartScheduler();

	vTaskDelete(NULL);
	while (1)
	{
		printf("shouldn't run at here!!\n");
	}
}