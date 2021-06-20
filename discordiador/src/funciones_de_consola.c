#include "funciones_de_consola.h"

void liberar_lista_string(char **lista)
{
    int i = 0;
    while (lista[i] != NULL)
    {
        free(lista[i]);
        i++;
    }
    free(lista);
}

int longitud_lista_string(char **lista)
{
    int i = 0;
    while (lista[i] != NULL)
    {
        i++;
    }
    return i;
}

void tarea_destroy(dis_tarea *t)
{
    free(t->nombre_tarea);
    free(t);
}

void iterator(dis_tarea *t)
{ // [ITERAR USANDO COMMONS]
    printf("%-18s | %3d | %3d | %3d | %3d\n", t->nombre_tarea, t->parametro, t->pos_x, t->pos_y, t->tiempo);
}

void iterador_listar_tripulantes(dis_tripulante *t)
{
    char *status;
    switch (t->estado)
    {
    case 0:
        status = "NEW";
        break;
    case 1:
        status = "READY";
        break;
    case 2:
        status = "EXEC";
        break;
    case 3:
        status = "BLOCK I/O";
        break;
    case 4:
        status = "EXIT";
        break;    
    default:
        break;
    }

    printf("Tripulante: %2d    Patota: %2d\tStatus: %s\n", t->id, t->id_patota, status);
}
void listar_tripulantes()
{
    printf("----------------------------------------------------------\n");
    char *tiempo = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    printf("Estado de la Nave: %s\n", tiempo);
    list_iterate(list_total_tripulantes, (void *)iterador_listar_tripulantes);
    // Lo mismo con las demas colas
    // list_iterate(cola_de_ready, (void*)iterador_listar_tripulantes);
    free(tiempo);
}

char * armar_tareas_para_enviar(char *path)
{
    FILE *archivo = fopen(path, "r");
    char caracteres[1000];
    strcpy(caracteres,"");
    while (feof(archivo) == 0)
    {
        char aux[100];
        fgets(aux, 100, archivo);

        aux[strlen(aux)-1] = '$';
        aux[strlen(aux)] = '\0';
        strcat(caracteres,aux);        
        printf("contenido de archivo actual %s",caracteres);

    }
    fclose(archivo);
    char * tareas = malloc(strlen(caracteres));
    tareas = caracteres;
    return tareas;
}

t_list *crear_tareas_global(char *path)
{
    FILE *archivo = fopen(path, "r");
    char caracteres[100];

    t_list * lista_tareas = list_create();

    while (feof(archivo) == 0)
    {
        fgets(caracteres, 100, archivo);
        char **str_spl = string_split(caracteres, ";");
        char **tarea_p = string_split(str_spl[0], " ");

        tarea *nueva_tarea = malloc(sizeof(tarea));

        char *nombre_de_tarea = string_duplicate(tarea_p[0]);
        
        nueva_tarea->nombre_tarea = nombre_de_tarea;
        nueva_tarea->cantidad_parametro = 1;
        nueva_tarea->parametro = atoi(tarea_p[1]);
        nueva_tarea->pos_x = atoi(str_spl[1]);
        nueva_tarea->pos_y = atoi(str_spl[2]);
        nueva_tarea->tiempo = atoi(str_spl[3]);

        list_add(lista_tareas, nueva_tarea);
        liberar_lista_string(str_spl);
        liberar_lista_string(tarea_p);
    }
    fclose(archivo);

    return lista_tareas;
}


t_list *crear_tareas_local(char *path)
{
    // TAREA PARAMETROS;POS X;POS Y;TIEMPO

    FILE *archivo = fopen(path, "r"); // ESTO SE CAMBIARA POR UNA RESPUESTA DEL FILE SYSTEM CREO
    char caracteres[100];

    // [LISTA]
    t_list *lista_tareas = list_create();

    while (feof(archivo) == 0)
    {
        fgets(caracteres, 100, archivo);
        char **str_spl = string_split(caracteres, ";");
        char **tarea_p = string_split(str_spl[0], " ");

        dis_tarea *nueva_tarea = malloc(sizeof(dis_tarea));

        char *nombre_de_tarea = string_duplicate(tarea_p[0]);
        nueva_tarea->estado_tarea = DISPONIBLE;
        nueva_tarea->nombre_tarea = nombre_de_tarea;
        nueva_tarea->parametro = atoi(tarea_p[1]);
        nueva_tarea->pos_x = atoi(str_spl[1]);
        nueva_tarea->pos_y = atoi(str_spl[2]);
        nueva_tarea->tiempo = atoi(str_spl[3]);

        list_add(lista_tareas, nueva_tarea);
        liberar_lista_string(str_spl);
        liberar_lista_string(tarea_p);
    }
    fclose(archivo);

    //Vemos que esta en memoria
    // printf("TAREA              |PARAM|POS_X|POS_Y| TIEMPO\n");
    // list_iterate(lista_tareas, (void*)iterator);

    return lista_tareas;
    //list_destroy_and_destroy_elements(lista_tareas, (void*) tarea_destroy);
}


