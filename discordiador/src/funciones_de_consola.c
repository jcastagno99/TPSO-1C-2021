#include "funciones_de_consola.h"

void *gestion_patotas_a_memoria()
{

    while (1)
    {
        sem_wait(&sem_contador_cola_iniciar_patota);

        pthread_mutex_lock(&mutex_cola_iniciar_patota);
        char **res = queue_pop(cola_de_iniciar_patotas);// TA BIEN
        pthread_mutex_unlock(&mutex_cola_iniciar_patota);

        log_info(logger, "[ Discordiador ] Procesando la patota con %s tripulantes...", res[1]);
        iniciar_patota(res);
        liberar_lista_string(res);
    }
}

// Cambiar nombre quitar el global
void iniciar_patota(char **str_split)
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

    // ENVIAR A MI-RAM
    uint32_t size_paquete;
    void *info = serializar_pid_con_tareas_y_tripulantes(patota_full,&size_paquete); 
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    enviar_paquete(conexion_mi_ram_hq, INICIAR_PATOTA, size_paquete, info);
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    close(conexion_mi_ram_hq);

    if (paquete_recibido->codigo_operacion == RESPUESTA_INICIAR_PATOTA){
        printf("Recibi opcode de respuesta okfail\n");
        respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

        // ANALIZAR LAS RESPUESTAS
        if (respuesta == RESPUESTA_OK)
        {
            log_info(logger, "[ Discordiador ] Patota creada con ID %i", id_patota);
            iniciar_patota_local(patota_full);
        }
        else {
            if (respuesta == RESPUESTA_FAIL)
                printf("Recibi respuesta FAIL de iniciar patota\n");
            else
                printf("Recibi respuesta INVALIDA\n");

            for (int i = 0; i < cant_tripulantes; i++){
                id_tcb--;
            }
            //destroy_pid_con_tareas_y_tripulantes(patota_full); //YA NO ME SIRVE
            log_error(logger, "[ MI-RAM-HQ ]  No se pudo crear la patota");
        }
    }
    else{
        printf("Recibi opcode de respuesta INVALIDO\n");
        for (int i = 0; i < cant_tripulantes; i++){
                id_tcb--;
        }
        //destroy_pid_con_tareas_y_tripulantes(patota_full); //YA NO ME SIRVE
        log_error(logger, "[ MI-RAM-HQ ]  No se pudo crear la patota");
    }
        
}

// Carga las estrucuras localmente
void iniciar_patota_local(pid_con_tareas_y_tripulantes *patota_full)
{
    //t_list *tareas_de_serializacion = patota_full->tareas;
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

        agregar_elemento_a_cola(cola_de_new, &mutex_cola_de_new, tripulante);
        list_add(list_total_tripulantes, tripulante); // [TODO] esto lo pondre en el for de confirmacion [PENDIENTE]
        
        pthread_t hilo_tripulante;
        pthread_create(&hilo_tripulante, NULL, (void *)ejecucion_tripulante, (int *)tripulante->id);
        tripulante->threadid = hilo_tripulante;

        sem_post(&sem_contador_cola_de_new);
    }

    id_patota++;

    dis_patota *new_patota = malloc(sizeof(dis_patota));
    new_patota->id_patota = id_patota;
    new_patota->cantidad_de_tareas = patota_full->tareas->elements_count;
    list_add(lista_de_patotas, new_patota);

    //destroy_pid_con_tareas_y_tripulantes(patota_full); //CUIDADO CON ESTO HACIA SEGMENT FAULT QUISA CON ALGUNA ACTUALIZACION DE LAS ESTRUCTURAS
}

