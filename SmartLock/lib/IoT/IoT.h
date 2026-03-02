#ifndef IoT_H
#define IoT_H
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
/*
La clase IoT es responsable de almacenar informacion crucial:
- necesita de:
    * el id de relacion
    * el estado
- sus funciones son:
    * get/set para el id de relacion
    * get/set del estado
*/
class IoT{
private:
    int REL_ID = -1;
    bool BLOQUEADO = true;
    // bool INTENTOS = true;
    
    String localpssw = ""; // 48754849, para poder visualizar y hash
    String hashpssw = "";
    byte current_len_passw = 0;

public:
    IoT(int rel_id, bool bloqueado){
        REL_ID = rel_id;
        BLOQUEADO =  bloqueado;
        // INTENTOS = intentos;
    }
    //~IoT();

    //consigue el id de relacion
    int get_relationID();

    //define el id de relacion
    void set_relationID(int );

    //validar el id de relacion
    bool validate_relationID(int );

    //consigue el estado del dispositivo
    bool is_block();

    //define o cambia el estado del dispositivo
    void set_block(bool );

    //consigue la contraseña en texto plano
    String get_localpssw();

    //define la contraseña en texto plano
    //void set_localpssw(String );

    //consigue la contraseña hash
    String get_hashpssw();

    //define la constraseña hash
    void set_hashpssw(String );

    //verifica si tiene intentos posibles
    // bool have_trys();

    //elimina la posibilidad de intentos
    // void no_trys();

    //limpia contraseña ingresada
    void reset();

    //despliega informacion
    void display(const char* ,const char* ,int );
    
    //abrir cerradura
    void open();

    //cerrar cerradura
    void close();

    //maneja el estado del dispositivo
    void status();

    //procesa la contraseña
    void proces_passw(char );
};
#endif 