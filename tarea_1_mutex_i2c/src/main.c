#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

#define MPU1_ADDR 0x68
#define MPU2_ADDR 0x69
#define OLED_ADDR 0x3C

#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT_H 0x3B

#define ACCEL_SCALE 16384.0f

#define SCREEN_WIDTH 128
#define PAGES 4

#define MUESTRAS_PROMEDIO 10

#define MPU1_PRIO 2
#define MPU1_STK 384
#define MPU2_PRIO 2
#define MPU2_STK 384
#define DISP_PRIO 2
#define DISP_STK 384

static SemaphoreHandle_t i2c_mutex;

static uint8_t oledBuf[SCREEN_WIDTH * PAGES];

static uint8_t mpu1_ok = 0;
static uint8_t mpu2_ok = 0;

static float mpu1_ax_b[MUESTRAS_PROMEDIO], mpu1_ay_b[MUESTRAS_PROMEDIO], mpu1_az_b[MUESTRAS_PROMEDIO];
static float mpu2_ax_b[MUESTRAS_PROMEDIO], mpu2_ay_b[MUESTRAS_PROMEDIO], mpu2_az_b[MUESTRAS_PROMEDIO];
static float mpu1_ax_s = 0, mpu1_ay_s = 0, mpu1_az_s = 0;
static float mpu2_ax_s = 0, mpu2_ay_s = 0, mpu2_az_s = 0;
static uint8_t idx1 = 0, idx2 = 0;

static const uint8_t font5x7[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
	0x14, 0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
	0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00,
	0x00, 0x41, 0x22, 0x1C, 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
	0x00, 0x50, 0x30, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
	0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00,
	0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4B, 0x31, 0x18, 0x14, 0x12, 0x7F, 0x10,
	0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03,
	0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x36, 0x36, 0x00, 0x00,
	0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x14, 0x14, 0x14, 0x14, 0x14,
	0x41, 0x22, 0x14, 0x08, 0x00, 0x02, 0x01, 0x51, 0x09, 0x06, 0x32, 0x49, 0x79, 0x41, 0x3E,
	0x7E, 0x11, 0x11, 0x11, 0x7E, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
	0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x01, 0x01,
	0x3E, 0x41, 0x41, 0x51, 0x32, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00,
	0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40,
	0x7F, 0x02, 0x04, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46,
	0x46, 0x49, 0x49, 0x49, 0x31, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
	0x1F, 0x20, 0x40, 0x20, 0x1F, 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x63, 0x14, 0x08, 0x14, 0x63,
	0x03, 0x04, 0x78, 0x04, 0x03, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x7F, 0x41, 0x41,
	0x02, 0x04, 0x08, 0x10, 0x20, 0x41, 0x41, 0x7F, 0x00, 0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x01, 0x02, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78,
	0x7F, 0x48, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x20, 0x38, 0x44, 0x44, 0x48, 0x7F,
	0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0x7E, 0x09, 0x01, 0x02, 0x08, 0x14, 0x54, 0x54, 0x3C,
	0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x20, 0x40, 0x44, 0x3D, 0x00,
	0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x18, 0x04, 0x78,
	0x7C, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0x7C, 0x14, 0x14, 0x14, 0x08,
	0x08, 0x14, 0x14, 0x18, 0x7C, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48, 0x54, 0x54, 0x54, 0x20,
	0x04, 0x3F, 0x44, 0x40, 0x20, 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20, 0x1C,
	0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x0C, 0x50, 0x50, 0x50, 0x3C,
	0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00,
	0x00, 0x41, 0x36, 0x08, 0x00, 0x08, 0x08, 0x2A, 0x1C, 0x08};

void I2C1_Init(void);
void I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg);
void I2C_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
void OLED_Init(void);
void OLED_ClearBuf(void);
void OLED_DrawChar(uint8_t x, uint8_t page, char c);
void OLED_DrawString(uint8_t x, uint8_t page, const char *s);
void OLED_Flush(void);

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

void I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t data)
{
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
		;
	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(I2C1, reg);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	I2C_SendData(I2C1, data);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	I2C_GenerateSTOP(I2C1, ENABLE);
}

uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg)
{
	uint8_t data;
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
		;
	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(I2C1, reg);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;

	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
		;
	data = I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	return data;
}

void I2C_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
		;
	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	I2C_SendData(I2C1, reg);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;

	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
	while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;

	for (uint8_t i = 0; i < len; i++)
	{
		if (i == (len - 1))
		{
			I2C_AcknowledgeConfig(I2C1, DISABLE);
			I2C_GenerateSTOP(I2C1, ENABLE);
		}
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
			;
		buf[i] = I2C_ReceiveData(I2C1);
	}
	I2C_AcknowledgeConfig(I2C1, ENABLE);
}

void OLED_WriteCommand(uint8_t cmd)
{
	I2C_WriteReg(OLED_ADDR, 0x00, cmd);
}