//  FUNCION DEL HILO: TRIPULANTE ----------------------------
void ejecucion_tripulante(int indice)
{
    dis_tripulante *trip = list_get(list_total_tripulantes, indice - 1);

    sem_wait(&(trip->sem_tri)); // CUANDO PASA A READY EL PLANI LARGO PLAZO LE TIRA UN SIGNAL

    dis_tarea *tarea = pedir_tarea_miriam((uint32_t)trip->id);
    log_info(logger,"[ Tripulante %d ] pidio una tarea a MI-RAM-HQ y recibio %s\n",trip->id,tarea->nombre_tarea);
    // ESTOY EN EXEC voy a ejecutar la tarea que tengo

    while (tarea != NULL){
        tarea->estado_tarea = EN_CURSO;// ME parece que no sirve porque cuando creo una tarea la seteo con 'EN_CURSO'
        trip->tarea_actual = tarea;
        while (trip->tarea_actual->estado_tarea == EN_CURSO)
        {
            // CUANDO la tarea sea de sabotaje va a poder ejecutarse por si solo sin signals que mande el plani corto plazo (éste, en sabotaje, debe estar pausado [ALTAS ESFERAS])
            //if (!(trip->tarea_actual->es_de_sabotaje))
            sem_wait(&(trip->sem_tri));
            realizar_operacion(trip->tarea_actual, trip);
            // NO pord dios ese log de la linea 180
            //log_info(logger, "[ Tripulante %i ] Tarea %s en estado %i. El estado TERMINADO es %i", trip->id, tarea->nombre_tarea, trip->tarea_actual->estado_tarea, TERMINADA);
        }
        log_info(logger, "[ Tripulante %i ] Tarea %s TERMINADA", trip->id, trip->tarea_actual->nombre_tarea);
        // Sale porque la funcion cambia realizar_operacion() : tarea->estado_tarea = TERMINADA;
        /*  1. NOTIFICAR A IMONGO que la tarea finalizo ? Para la BITACORA
            2. LIBERAR RECURSO de la tarea (free_tarea(*tarea))
        */

        // Solo el trip que tenga una tarea con nombre SABOTAJE podra entrar en el IF
        if (strcmp(trip->tarea_actual->nombre_tarea, "SABOTAJE") == 0){
            //No hago nada para que se setee la tarea vieja. El wait lo hago para no pisar el valor y que el hilo sabotaje lea "EN_CURSO".
            sem_wait(&sem_sabotaje);
            sem_post(&sem_sabotaje_fin);
        }
        else
        {
            printf("\033[1;34mTRIP %d : pidiendo prox TAREA\033[0m\n",trip->id);
            tarea = pedir_tarea_miriam((uint32_t)trip->id);
            if (tarea!=NULL)
                log_info(logger, "[ Tripulante %i ] Tarea %s TERMINADA", trip->id, trip->tarea_actual->nombre_tarea);
        }
    }
    // EL TRIPULANTE YA NO TIENE MAS TAREAS POR REALIZAR
    log_info(logger, "[ Tripulante %i ] Lista de tareas TERMINADA. Estado %i. El de EXIT es %i. Saliendo de la nave...", trip->id, trip->estado, EXIT);
}

void realizar_operacion(dis_tarea *tarea, dis_tripulante *trip)
{
    if (chequear_tripulante_expulsado(trip))
    {
        switch (trip->estado)
        {
        case EXEC:
            trip->estado = EXPULSADO;
            actualizar_estado_miriam(trip->id,trip->estado);
            sem_post(&(trip->procesador));
            break;
        case BLOCKED_E_S:
            trip->estado = EXPULSADO;
            actualizar_estado_miriam(trip->id,trip->estado);
            sem_post(&operacion_entrada_salida_finalizada);
            break;
        default:
            trip->estado = EXPULSADO;
            actualizar_estado_miriam(trip->id,trip->estado);
            break;
        }
        // Terminna el swicht y chau
        int status = pthread_kill(trip->threadid, 0);
        if (status < 0)
            perror("pthread_kill failed");
    }

    if (tarea->pos_x != trip->pos_x || tarea->pos_y != trip->pos_y)
    {
        int pos_vieja_x = trip->pos_x;
        int pos_vieja_y = trip->pos_y;
        mover_tri_hacia_tarea(tarea, trip);
        notificar_movimiento_a_miram(trip);
        log_info(logger, "[ Tripulante %i ] (%d,%d) => (%d,%d) | TAREA: %s - UBICACION: (%d,%d)", trip->id, pos_vieja_x, pos_vieja_y, trip->pos_x, trip->pos_y, tarea->nombre_tarea, tarea->pos_x, tarea->pos_y);
        sleep(tiempo_retardo_ciclo_cpu);
        sem_post(&(trip->procesador));
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
                    trip->tareas_realizadas++; // TODO hay un error por aca. por alguna razon no se incrementa y no me pasa a exit al tripulante, por lo que los procesadores se bloquean
                    tarea->estado_tarea = TERMINADA;

                    dis_patota *patota = list_get(lista_de_patotas, trip->id_patota - 1); //TODO borrar estas 2 lineas
                    log_info(logger, "[ Tripulante %i ] Realizadas %i - Totales %i\n", trip->id, trip->tareas_realizadas, patota->cantidad_de_tareas);
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
            log_info(logger, "[ Tripulante %i ] Realizando Tarea no bloqueante %s. Tiempo restante: %i", trip->id, tarea->nombre_tarea, tarea->tiempo);
            sleep(tiempo_retardo_ciclo_cpu);
            tarea->tiempo = tarea->tiempo - 1;
            //log_info(logger, "[ Tripulante %i ] Ciclo de Tarea no bloqueante %s. Tiempo restante: %i", trip->id, tarea->nombre_tarea, tarea->tiempo);
            // ACTUALIZAMOS EL ESTADO DE LA TAREA
            chequear_tarea_terminada(tarea, trip);
            chequear_tripulante_finalizado(trip);
            sem_post(&(trip->procesador));
        }
    }
}

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
    //log_info(logger, "[ Tripulante %i ] Realizando Tarea no bloqueante %s. Tiempo restante: %i", tripulante->id, tarea->nombre_tarea, tarea->tiempo);
    // log_info(logger, "[ Tripulante %i ] Tiempo %i", tripulante->id, tarea->tiempo);
    if (tarea->tiempo == 0)
    {
        log_info(logger, "[ Tripulante %i ] Realizando Tarea no bloqueante %s. Tiempo restante: %i", tripulante->id, tarea->nombre_tarea, tarea->tiempo);
        tarea->estado_tarea = TERMINADA;
        
        if (strcmp(tarea->nombre_tarea, "SABOTAJE") != 0)
        {   // LA TAREA DE SABOTAJE NO CUENTA COMO TAREA REALIZADA
            tripulante->tareas_realizadas++;
        }
        
    }
}

