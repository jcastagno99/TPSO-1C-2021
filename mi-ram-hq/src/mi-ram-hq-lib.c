#include "mi-ram-hq-lib.h"
#include <stdlib.h>

void funcion_test_memoria(uint32_t);
void recorrer_pcb(t_segmento * );
void recorrer_tareas(t_segmento * );
void recorrer_tcb(t_list * );

mi_ram_hq_config *leer_config_mi_ram_hq(char *path)
{
	t_config *config_aux = config_create(path);
	mi_ram_hq_config *config_mi_ram_hq_aux = malloc(sizeof(mi_ram_hq_config));

	config_mi_ram_hq_aux->TAMANIO_MEMORIA = config_get_int_value(config_aux, "TAMANIO_MEMORIA");
	config_mi_ram_hq_aux->ESQUEMA_MEMORIA = config_get_string_value(config_aux, "ESQUEMA_MEMORIA");
	config_mi_ram_hq_aux->TAMANIO_PAGINA = config_get_int_value(config_aux, "TAMANIO_PAGINA");
	config_mi_ram_hq_aux->TAMANIO_SWAP = config_get_int_value(config_aux, "TAMANIO_SWAP");
	config_mi_ram_hq_aux->PATH_SWAP = config_get_string_value(config_aux, "PATH_SWAP");
	config_mi_ram_hq_aux->ALGORITMO_REEMPLAZO = config_get_string_value(config_aux, "ALGORITMO_REEMPLAZO");
	config_mi_ram_hq_aux->CRITERIO_SELECCION = config_get_string_value(config_aux, "CRITERIO_SELECCION");
	config_mi_ram_hq_aux->PUERTO = config_get_int_value(config_aux, "PUERTO");

	return config_mi_ram_hq_aux;
}

t_log *iniciar_logger_mi_ram_hq(char *path)
{

	t_log *log_aux = log_create(path, "MI-RAM-HQ", 1, LOG_LEVEL_INFO);
	return log_aux;
}

void *esperar_conexion(int puerto)
{
	t_list *hilos = list_create();
	int socket_escucha = iniciar_servidor_mi_ram_hq(puerto);
	int socket_mi_ram_hq;
	log_info(logger_ram_hq, "SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_mi_ram_hq = accept(socket_escucha, &cliente, &len);
		if (socket_mi_ram_hq > 0)
		{
			log_info(logger_ram_hq, "NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_mi_ram_hq);
		}
		else
		{
			log_error(logger_ram_hq, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
		}
	} while (1);
}

int iniciar_servidor_mi_ram_hq(int puerto)
{
	int socket_v;
	int val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(puerto);

	socket_v = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_v < 0)
	{
		char *error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket_v, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket_v, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		return EXIT_FAILURE;
	}
	if (listen(socket_v, MAX_CLIENTS) < 0)
	{
		return EXIT_FAILURE;
	}

	return socket_v;
}

void crear_hilo_para_manejar_suscripciones(t_list *lista_hilos, int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_create(&hilo_conectado, NULL, (void *)manejar_suscripciones_mi_ram_hq, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);

}

void *manejar_suscripciones_mi_ram_hq(int *socket_hilo)
{
	log_info(logger_ram_hq, "Se creó el hilo correctamente");
	//sacar este while 1 que lo hice por como esta la comunicacion ahora.
	// cuando discord lo haga de a 1 mensaje x socket lo saco
	t_paquete *paquete = recibir_paquete(*socket_hilo);
	if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		switch (paquete->codigo_operacion)
		{
		case INICIAR_PATOTA:
		{
			pid_con_tareas_y_tripulantes_miriam patota_con_tareas_y_tripulantes = deserializar_pid_con_tareas_y_tripulantes(paquete->stream);
			respuesta_ok_fail resultado = iniciar_patota_segmentacion(patota_con_tareas_y_tripulantes);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case INICIAR_TRIPULANTE:
		{
			nuevo_tripulante nuevo_tripulante = deserializar_nuevo_tripulante(paquete->stream);
			respuesta_ok_fail resultado = iniciar_tripulante_segmentacion(nuevo_tripulante);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			respuesta_ok_fail resultado = actualizar_ubicacion_segmentacion(tripulante_y_posicion);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			tarea proxima_tarea = obtener_proxima_tarea_segmentacion(tripulante_tid); 
			void *respuesta = serializar_tarea(proxima_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, sizeof(tarea), respuesta);
			break;
		}
		case EXPULSAR_TRIPULANTE:
		{
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			//funcion_test_memoria(tripulante_tid);
			log_info(logger_ram_hq,"Llego el mensaje expulsar tripulante %i",tripulante_tid);
			respuesta_ok_fail resultado = expulsar_tripulante_segmentacion(tripulante_tid);
			//obtener_todos_los_datos(); // buscar el pid que tenga esto. Instanciar todo
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_EXPULSAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_ESTADO:
		{
			uint32_t tripulante_tid = deserializar_pid(paquete->stream);
			estado estado = obtener_estado_segmentacion(tripulante_tid);
			void *respuesta = serializar_estado(estado);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_ESTADO, sizeof(estado), respuesta);
			break;
		}
		case OBTENER_UBICACION:
		{
			uint32_t tripulante_tid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_UBICACION
			posicion posicion = obtener_ubicacion_segmentacion(tripulante_tid);
			void *respuesta = serializar_posicion(posicion);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_UBICACION, sizeof(posicion), respuesta);
			break;
		}
		default:
		{
			log_error(logger_ram_hq, "El codigo de operacion es invalido");
			break;
		}
		}
	}
	else if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
	{
		//TODO: Mismo handler pero con las funciones que utilizan el esquema de memoria de PAGINACION
		switch (paquete->codigo_operacion)
		{
		case INICIAR_PATOTA:
		{
			pid_con_tareas patota_con_tareas = deserializar_pid_con_tareas(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de INICIAR_PATOTA
			respuesta_ok_fail resultado = iniciar_patota_paginacion(patota_con_tareas);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case INICIAR_TRIPULANTE:
		{
			nuevo_tripulante nuevo_tripulante = deserializar_nuevo_tripulante(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de INICIAR_TRIPULANTE
			respuesta_ok_fail resultado = iniciar_tripulante_paginacion(nuevo_tripulante);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de ACTUALIZAR_UBICACION
			respuesta_ok_fail resultado = actualizar_ubicacion_paginacion(tripulante_y_posicion);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_PROXIMA_TAREA
			tarea proxima_tarea = obtener_proxima_tarea_paginacion(tripulante_pid); //Aca podemos agregar un control, tal vez si falla enviar respuesta_ok_fail...
			void *respuesta = serializar_tarea(proxima_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, sizeof(tarea), respuesta);
			break;
		}
		case EXPULSAR_TRIPULANTE:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_PROXIMA_TAREA
			respuesta_ok_fail resultado = expulsar_tripulante_paginacion(tripulante_pid);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_EXPULSAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_ESTADO:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			estado estado = obtener_estado_paginacion(tripulante_pid);
			void *respuesta = serializar_estado(estado);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_ESTADO, sizeof(estado), respuesta);
			break;
		}
		case OBTENER_UBICACION:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_UBICACION
			posicion posicion = obtener_ubicacion_paginacion(tripulante_pid);
			void *respuesta = serializar_posicion(posicion);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_UBICACION, sizeof(posicion), respuesta);
			break;
		}
		default:
		{
			log_error(logger_ram_hq, "El codigo de operacion es invalido");
			break;
		}
		}
	}
	liberar_paquete(paquete);
	close(*socket_hilo);
	free(socket_hilo);
	imprimir_dump();
}

