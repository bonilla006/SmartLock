#include "config.h"
#include "MQTT_Manager.h"
#include "IoT.h"
#include <Keypad.h>
#include "mbedtls/md.h"

Servo servo; //mecanismo de cerradura
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); //pantalla
//Keypad
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'c'},
  {'*', '0', '#', 'D'},
};
byte rowPins[KEYPAD_ROWS] = {23, 19, 18, 5};
byte colPins[KEYPAD_COLS] = {17, 16, 4, 2};
//creacion del objeto Keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

//clases propias
MQTTManager mssg_manager(MQTT_BROKER, MQTT_PORT, ASUNTO_INICIO, ASUNTO_COMANDO, ASUNTO_RESPUESTA);
IoT dispositivo(-1, true);

byte shaResult[32]; //almacena los bytes del hash
String computeSHA256(const String& data);

void setup() {
  Serial.begin(115200);
  Serial.println("Conectandoce a internet...");
  //conectarse a internet
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println(WiFi.status());
    delay(2000);
  }
  Serial.println("Conectado al WiFi!");

  //conectar el mecanismo con el microprocesador
  servo.attach(SERVO_PIN);
  servo.write(0);

  mssg_manager.use_IoT(&dispositivo);
  mssg_manager.handshake();

  lcd.init();
  lcd.backlight();
  //en que seccion de la pantalla comenzara a escribir
  lcd.setCursor(0,0);
  lcd.print("Inicio de Sim");
  delay(2000);
  lcd.clear();
}

void loop() {
  //mantener la comunicacion
  mssg_manager.loop_wrapper();

  //validar que no se haya cortado la comunicacion
  mssg_manager.topic_conection();
  
  lcd.setCursor(0,0);
  
  //verificar el estado
  dispositivo.status();

  //devuelve el digito que se ingreso en el keypad
  char digito = keypad.getKey();

  //siempre que no sea nulo
  if(digito != NO_KEY){
    delay(100);
    if(digito != '*'){
      Serial.println(digito);
      //procesar el codigo ingresado
      dispositivo.proces_passw(digito);
    }else{
      lcd.clear();
      //conseguir la contraseña en texto plano
      //hash la contraseña
      dispositivo.set_hashpssw(computeSHA256(dispositivo.get_localpssw()));

      //enviar la data hacia el servidor
      mssg_manager.validate_pssw();
    }
  }
}

String computeSHA256(const String& data) {
  String hashdata  = "";

  //track el estado de las funciones
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  //creacion del objeto mbedtls
  mbedtls_md_init(&ctx);
  //setup que recibe el track, el tipo de algoritmo para hash y si vamos a usar HMAC
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  //comienza el proceso para digist el string
  mbedtls_md_starts(&ctx);
  //proceso de hash
  mbedtls_md_update(&ctx, (const unsigned char*)data.c_str(), strlen(data.c_str())); 
  //finaliza el proceso de hash
  mbedtls_md_finish(&ctx, shaResult);
  //libera el contexto
  mbedtls_md_free(&ctx); 

  //unificar todo el hash para pasarlo como string
  for(int i= 0; i< sizeof(shaResult); i++){
      char hashchar[3];

      sprintf(hashchar, "%02x", (int)shaResult[i]);
      hashdata += hashchar;
  }

  return hashdata;

}
