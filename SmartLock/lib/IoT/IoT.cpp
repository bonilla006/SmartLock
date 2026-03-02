#include "IoT.h"

//variables globales
extern LiquidCrystal_I2C lcd;
extern Servo servo;

//consigue el id de relacion
int IoT::get_relationID(){ return REL_ID; }

//inicializa el id de relacion
void IoT::set_relationID(int id){ REL_ID = id; }

//validar el id de relacion
bool IoT::validate_relationID(int rel_id){ return rel_id > 0; }

//consigue el estado del dispositivo
bool IoT::is_block(){ return BLOQUEADO; }

//inicializa o cambia el estado del dispositivo
void IoT::set_block(bool bloqueado){ BLOQUEADO = bloqueado; }

String IoT::get_localpssw(){ return localpssw; }

 //consigue la contraseña hash
String IoT::get_hashpssw(){ return hashpssw; }

//define la constraseña hash
void IoT::set_hashpssw(String pssw){ hashpssw = pssw; }

//verifica si tiene intentos posibles
// bool IoT::have_trys(){ return INTENTOS; }

// //elimina la posibilidad de intentos
// void IoT::no_trys(){ INTENTOS = false; }

//reset a la variable control para la contraseña
void IoT::reset(){
    current_len_passw = 0;
    localpssw = "";
    lcd.clear();
    lcd.setCursor(0, 0);
}

//despleager informacion al usuario
void IoT::display(const char *line1, const char *line2, int time){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    delay(time);
    lcd.clear();
}

//simula que abrio la cerradura
void IoT::open(){ servo.write(180); }

//simula que cerro la cerradura
void IoT::close(){ servo.write(90); }

//verifica si el dispositivo esta bloqueado o no
void IoT::status(){
    // Serial.print("en status");
    // Serial.print(BLOQUEADO);
    if(BLOQUEADO){
        //simula cerradura cerrada
        close();
        lcd.print("Contraseña:");
    }else{
        open();
        lcd.print("Bienvenido!");
    }
}

//procesa los caractares recibidos desde el keypad
void IoT::proces_passw(char digito){
    //llenar el posible passw con los digitos ingresados
    if(current_len_passw < 16){
        //mover el cursor segun ingresado
        lcd.setCursor(current_len_passw, 1);
        lcd.print("*");
        localpssw += digito;
        current_len_passw++;
    }
}