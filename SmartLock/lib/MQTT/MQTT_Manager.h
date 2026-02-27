#ifndef MQTT_Manager_H
#define MQTT_Manager_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class IoT;

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
    //TOPICOS
    const char* INICIO;
    const char* COMANDO;
    const char* RESPUESTA;

    //OBJETOS EXTERNOS
    WiFiClient conexion_http;
    PubSubClient iot;
    JsonDocument parser;

    static MQTTManager* instancia;

    //PUNTEROS PARA INTERACTUAR CON LA CLASE IoT
    IoT* p_dispositivo; 

public:
    //funcion para interactuar con el objeto IoT
    void use_IoT(IoT* );

    //constructor
    //primero se crea la instancia PubSubClient
    MQTTManager(const char* broker, const int port, const char* inicio, const char* comando, const char* respuesta):iot(conexion_http){
        iot.setServer(broker, port); //iniciar conexion con el broker

        INICIO = inicio;
        COMANDO = comando;
        RESPUESTA = respuesta;

        instancia = this;
    }
    ~MQTTManager();

    //wrapper de la funcion loop de PubSubClient
    void loop_wrapper();

    //traduce el json
    String translate_json(byte* , unsigned int );

    //maneja la lectura del mensaje
    String read_message(const char* );

    //maneja el envio del mensaje

    //maneja las respuestas
    void response(const char*, IoT* );

    //maneja los comandos
    void command(const char*, IoT* );

    //maneja la logica de los mensajes
    void message_manager(char* , byte* , unsigned int );
    
    //maneja la conexion a los asuntos
    void topic_conection();
    static void staticCallback(char*, byte*, unsigned int);

};

#endif