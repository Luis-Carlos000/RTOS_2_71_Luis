#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define TASK_RX_PRIO 2
#define TASK_PROC_PRIO 2
#define TASK_TX_PRIO 2

#define TASK_RX_STK_SIZE 256
#define TASK_PROC_STK_SIZE 256
#define TASK_TX_STK_SIZE 256

#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 128

TaskHandle_t TaskRx_Handler;
TaskHandle_t TaskProc_Handler;
TaskHandle_t TaskTx_Handler;

typedef enum // Maquina de estados del sistema
{
	ESTADO_ESPERANDO = 0, // Sistema esperando entrada del usuario
	ESTADO_PROCESANDO,	  // Sistema procesando (invirtiendo cadena)
	ESTADO_TRANSMITIENDO  // Sistema transmitiendo respuesta
} EstadoSistema;

volatile EstadoSistema estado = ESTADO_ESPERANDO; // Estado global del sistema
volatile uint8_t actividad = 0;					  // Bandera: 1 si llego cualquier caracter desde el ultimo chequeo de timeout

void UART3_Init(uint32_t baudrate) // Inicializa USART3 en PB10 (TX) y PB11 (RX)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); // Habilita reloj de GPIOB y AFIO
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);						// Habilita reloj de USART3

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PB10 que será TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // Salida push-pull alterna
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;			  // PB11 que será RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Entrada flotante
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure = {0};
	USART_InitStructure.USART_BaudRate = baudrate;									// Baudrate solicitado
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 8 bits de datos
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 1 bit de stop
	USART_InitStructure.USART_Parity = USART_Parity_No;								// Sin paridad
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // Sin control de flujo
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART3, &USART_InitStructure);

	USART_Cmd(USART3, ENABLE);
}

void UART3_Send(const char *format, ...) // Envio tipo printf por USART3
{
	char txBuffer[TX_BUFFER_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(txBuffer, TX_BUFFER_SIZE, format, args);
	va_end(args);

	size_t len = strlen(txBuffer);
	for (size_t i = 0; i < len; ++i)
	{
		while (!USART_GetFlagStatus(USART3, USART_FLAG_TXE)) // Espera que esté disponible para transmitir
			;
		USART_SendData(USART3, (uint8_t)txBuffer[i]);
	}
}

void task_rx(void *pvParameters)
{
	char buffer[RX_BUFFER_SIZE];
	uint16_t index = 0;

	while (1)
	{
		if (estado != ESTADO_ESPERANDO) // Si el sistema no esta en ESPERANDO, no lee el UART
		{
			index = 0;	 // Reinicia indice por si quedo a medias
			taskYIELD(); // Cede CPU brevemente
			continue;	 // Vuelve a chequear el estado
		}

		if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET) // Disponible en RX
		{
			char c = (char)USART_ReceiveData(USART3); // Lee el byte recibido

			actividad = 1; // Marca actividad: llego cualquier caracter

			if (c == '\n') // Detecta fin de linea
			{
				buffer[index] = '\0'; // Termina la cadena en el buffer

				if (index > 0) // Solo procesa si hay contenido
				{
					uint32_t len = index + 1; // Longitud incluyendo el '\0'

					if (xPortGetFreeHeapSize() > (len + 64)) // Verifica que haya heap suficiente
					{
						char *msg = (char *)pvPortMalloc(len); // Reserva memoria exacta en el heap
						if (msg != NULL)					   // Verifica que la reserva haya sido exitosa
						{
							memcpy(msg, buffer, len); // Copia la cadena al bloque reservado

							estado = ESTADO_PROCESANDO;											  // Cambia el estado antes de notificar para que PROC lo vea listo
							xTaskNotify(TaskProc_Handler, (uint32_t)msg, eSetValueWithOverwrite); // Notifica a PROC pasando el puntero de 32 bits
						}
						else // Fallo la reserva a pesar del chequeo
						{
							UART3_Send("Error: no hay memoria Heap suficiente\r\n");
						}
					}
					else
					{
						UART3_Send("Error: no hay memoria Heap suficiente\r\n");
					}
				}

				index = 0; // Reinicia el buffer tras el '\n'
			}
			else // Caracter normal (no '\n')
			{
				if (c != '\r' && index < (RX_BUFFER_SIZE - 1)) // Ignora '\r' y respeta el limite del buffer
				{
					buffer[index++] = c; // Guarda el caracter y avanza indice
				}
			}
		}
		else // No hay byte disponible
		{
			taskYIELD(); // Cede CPU para no bloquear otras tareas
		}
	}
}

void task_proc(void *pvParameters)
{
	uint32_t value = 0;

	while (1)
	{
		if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &value, pdMS_TO_TICKS(5000)) == pdTRUE) // Espera notificacion
		{
			if (estado == ESTADO_PROCESANDO) // Solo procesa si el sistema esta en PROCESANDO
			{
				char *msg = (char *)value;

				uint16_t len = strlen(msg);
				for (uint16_t i = 0; i < len / 2; i++) // Recorre hasta la mitad de la cadena
				{
					char tmp = msg[i];		   // Guarda temporalmente el caracter izquierdo
					msg[i] = msg[len - 1 - i]; // Coloca el caracter derecho a la izquierda
					msg[len - 1 - i] = tmp;	   // Coloca el caracter izquierdo a la derecha
				}

				estado = ESTADO_TRANSMITIENDO;										// Cambia el estado antes de notificar a TX
				xTaskNotify(TaskTx_Handler, (uint32_t)msg, eSetValueWithOverwrite); // Notifica a TX con el puntero ya modificado
			}
		}
		else
		{
			if (actividad == 0) // Solo imprime si no recibe nada en 5s
			{
				UART3_Send("Estado: Sistema inactivo, esperando comandos...\r\n");
			}
			actividad = 0; // Reinicia la bandera de actividad para el proximo ciclo
		}
	}
}

void task_tx(void *pvParameters)
{
	uint32_t value = 0;

	while (1)
	{
		if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &value, portMAX_DELAY) == pdTRUE) // Espera la notificacion de PROC
		{
			if (estado == ESTADO_TRANSMITIENDO) // Solo transmite si el sistema esta en TRANSMITIENDO
			{
				char *msg = (char *)value;

				UART3_Send("%s\r\n", msg);												  // Envia la cadena invertida al monitor serie
				UART3_Send("Heap libre: %u bytes\r\n", (unsigned)xPortGetFreeHeapSize()); // Heap disponible restante

				vPortFree(msg); // Libera el bloque de memoria

				estado = ESTADO_ESPERANDO; // Regresa a ESPERANDO para reactivar a RX
			}
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
	Delay_Init(); // Necesario en la implementacion, no borrar para evitar que deje de recibir al programador
	UART3_Init(9600);
	// Delay_Ms(3000);	//Espera 3s a que inicie bien el uart

	xTaskCreate((TaskFunction_t)task_tx,
				(const char *)"tx",
				(uint16_t)TASK_TX_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_TX_PRIO,
				(TaskHandle_t *)&TaskTx_Handler);

	xTaskCreate((TaskFunction_t)task_proc,
				(const char *)"proc",
				(uint16_t)TASK_PROC_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_PROC_PRIO,
				(TaskHandle_t *)&TaskProc_Handler);

	xTaskCreate((TaskFunction_t)task_rx,
				(const char *)"rx",
				(uint16_t)TASK_RX_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)TASK_RX_PRIO,
				(TaskHandle_t *)&TaskRx_Handler);

	vTaskStartScheduler();

	while (1)
	{
	}
}