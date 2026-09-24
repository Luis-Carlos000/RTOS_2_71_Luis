#include <Arduino.h>

#define LED0 PA0
#define LED1 PA1

#define t_1 500
#define t_2 330

unsigned long ultimoCambio = 0;
int estado = 0;

void setup()
{
	pinMode(LED0, OUTPUT);
	pinMode(LED1, OUTPUT);
	ultimoCambio = millis();
	estado = 0;
}

void loop()
{
	unsigned long ahora = millis();
	switch (estado)
	{
	case 0:
		digitalWrite(LED0, HIGH);
		if (ahora - ultimoCambio >= t_1)
		{
			digitalWrite(LED0, LOW);
			ultimoCambio = ahora;
			estado = 1;
		}
		break;
	case 1:
		if (ahora - ultimoCambio >= t_1)
		{
			ultimoCambio = ahora;
			estado = 2;
		}
		break;
	case 2:
		digitalWrite(LED1, HIGH);
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED1, LOW);
			ultimoCambio = ahora;
			estado = 3;
		}
		break;
	case 3:
		if (ahora - ultimoCambio >= t_2)
		{
			ultimoCambio = ahora;
			estado = 0;
		}
		break;
	}
}