//--------------------------------------------------------
void iniciar_patota_global(char **str_split)
{
    //pid_con_tareas = dis_patota
    int cant_tripulantes = atoi(str_split[1]);
    int tripu_pos_inicial_no_nula = longitud_lista_string(str_split) - 3;
    char *path = str_split[2]; // ACA ESTA el archivo con LAS TAREAS de la patota
    
    pid_con_tareas_y_tripulantes * patota_full = malloc(sizeof(pid_con_tareas_y_tripulantes));

    patota_full->pid = id_patota;
    //patota_full->tareas = crear_tareas_global(path); // crear_tareas crear uno que no spliteee o partir mi funcion ??
    patota_full->tareas = armar_tareas_para_enviar(path); // crear_tareas crear uno que no spliteee o partir mi funcion ??

    patota_full->tripulantes = list_create();

    for (int i = 0; i < cant_tripulantes; i++)
    {
        nuevo_tripulante_sin_pid *tripulante = malloc(sizeof(nuevo_tripulante_sin_pid)); // ESTE ES EL QUE USA MIRAM

        if (i < tripu_pos_inicial_no_nula)
        {
            char **posicion_tripulante = string_split(str_split[i + 3], "|");
            tripulante->pos_x = atoi(posicion_tripulante[0]);
            tripulante->pos_y = atoi(posicion_tripulante[1]);
            liberar_lista_string(posicion_tripulante);
        }
        else
        {
            tripulante->pos_x = 0;
            tripulante->pos_y = 0;
        }
        tripulante->tid = id_tcb;
        list_add(patota_full->tripulantes , tripulante);
        id_tcb++;
    }
    //guardo el tamaño en una variable para poder debugearlo
    uint32_t size_paquete;

    //serializo el paquete ------------------------------------------
    //VER SI ACTUALIZA SIZE_PAQUETE
    void *info = serializar_pid_con_tareas_y_tripulantes(patota_full,&size_paquete); 
    
    //socket = crear_conexion()
    enviar_paquete(conexion_mi_ram_hq, INICIAR_PATOTA, size_paquete, info);
    
    //recibo respuesta
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    //cerrar conexion
    
    if (paquete_recibido->codigo_operacion == RESPUESTA_INICIAR_PATOTA){
        printf("Recibi opcode de respuesta okfail\n");
        //desserializo la respuesta
        respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

        //analizo respuesta
        if (respuesta == RESPUESTA_OK)
        {
            printf("Recibi respuesta OK\n");
            //limpiar todo lo del envio
            //list_destroy_and_destroy_elements(lista_tareas, (void*) tarea_destroy); de la global 
            // CREAR UNA PARA DESTRUIRLA 
           // iniciar_patota_local(str_split);

        }
        else if (respuesta == RESPUESTA_FAIL)
        {
            printf("Recibi respuesta FAIL\n");
            // RESETEAR LOS VALORES DEL PATOTA Y ID_TRIP DE LAS VAR. GLOBALES
        }
        else
            printf("Recibi respuesta INVALIDA\n");
    }
    else
        printf("Recibi opcode de respuesta INVALIDO\n");

}


void iniciar_patota_local(t_list* tripulantes_sin_pid, t_list* tareas_de_serializacion,char **str_split)
{
    printf("LLEGO A PATOTA LOCAL\n");

    for (int i = 0; i < tripulantes_sin_pid->elements_count; i++)
    {
        nuevo_tripulante_sin_pid *trip = list_get(tripulantes_sin_pid,i);
        dis_tripulante *tripulante = malloc(sizeof(dis_tripulante));
        tripulante->pos.x = trip->pos_x;
        tripulante->pos.y = trip->pos_y;
        tripulante->id = trip->tid;
        //id_tcb++; ya estan creados en el global
        tripulante->id_patota = id_patota;
        tripulante->estado = NEW;

        sem_init(&tripulante->sem_tri, 0, 0); // inicializo el semaforo en 0 del tripulante

        pthread_t hilo_tripulante;
        pthread_create(&hilo_tripulante, NULL, (void *)ejecucion_tripulante, (int *)tripulante->id);

        agregar_elemento_a_cola(cola_de_new, &mutex_cola_de_new, tripulante);
        list_add(list_total_tripulantes, tripulante); // esto lo pondre en el for de confirmacion [PENDIENTE]
        sem_post(&sem_contador_cola_de_new);
    }

    id_patota++; //este si esta bien aca

    // LO DE TAREAS TAMBIEN UN FOR QUE SE ARREGLA
    char *path = str_split[2]; // ACA ESTA el archivo con LAS TAREAS de la patota
    dis_patota *new_patota = malloc(sizeof(dis_patota));
    new_patota->list_tareas = crear_tareas_local(path);
    new_patota->id_patota = id_patota;
    list_add(lista_de_patotas, new_patota);
}

