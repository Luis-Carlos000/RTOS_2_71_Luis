#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

#define MPU1_ADDR 0x68
#define MPU2_ADDR 0x69

#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT_H 0x3B

#define ACCEL_SCALE 16384.0f
#define EMA_ALPHA 0.2f
#define QUEUE_LEN 5
#define I2C_TIMEOUT 200000

#define TAREA_A1_PRIO 2
#define TAREA_A1_STK 512
#define TAREA_B1_PRIO 2
#define TAREA_B1_STK 512
#define TAREA_A2_PRIO 2
#define TAREA_A2_STK 512
#define TAREA_B2_PRIO 2
#define TAREA_B2_STK 512

typedef struct
{
	float ax;
	float ay;
	float az;
} AccelData;

static SemaphoreHandle_t i2c_mutex;
static SemaphoreHandle_t uart_mutex;
static QueueHandle_t q1_raw, q1_filt;
static QueueHandle_t q2_raw, q2_filt;

void I2C1_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	I2C_InitTypeDef I2C_InitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_InitStructure.I2C_ClockSpeed = 100000;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(I2C1, &I2C_InitStructure);
	I2C_Cmd(I2C1, ENABLE);
}

static uint8_t i2c_wait_free(void)
{
	uint32_t t = I2C_TIMEOUT;
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
	{
		if (--t == 0)
			return 0;
	}
	return 1;
}

static uint8_t i2c_wait_event(uint32_t event)
{
	uint32_t t = I2C_TIMEOUT;
	while (!I2C_CheckEvent(I2C1, event))
	{
		if (--t == 0)
			return 0;
	}
	return 1;
}

static void i2c_abort(void)
{
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
}

uint8_t I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t data)
{
	if (!i2c_wait_free())
		return 0;
	I2C_GenerateSTART(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		i2c_abort();
		return 0;
	}
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_SendData(I2C1, reg);
	if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_SendData(I2C1, data);
	if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_GenerateSTOP(I2C1, ENABLE);
	return 1;
}

uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *out)
{
	if (!i2c_wait_free())
		return 0;
	I2C_GenerateSTART(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		i2c_abort();
		return 0;
	}
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_SendData(I2C1, reg);
	if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		i2c_abort();
		return 0;
	}

	I2C_GenerateSTART(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		i2c_abort();
		return 0;
	}
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
	if (!i2c_wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED))
	{
		i2c_abort();
		return 0;
	}
	*out = I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	return 1;
}

uint8_t I2C_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
	if (!i2c_wait_free())
		return 0;
	I2C_GenerateSTART(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		i2c_abort();
		return 0;
	}
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	if (!i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		i2c_abort();
		return 0;
	}
	I2C_SendData(I2C1, reg);
	if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		i2c_abort();
		return 0;
	}

	I2C_GenerateSTART(I2C1, ENABLE);
	if (!i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT))
	{
		i2c_abort();
		return 0;
	}
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
	if (!i2c_wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
		i2c_abort();
		return 0;
	}

	for (uint8_t i = 0; i < len; i++)
	{
		if (i == (len - 1))
		{
			I2C_AcknowledgeConfig(I2C1, DISABLE);
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		if (!i2c_wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED))
		{
			i2c_abort();
			return 0;
		}
		buf[i] = I2C_ReceiveData(I2C1);
	}
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	return 1;
}

static uint8_t mpuInit(uint8_t addr)
{
	uint8_t who = 0;
	if (!I2C_ReadReg(addr, REG_WHO_AM_I, &who))
		return 0;
	if (who != 0x68)
		return 0;
	if (!I2C_WriteReg(addr, REG_PWR_MGMT_1, 0x00))
		return 0;
	return 1;
}

static uint8_t mpuReadAccel(uint8_t addr, AccelData *d)
{
	uint8_t raw[6];
	if (!I2C_ReadMulti(addr, REG_ACCEL_XOUT_H, raw, 6))
		return 0;
	d->ax = (int16_t)((raw[0] << 8) | raw[1]) / ACCEL_SCALE;
	d->ay = (int16_t)((raw[2] << 8) | raw[3]) / ACCEL_SCALE;
	d->az = (int16_t)((raw[4] << 8) | raw[5]) / ACCEL_SCALE;
	return 1;
}

static void fmt_centi(char *out, size_t n, float v)
{
	int neg = (v < 0.0f);
	if (neg)
		v = -v;
	int32_t s = (int32_t)(v * 100.0f + 0.5f);
	snprintf(out, n, "%s%ld.%02ld", neg ? "-" : "", (long)(s / 100), (long)(s % 100));
}

static void print_accel(const char *tag, const AccelData *d)
{
	char sx[14], sy[14], sz[14];
	fmt_centi(sx, sizeof(sx), d->ax);
	fmt_centi(sy, sizeof(sy), d->ay);
	fmt_centi(sz, sizeof(sz), d->az);
	xSemaphoreTake(uart_mutex, portMAX_DELAY);
	printf("%s X=%s Y=%s Z=%s\r\n", tag, sx, sy, sz);
	xSemaphoreGive(uart_mutex);
}

