#include <Arduino.h>

#define LED0 PA0
#define LED1 PA1
#define LED2 PA2

#define t_1 2000
#define t_2 500

unsigned long ultimoCambio = 0;
int paso = 0;

void setup()
{
	pinMode(LED0, OUTPUT);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	Serial.begin(115200);
	ultimoCambio = millis();
	paso = 0;
}

void loop()
{
	unsigned long ahora = millis();
	switch (paso)
	{
	case 0:
		digitalWrite(LED0, HIGH);
		digitalWrite(LED1, LOW);
		digitalWrite(LED2, LOW);
		if (ahora - ultimoCambio >= t_1)
		{
			digitalWrite(LED0, LOW);
			ultimoCambio = ahora;
			paso = 1;
		}
		break;
	case 1:
		digitalWrite(LED1, HIGH);
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED1, LOW);
			ultimoCambio = ahora;
			paso = 2;
		}
		break;
	case 2:
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED1, HIGH);
			ultimoCambio = ahora;
			paso = 3;
		}
		break;
	case 3:
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED1, LOW);
			ultimoCambio = ahora;
			paso = 4;
		}
		break;
	case 4:
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED2, HIGH);
			ultimoCambio = ahora;
			paso = 5;
		}
		break;
	case 5:
		if (ahora - ultimoCambio >= t_2)
		{
			digitalWrite(LED2, LOW);
			ultimoCambio = ahora;
			paso = 0;
		}
		break;
	}
}