dis_tarea *pedir_tarea(int id_patota)
{
    dis_patota *patota = list_get(lista_de_patotas, id_patota - 1);
    t_list *lista_tareas = patota->list_tareas;
    dis_tarea *tarea = NULL;
    pthread_mutex_lock(&mutex_tarea);
    printf("Tamaño lista tareas = %d\n", list_size(lista_tareas));
    if (list_size(lista_tareas) >= 1)
        tarea = list_remove(lista_tareas, 0);
    pthread_mutex_unlock(&mutex_tarea);

    return tarea; // como se manejara esto si ya no hay tareas
}

void mover_tri_hacia_tarea(dis_tarea *tarea, dis_tripulante *trip)
{
    int pasos = 0; // centinela
    if (trip->pos.x < tarea->pos_x)
    {
        trip->pos.x++;
        pasos++;
    }
    if (trip->pos.x > tarea->pos_x)
    {
        trip->pos.x--;
        pasos++;
    }

    if (pasos == 0)
    { // Me indica que esta en la misma posicion x
        if (trip->pos.y < tarea->pos_y)
        {
            trip->pos.y++;
        }
        if (trip->pos.y > tarea->pos_y)
        {
            trip->pos.y--;
        }
    }
}

bool tarea_es_de_entrada_salida(dis_tarea *tarea)
{

    if (strcmp(tarea->nombre_tarea, "GENERAR_OXIGENO") == 0)
        return true;
    if (strcmp(tarea->nombre_tarea, "CONSUMIR_OXIGENO") == 0)
        return true;
    if (strcmp(tarea->nombre_tarea, "GENERAR_COMIDA") == 0)
        return true;
    if (strcmp(tarea->nombre_tarea, "CONSUMIR_COMIDA") == 0)
        return true;
    if (strcmp(tarea->nombre_tarea, "GENERAR_BASURA") == 0)
        return true;
    if (strcmp(tarea->nombre_tarea, "DESCARTAR_BASURA") == 0)
        return true;

    return false;
}

void realizar_operacion(dis_tarea *tarea, dis_tripulante *trip)
{
    bool hice_ciclo_inicial_tarea_es = 0;
    //lock(tripulante->contador_ejecucion); mmm no esta bueno si estoy en FIFO
    if (tarea->pos_x != trip->pos.x || tarea->pos_y != trip->pos.y)
    {
        mover_tri_hacia_tarea(tarea, trip);
        // sleep(tiempo_retardo_ciclo_cpu); // CONSUME 1 CICLO DE CPU? EN REALIZAR UN PASO
        // No estaria mejor hacer el sleep afuera de todo
    }
    else
    {
        // [TRIPULANTE] Estoy parado en la posicion de la tarea  --------------------------------
        // [TIPO TAREA] ESTE ES de la E/S el de caminar es el de arriba
        // [TODO] tarea.es_de_entrada_salida  // Un BOOL
        // Implementar en la creacion {GENERAR_OXÍGENO,CONSUMIR_OXIGENO,GENERAR_COMIDA,CONSUMIR_COMIDA,GENERAR_BASURA,DESCARTAR_BASURA}
        if (tarea_es_de_entrada_salida(tarea))
        {

            // [VERIFICAMOS] Si ya realico el ciclo inicial de CPU de la tarea e/s
            if (hice_ciclo_inicial_tarea_es)
            {
                // Mandamos a mi-ram-hq
                // send(operacion_con_recurso);
                // recv(resultado_operacion_con_recurso);
                // Lo dormimos todo el tiempo y seteamos el tiempo----------------
                sleep(tarea->tiempo * tiempo_retardo_ciclo_cpu); //RETARDO_CICLO_CPU : VAR GLOBAL
                tarea->tiempo = 0;
                //wait(mutexA)
                trip->estado = READY;
                //signal(mutexA)
                hice_ciclo_inicial_tarea_es = 0;
            }
            else
            {
                // Lo duermo un ciclo y le cambio el estado al tripulante, en la siguiente iteracion entrara por el if
                sleep(tiempo_retardo_ciclo_cpu);
                hice_ciclo_inicial_tarea_es = 1;
                //wait(mutexA)
                trip->estado = BLOCKED_E_S;
                //signal(mutexA)
            }
        }
        else
        {
            // [NO ES TAREA DE RECURSO E/S] ----------------------------------------
            //  POR LO QUE SOLO CONSUMIMOS DE A 1 EL TIEMPO
            sleep(tiempo_retardo_ciclo_cpu);
            tarea->tiempo--;
        }
    }
    // ACTUALIZAMOS EL ESTADO DE LA TAREA
    if (tarea->tiempo == 0)
    {
        tarea->estado_tarea = TERMINADA; // Para que la tarea no se siga ejecutando
    }
}