void crear_estructuras_administrativas()
{
	patotas = list_create();
	segmentos_memoria = list_create();
	pthread_mutex_init(&mutex_patotas,NULL);
	pthread_mutex_init(&mutex_memoria,NULL);
	pthread_mutex_init(&mutex_swap,NULL);

	if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		numero_segmento_global = 0;
		t_segmento_de_memoria* segmento = malloc(sizeof(t_segmento_de_memoria));
		segmento->inicio_segmento =  memoria_principal;
		segmento->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		segmento->libre = true;
		list_add(segmentos_memoria,segmento);
		
	}
	else if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
	{
		//crear_estructuras_administrativas_paginacion;
	}
	else
	{
		log_error(logger_ram_hq, "El esquema de memoria no es soportado por el sistema");
	}
}

//------------------------------------------MANEJO DE LOGICA DE MENSAJES-------------------------------------

//Falta eliminar el elemento de la t_list cuando la operacion se rechace, necesito hacer funciones iterativas para encontrar el indice y usar list_remove...

respuesta_ok_fail iniciar_patota_segmentacion(pid_con_tareas_y_tripulantes_miriam patota_con_tareas_y_tripulantes)
{
	pthread_mutex_lock(&mutex_patotas);
	t_tabla_de_segmento* patota = buscar_patota(patota_con_tareas_y_tripulantes.pid);
	if(patota){
		pthread_mutex_unlock(&mutex_patotas);
		log_error(logger_ram_hq,"La patota de pid %i ya existe, solicitud rechazada",patota_con_tareas_y_tripulantes.pid);
		free(patota_con_tareas_y_tripulantes.tareas);
		list_destroy_and_destroy_elements(patota_con_tareas_y_tripulantes.tripulantes,free);
		return RESPUESTA_FAIL;
	}
	pthread_mutex_unlock(&mutex_patotas);
	
	patota = malloc(sizeof(t_tabla_de_segmento));
	patota->segmento_pcb = NULL;
	patota->segmento_tarea = list_create();
	patota->segmentos_tripulantes = list_create();
	patota->mutex_segmentos_tripulantes = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_segmentos_tripulantes,NULL);
	pthread_mutex_lock(&mutex_patotas);
	list_add(patotas,patota);
	pthread_mutex_unlock(&mutex_patotas);

	pthread_mutex_lock(&mutex_memoria);
	//t_list* bakcup_lista_memoria = list_duplicate(segmentos_memoria);
	t_list* bakcup_lista_memoria = duplicar_lista_memoria(segmentos_memoria);
	t_segmento_de_memoria* segmento_a_usar_pcb = buscar_segmento_pcb();
	if(!segmento_a_usar_pcb){
		//TODO: compactar
		log_info(logger_ram_hq,"No encontre un segmento disponible para el PCB de PID: %i , voy a compactar",patota_con_tareas_y_tripulantes.pid);
		segmento_a_usar_pcb = buscar_segmento_pcb();
		if(!segmento_a_usar_pcb){
			pthread_mutex_unlock(&mutex_memoria);
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar el PCB aun luego de compactar, solicitud rechazada");
			//TODO: Sacar patota de lista de patotas (con indice)
			free(patota_con_tareas_y_tripulantes.tareas);
			list_destroy_and_destroy_elements(patota_con_tareas_y_tripulantes.tripulantes,free);
			free(patota->mutex_segmentos_tripulantes);
			free(patota);
			t_list* auxiliar = segmentos_memoria;
			list_destroy_and_destroy_elements(auxiliar,free);
			segmentos_memoria = bakcup_lista_memoria;
			return RESPUESTA_FAIL;
		}
	}

	log_info(logger_ram_hq,"Encontre un segmento para el PCB de PID: %i , empieza en: %i ",patota_con_tareas_y_tripulantes.pid,(int) segmento_a_usar_pcb->inicio_segmento); 
	t_segmento* segmento_pcb = malloc(sizeof(t_segmento));
	segmento_pcb->base = segmento_a_usar_pcb->inicio_segmento;
	segmento_pcb->tamanio = segmento_a_usar_pcb->tamanio_segmento;
	segmento_pcb->numero_segmento = numero_segmento_global;
	numero_segmento_global++;
	segmento_pcb->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(segmento_pcb->mutex_segmento,NULL);
	patota->segmento_pcb = segmento_pcb;
	
	t_segmento_de_memoria* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.longitud_palabra); 
	if(!segmento_a_usar_tareas){
		log_info(logger_ram_hq,"No encontre un segmento disponible para las tareas, voy a compactar");
		//TODO: compactar
		t_segmento_de_memoria* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.tareas); 
		if(!segmento_a_usar_tareas){
			pthread_mutex_unlock(&mutex_memoria);
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar las tareas aun luego de compactar, solicitud rechazada");
			//TODO: Sacar patota de lista de patotas (con indice)
			free(patota_con_tareas_y_tripulantes.tareas);
			list_destroy_and_destroy_elements(patota_con_tareas_y_tripulantes.tripulantes,free);
			free(segmento_pcb->mutex_segmento);
			free(segmento_pcb);
			free(patota->mutex_segmentos_tripulantes);
			free(patota);
			t_list* auxiliar = segmentos_memoria;
			list_destroy_and_destroy_elements(auxiliar,free);
			segmentos_memoria = bakcup_lista_memoria;
			return RESPUESTA_FAIL;
		}
	}

	log_info(logger_ram_hq,"Encontre un segmento para las tareas, empieza en: %i ",(int) segmento_a_usar_pcb->inicio_segmento);
	t_segmento* segmento_tarea = malloc(sizeof(t_segmento));
	segmento_tarea->base = segmento_a_usar_tareas->inicio_segmento;
	segmento_tarea->tamanio = segmento_a_usar_tareas->tamanio_segmento;
	segmento_tarea->numero_segmento = numero_segmento_global;
	numero_segmento_global++;
	segmento_tarea->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(segmento_tarea->mutex_segmento,NULL);
	patota->segmento_tarea = segmento_tarea;

	for(int i=0; i<patota_con_tareas_y_tripulantes.tripulantes->elements_count; i++){
		t_segmento_de_memoria* segmento_a_usar_tripulante = buscar_segmento_tcb();
		if(!segmento_a_usar_tripulante){
		log_info(logger_ram_hq,"No encontre un segmento disponible para el tripulante numero: %i, voy a compactar",i+1);
		//TODO: compactar
		t_segmento_de_memoria* segmento_a_usar_tripulante = buscar_segmento_tcb();
			if(!segmento_a_usar_tripulante){
				log_error(logger_ram_hq,"No hay suficiente espacio para almacenar al tripulante numero: %i , solicitud rechazada", i+1);
				//TODO: Sacar patota de lista de patotas (con indice)
				free(patota_con_tareas_y_tripulantes.tareas);
				list_destroy_and_destroy_elements(patota_con_tareas_y_tripulantes.tripulantes,free);
				free(segmento_pcb->mutex_segmento);
				free(segmento_pcb);
				free(segmento_tarea->mutex_segmento);
				free(segmento_tarea);
				free(patota->mutex_segmentos_tripulantes);
				//TODO: liberar tripulantes de patota
				free(patota);
				t_list* auxiliar = segmentos_memoria;
				list_destroy_and_destroy_elements(auxiliar,free);
				segmentos_memoria = bakcup_lista_memoria;
				return RESPUESTA_FAIL;
			}
		}	
		log_info(logger_ram_hq,"Encontre un segmento para el tripulante numero: %i , empieza en: %i ",i+1,(int) segmento_a_usar_tripulante->inicio_segmento);
		t_segmento* segmento_tripulante = malloc(sizeof(t_segmento));
		segmento_tripulante->base = segmento_a_usar_tripulante->inicio_segmento;
		segmento_tripulante->tamanio = segmento_a_usar_tripulante->tamanio_segmento;
		segmento_tripulante->numero_segmento = numero_segmento_global;
		numero_segmento_global++;
		segmento_tripulante->mutex_segmento = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(segmento_tripulante->mutex_segmento,NULL);
		list_add(patota->segmentos_tripulantes,segmento_tripulante);
		
		cargar_tcb_sinPid_en_segmento((nuevo_tripulante_sin_pid*)list_get(patota_con_tareas_y_tripulantes.tripulantes,i),patota->segmento_pcb,segmento_tripulante);

	}

	log_info(logger_ram_hq,"Las estructuras administrativas fueron creadas y los segmentos asignados, procedo a cargar la informacion a memoria");

	uint32_t direccion_logica_segmento = 0;
	uint32_t direccion_logica_tareas = 0; //TODO: Calcular la direccion logica real
	//Los semaforos se toman dentro de las funciones:
	cargar_pcb_en_segmento(patota_con_tareas_y_tripulantes.pid,direccion_logica_tareas,segmento_pcb);
	cargar_tareas_en_segmento(patota_con_tareas_y_tripulantes.tareas,patota_con_tareas_y_tripulantes.longitud_palabra,segmento_tarea);
	
	pthread_mutex_unlock(&mutex_memoria);
	list_destroy_and_destroy_elements(bakcup_lista_memoria,free);
	return RESPUESTA_OK;
}

