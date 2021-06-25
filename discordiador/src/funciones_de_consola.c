#include "funciones_de_consola.h"

void *gestion_patotas_a_memoria()
{

    while (1)
    {
        sem_wait(&sem_contador_cola_iniciar_patota);

        pthread_mutex_lock(&mutex_cola_iniciar_patota);
        char **res = queue_pop(cola_de_iniciar_patotas);
        pthread_mutex_unlock(&mutex_cola_iniciar_patota);

        log_info(logger, "[ Discordiador ] Procesando la patota con %s tripulantes...", res[1]);
        iniciar_patota_global(res);
        log_info(logger, "[ Discordiador ] Patota creada con ID %i", id_patota - 1);
        liberar_lista_string(res);
    }
}

void iniciar_patota_global(char **str_split)
{
    int cant_tripulantes = atoi(str_split[1]);
    int tripu_pos_inicial_no_nula = longitud_lista_string(str_split) - 3;
    char *path = str_split[2]; // ACA ESTA el archivo con LAS TAREAS de la patota

    pid_con_tareas_y_tripulantes *patota_full = malloc(sizeof(pid_con_tareas_y_tripulantes));

    patota_full->pid = id_patota;
    patota_full->tareas = armar_tareas_para_enviar(path);
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
        list_add(patota_full->tripulantes, tripulante);
        id_tcb++;
    }

    // Guardo el tamaño en una variable para poder debugearlo
    uint32_t size_paquete;

    // Serializo el paquete ------------------------------------------
    void *info = serializar_pid_con_tareas_y_tripulantes(patota_full,&size_paquete); 
    
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    enviar_paquete(conexion_mi_ram_hq, INICIAR_PATOTA, size_paquete, info);
    
    // recibo respuesta
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    close(conexion_mi_ram_hq);

    if (paquete_recibido->codigo_operacion == RESPUESTA_INICIAR_PATOTA){
        printf("Recibi opcode de respuesta okfail\n");
        //desserializo la respuesta
        respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

        //analizo respuesta
        if (respuesta == RESPUESTA_OK)
        {
            printf("Recibi respuesta OK\n");
            //ESTO SERA DEPRECADO ES TEMPORAL HASTA QUE MI RAM ME ENVIO LO QUE PIDO
            patota_full->tareas = crear_tareas_global(path); // sugiero cambiar nombre crear_tareas_discordiador()
            iniciar_patota_local(patota_full);
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

void iniciar_patota_local(pid_con_tareas_y_tripulantes *patota_full)
{
    t_list *tareas_de_serializacion = patota_full->tareas;
    t_list *tripulantes_sin_pid = patota_full->tripulantes;

    for (int i = 0; i < tripulantes_sin_pid->elements_count; i++)
    {
        nuevo_tripulante_sin_pid *trip = list_get(tripulantes_sin_pid, i);
        dis_tripulante *tripulante = malloc(sizeof(dis_tripulante));
        tripulante->pos_x = trip->pos_x;
        tripulante->pos_y = trip->pos_y;
        tripulante->id = trip->tid;
        tripulante->id_patota = id_patota;
        tripulante->estado = NEW;
        tripulante->hice_ciclo_inicial_tarea_es = false;
        tripulante->tareas_realizadas = 0;
        tripulante->expulsado = false;
        pthread_mutex_init(&(tripulante->mutex_expulsado), NULL);

        sem_init(&(tripulante->procesador), 0, 0); // Semaforo de fin de ciclo de ejecucion.
        sem_init(&(tripulante->sem_tri), 0, 0);    // Semaforo para que el tripulante pueda comenzar a ejecutar.
        pthread_t hilo_tripulante;
        pthread_create(&hilo_tripulante, NULL, (void *)ejecucion_tripulante, (int *)tripulante->id);

        tripulante->threadid = hilo_tripulante;

        agregar_elemento_a_cola(cola_de_new, &mutex_cola_de_new, tripulante);
        list_add(list_total_tripulantes, tripulante); // [TODO] esto lo pondre en el for de confirmacion [PENDIENTE]
        sem_post(&sem_contador_cola_de_new);
    }

    id_patota++;

    t_list *lista_tareas = list_create();
    // LO DE TAREAS TAMBIEN UN FOR QUE SE ARREGLA
    int cant_tareas = 0;
    for (int i = 0; i < tareas_de_serializacion->elements_count; i++)
    {
        tarea *tarea_ser = list_get(tareas_de_serializacion, i);
        dis_tarea *nueva_tarea = malloc(sizeof(dis_tarea));

        char *nombre_de_tarea = string_duplicate(tarea_ser->nombre_tarea);

        nueva_tarea->estado_tarea = DISPONIBLE;
        nueva_tarea->nombre_tarea = nombre_de_tarea;
        nueva_tarea->parametro = tarea_ser->parametro;
        nueva_tarea->pos_x = tarea_ser->pos_x;
        nueva_tarea->pos_y = tarea_ser->pos_y;
        nueva_tarea->tiempo = tarea_ser->tiempo;
        nueva_tarea->es_de_sabotaje = false;
        cant_tareas++;
        list_add(lista_tareas, nueva_tarea);
    }

    dis_patota *new_patota = malloc(sizeof(dis_patota));
    new_patota->list_tareas = lista_tareas;
    new_patota->id_patota = id_patota;
    new_patota->cantidad_de_tareas = cant_tareas;
    list_add(lista_de_patotas, new_patota);

    destroy_pid_con_tareas_y_tripulantes(patota_full); //YA NO ME SIRVE
}

//  FUNCION DEL HILO: TRIPULANTE ----------------------------
void ejecucion_tripulante(int indice)
{
    // TRUCAZO COMO EL ID DEL TRIPULANTE ES 0,1,2,3 concuerda con el indice de la lista total
    // UPDATE PARA BUSCAR AL TRIP ES CON "indice-1" decision de serializacion que usan el 0 como centinela
    dis_tripulante *trip = list_get(list_total_tripulantes, indice - 1);
    int indice_tarea = 0;
    sem_wait(&(trip->sem_tri)); // CUANDO PASA A READY EL PLANI LARGO PLAZO LE TIRA UN SIGNAL
    // modificar pedir_tarea(tid);
    char *tarea_miriam;
    tarea_miriam = pedir_tarea_miriam((uint32_t) trip->id);
    dis_tarea *tarea = pedir_tarea(trip->id_patota, indice_tarea);
    printf("Ya pedi mi tarea \n");
    // ESTOY EN EXEC voy a ejecuta2r la tarea que tengo

    while (tarea)
    { // tarea != NULL (PROBAR si no funciona)
        tarea->estado_tarea = EN_CURSO;
        trip->tarea_actual = tarea;
        int tiempo_original = tarea->tiempo;
        while (trip->tarea_actual->estado_tarea == EN_CURSO)
        {
            // CUANDO la tarea sea de sabotaje va a poder ejecutarse por si solo sin signals que mande el plani corto plazo (éste, en sabotaje, debe estar pausado [ALTAS ESFERAS])
            //if (!(trip->tarea_actual->es_de_sabotaje))
            sem_wait(&(trip->sem_tri));
            realizar_operacion(trip->tarea_actual, trip);
        }
        log_info(logger, "[ Tripulante %i ] Tarea %s TERMINADA", trip->id, tarea->nombre_tarea);
        // Sale porque la funcion cambia realizar_operacion() : tarea->estado_tarea = TERMINADA;
        /*  1. NOTIFICAR A MIRAMHQ que la tarea finalizo ? Para la BITACORA tambien a i-Mongo-Store ?
            2. LIBERAR RECURSO de la tarea (free_tarea(*tarea))
        */
        tarea->tiempo = tiempo_original; // Le devuelvo su tiempo original
        tarea->estado_tarea = EN_CURSO;  // Le devuelvo su estado dado que fue modificado a FINALIZAR
        //if (!(trip->tarea_actual->es_de_sabotaje))
        indice_tarea++;
        tarea_miriam = pedir_tarea_miriam((uint32_t)trip->id);
        tarea = pedir_tarea(trip->id_patota, indice_tarea);
    }
    // EL TRIPULANTE YA NO TIENE MAS TAREAS POR REALIZAR
    log_info(logger, "[ Tripulante %i ] Lista de tareas TERMINADA. Saliendo de la nave...", trip->id);
}

// FUNCIONES QUE USA EL HILO TRIPULANTE ----------------------
char *pedir_tarea_miriam(uint32_t tid)
{
    //DAMI crear conexion y enviar mensaje obtener proxima tarea
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void *info = pserializar_tid(tid); 
    uint32_t size_paquete = sizeof(uint32_t);
    enviar_paquete(conexion_mi_ram_hq, OBTENER_PROXIMA_TAREA, size_paquete, info); 
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    char* tarea_recibida = deserializar_tarea(paquete_recibido->stream);
    printf("Recibi tarea %s del tripulante %d\n",tarea_recibida,tid);
    close(conexion_mi_ram_hq);
    //chequear respuesta
    return tarea_recibida;
}

dis_tarea *pedir_tarea(int id_patota, int indice)
{

    dis_patota *patota = list_get(lista_de_patotas, id_patota - 1);
    t_list *lista_tareas = patota->list_tareas;
    dis_tarea *tarea = NULL;

    if (lista_tareas->elements_count > indice)
        tarea = list_get(lista_tareas, indice);

    return tarea;
}

void realizar_operacion(dis_tarea *tarea, dis_tripulante *trip)
{
    if (chequear_tripulante_expulsado(trip))
    {
        switch (trip->estado)
        {
        case EXEC:
            trip->estado = EXPULSADO;
            sem_post(&(trip->procesador));
            break;
        case BLOCKED_E_S:
            trip->estado = EXPULSADO;
            sem_post(&operacion_entrada_salida_finalizada);
            break;
        default:
            trip->estado = EXPULSADO;
            break;
        }
        int status = pthread_kill(trip->threadid, 0);
        if (status < 0)
            perror("pthread_kill failed");
        return;
    }
    /* crear_conexion
    enviar_mensaje
    close
    comparar mensaje con local*/
    //Mandar mensaje obtener_ubicacion a miriam
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void * info = pserializar_tid((uint32_t)trip->id);
    uint32_t size_paquete = sizeof(uint32_t);
    enviar_paquete(conexion_mi_ram_hq, OBTENER_UBICACION, size_paquete, info); 
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    posicion pos = deserializar_posicion(paquete_recibido->stream);
    printf("Recibi pos %d:%d del tripulante %d\n",pos.pos_x,pos.pos_y,trip->id);
    close(conexion_mi_ram_hq);
    //chequear respuesta

    if (tarea->pos_x != trip->pos_x || tarea->pos_y != trip->pos_y)
    {
        int pos_vieja_x = trip->pos_x;
        int pos_vieja_y = trip->pos_y;
       
        mover_tri_hacia_tarea(tarea, trip);
        
        //mensaje actualizar ubicacion a miriam
        tripulante_y_posicion tripulante_posicion;
        tripulante_posicion.tid = trip->id;
        tripulante_posicion.pos_x = trip->pos_x;
        tripulante_posicion.pos_y = trip->pos_y;
        
        int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
        void * info = serializar_tripulante_y_posicion(tripulante_posicion);
        uint32_t size_paquete = sizeof(uint32_t) *3;
        enviar_paquete(conexion_mi_ram_hq, ACTUALIZAR_UBICACION, size_paquete, info); 
        t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
        close(conexion_mi_ram_hq);

        if (paquete_recibido->codigo_operacion == RESPUESTA_ACTUALIZAR_UBICACION){
            printf("Recibi opcode de respuesta okfail\n");
            //desserializo la respuesta
            respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

            //analizo respuesta
            if (respuesta == RESPUESTA_OK)
            {
                printf("Recibi respuesta OK\n");
            }
            else if (respuesta == RESPUESTA_FAIL)
            {
                printf("Recibi respuesta FAIL\n");
            }
            else
                printf("Recibi respuesta INVALIDA\n");
        }
        else
            printf("Recibi opcode de respuesta INVALIDO\n");

        log_info(logger, "[ Tripulante %i ] (%d,%d) => (%d,%d) | TAREA: %s - UBICACION: (%d,%d)", trip->id, pos_vieja_x, pos_vieja_y, trip->pos_x, trip->pos_y, tarea->nombre_tarea, tarea->pos_x, tarea->pos_y);
        sem_post(&(trip->procesador));
        sleep(tiempo_retardo_ciclo_cpu);
    }
    else
    {
        // [TRIPULANTE] Estoy parado en la posicion de la tarea  --------------------------------
        // [TIPO TAREA] ESTE ES de la E/S el de caminar es el de arriba
        if (tarea_es_de_entrada_salida(tarea))
        {
            // [VERIFICAMOS] Si ya realico el ciclo inicial de CPU de la tarea e/s
            if (trip->hice_ciclo_inicial_tarea_es)
            {
                // Mandamos a mi-ram-hq
                // send(operacion_con_recurso);
                // recv(resultado_operacion_con_recurso);
                // Lo dormimos todo el tiempo y seteamos el tiempo----------------
                log_info(logger, "[ Tripulante %i ] Comenzando Tarea bloqueante %s. Duracion: %i", trip->id, tarea->nombre_tarea, tarea->tiempo);
                sleep(tiempo_retardo_ciclo_cpu);
                tarea->tiempo--;

                if (tarea->tiempo == 0)
                {
                    log_info(logger, "[ Tripulante %i ] Tarea bloqueante %s TERMINADA", trip->id, tarea->nombre_tarea);
                    trip->estado = READY;
                    trip->hice_ciclo_inicial_tarea_es = false;
                    trip->tareas_realizadas++;
                    tarea->estado_tarea = TERMINADA;
                }
                else
                {
                    log_info(logger, "[ Tripulante %i ] Tarea bloqueante %s. Tiempo restante: %i", trip->id, tarea->nombre_tarea, tarea->tiempo);
                }

                sem_post(&operacion_entrada_salida_finalizada);
            }
            else
            {
                // Lo duermo un ciclo y le cambio el estado al tripulante, en la siguiente iteracion entrara por el if
                sleep(tiempo_retardo_ciclo_cpu);
                trip->hice_ciclo_inicial_tarea_es = true;
                log_info(logger, "[ Tripulante %i ] Syscall realizada", trip->id);
                trip->estado = BLOCKED_E_S;
                sem_post(&(trip->procesador));
            }
        }
        else
        {
            // [NO ES TAREA DE RECURSO E/S] ----------------------------------------
            //  POR LO QUE SOLO CONSUMIMOS DE A 1 EL TIEMPO
            log_info(logger, "[ Tripulante %i ] Realizando Tarea no bloqueante %s. Tiempo restante: %d", trip->id, tarea->nombre_tarea, tarea->tiempo);
            sleep(tiempo_retardo_ciclo_cpu);
            tarea->tiempo--;

            // ACTUALIZAMOS EL ESTADO DE LA TAREA
            chequear_tarea_terminada(tarea, trip);
            chequear_tripulante_finalizado(trip);
            sem_post(&(trip->procesador));
        }
    }
}

//  CONSOLA: LISTAR_TRIPULANTES -----------------------------
void listar_tripulantes()
{
    printf("----------------------------------------------------------\n");
    char *tiempo = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    printf("Estado de la Nave: %s\n", tiempo);
    list_iterate(list_total_tripulantes, (void *)iterador_listar_tripulantes);
    free(tiempo);
}

//  CONSOLA: EXPULSAR_TRIPULANTE-----------------------------
void expulsar_tripulante(int id)
{
    dis_tripulante *trip = list_get(list_total_tripulantes, id - 1);
    pthread_mutex_lock(&(trip->mutex_expulsado));
    trip->expulsado = true;
    pthread_mutex_unlock(&(trip->mutex_expulsado));
}

/********************************************
***              𝗠𝗜-𝗥𝗔𝗠 𝗛𝗤             ***
********************************************/

t_list * armar_tareas_para_enviar(char *path)
{
    FILE *archivo = fopen(path, "r");
    char caracteres[100];
    t_list *lista_tareas = list_create();

    while (feof(archivo) == 0)
    {
        fgets(caracteres, 99, archivo);
        char * aux = malloc(strlen(caracteres));
        if(caracteres[strlen(caracteres)-1] == '\n')
            caracteres[strlen(caracteres)-1] = '\0';
        //else
        //    caracteres[strlen(caracteres)] = '\0';
        strcpy(aux,caracteres);
        list_add(lista_tareas, aux);
        log_info(logger, "contenido de archivo actual %s", aux);

    }
    fclose(archivo);
    return lista_tareas;
}


/************************************************************
***              𝗙𝗨𝗡𝗖𝗜𝗢𝗡𝗘𝗦   𝗦𝗘𝗖𝗨𝗡𝗗𝗔𝗥𝗜𝗔𝗦             ***
************************************************************/

bool chequear_tripulante_expulsado(dis_tripulante *tripulante)
{
    bool res = false;
    pthread_mutex_lock(&(tripulante->mutex_expulsado));
    res = tripulante->expulsado;
    pthread_mutex_unlock(&(tripulante->mutex_expulsado));
    return res;
}

void chequear_tarea_terminada(dis_tarea *tarea, dis_tripulante *tripulante)
{
    if (tarea->tiempo == 0)
    {
        tarea->estado_tarea = TERMINADA;
        tripulante->tareas_realizadas++;
    }
}

void chequear_tripulante_finalizado(dis_tripulante *tripulante)
{
    dis_patota *patota = list_get(lista_de_patotas, tripulante->id_patota - 1);
    printf("Realizadas vs totales: %i %i\n", tripulante->tareas_realizadas, patota->cantidad_de_tareas);
    if (tripulante->tareas_realizadas == patota->cantidad_de_tareas)
        tripulante->estado = EXIT;
}

// FUNCIONES COMUNES DE LISTAS ---------------------------
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

// LIBERAR - DESTRUIR ------------------------------------
void tarea_destroy(tarea *t)
{
    // USA tarea NO dis_tarea | Si quieres hacer con el otro cambialo
    free(t->nombre_tarea);
    free(t);
}

void destroy_pid_con_tareas_y_tripulantes(pid_con_tareas_y_tripulantes *patota_full)
{
    list_destroy_and_destroy_elements(patota_full->tareas, (void *)tarea_destroy);
    list_destroy_and_destroy_elements(patota_full->tripulantes, free);
    free(patota_full);
}

// ITERADORES - USO DE LAS COMMOS PARA LISTAS --------------
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
        status = "BLOCK SABOTAJE";
        break;
    case 5:
        status = "EXIT";
        break;
    case 6:
        status = "EXPULSADO";
        break;
    default:
        break;
    }

    printf("Tripulante: %2d    Patota: %2d\tStatus: %s\n", t->id, t->id_patota, status);
}