void OLED_Init(void)
{
	Delay_Ms(100); // Delay_Ms y no vTaskDelay: se ejecuta antes del scheduler
	OLED_WriteCommand(0xAE);
	OLED_WriteCommand(0xD5);
	OLED_WriteCommand(0x80);
	OLED_WriteCommand(0xA8);
	OLED_WriteCommand(0x1F);
	OLED_WriteCommand(0xD3);
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0x40);
	OLED_WriteCommand(0x8D);
	OLED_WriteCommand(0x14);
	OLED_WriteCommand(0x20);
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0xA1);
	OLED_WriteCommand(0xC8);
	OLED_WriteCommand(0xDA);
	OLED_WriteCommand(0x02);
	OLED_WriteCommand(0x81);
	OLED_WriteCommand(0x8F);
	OLED_WriteCommand(0xD9);
	OLED_WriteCommand(0xF1);
	OLED_WriteCommand(0xDB);
	OLED_WriteCommand(0x40);
	OLED_WriteCommand(0xA4);
	OLED_WriteCommand(0xA6);
	OLED_WriteCommand(0xAF);
}

void OLED_ClearBuf(void)
{
	memset(oledBuf, 0, sizeof(oledBuf));
}

void OLED_DrawChar(uint8_t x, uint8_t page, char c)
{
	if (page >= PAGES)
		return;
	if (x + 6 > SCREEN_WIDTH)
		return;
	if ((uint8_t)c < 32 || (uint8_t)c > 127)
		c = '?';
	const uint8_t *p = &font5x7[((uint8_t)c - 32) * 5];
	for (uint8_t i = 0; i < 5; i++)
		oledBuf[page * SCREEN_WIDTH + x + i] = p[i];
	oledBuf[page * SCREEN_WIDTH + x + 5] = 0x00;
}

void OLED_DrawString(uint8_t x, uint8_t page, const char *s)
{
	while (*s)
	{
		OLED_DrawChar(x, page, *s++);
		x += 6;
		if (x + 6 > SCREEN_WIDTH)
			break;
	}
}

void OLED_Flush(void)
{
	for (uint8_t page = 0; page < PAGES; page++)
	{
		OLED_WriteCommand(0xB0 + page);
		OLED_WriteCommand(0x00);
		OLED_WriteCommand(0x10);

		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
			;
		I2C_GenerateSTART(I2C1, ENABLE);
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
			;
		I2C_Send7bitAddress(I2C1, OLED_ADDR << 1, I2C_Direction_Transmitter);
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			;
		I2C_SendData(I2C1, 0x40);
		while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
			;
		for (uint8_t col = 0; col < SCREEN_WIDTH; col++)
		{
			I2C_SendData(I2C1, oledBuf[page * SCREEN_WIDTH + col]);
			while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
				;
		}
		I2C_GenerateSTOP(I2C1, ENABLE);
	}
}

static uint8_t mpuInit(uint8_t addr)
{
	uint8_t who = I2C_ReadReg(addr, REG_WHO_AM_I);
	if (who != 0x68)
		return 0;
	I2C_WriteReg(addr, REG_PWR_MGMT_1, 0x00);
	return 1;
}

static void mpuReadAccel(uint8_t addr, float *ax, float *ay, float *az)
{
	uint8_t raw[6];
	I2C_ReadMulti(addr, REG_ACCEL_XOUT_H, raw, 6);
	*ax = (int16_t)((raw[0] << 8) | raw[1]) / ACCEL_SCALE;
	*ay = (int16_t)((raw[2] << 8) | raw[3]) / ACCEL_SCALE;
	*az = (int16_t)((raw[4] << 8) | raw[5]) / ACCEL_SCALE;
}

static void fmt_centi(char *out, size_t n, float v)
{
	int neg = (v < 0.0f);
	if (neg)
		v = -v;
	int32_t scaled = (int32_t)(v * 100.0f + 0.5f);
	int32_t ent = scaled / 100;
	int32_t dec = scaled % 100;
	snprintf(out, n, "%s%ld.%02ld", neg ? "-" : "", (long)ent, (long)dec);
}