respuesta_ok_fail iniciar_patota_paginacion(pid_con_tareas patota_con_tareas)
{
	return RESPUESTA_OK;
}

respuesta_ok_fail iniciar_tripulante_segmentacion(nuevo_tripulante tripulante)
{	
	return RESPUESTA_OK;
}

respuesta_ok_fail iniciar_tripulante_paginacion(nuevo_tripulante tripulante)
{
	return RESPUESTA_OK;
}

respuesta_ok_fail actualizar_ubicacion_segmentacion(tripulante_y_posicion tripulante_con_posicion){
	t_tabla_de_segmento* auxiliar_patota;
	t_segmento* segmento_tripulante_auxiliar;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tripulante_con_posicion.tid);
	for(int i=0; i<patotas->elements_count; i++){
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		auxiliar_patota = list_get(patotas,i);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			uint32_t tid_aux;
			pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar -> base,sizeof(uint32_t)); 
			
			if(tid_aux == tripulante_con_posicion.tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria, actualizando estado",tripulante_con_posicion.tid);
				uint32_t offset_x = sizeof(uint32_t) + sizeof(estado);
				uint32_t offset_y = offset_x + sizeof(uint32_t);
				memcpy(segmento_tripulante_auxiliar -> base + offset_x,&tripulante_con_posicion.pos_x,sizeof(uint32_t));
				memcpy(segmento_tripulante_auxiliar -> base + offset_y,&tripulante_con_posicion.pos_y,sizeof(uint32_t));
				pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				return RESPUESTA_OK;
			}
			pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
		}
		pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
		pthread_mutex_unlock(&mutex_patotas);	
		log_error(logger_ram_hq,"No se encontro el tripulante %d en memoria",tripulante_con_posicion.tid);
	}

	return RESPUESTA_FAIL;
}

