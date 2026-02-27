#include "MQTT_Manager.h"
#include "IoT.h"

//interactua con el objeto IoT
void MQTTManager::use_IoT(IoT* dispositivo){ p_dispositivo = dispositivo; }

//wrapper de la funcion loop de PubSubClient
void MQTTManager::loop_wrapper(){ iot.loop(); }

//traduce el json
String translate_json(byte* payload, unsigned int length){
    String mensaje = "";
    //recorrer el payload para conseguir el mensaje
    for (int i = 0; i < length; i++) {
        mensaje += (char)payload[i]; //transformar byte a char
    }
    
    return mensaje;
}

//maneja la lectura del mensaje
String MQTTManager::read_message(const char* mensaje){
    String accion = "";
    DeserializationError err = deserializeJson(parser, mensaje);
    switch (err.code()) {
        case DeserializationError::Ok:
        Serial.println("Deserialisacion completada");
        break;

        case DeserializationError::InvalidInput:
        Serial.print("Input Invalido");
        break;

        case DeserializationError::NoMemory:
        Serial.println("No hay memoria");
        break;

        default:
        Serial.println("Error fatal");
        break;
    }

    //conseguir la accion
    //validar que exista una accion
    if(parser["acc"] == nullptr){
        //puedo crear una excepcion
        return "Error: accion nula";
    }
    accion = parser["acc"].as<String>();
    return accion;
}
//maneja el envio del mensaje

//maneja las respuestas
void MQTTManager::response(const char* accion, IoT* dispositivo){
    Serial.println("Procesando respuestas...");
    String payload = "";
    int rel_id = -1;
    bool bloqueado;

    //identificacion del servidor con el IoT
    if(accion == "iot-ack"){
        //conseguir el id de relacion
        rel_id = parser["rel_id"];
        dispositivo->set_relationID(rel_id);

        //conseguir el estado inicial
        bloqueado = parser["estado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);

    }else if(accion == "iot-val-pssw"){
        //ver si fue correcta a no la contraseña ingresada
        String estado_val = parser["val"].as<String>();
        if(estado_val == "exito"){
            parser["acc"] = "serv-dsblk";
            parser["rel_id"] = dispositivo->get_relationID();
            serializeJson(parser, payload);
            iot.publish(COMANDO, payload.c_str());

        }else if(estado_val == "fallo"){
            parser["acc"] = "serv-wrg-pssw";
            parser["rel_id"] = dispositivo->get_relationID();
            serializeJson(parser, payload);
            iot.publish(RESPUESTA, payload.c_str());

        }else{
            Serial.println("else in val-pssw");
        }
    }else if(accion == "iot-dsblk"){
        bloqueado = parser["estado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        //servo.write(180); //simula que se abrio la cerradura
    }else{
        Serial.println("...mas respuestas");
    }
}

//maneja los comandos
void MQTTManager::command(const char* accion, IoT* dispositivo){
    Serial.println("Procesando comandos..."); 
    String payload = "";
    int rel_id = -1;
    bool bloqueado;
    bool intentos;

    //la accion de abrir/cerrar viene del app
    if(accion == "iot-dsblk"){
        bloqueado = parser["estado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        //servo.write(180); //simula que se abrio la cerradura
    }else if(accion == "iot-blk"){
        bloqueado = parser["estado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        //servo.write(90); //simula que se cerro la cerradura

    }else if(accion == "iot-time-out"){
        //verificar si tengo intentos
        intentos = parser["try"];
        if(intentos){
            // lcd.clear();
            // displayMessage("No contraseña ", "30seg de bloqueo", parser["time"]);
            // reset();
        }else{
            // lcd.clear();
            // displayMessage("No contraseña ", "5min de bloqueo", parser["time"]);
            // reset();
        }
    }else{
        Serial.println("...mas comandos");
    }
}

//maneja la logica de los mensajes
void MQTTManager::message_manager(char* topic, byte* payload, unsigned int length){
    String mensaje = "";
    mensaje = translate_json(payload, length);
    Serial.print("Mensaje recibido en: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(mensaje);

    //parsear el json; aka mensaje
    String accion = "";
    accion = read_message(mensaje.c_str());

    //validar que MQTT conoce sobre IoT
    if(!p_dispositivo){ }//mandar mensaje de error}

    // Procesar según el asunto
    if (String(topic) == RESPUESTA) {
        response(accion.c_str(), p_dispositivo);

    }else if (String(topic) == COMANDO) {
        command(accion.c_str(), p_dispositivo);

    }else{
        Serial.println("standby in else");
    }
}

//maneja la conexion a los asuntos
void MQTTManager::topic_conection(){
    while(!iot.connected()){
        Serial.println("Iniciando conexion con broker-mqtt...");
        if(iot.connect("wokwi_smartlock")){
            Serial.println("conexion con broker-mqtt exitosa!");

            iot.setCallback(staticCallback);
            Serial.println("funcion callback inicializada");

            iot.subscribe(INICIO);
            Serial.println("subscrito a los inicios");

            iot.subscribe(COMANDO);
            Serial.println("subscrito a los comandos");

            iot.subscribe(RESPUESTA);
            Serial.println("subscrito a las respuestas");
        
        }else{
            Serial.print(" failed, rc=");
            Serial.print(iot.state());
            Serial.println("intentar en 5 segundos...");
            delay(5000);
        }
    }
}
MQTTManager* MQTTManager::instancia = nullptr;
void MQTTManager::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (instancia) {
        instancia->message_manager(topic, payload, length);
    }
}