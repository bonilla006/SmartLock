#include "config.h"
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "mbedtls/md.h"

// ========== PROTOTIPOS ==========
void conexionMQTT();
void manejadorMensajesMQTT(char* topic, byte* payload, unsigned int length);
String computeSHA256(const String& data);
void procesarDigito(char digito);
void reset();
void displayMessage(const char *line1, const char *line2, int time);

Servo servo;

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'c'},
  {'*', '0', '#', 'D'},
};
byte rowPins[KEYPAD_ROWS] = {23, 19, 18, 5};
byte colPins[KEYPAD_COLS] = {17, 16, 4, 2};

///////////////////////////////////
//variables de control
bool isDoorLocked = true;
byte shaResult[32]; //almacena los bytes del hash
// 48754849
String localpssw = ""; //para poder visualizar y hash
String hashpssw = "";
bool pssw_correcto = false;
byte current_len_passw = 0;
//creacion del objeto Keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

///////////////////////////////////


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

  //mandar mensaje de presentacion
  String payload = "{\"acc\":\"serv-ack\",\"user_id\":\""+String(USER_ID)+"\"}";
  cliente.publish(ASUNTO_RESPUESTA, payload.c_str());

  //conectar el mecanismo con el microprocesador
  servo.attach(SERVO_PIN);
  servo.write(0);

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
  cliente.loop();
  
  //validar que no se haya cortado la comunicacion
  if(!cliente.connected()){
    conexionMQTT();
  }
  
  lcd.setCursor(0,0);
  //verificar el estado
  if(ESTADO == "Bloqueado"){
    //simula cerradura cerrada
    servo.write(90); 
    lcd.print("Contraseña:");
  }else if(ESTADO == "Desbloqueado"){
    servo.write(180);
    lcd.print("Bienvenido!");
  }else{
    Serial.println("error con el estado:");
    Serial.println(ESTADO);
    //volver a hacer el ack con el servidor
    String payload = "{\"acc\":\"serv-ack\",\"user_id\":\""+String(USER_ID)+"\"}";
  cliente.publish(ASUNTO_RESPUESTA, payload.c_str());
  }

  //devuelve el digito que se ingreso en el keypad
  char digito = keypad.getKey();

  //siempre que no sea nulo
  if(digito != NO_KEY){
    delay(100);
    if(digito != '*'){
      Serial.println(digito);
      //procesar el codigo ingresado
      procesarDigito(digito);
    }else{
      lcd.clear();
      
      //hash la contraseña
      hashpssw = computeSHA256(localpssw);
      Serial.println(hashpssw);
      //enviar la data hacia el servidor
      String payload = "{\"acc\":\"serv-val-pssw\",\"rel_id\":\" "+REL_ID+" \",\"pssw\":\"" + hashpssw + "\"}";
      Serial.println(payload);
      cliente.publish(ASUNTO_COMANDO, payload.c_str());
    }
  }
}

//String payload = "{\"acc\":\"error\",\"rel_id\":\" "+REL_ID+" \",\"err\":\"el val que recibio el iot fue:"+estado_val+"\"}";
//cliente.publish(ASUNTO_RESPUESTA, payload.c_str());
void manejadorMensajesMQTT(char* topic, byte* payload, unsigned int length){
  
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

/*---------------------------------------------------------------*/

/*Funciones para manejo del hardware*/
void procesarDigito(char digito){
  //llenar el posible passw con los digitos ingresados
  if(current_len_passw < MAX_PASSW_LEN){
    //mover el cursor segun ingresado
    lcd.setCursor(current_len_passw, 1);
    lcd.print("*");
    localpssw += digito;
    current_len_passw++;
  }
}

void reset(){
  current_len_passw = 0;
  localpssw = "";
  lcd.clear();
  lcd.setCursor(0, 0);
}

void displayMessage(const char *line1, const char *line2, int time) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  delay(time);
  lcd.clear();
}
/*---------------------------------------------------------------*/