respuesta_ok_fail actualizar_ubicacion_paginacion(tripulante_y_posicion tripulante_con_posicion){
	respuesta_ok_fail respuesta;
	//TODO
	return respuesta;
}


tarea obtener_proxima_tarea_paginacion(uint32_t tripulante_pid)
{
	tarea proxima_tarea;
	return proxima_tarea;
}

tarea obtener_proxima_tarea_segmentacion(uint32_t tripulante_tid)
{
	tarea proxima_tarea;
	t_segmento* tripulante_aux;
	uint32_t tid_aux;
	t_tabla_de_segmento* auxiliar_patota = buscar_patota_con_tid(tripulante_tid);
	if(!auxiliar_patota){
		for(int i=0; i<auxiliar_patota->segmentos_tripulantes->elements_count; i++){
		pthread_mutex_lock(auxiliar_patota->mutex_segmentos_tripulantes);
		tripulante_aux = list_get(auxiliar_patota->segmentos_tripulantes,i);
		memcpy(&tid_aux,tripulante_aux->base,sizeof(uint32_t));
			if(tid_aux == tripulante_tid){
				char* tareas;
				uint32_t id_tarea;
				memcpy(&id_tarea,tripulante_aux->base + 3*(sizeof(uint32_t)) + 1, sizeof(uint32_t));
				memcpy(tareas,auxiliar_patota->segmento_tarea,auxiliar_patota->segmento_tarea->tamanio); //Me traigo todo el contenido del segmento de tareas
				
			}
		}
	}
	log_error(logger_ram_hq,"El tripulante %d no esta dado de alta en ninguna patota", tripulante_tid);
	//uint32 pid = obtener_pid(auxiliar_patota);
	//tarea = obtener_proxima_tarea(pid);
	//Ta bueno pasarle el pid ya que lo tengo para ahorrar iteraciones
	//actualizarTareaActual(pid,tid,tarea);

	return proxima_tarea;
}