void tarea_mpu1(void *pvParameters)
{
	float ax, ay, az;

	xSemaphoreTake(i2c_mutex, portMAX_DELAY);
	mpu1_ok = mpuInit(MPU1_ADDR); // Inicializa una sola vez al arrancar la tarea
	xSemaphoreGive(i2c_mutex);

	while (1)
	{
		if (mpu1_ok)
		{
			xSemaphoreTake(i2c_mutex, portMAX_DELAY); // Toma el bus I2C para leer el sensor
			mpuReadAccel(MPU1_ADDR, &ax, &ay, &az);
			xSemaphoreGive(i2c_mutex);

			mpu1_ax_s -= mpu1_ax_b[idx1];
			mpu1_ax_b[idx1] = ax;
			mpu1_ax_s += ax; // Buffer circular: saca la muestra vieja y mete la nueva
			mpu1_ay_s -= mpu1_ay_b[idx1];
			mpu1_ay_b[idx1] = ay;
			mpu1_ay_s += ay;
			mpu1_az_s -= mpu1_az_b[idx1];
			mpu1_az_b[idx1] = az;
			mpu1_az_s += az;
			idx1 = (idx1 + 1) % MUESTRAS_PROMEDIO;
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void tarea_mpu2(void *pvParameters)
{
	float ax, ay, az;

	xSemaphoreTake(i2c_mutex, portMAX_DELAY);
	mpu2_ok = mpuInit(MPU2_ADDR);
	xSemaphoreGive(i2c_mutex);

	while (1)
	{
		if (mpu2_ok)
		{
			xSemaphoreTake(i2c_mutex, portMAX_DELAY);
			mpuReadAccel(MPU2_ADDR, &ax, &ay, &az);
			xSemaphoreGive(i2c_mutex);

			mpu2_ax_s -= mpu2_ax_b[idx2];
			mpu2_ax_b[idx2] = ax;
			mpu2_ax_s += ax;
			mpu2_ay_s -= mpu2_ay_b[idx2];
			mpu2_ay_b[idx2] = ay;
			mpu2_ay_s += ay;
			mpu2_az_s -= mpu2_az_b[idx2];
			mpu2_az_b[idx2] = az;
			mpu2_az_s += az;
			idx2 = (idx2 + 1) % MUESTRAS_PROMEDIO;
		}
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void tarea_display(void *pvParameters)
{
	char linea[32];
	char vx[14], vy[14], vz[14];

	while (1)
	{
		float p1ax = mpu1_ax_s / MUESTRAS_PROMEDIO; // Promedio listo: suma acumulada / N
		float p1ay = mpu1_ay_s / MUESTRAS_PROMEDIO;
		float p1az = mpu1_az_s / MUESTRAS_PROMEDIO;
		float p2ax = mpu2_ax_s / MUESTRAS_PROMEDIO;
		float p2ay = mpu2_ay_s / MUESTRAS_PROMEDIO;
		float p2az = mpu2_az_s / MUESTRAS_PROMEDIO;

		xSemaphoreTake(i2c_mutex, portMAX_DELAY); // La pantalla es la seccion critica: se retiene el bus hasta terminar el flush

		OLED_ClearBuf();

		if (mpu1_ok)
		{
			OLED_DrawString(0, 0, "MPU1");
			fmt_centi(vx, sizeof(vx), p1ax);
			fmt_centi(vy, sizeof(vy), p1ay);
			fmt_centi(vz, sizeof(vz), p1az);
			snprintf(linea, sizeof(linea), "%s %s %s", vx, vy, vz);
			OLED_DrawString(0, 1, linea);
		}
		else
		{
			OLED_DrawString(0, 0, "MPU1 sin sensor");
		}

		if (mpu2_ok)
		{
			OLED_DrawString(0, 2, "MPU2");
			fmt_centi(vx, sizeof(vx), p2ax);
			fmt_centi(vy, sizeof(vy), p2ay);
			fmt_centi(vz, sizeof(vz), p2az);
			snprintf(linea, sizeof(linea), "%s %s %s", vx, vy, vz);
			OLED_DrawString(0, 3, linea);
		}
		else
		{
			OLED_DrawString(0, 2, "MPU2 sin sensor");
		}

		OLED_Flush();

		xSemaphoreGive(i2c_mutex);

		vTaskDelay(pdMS_TO_TICKS(500));
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

	printf("\r\nSistema 2xMPU6050 + OLED + FreeRTOS\r\n");
	printf("SystemClk: %u Hz\r\n", (unsigned)SystemCoreClock);
	printf("FreeRTOS Kernel Version: %s\r\n", tskKERNEL_VERSION_NUMBER);

	I2C1_Init();

	i2c_mutex = xSemaphoreCreateMutex();
	if (i2c_mutex == NULL)
	{
		while (1)
			;
	}

	xSemaphoreTake(i2c_mutex, portMAX_DELAY);
	OLED_Init();
	OLED_ClearBuf();
	OLED_DrawString(0, 1, "Iniciando...");
	OLED_Flush();
	xSemaphoreGive(i2c_mutex);

	xTaskCreate(tarea_mpu1, "MPU1", MPU1_STK, NULL, MPU1_PRIO, NULL);
	xTaskCreate(tarea_mpu2, "MPU2", MPU2_STK, NULL, MPU2_PRIO, NULL);
	xTaskCreate(tarea_display, "DISP", DISP_STK, NULL, DISP_PRIO, NULL);

	vTaskStartScheduler();

	while (1)
		;
}