#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//PINES 
#define SERVO_PIN 15
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

//CONFIGURACIÓN WIFI 
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

//CONFIGURACIÓN MQTT 
extern const char* MQTT_BROKER;
extern const int MQTT_PORT;
extern const char* MQTT_CLIENT_ID;

//IDENTIFICACIÓN DEL DISPOSITIVO 
extern const char* USER_ID;      
extern const char* DEVICE_ID;    

//ASUNTOS MQTT 
extern const char* ASUNTO_INICIO;
extern const char* ASUNTO_COMANDO;
extern const char* ASUNTO_RESPUESTA;

//CONSTANTES GENERALES 
#define MAX_PASSW_LEN 16

#endif