respuesta_ok_fail expulsar_tripulante_paginacion(uint32_t tripulante_pid)
{
	respuesta_ok_fail respuesta;
	//TODO
	return respuesta;
}

respuesta_ok_fail expulsar_tripulante_segmentacion(uint32_t tid)
{
	t_tabla_de_segmento* patota_aux;
	t_segmento* tripulante_aux;
	t_segmento_de_memoria* segmento_aux;
	uint32_t tid_aux;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tid);
	for(int i=0 ; i<patotas->elements_count; i++){
		//itero cada patota
		patota_aux = list_get(patotas,i);
		//bloqueo segmentos de tripulantes de esa patota
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			//recorro todos los segmentos de tripulantes de la patota
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			//bloqueo al tripulante
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			//obtengo su tid
			memcpy(&tid_aux,tripulante_aux->base,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria, se procede a expulsarlo, motivo: SUS", tid);
				//encontre al tripulante, lo borro de memoria
				//bloque memoria
				pthread_mutex_lock(&mutex_memoria);
				for(int k=0; k<segmentos_memoria->elements_count; k++){
					// recorro segmentos de memoria
					segmento_aux = list_get(segmentos_memoria,k);
					if((int)(segmento_aux->inicio_segmento) == (int)(tripulante_aux->base)){
						log_info(logger_ram_hq,"Encontre el segmento perteneciente al tripulante en %d , procedo a liberarlo", (int)segmento_aux->inicio_segmento);
						segmento_aux->libre = true;
					}
				}
				pthread_mutex_unlock(&mutex_memoria);
				pthread_mutex_unlock(tripulante_aux->mutex_segmento);
				free(tripulante_aux->mutex_segmento);
				free(tripulante_aux);
				list_remove(patota_aux->segmentos_tripulantes,j);
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				//TODO: Borrar al tripulante del mapa
				return RESPUESTA_OK;
			}
			pthread_mutex_unlock(tripulante_aux->mutex_segmento);
		}
	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
	pthread_mutex_unlock(&mutex_patotas);
	return RESPUESTA_FAIL;
}


estado obtener_estado_paginacion(uint32_t tripulante_tid)
{
	estado estado_obtenido;
	//TODO
	return estado_obtenido;
}

estado obtener_estado_segmentacion(uint32_t tid)
{
	t_tabla_de_segmento* patota_aux;
	t_segmento* tripulante_aux;
	uint32_t tid_aux;
	char* estado;
	int estado_numerico;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tid);
	for(int i=0; i<patotas->elements_count; i++){
		patota_aux = list_get(patotas,i);
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->base,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria", tid);
				memcpy(estado,tripulante_aux->base + sizeof(uint32_t),sizeof(char));
				estado_numerico = atoi(estado);
				return estado_numerico;

			}
		}
	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	return estado_numerico = 6;
}

posicion obtener_ubicacion_paginacion(uint32_t tripulante_tid)
{
	posicion ubicacion_obtenida;
	//TODO
	return ubicacion_obtenida;
}

posicion obtener_ubicacion_segmentacion(uint32_t tid)
{
	t_tabla_de_segmento* patota_aux;
	t_segmento* tripulante_aux;
	uint32_t tid_aux;
	posicion posicion;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tid);
	for(int i=0; i<patotas->elements_count; i++){
		patota_aux = list_get(patotas,i);
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->base,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria", tid);
				memcpy(&posicion.pos_x,tripulante_aux->base + sizeof(uint32_t) + sizeof(char),sizeof(uint32_t));
				memcpy(&posicion.pos_y,tripulante_aux->base + sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t), sizeof(uint32_t));
				return posicion;
			}
		}
	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	return posicion; // Los campos estan vacios
}

//-----------------------------------------------------------FUNCIONES DE BUSQUEDA DE SEGMENTOS--------------------------------------