void chequear_tripulante_finalizado(dis_tripulante *tripulante)
{
    dis_patota *patota = list_get(lista_de_patotas, tripulante->id_patota - 1);
    //log_info(logger, "[ Tripulante %i ] Realizadas %i - Totales %i\n", tripulante->id, tripulante->tareas_realizadas, patota->cantidad_de_tareas); //TODO borrarlo
    if (tripulante->tareas_realizadas == patota->cantidad_de_tareas)
    {
        tripulante->estado = EXIT;
        actualizar_estado_miriam(tripulante->id,tripulante->estado); 
    }
}

//  CONSOLA: LISTAR_TRIPULANTES -----------------------------
void listar_tripulantes()
{
    printf("----------------------------------------------------------\n");
    char *tiempo = temporal_get_string_time("%d/%m/%y %H:%M:%S");
    printf("Estado de la Nave: %s\n", tiempo);
    list_iterate(list_total_tripulantes, (void *)iterador_listar_tripulantes);
    printf("Ready: %i | Exec: %i | Bloq: %i\n", list_size(cola_de_ready), list_size(cola_de_exec), list_size(cola_de_block));
    free(tiempo);
}

int get_indice(t_list* l,int id_trip){
    for (int i = 0; i < l->elements_count; i++)
    {
        dis_tripulante* t = list_get(l,i);
        if ( t->id == id_trip )
            return i;   
    }
    return 666;
} 

//  CONSOLA: EXPULSAR_TRIPULANTE-----------------------------
void expulsar_tripulante_local(int id)
{
    dis_tripulante *trip = list_get(list_total_tripulantes, id - 1);
    // [MUTEX] Para asegurar que se eliminen de auno
    pthread_mutex_lock(&(trip->mutex_expulsado));
    trip->expulsado = true;
    pthread_mutex_unlock(&(trip->mutex_expulsado));
    
    // [PARCHE] SI ESTA EN NEW, no deberia pedir tarea ni seguir estando en new
    // [ISSUE] esto funciona, supongo que cuando este en READY cuando pase a exec se dara cuenta
    if (trip->estado==NEW)
    {
        trip->estado=EXPULSADO;
        actualizar_estado_miriam(trip->id,trip->estado);
        sem_wait(&sem_contador_cola_de_new); // esto es para quitarle 1 push

        //tengo que sacarlo de NEW porque se quedaria en esa cola/ listar trip usa la lista global
        int indice = get_indice(cola_de_new,trip->id); // se podria fucionar con el list_remove con get_indice

        pthread_mutex_lock(&mutex_cola_de_new);
	    list_remove(cola_de_new, indice);
	    pthread_mutex_unlock(&mutex_cola_de_new);

        int status = pthread_kill(trip->threadid, 0);
        if (status < 0)
            perror("pthread_kill failed");
    }
    
    if (trip->estado==READY && planificacion_pausada==true){ // Si no esta pausado corre el protocolo que acordamos
        trip->estado=EXPULSADO;
        actualizar_estado_miriam(trip->id,trip->estado);
        sem_wait(&sem_contador_cola_de_ready); // De lo contrario el procesador pedira 1 trip mas cuando este vacia la lista de READY

        int indice = get_indice(cola_de_ready,trip->id);

        pthread_mutex_lock(&mutex_cola_de_ready);
	    list_remove(cola_de_ready, indice);
	    pthread_mutex_unlock(&mutex_cola_de_ready);

        int status = pthread_kill(trip->threadid, 0);
        if (status < 0)
            perror("pthread_kill failed");
    }
    

}

/*******************************************************
***                   𝗠𝗜-𝗥𝗔𝗠 𝗛𝗤                   ***
*******************************************************/