/* -------------------- Anillo 1 -------------------- */
void TAREA_A1(void *pvParameters)
{
	AccelData raw, filt;
	uint8_t ok;

	xSemaphoreTake(i2c_mutex, portMAX_DELAY);
	ok = mpuInit(MPU1_ADDR);
	xSemaphoreGive(i2c_mutex);

	if (!ok)
	{
		xSemaphoreTake(uart_mutex, portMAX_DELAY);
		printf("MPU1 no responde\r\n");
		xSemaphoreGive(uart_mutex);
		vTaskDelete(NULL);
	}

	while (1)
	{
		xSemaphoreTake(i2c_mutex, portMAX_DELAY);
		uint8_t r = mpuReadAccel(MPU1_ADDR, &raw);
		xSemaphoreGive(i2c_mutex);

		if (!r)
		{
			vTaskDelay(pdMS_TO_TICKS(200));
			continue;
		}

		xQueueSend(q1_raw, &raw, portMAX_DELAY);
		xQueueReceive(q1_filt, &filt, portMAX_DELAY);
		print_accel("MPU1 filt", &filt);

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void TAREA_B1(void *pvParameters)
{
	AccelData raw, filt;
	float ea = 0, ey = 0, ez = 0;
	uint8_t first = 1;

	while (1)
	{
		xQueueReceive(q1_raw, &raw, portMAX_DELAY);
		print_accel("MPU1 crudo", &raw);

		if (first)
		{
			ea = raw.ax;
			ey = raw.ay;
			ez = raw.az;
			first = 0;
		}
		else
		{
			ea = EMA_ALPHA * raw.ax + (1.0f - EMA_ALPHA) * ea;
			ey = EMA_ALPHA * raw.ay + (1.0f - EMA_ALPHA) * ey;
			ez = EMA_ALPHA * raw.az + (1.0f - EMA_ALPHA) * ez;
		}

		filt.ax = ea;
		filt.ay = ey;
		filt.az = ez;
		xQueueSend(q1_filt, &filt, portMAX_DELAY);
	}
}

/* -------------------- Anillo 2 -------------------- */
void TAREA_A2(void *pvParameters)
{
	AccelData raw, filt;
	uint8_t ok;

	xSemaphoreTake(i2c_mutex, portMAX_DELAY);
	ok = mpuInit(MPU2_ADDR);
	xSemaphoreGive(i2c_mutex);

	if (!ok)
	{
		xSemaphoreTake(uart_mutex, portMAX_DELAY);
		printf("MPU2 no responde\r\n");
		xSemaphoreGive(uart_mutex);
		vTaskDelete(NULL);
	}

	while (1)
	{
		xSemaphoreTake(i2c_mutex, portMAX_DELAY);
		uint8_t r = mpuReadAccel(MPU2_ADDR, &raw);
		xSemaphoreGive(i2c_mutex);

		if (!r)
		{
			vTaskDelay(pdMS_TO_TICKS(200));
			continue;
		}

		xQueueSend(q2_raw, &raw, portMAX_DELAY);
		xQueueReceive(q2_filt, &filt, portMAX_DELAY);
		print_accel("MPU2 filt", &filt);

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void TAREA_B2(void *pvParameters)
{
	AccelData raw, filt;
	float ea = 0, ey = 0, ez = 0;
	uint8_t first = 1;

	while (1)
	{
		xQueueReceive(q2_raw, &raw, portMAX_DELAY);
		print_accel("MPU2 crudo", &raw);

		if (first)
		{
			ea = raw.ax;
			ey = raw.ay;
			ez = raw.az;
			first = 0;
		}
		else
		{
			ea = EMA_ALPHA * raw.ax + (1.0f - EMA_ALPHA) * ea;
			ey = EMA_ALPHA * raw.ay + (1.0f - EMA_ALPHA) * ey;
			ez = EMA_ALPHA * raw.az + (1.0f - EMA_ALPHA) * ez;
		}

		filt.ax = ea;
		filt.ay = ey;
		filt.az = ez;
		xQueueSend(q2_filt, &filt, portMAX_DELAY);
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

	I2C1_Init();

	i2c_mutex = xSemaphoreCreateMutex();
	uart_mutex = xSemaphoreCreateMutex();
	q1_raw = xQueueCreate(QUEUE_LEN, sizeof(AccelData));
	q1_filt = xQueueCreate(QUEUE_LEN, sizeof(AccelData));
	q2_raw = xQueueCreate(QUEUE_LEN, sizeof(AccelData));
	q2_filt = xQueueCreate(QUEUE_LEN, sizeof(AccelData));

	if (i2c_mutex == NULL || uart_mutex == NULL ||
		q1_raw == NULL || q1_filt == NULL ||
		q2_raw == NULL || q2_filt == NULL)
	{
		printf("Error creando objetos RTOS\r\n");
		while (1)
			;
	}

	xTaskCreate(TAREA_A1, "A1", TAREA_A1_STK, NULL, TAREA_A1_PRIO, NULL);
	xTaskCreate(TAREA_B1, "B1", TAREA_B1_STK, NULL, TAREA_B1_PRIO, NULL);
	xTaskCreate(TAREA_A2, "A2", TAREA_A2_STK, NULL, TAREA_A2_PRIO, NULL);
	xTaskCreate(TAREA_B2, "B2", TAREA_B2_STK, NULL, TAREA_B2_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
		;
}