t_tabla_de_segmento* buscar_patota(uint32_t pid){
	t_tabla_de_segmento* auxiliar;
	uint32_t pid_auxiliar;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		memcpy(&pid_auxiliar,auxiliar->segmento_pcb->base,sizeof(uint32_t));
		if(pid_auxiliar == pid){
			return auxiliar;
		}
	}
	return NULL;
}

t_tabla_de_segmento* buscar_patota_con_tid(uint32_t tid){
	t_tabla_de_segmento* auxiliar_patota;
	t_segmento* segmento_tripulante_auxiliar;
	log_info(logger_ram_hq,"Buscando la patota a partir del tid %d",tid);
	pthread_mutex_lock(&mutex_patotas);
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar_patota = list_get(patotas,i);
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			uint32_t tid_aux;
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar->base,sizeof(uint32_t));
			if(tid_aux == tid){
				pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				log_info(logger_ram_hq,"Patota correspondiente al tid: %d encontrada",tid);
				return auxiliar_patota;
			}
			pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
		}
		pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
	}
	pthread_mutex_unlock(&mutex_patotas);
	log_error(logger_ram_hq,"No se encontro una patota con el tid: %d asociada",tid);
	return NULL;
}

t_segmento_de_memoria* buscar_segmento_pcb(){
	t_segmento_de_memoria* iterador;
	t_segmento_de_memoria* auxiliar = malloc(sizeof(t_segmento_de_memoria));
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if(iterador->tamanio_segmento >= 2*(sizeof(uint32_t)) && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = 2*(sizeof(uint32_t)); 
				auxiliar->libre = false;
				list_add(segmentos_memoria,auxiliar);
				iterador->inicio_segmento += 2*(sizeof(uint32_t));
				iterador->tamanio_segmento -= 2*(sizeof(uint32_t));
				return auxiliar;
			}
		} 
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){ 
		t_segmento_de_memoria* vencedor;
		vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if((iterador->tamanio_segmento >= 2*(sizeof(uint32_t))) && (iterador->tamanio_segmento < vencedor->tamanio_segmento) && iterador->libre){
				//hay que armar bien esto, para todas las best fil
				iterador->libre = false;
				vencedor = iterador;
			}
		}
		return vencedor;
	}
	
	free(auxiliar);
	return NULL;
};

t_segmento_de_memoria* buscar_segmento_tareas(uint32_t tamanio_tareas){	
	t_segmento_de_memoria* iterador;
	t_segmento_de_memoria* auxiliar = malloc(sizeof(t_segmento_de_memoria));
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if(iterador->tamanio_segmento >= tamanio_tareas && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = tamanio_tareas; 
				auxiliar->libre = false;
				list_add(segmentos_memoria,auxiliar);
				iterador->inicio_segmento += tamanio_tareas;
				iterador->tamanio_segmento -= tamanio_tareas;
				return auxiliar;
			}
		}
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){
		t_segmento_de_memoria* vencedor;
		vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			auxiliar = list_get(segmentos_memoria,i);
			if((auxiliar->tamanio_segmento >= tamanio_tareas) && (auxiliar->tamanio_segmento < vencedor->tamanio_segmento) && auxiliar->libre){
				//fix
				auxiliar->libre = false;
				vencedor = auxiliar;
			}
		}
		return vencedor;
	}
	return NULL;
};

t_segmento_de_memoria* buscar_segmento_tcb(){
	t_segmento_de_memoria* iterador;
	t_segmento_de_memoria* auxiliar = malloc(sizeof(t_segmento_de_memoria));
	uint32_t size_tcb = sizeof(uint32_t)*5 + sizeof(estado);
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if(iterador->tamanio_segmento >= size_tcb && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = size_tcb; 
				auxiliar->libre = false;
				//sema?
				list_add(segmentos_memoria,auxiliar);
				iterador->inicio_segmento += size_tcb;
				iterador->tamanio_segmento -= size_tcb;
				return auxiliar;
			}
		}
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){ 
		t_segmento_de_memoria* vencedor;
		vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			auxiliar = list_get(segmentos_memoria,i);
			if((auxiliar->tamanio_segmento >= size_tcb) && (auxiliar->tamanio_segmento < vencedor->tamanio_segmento) && auxiliar->libre){
				auxiliar->libre = false;
				vencedor = auxiliar;
			}
		}
		return vencedor;
	}
	return NULL;
};
//----------------------------------------------CARGAR SEGMENTOS A MEMORIA-------------------------------------------------------------------------

