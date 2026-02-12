// #include <ESP32Servo.h>
// #include <LiquidCrystal_I2C.h>
// #include <Keypad.h>
// #include <Password.h>
// #include "mbedtls/md.h"
// #include <WiFi.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>
// #define ServoPin 15

// Servo servo;

// LiquidCrystal_I2C lcd(0x27, 16, 2);

// const byte ROWS = 4;
// const byte COLS = 4;

// char keys[ROWS][COLS] = {
//   {'1', '2', '3', 'A'},
//   {'4', '5', '6', 'B'},
//   {'7', '8', '9', 'c'},
//   {'*', '0', '#', 'D'},
// };
// byte rowPins[ROWS] = {23, 19, 18, 5};
// byte colPins[COLS] = {17, 16, 4, 2};

// ///////////////////////////////////
// //variables de control
// bool isDoorLocked = true;
// Password password = Password("48754849");
// byte shaResult[32]; //almacena los bytes del hash
// String localpssw = ""; //para poder visualizar y hash
// String hashpssw = "";
// bool pssw_correcto = false;
// byte max_len_passw = 16;
// byte current_len_passw = 0;
// //creacion del objeto Keypad
// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// //credenciales para wifi
// const char* SSID = "Wokwi-GUEST";
// const char* PASSW = "";

// //credenciales para comunicacion con mqtt
// const char* SERV_MQTT = "broker.hivemq.com";
// const int PORT_MQTT =  1883;
// const char* ASUNTO_MQTT = "/smartlock/mqtt/";
// const String USER_ID = "1";
// const String ASUNTO_INICIO = "smartlock/BFUA-6044/inicio";
// const String ASUNTO_COMANDO = "smartlock/BFUA-6044/comando";
// const String ASUNTO_RESPUESTA = "smartlock/BFUA-6044/respuesta";
// String REL_ID = "";
// String ESTADO = "";

// //objeto json
// JsonDocument parser;

// //objeto cliente
// WiFiClient smartlock;
// PubSubClient cliente(smartlock);
// ///////////////////////////////////


// void setup() {
//   Serial.begin(115200);
//   Serial.println("Conectandoce a internet...");
//   //conectarse a internet
//   WiFi.begin(SSID, PASSW);
//   while(WiFi.status() != WL_CONNECTED){
//     Serial.println(WiFi.status());
//     delay(2000);
//   }
//   Serial.println("Conectado al WiFi!");

//   //set mqtt
//   cliente.setServer(SERV_MQTT, PORT_MQTT);
//   conexionMQTT();

//   //mandar mensaje de presentacion
//   String payload = "{\"acc\":\"serv-ack\",\"user_id\":\""+USER_ID+"\"}";
//   cliente.publish(ASUNTO_RESPUESTA.c_str(), payload.c_str());

//   //conectar el mecanismo con el microprocesador
//   servo.attach(ServoPin);
//   servo.write(0);

//   lcd.init();
//   lcd.backlight();
//   //en que seccion de la pantalla comenzara a escribir
//   lcd.setCursor(0,0);
//   lcd.print("Inicio de Sim");
//   delay(2000);
//   lcd.clear();
// }

// void loop() {
//   //mantener la comunicacion
//   cliente.loop();
  
//   //validar que no se haya cortado la comunicacion
//   if(!cliente.connected()){
//     conexionMQTT();
//   }
  
//   lcd.setCursor(0,0);
//   //verificar el estado
//   if(ESTADO == "Bloqueado"){
//     //simula cerradura cerrada
//     servo.write(90); 
//     lcd.print("Contraseña:");
//   }else if(ESTADO == "Desbloqueado"){
//     servo.write(180);
//     lcd.print("Bienvenido!");
//   }else{
//     Serial.println("error con el estado:");
//     Serial.println(ESTADO);
//     //volver a hacer el ack con el servidor
//     String payload = "{\"acc\":\"serv-ack\",\"user_id\":\""+USER_ID+"\"}";
//   cliente.publish(ASUNTO_RESPUESTA.c_str(), payload.c_str());
//   }

//   //devuelve el digito que se ingreso en el keypad
//   char digito = keypad.getKey();

//   //siempre que no sea nulo
//   if(digito != NO_KEY){
//     delay(100);
//     if(digito != '*'){
//       Serial.println(digito);
//       //procesar el codigo ingresado
//       procesarDigito(digito);
//     }else{
//       lcd.clear();
      
//       //hash la contraseña
//       hashpssw = computeSHA256(localpssw);
//       Serial.println(hashpssw);
//       //enviar la data hacia el servidor
//       String payload = "{\"acc\":\"serv-val-pssw\",\"rel_id\":\" "+REL_ID+" \",\"pssw\":\"" + hashpssw + "\"}";
//       Serial.println(payload);
//       cliente.publish(ASUNTO_COMANDO.c_str(), payload.c_str());
//     }
//   }
// }


// /*Funciones para manejo de MQTT*/
// void conexionMQTT(){
//   //si el dispositivo no esta conectado
//   while(!cliente.connected()){
//     Serial.println("Iniciando conexion con broker-mqtt...");
//     if(cliente.connect("wokwi_smartlock")){
//       Serial.println("conexion con broker-mqtt exitosa!");
//       cliente.setCallback(manejadorMensajesMQTT);
//       Serial.println("funcion callback inicializada");
//       cliente.subscribe(ASUNTO_INICIO.c_str());
//       Serial.println("subscrito a los inicios");
//       cliente.subscribe(ASUNTO_COMANDO.c_str());
//       Serial.println("subscrito a los comandos");
//       cliente.subscribe(ASUNTO_RESPUESTA.c_str());
//       Serial.println("subscrito a las respuestas");
      
