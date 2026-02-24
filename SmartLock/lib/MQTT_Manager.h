#ifndef MQTT_Manager_H
#define MQTT_Manager_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
/*
la clase MQTTManager es responsable de la comunicacion entre el IoT y el API
- necesita de:
    * el objeto cliente-wifi(priv)
    * el objeto mqtt(priv)
    * el broker mqtt(priv)
    * el puerto(priv)
- sus funciones son:
    * conectarse a los asuntos (publ)
    * publicar (publ)
    * subscribirse (publ)
*/
class MQTTManager{
private:
    WiFiClient conexion_http;
    PubSubClient iot;
    JsonDocument parser;

    const char* INICIO;
    const char* COMANDO;
    const char* RESPUESTA;

public:
    //constructor
    //primero se crea la instancia PubSubClient
    MQTTManager(const char* broker, const int port, const char* inicio, const char* comando, const char* respuesta):iot(conexion_http){
        iot.setServer(broker, port); //iniciar conexion con el broker

        INICIO = inicio;
        COMANDO = comando;
        RESPUESTA = respuesta;
    }
    ~MQTTManager();

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
    String read_message(const char* mensaje){
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
    void response(const char* accion){
        Serial.println("Procesando respuestas...");
        
        //identificacion del servidor con el IoT
        if(accion == "iot-ack"){
            //conseguir el id de relacion
            REL_ID = parser["rel_id"].as<String>();
            ESTADO = parser["estado"].as<String>();

        }else if(accion == "iot-val-pssw"){
            //ver si fue correcta a no la contraseña ingresada
            String estado_val = parser["val"].as<String>();
            if(estado_val == "exito"){
                String payload = "{\"acc\":\"serv-dsblk\",\"rel_id\":\" "+REL_ID+" \"}";
                iot.publish(COMANDO, payload.c_str());

            }else if(estado_val == "fallo"){
                String payload = "{\"acc\":\"serv-wrg-pssw\",\"rel_id\":\" "+REL_ID+" \"}";
                iot.publish(RESPUESTA, payload.c_str());

            }else{
                Serial.println("else in val-pssw");
            }
        }else if(accion == "iot-dsblk"){
            ESTADO = parser["estado"].as<String>(); //actualizar estado
            servo.write(180); //simula que se abrio la cerradura
        }else{
            Serial.println("...mas respuestas");
        }
    }

    //maneja los comandos
    void command(){}

    //maneja la logica de los mensajes
    void message_manager(char* topic, byte* payload, unsigned int length){
        String mensaje = "";
        mensaje = translate_json(payload, length);
        Serial.print("Mensaje recibido en: ");
        Serial.print(topic);
        Serial.print(" -> ");
        Serial.println(mensaje);

        //parsear el json; aka mensaje
        String accion = "";
        accion = read_message(mensaje.c_str());

        // Procesar según el asunto
        if (String(topic) == RESPUESTA) {
            response(accion.c_str());

        }else if (String(topic) == COMANDO) {
            Serial.println("Procesando comandos..."); 
            //la accion de abrir/cerrar viene del app
            if(accion == "iot-dsblk"){
                ESTADO = parser["estado"].as<String>(); //actualizar estado
                servo.write(180); //simula que se abrio la cerradura
            }else if(accion == "iot-blk"){
                ESTADO = parser["estado"].as<String>(); //actualizar estado
                servo.write(90); //simula que se cerro la cerradura

            }else if(accion == "iot-time-out"){
                //verificar si tengo intentos
                String intentos = parser["try"].as<String>();
                if(intentos == "si"){
                    lcd.clear();
                    displayMessage("No contraseña ", "30seg de bloqueo", parser["time"]);
                    reset();
                }else{
                    lcd.clear();
                    displayMessage("No contraseña ", "5min de bloqueo", parser["time"]);
                    reset();
                }
            }else{
                Serial.println("...mas comandos");
            }
        }else{
            Serial.println("standby in else");
        }
    }
    
    //maneja la conexion a los asuntos
    void topic_conection(){
        while(!iot.connected()){
            Serial.println("Iniciando conexion con broker-mqtt...");
            if(iot.connect("wokwi_smartlock")){
                Serial.println("conexion con broker-mqtt exitosa!");

                iot.setCallback(message_manager);
                Serial.println("funcion callback inicializada");

                iot.subscribe(inicio);
                Serial.println("subscrito a los inicios");

                iot.subscribe(comando);
                Serial.println("subscrito a los comandos");

                iot.subscribe(respuesta);
                Serial.println("subscrito a las respuestas");
            
            }else{
                Serial.print(" failed, rc=");
                Serial.print(iot.state());
                Serial.println("intentar en 5 segundos...");
                delay(5000);
            }
        }
    }

    
};

#endif