// CONSOLA: INICIAR PATOTA ------------------------------------
t_list *crear_tareas_global(char *path)
{
    FILE *archivo = fopen(path, "r");
    char caracteres[100];

    t_list *lista_tareas = list_create();

    while (feof(archivo) == 0)
    {
        fgets(caracteres, 100, archivo);
        char **str_spl = string_split(caracteres, ";");
        char **tarea_p = string_split(str_spl[0], " ");

        tarea *nueva_tarea = malloc(sizeof(tarea));

        char *nombre_de_tarea = string_duplicate(tarea_p[0]);
        nueva_tarea->nombre_tarea = nombre_de_tarea;

        bool tiene_parametro = longitud_lista_string(tarea_p) - 1;

        if (tiene_parametro)
        {
            nueva_tarea->cantidad_parametro = 1;
            nueva_tarea->parametro = atoi(tarea_p[1]);
        }
        else
            nueva_tarea->cantidad_parametro = 0;

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

void mover_tri_hacia_tarea(dis_tarea *tarea, dis_tripulante *trip)
{
    int pasos = 0; // centinela
    if (trip->pos_x < tarea->pos_x)
    {
        trip->pos_x++;
        pasos++;
    }
    if (trip->pos_x > tarea->pos_x)
    {
        trip->pos_x--;
        pasos++;
    }

    if (pasos == 0)
    { // Me indica que esta en la misma posicion x
        if (trip->pos_y < tarea->pos_y)
        {
            trip->pos_y++;
        }
        if (trip->pos_y > tarea->pos_y)
        {
            trip->pos_y--;
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