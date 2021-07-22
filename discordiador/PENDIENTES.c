Ｉ－ＭＯＮＧＯ 


// USAR la linea del log crear un char que sea esto
// log_info(logger, "[ Tripulante %i ] (%d,%d) => (%d,%d) | TAREA: %s - UBICACION: (%d,%d)", trip->id, pos_vieja_x, pos_vieja_y, trip->pos_x, trip->pos_y, tarea->nombre_tarea, tarea->pos_x, tarea->pos_y);
// Esta linea deberia estar despues de notificar_movimiento_a_miram(trip);
void guardar_movimiento_en_imongo(int id_trip , char* log_desplazamiento){
    /*      id_trip : id del Tripulante
            log_desplazamiento : "[ Tripulante 1 ] (2,1) => (3,1) | TAREA: REGAR_PLANTAS - UBICACION: (3,3)"
    */
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
    void * info = serializar_trip_con_char(id_trip,log_desplazamiento);
    uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(log_desplazamiento) + 1;
    enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
    close(conexion_i_mongo_store);
}    


'SHARED UTILS'
void *serializar_trip_con_char(uint32_t tid,char* palabra){
    /*    [  tid  | longitud_tarea |  palabra  ]
          uint32_t     uint32_t       uint32_t
    */
    int offset = 0;
    uint32_t longitud_nombre = strlen(palabra) + 1;
	void* stream = malloc(2*sizeof(uint32_t)+longitud_nombre);
    memcpy(stream+offset,&tid,sizeof(uint32_t));
    offset+= sizeof(uint32_t);
	memcpy(stream+offset,&longitud_nombre,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,tarea,longitud_nombre);
    return stream;
}

'-------------------------------------------------------------------------'



> Inicio de una tarea
// Averiguar como ponerlo al inicio para que solo se ejecute 1 ves la primera
void notificar_inicio_tarea_imongo(int id_trip , char* tarea){
    /*      id_trip : id del Tripulante
            tarea : "GENERAR_OXIGENO 10;4;4;15"  la nesecita para saber cuanto de oxigeno va a generar
    */
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
    void * info = serializar_trip_con_char(id_trip,tarea);
    uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(tarea) + 1;
    enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
    close(conexion_i_mongo_store);
} 


> Fin de una tarea
// Ponerlo cuando la duracion de la tarea es igual a 0
void notificar_fin_tarea_imongo(int id_trip , char* tarea){// char* nombre_tarea
    /*      id_trip : id del Tripulante
            tarea : "GENERAR_OXIGENO 10;4;4;15" / o "Fin de Trea GENERAR_OXIGENO" tarea->nombre
    */
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
    void * info = serializar_trip_con_char(id_trip,tarea);
    uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(tarea) + 1;
    enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
    close(conexion_i_mongo_store);
} 


> Atencion de un sabotaje
void notificar_atencion_sabotaje_imongo(int id_trip , char* tarea){// podriamos solo serializar el tid y luego desde imongo crear y guardar el log(decidan)
    /*      id_trip : id del Tripulante
            tarea : "Fin de Tarea SABOTAJE"
    */
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
    void * info = serializar_trip_con_char(id_trip,tarea);
    uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(tarea) + 1;
    enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
    close(conexion_i_mongo_store);
} 


> Resolucion del sabotaje
void notificar_fin_sabotaje_imongo(int id_trip , char* tarea){// podriamos solo serializar el tid y luego desde imongo crear y guardar el log(decidan)
    /*      id_trip : id del Tripulante
            tarea : "Fin de Tarea SABOTAJE"
    */
    int conexion_i_mongo_store = crear_conexion(ip_i_mongo_store, puerto_i_mongo_store);
    void * info = serializar_trip_con_char(id_trip,tarea);
    uint32_t size_paquete = 2*sizeof(uint32_t) + strlen(tarea) + 1;
    enviar_paquete(conexion_i_mongo_store, ACTUALIZAR_UBICACION, size_paquete, info); // Ver el opcode si hay uno mejor
    close(conexion_i_mongo_store);
}