void cargar_pcb_en_segmento(uint32_t pid, uint32_t direccion_logica_tareas, t_segmento* segmento){
	pthread_mutex_lock(segmento->mutex_segmento);
	int offset = 0;
	memcpy(segmento->base + offset, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(segmento->base + offset, &direccion_logica_tareas, sizeof(uint32_t));
	pthread_mutex_unlock(segmento->mutex_segmento);
}

//Aclaración: Esta función asume que las tareas incluyen un /0 al final de la cadena de char*
void cargar_tareas_en_segmento(char* tareas,uint32_t longitud_palabra, t_segmento* segmento){
	pthread_mutex_lock(segmento->mutex_segmento);
	memcpy(segmento->base, tareas, longitud_palabra);
	pthread_mutex_unlock(segmento->mutex_segmento);
	recorrer_tareas(segmento);
}

//No la estamos usando
void cargar_tcb_en_segmento(uint32_t tid,estado estado_nuevo,uint32_t pos_x,uint32_t pos_y,uint32_t direccion_tarea,uint32_t direccion_patota,t_segmento* segmento){
//void cargar_tcb_en_segmento(nuevo_tripulante_sin_pid tripulante, segmento){
	pthread_mutex_lock(segmento->mutex_segmento);
	
	int offset = 0;
	memcpy(segmento->base + offset, &tid, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &estado_nuevo, sizeof(estado));
	offset = offset + sizeof(estado);
	memcpy(segmento->base + offset, &pos_x, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &pos_y, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &direccion_tarea, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &direccion_patota, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	
	pthread_mutex_unlock(segmento->mutex_segmento);
}
void cargar_tcb_sinPid_en_segmento(nuevo_tripulante_sin_pid* tripulante,t_segmento* patota, t_segmento* segmento){
	pthread_mutex_lock(segmento->mutex_segmento);
	
	uint32_t cero = 0;
	char est = '0';

	int offset = 0;
	memcpy(segmento->base + offset, & (tripulante->tid), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &est, sizeof(char));
	offset = offset + sizeof(char);
	memcpy(segmento->base + offset, & (tripulante->pos_x), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, & (tripulante->pos_y), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, &cero, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->base + offset, patota, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	pthread_mutex_unlock(segmento->mutex_segmento);
}

//-------------------------------------------------------------------------FUNCION DE COMPACTACION-----------------------------------------------------


void compactar_memoria(){
	//TODO: ORDENAR LISTA DE SEGMENTOS POR DIRECCIONES
	list_sort(segmentos_memoria,ordenar_direcciones_de_memoria); // No se si esta bien usada la funcion, lo voy a preguntar en soporte pero dejo la idea
	//-----------------------------------------------
	t_list* lista_de_ocupados = list_create();
	t_segmento_de_memoria* auxiliar;
	for(int i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		if(!auxiliar->libre){
			list_add(lista_de_ocupados,auxiliar);
		}
	}
	int offset = 0;
	void* inicio_segmento_libre;
	uint32_t tamanio_acumulado;
	uint32_t tamanio_segmento_libre;
	for(int i=0; i<lista_de_ocupados->elements_count; i++){
		auxiliar = list_get(lista_de_ocupados,i);
		memcpy(memoria_principal + offset, auxiliar->inicio_segmento, auxiliar->tamanio_segmento);
		tamanio_acumulado += auxiliar->tamanio_segmento;
		offset += auxiliar->tamanio_segmento + 1;
		if(auxiliar = list_get(lista_de_ocupados,i+1) == NULL){
			inicio_segmento_libre = auxiliar->inicio_segmento + auxiliar->tamanio_segmento; // Si rompe, castear el inicio de auxiliar como uint32_t
			tamanio_segmento_libre = mi_ram_hq_configuracion->TAMANIO_MEMORIA - tamanio_acumulado + 1; //No se si va el +1
		}
	}
	t_segmento_de_memoria* segmento_libre = malloc(sizeof(t_segmento_de_memoria));
	segmento_libre->inicio_segmento = inicio_segmento_libre;
	segmento_libre->tamanio_segmento = tamanio_segmento_libre;
	list_add(lista_de_ocupados,segmento_libre); // Al final de la ordenada le agrego el segmento libre
	segmentos_memoria = lista_de_ocupados; // Actualizamos la lista de segmentos global con la nueva que ya esta ordenada
}

int ordenar_direcciones_de_memoria(t_segmento_de_memoria* segmento1, t_segmento_de_memoria* segmento2){
	return segmento1->inicio_segmento < segmento2->inicio_segmento;
}

void funcion_test_memoria( uint32_t tid){
	//patotas
//	t_list* aux = patotas
	printf ("Iniciando rastillaje de memoria guardada\n");
	for(int i = 0;i < patotas->elements_count;i++){
		t_tabla_de_segmento* patota = list_get(patotas,i);
		recorrer_pcb(patota->segmento_pcb);
		recorrer_tareas(patota->segmento_tarea);
		recorrer_tcb(patota->segmentos_tripulantes);
	}
}
void recorrer_pcb(t_segmento * pcb){
	uint32_t pid;
	pthread_mutex_lock(pcb->mutex_segmento);
	memcpy(&pid, pcb->base, sizeof(uint32_t));
	pthread_mutex_unlock(pcb->mutex_segmento);
	printf ("Patota pid = %i\n",pid);
}
void recorrer_tareas(t_segmento * tareas){
	pthread_mutex_lock(tareas->mutex_segmento);
	char* auxiliar = malloc (tareas->tamanio);
	memcpy(auxiliar, tareas->base, tareas->tamanio);
	pthread_mutex_unlock(tareas->mutex_segmento);
	//quiza hay que recorrer distinto pq frena en el primer /0
	
	printf ("Lista de tareas: \n",auxiliar);
	printf ("%s\n",auxiliar);
	printf ("%s\n",auxiliar+1);
	printf ("%c\n",*auxiliar);
	for(int i = 0;i<tareas->tamanio -1;i++){
		//printf ("%c\n",*(auxiliar+i));
		 if( *(auxiliar+i) == '\0'){
			printf ("%s\n",auxiliar+i+1);
		}

	}
}
void recorrer_tcb(t_list * tripulantes_list){
	for(int i = 0;i < tripulantes_list->elements_count;i++){
		t_segmento * tripulante = list_get(tripulantes_list,i);
		pthread_mutex_lock(tripulante->mutex_segmento);
		
		uint32_t tid = 0;
		estado estado_actual = NEW;
		uint32_t posX = 0;
		uint32_t posY = 0;
		uint32_t tareaActual = 0;
		uint32_t patotaActual = 0;

		int offset = 0;
		memcpy(&tid, tripulante->base + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&estado_actual, tripulante->base + offset, sizeof(estado));
		offset = offset + sizeof(estado);
		memcpy(&posX, tripulante->base + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&posY, tripulante->base + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&tareaActual, tripulante->base + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&patotaActual, tripulante->base + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		
		pthread_mutex_unlock(tripulante->mutex_segmento);
		printf ("Tripulante tid %i, estado %i, posicion %d;%d, tarea %i, patota %i\n",tid,estado_actual,posX,posY,tareaActual,patotaActual);
	}
}


void imprimir_dump(void){
	printf ("--------------------------------------------------------------------------\n");printf ("--------------------------------------------------------------------------\n");
	char * time =  temporal_get_string_time("%d/%m/%y %H:%M:%S");
	printf ("Dump: %s \n",time);

	for(int i = 0;i < patotas->elements_count;i++){
		t_tabla_de_segmento* patota = list_get(patotas,i);
		uint32_t pid = obtener_patota_memoria(patota->segmento_pcb);
		//recorrer_pcb_dump(pid,patota->segmento_pcb);
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_pcb->numero_segmento,patota->segmento_pcb->base,patota->segmento_pcb->tamanio);
		//recorrer_tareas_dump(pid,patota->segmento_tarea);
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_tarea->numero_segmento,patota->segmento_tarea->base,patota->segmento_tarea->tamanio);
		pthread_mutex_lock(patota->mutex_segmentos_tripulantes);
		recorrer_tcb_dump(pid,patota->segmentos_tripulantes);
		pthread_mutex_unlock(patota->mutex_segmentos_tripulantes);
	}
}

uint32_t obtener_patota_memoria(t_segmento* pcb){
	uint32_t pid;
	pthread_mutex_lock(pcb->mutex_segmento);
	memcpy(&pid, pcb->base, sizeof(uint32_t));
	pthread_mutex_unlock(pcb->mutex_segmento);
	return pid;
}

void recorrer_pcb_dump(t_segmento* pcb){
	uint32_t pid;
	pthread_mutex_lock(pcb->mutex_segmento);
	
	memcpy(&pid, pcb->base, sizeof(uint32_t));
	pthread_mutex_unlock(pcb->mutex_segmento);
	printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,pcb->numero_segmento,pcb->base,pcb->tamanio);
}
void recorrer_tareas_dump(uint32_t pid,t_segmento* tareas){
	printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tareas->numero_segmento,tareas->base,tareas->tamanio);
}
void recorrer_tcb_dump(uint32_t pid,t_list* tripulantes){
	for(int i = 0;i < tripulantes->elements_count;i++){
		t_segmento * tripulante = list_get(tripulantes,i);
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tripulante->numero_segmento,tripulante->base,tripulante->tamanio);
	}
}
/*
--------------------------------------------------------------------------
Dump: 09/07/21 10:11:12
Proceso: 1	Segmento: 1	Inicio: 0x0000	Tam: 20b
Proceso: 1	Segmento: 2	Inicio: 0x0013	Tam: 450b
Proceso: 2	Segmento: 1	Inicio: 0x01D6	Tam: 20b
--------------------------------------------------------------------------
*/

t_list* duplicar_lista_memoria(t_list* memoria){
	t_list *aux = list_create();
	for(int i=0; i<segmentos_memoria->elements_count; i++){
		t_segmento_de_memoria* segmento_aux = list_get(segmentos_memoria,i);
		t_segmento_de_memoria* segmento_copia = malloc(sizeof(t_segmento_de_memoria));
		segmento_copia->inicio_segmento = segmento_aux->inicio_segmento;
		segmento_copia->libre = segmento_aux->libre;
		segmento_copia->tamanio_segmento = segmento_aux->tamanio_segmento;
		list_add(aux,segmento_copia);
	}
	return aux;
}