//     }else{
//       Serial.print(" failed, rc=");
//       Serial.print(cliente.state());
//       Serial.println("intentar en 5 segundos...");
//       delay(5000);
//     }
//   }
// }
// //String payload = "{\"acc\":\"error\",\"rel_id\":\" "+REL_ID+" \",\"err\":\"el val que recibio el iot fue:"+estado_val+"\"}";
// //cliente.publish(ASUNTO_RESPUESTA, payload.c_str());
// void manejadorMensajesMQTT(char* topic, byte* payload, unsigned int length){
//   String mensaje = "";
//   //recorrer el payload para conseguir el mensaje
//   for (int i = 0; i < length; i++) {
//     mensaje += (char)payload[i]; //transformar byte a char
//   }
  
//   Serial.print("Mensaje recibido en: ");
//   Serial.print(topic);
//   Serial.print(" -> ");
//   Serial.println(mensaje);
//   //parsear el json; aka mensaje
//   DeserializationError err = deserializeJson(parser, mensaje.c_str());
//   switch (err.code()) {
//     case DeserializationError::Ok:
//       Serial.println("Deserialisacion completada");
//       break;

//     case DeserializationError::InvalidInput:
//       Serial.print("Input Invalido");
//       break;

//     case DeserializationError::NoMemory:
//       Serial.println("No hay memoria");
//       break;

//     default:
//       Serial.println("Error fatal");
//       break;
//   }
//   //conseguir la accion
//   String accion = String(parser["acc"]);
//   // Procesar según el asunto
//   if (String(topic) == ASUNTO_RESPUESTA) {
//     Serial.println("Procesando respuestas...");
    
//     //identificacion del servidor con el IoT
//     if(accion == "iot-ack"){
//       //conseguir el id de relacion
//       REL_ID = String(parser["rel_id"]);
//       ESTADO = String(parser["estado"]);

//     }else if(accion == "iot-val-pssw"){
//       //ver si fue correcta a no la contraseña ingresada
//       String estado_val = String(parser["val"]);
//       if(estado_val == "exito"){
//         String payload = "{\"acc\":\"serv-dsblk\",\"rel_id\":\" "+REL_ID+" \"}";
//         cliente.publish(ASUNTO_COMANDO.c_str(), payload.c_str());

//       }else if(estado_val == "fallo"){
//         String payload = "{\"acc\":\"serv-wrg-pssw\",\"rel_id\":\" "+REL_ID+" \"}";
//         cliente.publish(ASUNTO_RESPUESTA.c_str(), payload.c_str());

//       }else{
//         Serial.println("else in val-pssw");
//       }
//     }else if(accion == "iot-dsblk"){
//       ESTADO = String(parser["estado"]); //actualizar estado
//       servo.write(180); //simula que se abrio la cerradura
//     }else{
//       Serial.println("...mas respuestas");
//     }
//   }
//   else if (String(topic) == ASUNTO_COMANDO) {
//     Serial.println("Procesando comandos..."); 
//     //la accion de abrir/cerrar viene del app
//     if(accion == "iot-dsblk"){
//       ESTADO = String(parser["estado"]); //actualizar estado
//       servo.write(180); //simula que se abrio la cerradura
//     }else if(accion == "iot-blk"){
//       ESTADO = String(parser["estado"]); //actualizar estado
//       servo.write(90); //simula que se cerro la cerradura

//     }else if(accion == "iot-time-out"){
//       //verificar si tengo intentos
//       String intentos = String(parser["try"]);
//       if(intentos == "si"){
//         lcd.clear();
//         displayMessage("No contraseña ", "30seg de bloqueo", parser["time"]);
//         reset();
//       }else{
//         lcd.clear();
//         displayMessage("No contraseña ", "5min de bloqueo", parser["time"]);
//         reset();
//       }
//     }else{
//       Serial.println("...mas comandos");
//     }
//   }else{
//     Serial.println("standby in else");
//   }
// }

// String computeSHA256(const String& data) {
//   String hashdata  = "";

//   //track el estado de las funciones
//   mbedtls_md_context_t ctx;
//   mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
//   //creacion del objeto mbedtls
//   mbedtls_md_init(&ctx);
//   //setup que recibe el track, el tipo de algoritmo para hash y si vamos a usar HMAC
//   mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
//   //comienza el proceso para digist el string
//   mbedtls_md_starts(&ctx);
//   //proceso de hash
//   mbedtls_md_update(&ctx, (const unsigned char*)data.c_str(), strlen(data.c_str())); 
//   //finaliza el proceso de hash
//   mbedtls_md_finish(&ctx, shaResult);
//   //libera el contexto
//   mbedtls_md_free(&ctx); 

//   //unificar todo el hash para pasarlo como string
//   for(int i= 0; i< sizeof(shaResult); i++){
//       char hashchar[3];

//       sprintf(hashchar, "%02x", (int)shaResult[i]);
//       hashdata += hashchar;
//   }

//   return hashdata;

// }

// /*---------------------------------------------------------------*/

// /*Funciones para manejo del hardware*/
// void procesarDigito(char digito){
//   //llenar el posible passw con los digitos ingresados
//   if(current_len_passw < max_len_passw){
//     //mover el cursor segun ingresado
//     lcd.setCursor(current_len_passw, 1);
//     lcd.print("*");
//     password.append(digito);
//     localpssw += digito;
//     current_len_passw++;
//   }
// }

// void reset(){
//   password.reset();
//   current_len_passw = 0;
//   localpssw = "";
//   lcd.clear();
//   lcd.setCursor(0, 0);
// }

// void displayMessage(const char *line1, const char *line2, int time) {
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print(line1);
//   lcd.setCursor(0, 1);
//   lcd.print(line2);
//   delay(time);
//   lcd.clear();
// }
// /*---------------------------------------------------------------*/