void ejecucion_tripulante(int indice)
{
    // TRUCAZO COMO EL ID DEL TRIPULANTE ES 0,1,2,3 concuerda con el indice de la lista total
    // UPDATE PARA BUSCAR AL TRIP ES CON "indice-1" desicion de serializacion que usan el 0 como centinela
    dis_tripulante *trip = list_get(list_total_tripulantes, indice-1);
    sem_wait(&trip->sem_tri); // CUANDO PASA A READY EL PLANI LARGO PLAZO LE TIRA UN SIGNAL
    printf("No deberias pasar por aca\n");
    sleep(1);
    dis_tarea *tarea = pedir_tarea(trip->id_patota); // pedir_tarea tiene dentro un MUTEX
    
    sem_wait(&trip->sem_tri); // CUANDO PASA A EXEC LE TIRA UN SIGNAL
    // ESTOY EN EXEC voy a ejecutar la tarea que tengo

    while(tarea){ // tarea != NULL (PROBAR si no funciona)
        tarea -> estado_tarea = EN_CURSO;
        trip -> tarea_actual = tarea;

        while(trip -> tarea_actual -> estado_tarea == EN_CURSO){
            // CUANDO la tarea sea de sabotaje va a poder ejecutarse por si solo sin signals que mande el plani corto plazo(este en sabotaje debe estar pausado [ALTAS ESFERAS])
            if(trip-> tarea_actual -> es_de_sabotaje)
                sem_wait(&trip->sem_tri);

            realizar_operacion(trip -> tarea_actual, trip);
            // signal(trip->sem_tri); | Me lo da el planificador a corto plazo en condiciones normales
        }
        // Sale porque la funcion cambia realizar_operacion() : tarea->estado_tarea = TERMINADA;
        /*  1. NOTIFICAR A MIRAMHQ que la tarea finalizo ? Para la BITACORA tambien a i-Mongo-Store ?
            2. LIBERAR RECURSO de la tarea (free_tarea(*tarea))
        */

        // SI NO ES DE SABOTAJE: pido una nueva tarea / si lo era como la tarea estaba guardada en  (*tarea) se la vuelvo a asignar
        if(trip -> tarea_actual -> es_de_sabotaje)
            tarea = pedir_tarea(trip->id_patota);
    }

    // EL TRIPULANTE YA NO TIENE MAS TAREAS POR REALIZAR
    trip->estado = EXIT;
    // ME PASO A LA COLA DE FIN (usar la funcion_mutex de SANTIAGO)
}

void *gestion_patotas_a_memoria()
{

    while (1)
    {
        sem_wait(&sem_contador_cola_iniciar_patota);

        pthread_mutex_lock(&mutex_cola_iniciar_patota);
        // [POr que era el mutex] si es 1 solo hilo ???
        // Respuesta: en realidad son 2 hilos que van a estar toqueteando esta cola, el hilo de la consola haciendo
        //los push y este hilo haciendo el pop. Si yo soy hijo de puta puedo enviar a los pedos muchos mensajes juntos
        //de iniciar_patota y tal vez se caga.
        char **res = queue_pop(cola_de_iniciar_patotas);
        pthread_mutex_unlock(&mutex_cola_iniciar_patota);

        log_info(logger, "Procesando la patota con %s", res[1]);
        iniciar_patota_global(res);
        log_info(logger, "Patota %s creada", res[1]);
        liberar_lista_string(res);
    }
}