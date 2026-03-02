#include "MQTT_Manager.h"
#include "IoT.h"

//guarda la direccion del objeto para que MQTTManager pueda interactuar
//con los metodos de los otros objetos
void MQTTManager::use_IoT(IoT* dispositivo){ p_dispositivo = dispositivo; }

//se identifica con el API
void MQTTManager::handshake(){
    String payload = "";
    parser["acc"] = "serv-ack";
    parser["user_id"] = "1";
    serializeJson(parser, payload);
    cliente.publish(RESPUESTA, payload.c_str());
    Serial.print("ACK enviado");
}

//mandar la constraseña hash a el API
void MQTTManager::validate_pssw(){
    String payload = "";
    parser["acc"] = "serv-val-pssw";
    parser["rel_id"] = p_dispositivo->get_relationID();
    parser["pssw"] = p_dispositivo->get_hashpssw();
    serializeJson(parser, payload);
    cliente.publish(COMANDO, payload.c_str());
}

//wrapper de la funcion loop de PubSubClient
void MQTTManager::loop_wrapper(){ cliente.loop(); }

//traduce el json
String MQTTManager::translate_json(byte* payload, unsigned int length){
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
void MQTTManager::response(String accion, IoT* dispositivo){
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
        bloqueado = parser["bloqueado"]; //recibo un valor bool
        Serial.print(bloqueado);
        dispositivo->set_block(bloqueado);

    }else if(accion == "iot-val-pssw"){
        //ver si fue correcta a no la contraseña ingresada
        String estado_val = parser["val"].as<String>();
        if(estado_val == "exito"){
            parser["acc"] = "serv-dsblk";
            parser["rel_id"] = dispositivo->get_relationID();
            serializeJson(parser, payload);
            cliente.publish(COMANDO, payload.c_str());

        }else if(estado_val == "fallo"){
            parser["acc"] = "serv-wrg-pssw";
            parser["rel_id"] = dispositivo->get_relationID();
            serializeJson(parser, payload);
            cliente.publish(RESPUESTA, payload.c_str());

        }else{
            Serial.println("else in val-pssw");
        }
    }else if(accion == "iot-dsblk"){
        bloqueado = parser["bloqueado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        dispositivo->open(); 
    }else{
        Serial.println("...mas respuestas");
    }
}

//maneja los comandos
void MQTTManager::command(String accion, IoT* dispositivo){
    Serial.println("Procesando comandos..."); 
    String payload = "";
    int rel_id = -1;
    bool bloqueado;
    bool intentos;

    //la accion de abrir/cerrar viene del app
    if(accion == "iot-dsblk"){
        bloqueado = parser["bloqueado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        dispositivo->open(); 

    }else if(accion == "iot-blk"){
        bloqueado = parser["bloqueado"]; //recibo un valor bool
        dispositivo->set_block(bloqueado);
        dispositivo->close();

    }else if(accion == "iot-time-out"){
        //verificar si tengo intentos
        intentos = parser["try"];
        if(intentos){
            dispositivo->display("wrong!", "timeout(30seg)", parser["time"]);
            dispositivo->reset();

        }else{
            dispositivo->display("wrong!", "timeout(5min)", parser["time"]);
            dispositivo->reset();
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
        response(accion, p_dispositivo);

    }else if (String(topic) == COMANDO) {
        command(accion, p_dispositivo);

    }else{
        Serial.println("standby in else");
    }
}

//maneja la conexion a los asuntos
void MQTTManager::topic_conection(){
    while(!cliente.connected()){
        Serial.println("Iniciando conexion con broker-mqtt...");
        if(cliente.connect("wokwi_smartlock")){
            Serial.println("conexion con broker-mqtt exitosa!");

            cliente.setCallback(staticCallback);
            Serial.println("funcion callback inicializada");

            cliente.subscribe(INICIO);
            Serial.println("subscrito a los inicios");

            cliente.subscribe(COMANDO);
            Serial.println("subscrito a los comandos");

            cliente.subscribe(RESPUESTA);
            Serial.println("subscrito a las respuestas");
            
            handshake();
        }else{
            Serial.print(" failed, rc=");
            Serial.print(cliente.state());
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