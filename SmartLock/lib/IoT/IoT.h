#ifndef IoT_H
#define IoT_H

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

public:
    IoT(int rel_id, bool bloqueado){
        REL_ID = rel_id;
        BLOQUEADO =  bloqueado;
        // INTENTOS = intentos;
    }
    ~IoT();

    //consigue el id de relacion
    int get_relationID(){ return REL_ID; }

    //inicializa el id de relacion
    void set_relationID(int id){ REL_ID = id; }

    //validar el id de relacion
    bool validate_relationID(int rel_id){ return rel_id > 0; }

    //consigue el estado del dispositivo
    bool is_block(){ return BLOQUEADO; }

    //inicializa o cambia el estado del dispositivo
    void set_block(bool bloqueado){ BLOQUEADO = bloqueado; }

    //verifica si tiene intentos posibles
    // bool have_trys(){ return INTENTOS; }

    // //elimina la posibilidad de intentos
    // void no_trys(){ INTENTOS = false; }

    
};

#endif 