dis_tarea* pedir_tarea_miriam(uint32_t tid)
{
    // PEDIR A MI-RAM-HQ -----------------------------------------
    printf("\033[1;33mLinea 380 | %d * \033[0m\n",tid);
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void *info = pserializar_tid(tid); 
    uint32_t size_paquete = sizeof(uint32_t);
    printf("\033[1;33mLinea 380 | %d Pidiendo tarea \033[0m\n",tid);
    enviar_paquete(conexion_mi_ram_hq, OBTENER_PROXIMA_TAREA, size_paquete, info);
    printf("\033[1;33mLinea 380 | %d Tarea pedida! Recibiendo tarea ... \033[0m\n",tid);
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    printf("\033[1;33mLinea 380 | %d Paquete recibido!! \033[0m\n",tid);
    char* tarea_recibida = deserializar_tarea(paquete_recibido->stream);
    printf("\033[1;33mLinea 380 | %d Tarea deserializada! \033[0m\n",tid);
    //Recibi tarea IRSE_A_DORMIR;9;9;1 del tripulante 1 PODEMOS LOGUEARLO
    //printf("Recibi tarea %s del tripulante %d\n",tarea_recibida,tid);

    if(strlen(tarea_recibida)==0){
        printf("Ya no hay mas proxima tarea\n");
        return NULL;
    }
    
    close(conexion_mi_ram_hq);
    
    // SPLITEAR TAREA RECIBIDA -------------------------------------------
    char **str_spl = string_split(tarea_recibida, ";");
    char **tarea_p = string_split(str_spl[0], " ");

    dis_tarea *nueva_tarea = malloc(sizeof(dis_tarea));

    char *nombre_de_tarea = string_duplicate(tarea_p[0]);
    bool tiene_parametro = longitud_lista_string(tarea_p) - 1;

    if (tiene_parametro){
        //nueva_tarea->cantidad_parametro = 1; NO LO USAMOS APARTE CREO QUE CUANDO LE MANDAMOS A IMONGO LE MANDAMOS UN CHAR*
        nueva_tarea->parametro = atoi(tarea_p[1]);
    }
    nueva_tarea->estado_tarea = DISPONIBLE;
    nueva_tarea->nombre_tarea = nombre_de_tarea;

    nueva_tarea->pos_x = atoi(str_spl[1]);
    nueva_tarea->pos_y = atoi(str_spl[2]);
    nueva_tarea->tiempo = atoi(str_spl[3]);
    nueva_tarea->es_de_sabotaje = false;// NUNCA LO USAMOS para identificar usamos que el nombre de la tarea sea "SABOTAJE"

    liberar_lista_string(str_spl);
    liberar_lista_string(tarea_p);
    return nueva_tarea;
}

void notificar_movimiento_a_miram(dis_tripulante *trip){

    //              𝗦𝗘𝗥𝗜𝗔𝗟𝗜𝗭𝗔𝗥
    tripulante_y_posicion tripulante_posicion;
    tripulante_posicion.tid = trip->id;
    tripulante_posicion.pos_x = trip->pos_x;
    tripulante_posicion.pos_y = trip->pos_y;
    int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void * info = serializar_tripulante_y_posicion(tripulante_posicion);
    uint32_t size_paquete = sizeof(uint32_t) *3;
    enviar_paquete(conexion_mi_ram_hq, ACTUALIZAR_UBICACION, size_paquete, info); 
    //t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    close(conexion_mi_ram_hq);

    // Verificar respuesta: Ver si es nesesario en un futuro
    // if (paquete_recibido->codigo_operacion == RESPUESTA_ACTUALIZAR_UBICACION){
    //         printf("Recibi opcode de respuesta okfail\n");
    //         //desserializo la respuesta
    //         respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);

    //         //analizo respuesta
    //         if (respuesta == RESPUESTA_OK)
    //         {
    //             printf("Recibi respuesta OK\n");
    //         }
    //         else if (respuesta == RESPUESTA_FAIL)
    //         {
    //             printf("Recibi respuesta FAIL\n");
    //         }
    //         else
    //             printf("Recibi respuesta INVALIDA\n");
    //     }
    //     else
    //         printf("Recibi opcode de respuesta INVALIDO\n");
}


/************************************************************
***              𝗙𝗨𝗡𝗖𝗜𝗢𝗡𝗘𝗦   𝗦𝗘𝗖𝗨𝗡𝗗𝗔𝗥𝗜𝗔𝗦             ***
************************************************************/

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
    list_destroy_and_destroy_elements(patota_full->tareas, free);
    list_destroy_and_destroy_elements(patota_full->tripulantes, free);
    free(patota_full);
}

// ITERADORES - USO DE LAS COMMOS PARA LISTAS --------------
void iterator(dis_tarea *t)
{ // [ITERAR USANDO COMMONS]
    printf("%-18s | %3d | %3d | %3d | %3d\n", t->nombre_tarea, t->parametro, t->pos_x, t->pos_y, t->tiempo);
}

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

    }
    fclose(archivo);
    return lista_tareas;
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