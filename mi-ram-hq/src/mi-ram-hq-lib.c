#include "mi-ram-hq-lib.h"
#include <stdlib.h>

void funcion_test_memoria(uint32_t);
void recorrer_pcb(t_segmento_de_memoria * );
void recorrer_tareas(t_segmento_de_memoria * );
void recorrer_tcb(t_list * );
void ordenar_segmentos();
void swap(t_segmento_de_memoria **,t_segmento_de_memoria **);
bool ordenar_direcciones_de_memoria(void* p1, void* p2);
uint32_t calcular_memoria_libre(void);

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
			log_info(logger_ram_hq, "TEST LOGGER1");
			pid_con_tareas_y_tripulantes_miriam patota_con_tareas_y_tripulantes = deserializar_pid_con_tareas_y_tripulantes(paquete->stream);
			log_info(logger_ram_hq, "TEST LOGGER2");
			funcion_test_memoria(1);
			respuesta_ok_fail resultado = iniciar_patota_segmentacion(patota_con_tareas_y_tripulantes);
			log_info(logger_ram_hq, "TEST LOGGER3");
			//rompe porque quiere leer la patota 2 y no esta en memoria
			funcion_test_memoria(1);
			log_info(logger_ram_hq, "TEST LOGGER4");
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			log_info(logger_ram_hq, "TEST LOGGER5");
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			log_info(logger_ram_hq, "TEST LOGGER6");
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			respuesta_ok_fail resultado = actualizar_ubicacion_segmentacion(tripulante_y_posicion);
			funcion_test_memoria(1);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			char* proxima_tarea = obtener_proxima_tarea_segmentacion(tripulante_tid); 
			uint32_t long_tarea = strlen(proxima_tarea)+1;
			void *respuesta = serializar_tarea(proxima_tarea,long_tarea);
			free(proxima_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, long_tarea+sizeof(uint32_t), respuesta);
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
		case ACTUALIZAR_ESTADO:
		{
			//deserializar el estado tambien
			uint32_t tripulante_tid;
			estado estado_nuevo = deserializar_estado_tcb (paquete->stream,&tripulante_tid);
			//estado estado_tid = obtener_estado_segmentacion(tripulante_tid);	
			respuesta_ok_fail resultado = actualizar_estado_segmentacion(tripulante_tid,estado_nuevo);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_ESTADO, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_UBICACION:
		{
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_UBICACION
			posicion posicion = obtener_ubicacion_segmentacion(tripulante_tid);
			void *respuesta = serializar_posicion(posicion);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_UBICACION, sizeof(uint32_t)*2, respuesta);
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
		switch (paquete->codigo_operacion)
		{
		case INICIAR_PATOTA:
		{
			pid_con_tareas_y_tripulantes_miriam pid_con_tareas_y_tripulantes = deserializar_pid_con_tareas_y_tripulantes(paquete->stream);
			patota_stream_paginacion patota_con_tareas_y_tripulantes = orginizar_stream_paginacion(pid_con_tareas_y_tripulantes);
			respuesta_ok_fail resultado = iniciar_patota_paginacion(patota_con_tareas_y_tripulantes);
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
			char * proxima_tarea = obtener_proxima_tarea_paginacion(tripulante_pid); //Aca podemos agregar un control, tal vez si falla enviar respuesta_ok_fail...
			void *respuesta = serializar_tarea(proxima_tarea,0);
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
	if(!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION")){
		imprimir_dump_paginacion();
	}
	else if(!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION")){
		imprimir_dump();
	}
	liberar_paquete(paquete);
	close(*socket_hilo);
	free(socket_hilo);
	return;
}

void inicializar_swap(){
	int  fd;
	fd = open(mi_ram_hq_configuracion->PATH_SWAP, O_RDWR | O_CREAT, (mode_t) 0777);
	if(fd == -1){
		log_error(logger_ram_hq,"hubo un error al intentar abrir swap");
		exit(-1);
	}
	fallocate(fd,0,0,mi_ram_hq_configuracion->TAMANIO_SWAP);
	memoria_swap = mmap(NULL, mi_ram_hq_configuracion->TAMANIO_SWAP, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (memoria_swap == MAP_FAILED){
		log_error(logger_ram_hq,"hubo un error al mapear el archivo");
		exit(-1);
	}
	offset_swap = 0;
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
		printf("Creando los frames de la memoria: \n");
		frames = list_create();
		int cantidad_frames = mi_ram_hq_configuracion->TAMANIO_MEMORIA / mi_ram_hq_configuracion->TAMANIO_PAGINA;
		int offset = 0;
		for(int i =0; i<cantidad_frames; i++){
			t_frame_en_memoria* un_frame = malloc(sizeof(t_frame_en_memoria));
			un_frame->inicio = memoria_principal + offset;
			un_frame->libre = true;
			un_frame->pagina_a_la_que_pertenece = NULL;
			un_frame->pid_duenio = NULL;
			un_frame->indice_pagina = NULL;
			un_frame->mutex = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(un_frame->mutex,NULL);
			offset += mi_ram_hq_configuracion->TAMANIO_PAGINA;
			list_add(frames,un_frame);
			printf("Cree el frame %i, inicia en %i \n",i,un_frame->inicio);
		}
		inicializar_swap();
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
	
	//ver si entra todo, sino devolver fail
	
	uint32_t memoria_libre = calcular_memoria_libre();
	// tiene la longitud de todas las tareas + pid + cantidad de tripulantes * (estado + tid + tarea_actual + posx + posy + patota_direccion_virtual)
	uint32_t memoria_necesaria = patota_con_tareas_y_tripulantes.longitud_palabra + sizeof(uint32_t) + patota_con_tareas_y_tripulantes.tripulantes->elements_count * (sizeof(uint32_t)*5 + sizeof(char));

	if(memoria_libre < memoria_necesaria){
		pthread_mutex_unlock(&mutex_patotas);
		log_error(logger_ram_hq,"No hay lugar para guardar la patota %i",patota_con_tareas_y_tripulantes.pid);
		free(patota_con_tareas_y_tripulantes.tareas);
		while(patota_con_tareas_y_tripulantes.tripulantes->elements_count){
			list_remove(patota_con_tareas_y_tripulantes.tripulantes,0);
		}
		list_destroy(patota_con_tareas_y_tripulantes.tripulantes);
		return RESPUESTA_FAIL;
	}

	//inicializo tabla de segmentos para esta patota
	patota = malloc(sizeof(t_tabla_de_segmento));
	patota->segmento_pcb = NULL;
	patota->segmento_tarea = NULL;
	patota->segmentos_tripulantes = list_create();
	patota->mutex_segmentos_tripulantes = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_segmentos_tripulantes,NULL);
	
	//Busco segmento para el pcb
	pthread_mutex_lock(&mutex_memoria);
	t_segmento_de_memoria* segmento_a_usar_pcb = buscar_segmento_pcb();
	if(!segmento_a_usar_pcb){
		//TODO: compactar
		log_info(logger_ram_hq,"No encontre un segmento disponible para el PCB de PID: %i , voy a compactar",patota_con_tareas_y_tripulantes.pid);
		compactar_memoria();
		segmento_a_usar_pcb = buscar_segmento_pcb();
		//no deberia pasar
		if(!segmento_a_usar_pcb){
			pthread_mutex_unlock(&mutex_memoria);
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar el PCB aun luego de compactar, solicitud rechazada. FAIL");
			return RESPUESTA_FAIL;
		}
	}

	//agrego segmento pcb a mi patota (tabla de segmentos)
	log_info(logger_ram_hq,"Encontre un segmento para el PCB de PID: %i , empieza en: %i ",patota_con_tareas_y_tripulantes.pid,(int) segmento_a_usar_pcb->inicio_segmento); 
	patota->segmento_pcb = segmento_a_usar_pcb;
	
	//busco segmento para las tareas
	t_segmento_de_memoria* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.longitud_palabra); 
	if(!segmento_a_usar_tareas){
		log_info(logger_ram_hq,"No encontre un segmento disponible para las tareas, voy a compactar");
		compactar_memoria();
		segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.longitud_palabra); 
		//no deberia pasar
		if(!segmento_a_usar_tareas){
			//si es necesario rollback esta la rutina casi terminada en el escritorio
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar las tareas aun luego de compactar, solicitud rechazada. FAIL");
			pthread_mutex_unlock(&mutex_memoria);
			return RESPUESTA_FAIL;
		}
	}

	log_info(logger_ram_hq,"Encontre un segmento para las tareas, empieza en: %i ",(int) segmento_a_usar_tareas->inicio_segmento);
	
	//agrego segmento tareas a mi patota (tabla de segmentos)
	patota->segmento_tarea = segmento_a_usar_tareas;

	for(int i=0; i<patota_con_tareas_y_tripulantes.tripulantes->elements_count; i++){
		t_segmento_de_memoria* segmento_a_usar_tripulante = buscar_segmento_tcb();
		if(!segmento_a_usar_tripulante){
			log_info(logger_ram_hq,"No encontre un segmento disponible para el tripulante numero: %i, voy a compactar",i+1);
			compactar_memoria();
			segmento_a_usar_tripulante = buscar_segmento_tcb();
			//no deberia pasar
			if(!segmento_a_usar_tripulante){
				log_error(logger_ram_hq,"No hay suficiente espacio para almacenar al tripulante numero: %i , solicitud rechazada. FAIL", i+1);
				pthread_mutex_unlock(&mutex_memoria);
				return RESPUESTA_FAIL;
			}
		}	
		log_info(logger_ram_hq,"Encontre un segmento para el tripulante numero: %i , empieza en: %i ",i+1,(int) segmento_a_usar_tripulante->inicio_segmento);
		
		//agrego segmento tripulante a la lista de tripulantes de la patota (tabla de segmentos)
		list_add(patota->segmentos_tripulantes,segmento_a_usar_tripulante);
		
		cargar_tcb_sinPid_en_segmento((nuevo_tripulante_sin_pid*)list_get(patota_con_tareas_y_tripulantes.tripulantes,i), segmento_a_usar_tripulante,patota_con_tareas_y_tripulantes.pid);

	}

	log_info(logger_ram_hq,"Las estructuras administrativas fueron creadas y los segmentos asignados, procedo a cargar la informacion a memoria");

	uint32_t direccion_logica_segmento = 0;
	uint32_t direccion_logica_tareas = 0; //TODO: Calcular la direccion logica real
	//Los semaforos se toman dentro de las funciones:
	cargar_pcb_en_segmento(patota_con_tareas_y_tripulantes.pid,direccion_logica_tareas,patota->segmento_pcb);
	//hace falta longitud palabra?
	cargar_tareas_en_segmento(patota_con_tareas_y_tripulantes.tareas,patota_con_tareas_y_tripulantes.longitud_palabra,patota->segmento_tarea);
	
	//agrego patota a mi lista de patotas (agrego mi tabla de segmentos a la tabla de procesos)
	list_add(patotas,patota);
	
	pthread_mutex_unlock(&mutex_memoria);
	return RESPUESTA_OK;
}

respuesta_ok_fail iniciar_patota_paginacion(patota_stream_paginacion patota_con_tareas_y_tripulantes)
{
	uint32_t pid;
	memcpy(&pid,patota_con_tareas_y_tripulantes.stream,sizeof(uint32_t));
	pthread_mutex_lock(&mutex_patotas);
	t_tabla_de_paginas* patota = buscar_patota_paginacion(pid);
	if(patota){
		pthread_mutex_unlock(&mutex_patotas);
		log_error(logger_ram_hq,"La patota de pid %i ya existe, solicitud rechazada",pid);
		free(patota_con_tareas_y_tripulantes.stream);
		//free(&patota_con_tareas_y_tripulantes);
		return RESPUESTA_FAIL;
	}
	pthread_mutex_unlock(&mutex_patotas);

	patota = malloc(sizeof(t_tabla_de_paginas));
	patota->mutex_tabla_paginas = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_tabla_paginas,NULL);
	patota->paginas = list_create();
	patota->tareas = patota_con_tareas_y_tripulantes.tareas;
	pthread_mutex_lock(&mutex_patotas);
	list_add(patotas,patota);
	pthread_mutex_unlock(&mutex_patotas);

	float cant = patota_con_tareas_y_tripulantes.tamanio_patota / mi_ram_hq_configuracion->TAMANIO_PAGINA;
	int cantidad_paginas_a_usar = ceilf(cant);
	pthread_mutex_lock(&mutex_memoria);
	t_list* frames_patota = buscar_cantidad_frames_libres(cantidad_paginas_a_usar);

	t_frame_en_memoria* auxiliar;
	int bytes_ya_escritos = 0;
	int bytes_que_faltan = patota_con_tareas_y_tripulantes.tamanio_patota;
	int offset = 0;
	uint32_t id_pagina = 0;
	pthread_mutex_lock(patota->mutex_tabla_paginas);

	for(int i =0; i<frames_patota->elements_count; i++){
		auxiliar = list_get(frames_patota,i);
		t_pagina* pagina = malloc(sizeof(t_pagina));
		pagina->id_pagina = id_pagina;
		pagina->inicio_memoria = auxiliar->inicio;
		pagina->inicio_swap = NULL;
		pagina->mutex_pagina = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(pagina->mutex_pagina,NULL);
		pagina->presente = true;
		list_add(patota->paginas,pagina);
		auxiliar->pagina_a_la_que_pertenece = pagina;
		auxiliar->libre = false;
		// -------------------------------------------- ESTO ES PARA EL DUMP -------------------------------------------------------------
		auxiliar->pid_duenio = pid;
		auxiliar->indice_pagina = id_pagina;
		id_pagina ++;
		// -------------------------------------------------------------------------------------------------------------------------------
		memcpy(auxiliar->inicio,patota_con_tareas_y_tripulantes.stream + offset,minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,bytes_que_faltan));
		bytes_que_faltan -= minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,(patota_con_tareas_y_tripulantes.tamanio_patota - bytes_ya_escritos));
		bytes_ya_escritos = (patota_con_tareas_y_tripulantes.tamanio_patota - bytes_que_faltan);
		offset = bytes_ya_escritos;
	}

	if(frames_patota->elements_count < cantidad_paginas_a_usar){
		log_info(logger_ram_hq,"No hay espacio suficiente en memoria para guardar todo el proceso, se procede a almacenar en SWAP %i bytes",bytes_que_faltan);
		for(int i=0;i<(cantidad_paginas_a_usar - frames_patota->elements_count); i++){
			t_pagina* pagina = malloc(sizeof(t_pagina));
			pagina->id_pagina = id_pagina;
			id_pagina ++;
			pagina->inicio_memoria = NULL;
			pagina->inicio_swap = memoria_swap + offset_swap;
			pagina->mutex_pagina = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(pagina->mutex_pagina,NULL);
			pagina->presente = false;
			list_add(patota->paginas,pagina);
			memcpy(pagina->inicio_swap,patota_con_tareas_y_tripulantes.stream + offset,minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,bytes_que_faltan));
			//Renombrar a
			int a = patota_con_tareas_y_tripulantes.tamanio_patota - bytes_ya_escritos;
			bytes_que_faltan -= minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,a);
			bytes_ya_escritos = (patota_con_tareas_y_tripulantes.tamanio_patota - bytes_que_faltan);
			offset += bytes_ya_escritos;
			offset_swap = bytes_ya_escritos;
		}
		log_info(logger_ram_hq,"La informacion se guardo satisfactoriamente en el almacenamiento secundario");	
	}

	pthread_mutex_unlock(patota->mutex_tabla_paginas);
	pthread_mutex_unlock(&mutex_memoria);
	log_info(logger_ram_hq,"La estructuras administrativas fueron creadas correctamente");
	return RESPUESTA_OK;
}

int minimo_entre(int un_valor, int otro_valor){
	if(un_valor <= otro_valor){
		return un_valor;
	}
	else{
		return otro_valor;
	}
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
	t_segmento_de_memoria* segmento_tripulante_auxiliar;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tripulante_con_posicion.tid);
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar_patota = list_get(patotas,i);
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			uint32_t tid_aux;
			//pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar -> inicio_segmento,sizeof(uint32_t)); 
			
			if(tid_aux == tripulante_con_posicion.tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria, actualizando estado",tripulante_con_posicion.tid);
				uint32_t offset_x = sizeof(uint32_t) + sizeof(char);
				uint32_t offset_y = offset_x + sizeof(uint32_t);
				memcpy(segmento_tripulante_auxiliar -> inicio_segmento + offset_x,&tripulante_con_posicion.pos_x,sizeof(uint32_t));
				memcpy(segmento_tripulante_auxiliar -> inicio_segmento + offset_y,&tripulante_con_posicion.pos_y,sizeof(uint32_t));
				//pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				return RESPUESTA_OK;
			}
			//pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
		}
		pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
	}
	log_error(logger_ram_hq,"No se encontro el tripulante %d en memoria",tripulante_con_posicion.tid);
	pthread_mutex_unlock(&mutex_patotas);	

	return RESPUESTA_FAIL;
}

//FALTA SINCRONIZAR
respuesta_ok_fail actualizar_ubicacion_paginacion(tripulante_y_posicion tripulante_con_posicion){
	pthread_mutex_lock(&mutex_patotas);
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tripulante_con_posicion.tid);
	pthread_mutex_unlock(&mutex_patotas);
	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tripulante_con_posicion.tid);
		return RESPUESTA_FAIL;
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tripulante_con_posicion.tid);
	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	primer_indice += tamanio_tareas;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &primer_indice); // ESTO HAY QUE PASARLO A UN DECIMAL
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	primer_indice -= 1;
	inicio_tcb* tcb = buscar_inicio_tcb(tripulante_con_posicion.tid,patota,primer_indice,offset_entero);
	int nuevo_indice = 0;
	while(!tcb->pagina){
		nuevo_indice += primer_indice + 5*sizeof(uint32_t) + sizeof(char);
		tcb = buscar_inicio_tcb(tripulante_con_posicion.tid,patota,nuevo_indice,offset_entero);
	}
	double indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (tcb->indice * 5*sizeof(uint32_t) + sizeof(char)) + sizeof(uint32_t) + sizeof(char);
	double offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	int offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	indice_de_posicion -= 1;

	escribir_una_coordenada_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,tripulante_con_posicion.pos_x,patota);

	indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (tcb->indice * 5*sizeof(uint32_t) + sizeof(char)) + sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t);
	offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	offset_posicion = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	indice_de_posicion -= 1;

	escribir_una_coordenada_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,tripulante_con_posicion.pos_x,patota);

	return RESPUESTA_OK;
}


t_tabla_de_paginas* buscar_patota_con_tid_paginacion(uint32_t tid){
	t_tabla_de_paginas* auxiliar;
	tarea_ram* tarea_aux;
	t_pagina* pagina_aux;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		int indice_tripulantes = 2 * sizeof(uint32_t);
		uint32_t tid_aux = 0;
		for(int j=0; j<auxiliar->tareas->elements_count; j++){
			tarea_aux = list_get(auxiliar->paginas,j);
			indice_tripulantes += tarea_aux->tamanio;
		}
		double offset_pagina = modf((double)indice_tripulantes,(double*)&indice_tripulantes);
		int offset_pagina_entero = offset_pagina * mi_ram_hq_configuracion->TAMANIO_PAGINA;
		for(int k=indice_tripulantes-1; k<auxiliar->paginas->elements_count; k++){
			pagina_aux = list_get(auxiliar->paginas,k);
			pthread_mutex_lock(pagina_aux->mutex_pagina);
			int bytes_a_leer = sizeof(uint32_t);
			int bytes_escritos = 0;
			int espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_pagina;
			int espacio_leido = 0;
			/* NO SE SI ES NECESARIO TRAERSE LAS PAGINAS A MEMORIA MIENTRAS BUSCO EN LAS LISTAS, CREO QUE NO HACE FALTA PERO HAY QUE PREGUNTARLO
			if(!pagina_aux->presente){
				//TODO: traerme la pagina desde swap
				//traer_pagina_a_memoria(pagina_aux);
			}*/
			if(bytes_a_leer > espacio_disponible_en_pagina){
				memcpy(&tid_aux + espacio_leido,pagina_aux->inicio_memoria + offset_pagina_entero,espacio_disponible_en_pagina);
				espacio_leido += espacio_disponible_en_pagina;
				espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
				bytes_escritos = espacio_disponible_en_pagina;
				offset_pagina = 0;
			}
			else{
				memcpy(&tid_aux + espacio_leido,pagina_aux->inicio_memoria + offset_pagina_entero,sizeof(uint32_t) - bytes_escritos);
				if(tid_aux == tid){
					return auxiliar;
				}
			}
			pthread_mutex_unlock(pagina_aux->mutex_pagina);
		}
		
	}
	return NULL;
}

//TODO

inicio_tcb* buscar_inicio_tcb(uint32_t tid,t_tabla_de_paginas* patota,double indice, int offset_pagina){
	inicio_tcb* retornar;
	t_pagina* auxiliar;
	t_pagina* pagina_retorno;
	uint32_t tid_aux;
	uint32_t tid_aux_posta;
	int tamanio_tareas;
	tarea_ram* auxiliar_tarea;
	for(int i =0; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	double primer_indice = (2*sizeof(uint32_t) + tamanio_tareas) / mi_ram_hq_configuracion->TAMANIO_PAGINA;
	double offset_primer_pagina = modf(primer_indice,&primer_indice);
	int offset_primer_pagina_entero = offset_primer_pagina * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	primer_indice -= 1;
	int espacio_leido = 0;	
	pthread_mutex_lock(patota->mutex_tabla_paginas);
	while(espacio_leido < 4){
		//WARNING PARA NUMEROS NEGATIVOS
		for(int i=primer_indice; i<patota->paginas->elements_count; i++){
			auxiliar = list_get(patota->paginas,i);
			pagina_retorno = list_get(patota->paginas,i);
			int bytes_a_leer = sizeof(uint32_t);
			int espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_primer_pagina;
			/* NO SE SI ES NECESARIO TRAERSE LAS PAGINAS A MEMORIA MIENTRAS BUSCO EN LAS LISTAS, CREO QUE NO HACE FALTA PERO HAY QUE PREGUNTARLO
			if(!auxiliar->presente){
				//TODO: traerme la pagina desde swap
				//traer_pagina_a_memoria(pagina_aux);
			}*/
			if(bytes_a_leer > espacio_disponible_en_pagina){
				memcpy(&tid_aux + espacio_leido, auxiliar->inicio_memoria + offset_primer_pagina_entero, espacio_disponible_en_pagina);
				espacio_leido += espacio_disponible_en_pagina;
				espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
				offset_primer_pagina = 0;
				pagina_retorno = list_get(patota->paginas,minimo_entre(i - 1, 0)); // OJO CON ESTO

			}
			else {
				memcpy(&tid_aux + espacio_leido,auxiliar->inicio_memoria + offset_primer_pagina_entero,sizeof(uint32_t));
				memcpy(&tid_aux_posta,&tid_aux,4);
				if(tid_aux_posta == tid){
					retornar->indice = i;
					retornar->offset = offset_primer_pagina_entero;
					retornar->pagina = pagina_retorno;
					return retornar;

				}
			}
		}
	}
	retornar->indice = 0;
	retornar->offset = 0;
	retornar->pagina = NULL;
	return retornar;
}

void escribir_una_coordenada_a_partir_de_indice(double indice, int offset, uint32_t posicion, t_tabla_de_paginas* patota){
	t_pagina* auxiliar_pagina = list_get(patota->paginas,indice);
	int tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset;
	int bytes_escritos = 0;
	int bytes_desplazados_escritura = 0;
	int offset_lectura = 0;
	while(bytes_escritos < 4){
		pthread_mutex_lock(auxiliar_pagina->mutex_pagina);
		if(!auxiliar_pagina->presente){
			//TODO: traerme la pagina desde swap
			//traer_pagina_a_memoria(auxiliar_pagina);
		}
		if(tamanio_disponible_pagina < 4){
			memcpy(auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura,&posicion + offset_lectura,tamanio_disponible_pagina);
			bytes_escritos += tamanio_disponible_pagina;
			bytes_desplazados_escritura += tamanio_disponible_pagina;
			offset_lectura += tamanio_disponible_pagina;
			tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA;
			auxiliar_pagina = list_get(patotas,indice+1);
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
		}
		else{
			memcpy(auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura,&posicion + offset_lectura, 4 - bytes_escritos);
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			int escritura = 4 - bytes_escritos;
			bytes_escritos += escritura;
		}
	}

}

char * obtener_proxima_tarea_paginacion(uint32_t tripulante_tid)
{

	pthread_mutex_lock(&mutex_patotas);
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tripulante_tid);
	pthread_mutex_unlock(&mutex_patotas);
	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tripulante_tid);
		return RESPUESTA_FAIL;
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tripulante_tid);

	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	primer_indice += tamanio_tareas;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &primer_indice);
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	primer_indice -= 1;
	inicio_tcb* tcb = buscar_inicio_tcb(tripulante_tid,patota,primer_indice,offset_entero);
	int nuevo_indice = 0;
	while(!tcb->pagina){
		nuevo_indice += primer_indice + 5*sizeof(uint32_t) + sizeof(char);
		tcb = buscar_inicio_tcb(tripulante_tid,patota,nuevo_indice,offset_entero);
	}

	double indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (tcb->indice * 5*sizeof(uint32_t) + sizeof(char)) + 3 * sizeof(uint32_t) + sizeof(char);
	double offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	int offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	indice_de_posicion -= 1;


	int espacio_leido = 0;
	uint32_t indice_proxima_tarea;
	while(espacio_leido < 4){
		for(int k=indice_de_posicion; k<patota->paginas->elements_count; k++){
			t_pagina* auxiliar = list_get(patota->paginas,k);
			pthread_mutex_lock(auxiliar->mutex_pagina);
			int bytes_a_leer = sizeof(uint32_t);
			int bytes_escritos = 0;
			int espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_posicion_entero;
			int offset_pagina_entero = offset_posicion_entero;
			if(!auxiliar->presente){
				//TODO: traerme la pagina desde swap
				//traer_pagina_a_memoria(pagina_aux);
			}
			if(bytes_a_leer > espacio_disponible_en_pagina){
				memcpy(&indice_proxima_tarea + espacio_leido,auxiliar->inicio_memoria + offset_pagina_entero,espacio_disponible_en_pagina);
				espacio_leido += espacio_disponible_en_pagina;
				espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
				bytes_escritos = espacio_disponible_en_pagina;
				offset_pagina_entero = 0;
				}
			else{
			memcpy(&indice_proxima_tarea + espacio_leido,auxiliar->inicio_memoria + offset_pagina_entero,sizeof(uint32_t) - bytes_escritos);
			}
		}
 	}
	char* proxima_tarea = list_get(patota->tareas,indice_proxima_tarea);

	if(indice_proxima_tarea + 1 > patota->tareas->elements_count){
		log_info(logger_ram_hq,"El tripulante de tid: %i ya cumplio con todas sus tareas, no se actualizara la direccion logica");
		return proxima_tarea;
	}

	escribir_una_coordenada_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,indice_proxima_tarea + 2,patota);
	//+2 porque el indice arranca desde 0 y luego hay que incrementarlo una vez mas para la proxima tarea;

	return proxima_tarea;
}

char* obtener_proxima_tarea_segmentacion(uint32_t tripulante_tid)
{
	char* proxima_tarea;
	t_segmento_de_memoria* tripulante_aux;
	uint32_t tid_aux;
	t_tabla_de_segmento* auxiliar_patota = buscar_patota_con_tid(tripulante_tid);
	if(!auxiliar_patota){
		log_error(logger_ram_hq,"El tripulante %d no esta dado de alta en ninguna patota", tripulante_tid);
		return NULL;
	}
	//obtengo las tareas de la patota
	//pthread_mutex_lock(auxiliar_patota->segmento_tarea->mutex_segmento);
	char* tareas = malloc(auxiliar_patota->segmento_tarea->tamanio_segmento);
	memcpy(tareas,auxiliar_patota->segmento_tarea->inicio_segmento,auxiliar_patota->segmento_tarea->tamanio_segmento); //Me traigo todo el contenido del segmento de tareas
	//pthread_mutex_unlock(auxiliar_patota->segmento_tarea->mutex_segmento);
	
	//obtengo el numero de tarea que le corresponde al tripulante
	uint32_t id_tarea = -1;
	for(int i=0; i<auxiliar_patota->segmentos_tripulantes->elements_count; i++){
		pthread_mutex_lock(auxiliar_patota->mutex_segmentos_tripulantes);
		tripulante_aux = list_get(auxiliar_patota->segmentos_tripulantes,i);
		memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
		if(tid_aux == tripulante_tid){
			memcpy(&id_tarea,tripulante_aux->inicio_segmento + 3*(sizeof(uint32_t)) + 1, sizeof(uint32_t));			
			pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);
			break;	
		}
		pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);
	}
	if(id_tarea == -1){
		log_error(logger_ram_hq,"No encontre id tarea, %i", id_tarea);
	}

	strcpy(tareas,obtener_proxima_tarea(tareas,id_tarea,auxiliar_patota->segmento_tarea->tamanio_segmento));
	
	actualizarTareaActual(auxiliar_patota,tripulante_tid);

	return tareas;
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
	t_segmento_de_memoria* tripulante_aux;
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
			//pthread_mutex_lock(tripulante_aux->mutex_segmento);
			//obtengo su tid
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria, se procede a expulsarlo, motivo: SUS", tid);
				//encontre al tripulante, lo borro de memoria
				//bloque memoria
				pthread_mutex_lock(&mutex_memoria);
				for(int k=0; k<segmentos_memoria->elements_count; k++){
					// recorro segmentos de memoria
					segmento_aux = list_get(segmentos_memoria,k);
					if((int)(segmento_aux->inicio_segmento) == (int)(tripulante_aux->inicio_segmento)){
						log_info(logger_ram_hq,"Encontre el segmento perteneciente al tripulante en %d , procedo a liberarlo", (int)segmento_aux->inicio_segmento);
						segmento_aux->libre = true;
					}
				}
				pthread_mutex_unlock(&mutex_memoria);
				//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
				//free(tripulante_aux->mutex_segmento);
				//free(tripulante_aux);
				list_remove(patota_aux->segmentos_tripulantes,j);
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				//TODO: Borrar al tripulante del mapa
				return RESPUESTA_OK;
			}
			//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	pthread_mutex_unlock(&mutex_patotas);
	return RESPUESTA_FAIL;
}


estado obtener_estado_paginacion(uint32_t tripulante_tid)
{
	estado estado_obtenido;
	//TODO
	return estado_obtenido;
}

respuesta_ok_fail actualizar_estado_segmentacion(uint32_t tid,estado est)
{
	t_tabla_de_segmento* patota_aux;
	t_segmento_de_memoria* tripulante_aux;
	uint32_t tid_aux;
	
	char estado = obtener_char_estado(est);

	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tid);
	for(int i=0; i<patotas->elements_count; i++){
		patota_aux = list_get(patotas,i);
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			//pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
			if(tid_aux == tid){
				
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria", tid);
				memcpy(tripulante_aux->inicio_segmento + sizeof(uint32_t),&estado,sizeof(char));

				//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				return RESPUESTA_OK;
			}
			//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);

	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	pthread_mutex_unlock(&mutex_patotas);
	return RESPUESTA_FAIL;
}

char obtener_char_estado (estado est){
	switch (est)
	{
		case NEW:
		{
			return 'N';
		}
		case READY:
		{
			return 'R';
		}
		case BLOCKED_E_S:
		{
			return 'B';
		}
		case BLOCKED_SABOTAJE:
		{
			return 'S';
		}
		case EXEC:
		{
			return 'E';
		}
		case EXIT:
		{
			return 'Q';
		}
		case EXPULSADO:
		{
			return 'X';
		}
		case ERROR:
		{
			return 'F';
		}
		default:
		{
			log_error(logger_ram_hq, "El estado es invalido");
			return 'Z';
		}
	}

}
estado obtener_estado_char (char est){
	switch (est)
	{
		case 'N':
		{
			return NEW;
		}
		case 'R':
		{
			return READY;
		}
		case 'B':
		{
			return BLOCKED_E_S;
		}
		case 'S':
		{
			return BLOCKED_SABOTAJE;
		}		
		case 'E':
		{
			return EXEC;
		}
		case 'Q':
		{
			return EXIT;
		}
		case 'X':
		{
			return EXPULSADO;
		}
		case 'F':
		{
			return ERROR;
		}
		default:
		{
			log_error(logger_ram_hq, "El caracter es invalido");
			return ERROR;
		}
	}

}

estado obtener_estado_segmentacion(uint32_t tid)
{
	t_tabla_de_segmento* patota_aux;
	t_segmento_de_memoria* tripulante_aux;
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
			//pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria", tid);
				memcpy(estado,tripulante_aux->inicio_segmento + sizeof(uint32_t),sizeof(char));
				estado_numerico = atoi(estado);
				//faltan los unlock
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				return estado_numerico;

			}
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
	}
	pthread_mutex_unlock(&mutex_patotas);
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
	t_segmento_de_memoria* tripulante_aux;
	uint32_t tid_aux;
	posicion posicion;
	pthread_mutex_lock(&mutex_patotas);
	log_info(logger_ram_hq,"Buscando el tripulante %d",tid);
	for(int i=0; i<patotas->elements_count; i++){
		patota_aux = list_get(patotas,i);
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			//pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
			if(tid_aux == tid){
				log_info(logger_ram_hq,"Encontre al tripulante %d en memoria", tid);
				memcpy(&posicion.pos_x,tripulante_aux->inicio_segmento + sizeof(uint32_t) + sizeof(char),sizeof(uint32_t));
				memcpy(&posicion.pos_y,tripulante_aux->inicio_segmento + sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t), sizeof(uint32_t));
				//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				return posicion;
			}
			//pthread_mutex_unlock(tripulante_aux->mutex_segmento);
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
	}
	log_error(logger_ram_hq,"No se pudo encontrar el tripulante %d en memoria, solicitud rechazada", tid);
	pthread_mutex_unlock(&mutex_patotas);
	return posicion; // Los campos estan vacios
}

//-----------------------------------------------------------FUNCIONES DE BUSQUEDA DE SEGMENTOS--------------------------------------

t_tabla_de_paginas* buscar_patota_paginacion(uint32_t pid){
	t_tabla_de_paginas* auxiliar;
	t_pagina* auxiliar_pagina;
	uint32_t pid_auxiliar;
	uint32_t espacio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - 0; // 0 es la posicion donde arranca el dato que estoy buscando dentro de la pagina
	uint32_t espacio_a_leer = 4;
	uint32_t offset = 0;
	uint32_t espacio_leido = 0;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		for(int j=0; j<auxiliar->paginas->elements_count; j++){
			auxiliar_pagina = list_get(auxiliar->paginas,j);
			if(espacio_a_leer > espacio_disponible_pagina){
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->inicio_memoria + offset,espacio_disponible_pagina);
				espacio_leido += espacio_disponible_pagina;
				espacio_disponible_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;

			}
			else{
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->inicio_memoria + offset,espacio_a_leer - espacio_leido);
				if(pid_auxiliar == pid){
					return auxiliar;
				}
				else{
					break;
				}
			}
			
		}

	}
	return NULL;
}

t_list* buscar_cantidad_frames_libres(int cantidad){
	
	t_list* frames_a_usar = list_create();
	t_frame_en_memoria* auxiliar;
	int contador = 0;
	for(int i=0; i<frames->elements_count; i++){
		if(contador >= cantidad){
			return frames_a_usar;
		}
		auxiliar = list_get(frames,i);
		if(auxiliar->libre){
		auxiliar->libre = false;
		list_add(frames_a_usar,auxiliar);
		contador ++;
		}
	}
	return frames_a_usar;
}

t_tabla_de_segmento* buscar_patota(uint32_t pid){
	t_tabla_de_segmento* auxiliar;
	uint32_t pid_auxiliar;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		memcpy(&pid_auxiliar,auxiliar->segmento_pcb->inicio_segmento,sizeof(uint32_t));
		if(pid_auxiliar == pid){
			return auxiliar;
		}
	}
	return NULL;
}

t_tabla_de_segmento* buscar_patota_con_tid(uint32_t tid){
	t_tabla_de_segmento* auxiliar_patota;
	t_segmento_de_memoria* segmento_tripulante_auxiliar;
	log_info(logger_ram_hq,"Buscando la patota a partir del tid %d",tid);
	pthread_mutex_lock(&mutex_patotas);
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar_patota = list_get(patotas,i);
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			uint32_t tid_aux;
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			//pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar->inicio_segmento,sizeof(uint32_t));
			if(tid_aux == tid){
				//pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_patotas);
				log_info(logger_ram_hq,"Patota correspondiente al tid: %d encontrada",tid);
				return auxiliar_patota;
			}
			//pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);
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
				auxiliar->numero_segmento = numero_segmento_global;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
				numero_segmento_global++;
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
				auxiliar->numero_segmento = numero_segmento_global;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
				numero_segmento_global++;
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
	free(auxiliar);
	return NULL;
};

t_segmento_de_memoria* buscar_segmento_tcb(){
	t_segmento_de_memoria* iterador;
	t_segmento_de_memoria* auxiliar = malloc(sizeof(t_segmento_de_memoria));
	uint32_t size_tcb = sizeof(uint32_t)*5 + sizeof(char);
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if(iterador->tamanio_segmento >= size_tcb && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = size_tcb; 
				auxiliar->libre = false;
				auxiliar->numero_segmento = numero_segmento_global;
				numero_segmento_global++;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
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
	free(auxiliar);
	return NULL;
};
//----------------------------------------------CARGAR SEGMENTOS A MEMORIA-------------------------------------------------------------------------

void cargar_pcb_en_segmento(uint32_t pid, uint32_t direccion_logica_tareas, t_segmento_de_memoria* segmento){
	//pthread_mutex_lock(segmento->mutex_segmento);
	int offset = 0;
	memcpy(segmento->inicio_segmento + offset, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &direccion_logica_tareas, sizeof(uint32_t));
	//pthread_mutex_unlock(segmento->mutex_segmento);
}

//Aclaración: Esta función asume que las tareas incluyen un /0 al final de la cadena de char*
void cargar_tareas_en_segmento(char* tareas,uint32_t longitud_palabra, t_segmento_de_memoria* segmento){
	//pthread_mutex_lock(segmento->inicio_segmento);
	memcpy(segmento->inicio_segmento, tareas, longitud_palabra);
	//pthread_mutex_unlock(segmento->mutex_segmento);
	recorrer_tareas(segmento);
}

//No la estamos usando
void cargar_tcb_en_segmento(uint32_t tid,estado estado_nuevo,uint32_t pos_x,uint32_t pos_y,uint32_t direccion_tarea,uint32_t direccion_patota,t_segmento_de_memoria* segmento){
//void cargar_tcb_en_segmento(nuevo_tripulante_sin_pid tripulante, segmento){
	//pthread_mutex_lock(segmento->mutex_segmento);
	
	int offset = 0;
	memcpy(segmento->inicio_segmento + offset, &tid, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &estado_nuevo, sizeof(estado));
	offset = offset + sizeof(estado);
	memcpy(segmento->inicio_segmento + offset, &pos_x, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &pos_y, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &direccion_tarea, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &direccion_patota, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	
	//pthread_mutex_unlock(segmento->mutex_segmento);
}
void cargar_tcb_sinPid_en_segmento(nuevo_tripulante_sin_pid* tripulante, t_segmento_de_memoria* segmento,uint32_t pid){
	//pthread_mutex_lock(segmento->mutex_segmento);
	
	uint32_t cero = 0;
	char est = 'N';

	int offset = 0;
	memcpy(segmento->inicio_segmento + offset, & (tripulante->tid), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &est, sizeof(char));
	offset = offset + sizeof(char);
	memcpy(segmento->inicio_segmento + offset, & (tripulante->pos_x), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, & (tripulante->pos_y), sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &cero, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &pid, sizeof(uint32_t));
	offset = offset + sizeof(uint32_t);
	//pthread_mutex_unlock(segmento->mutex_segmento);
}


//-------------------------------------------------------------------------FUNCION DE COMPACTACION-----------------------------------------------------

void swap(t_segmento_de_memoria **aux1,t_segmento_de_memoria **aux2){
	t_segmento_de_memoria* temp = malloc(sizeof(t_segmento_de_memoria));
	temp->inicio_segmento = (*aux1)->inicio_segmento;
	temp->libre = (*aux1)->libre;
	temp->tamanio_segmento = (*aux1) ->tamanio_segmento;
	
	(*aux1)->inicio_segmento =  (*aux2)->inicio_segmento;
	(*aux1)->libre = (*aux2)->libre;
	(*aux1)->tamanio_segmento = (*aux2)->tamanio_segmento;
	
	(*aux2)->inicio_segmento = temp->inicio_segmento;
	(*aux2)->libre = temp->libre;
	(*aux2)->tamanio_segmento = temp->tamanio_segmento;
}

void ordenar_segmentos(){	
	t_segmento_de_memoria* aux3 = malloc(sizeof(t_segmento_de_memoria));
	//t_segmento_de_memoria aux3;
	int i, j; 
	for (i = 0; i < segmentos_memoria->elements_count -2; i++){
		for (j = 0; j < segmentos_memoria->elements_count-i-2; j++) {
			t_segmento_de_memoria* aux1 = list_get(segmentos_memoria,j);
			t_segmento_de_memoria* aux2 = list_get(segmentos_memoria,j+1);
			if(aux1->inicio_segmento > aux2->inicio_segmento){
				
				swap(&aux1,&aux2);
				int a = 0;
				/*
				aux3->inicio_segmento = aux1->inicio_segmento;
				aux3->libre = aux1->libre;
				aux3->tamanio_segmento = aux1 ->tamanio_segmento;
				
				aux1->inicio_segmento =  aux2->inicio_segmento;
				aux1->libre = aux2->libre;
				aux1->tamanio_segmento = aux2->tamanio_segmento;
				
				aux2->inicio_segmento = aux3->inicio_segmento;
				aux2->libre = aux3->libre;
				aux2->tamanio_segmento = aux3->tamanio_segmento;
				 */
				
			}
				
		}      
	}   
}


void compactar_memoria(){
	//usar mutex pa freezar todo

	t_segmento_de_memoria* auxiliar;
	auxiliar = list_get(segmentos_memoria,0);
	int i;
	//TODO: ORDENAR LISTA DE SEGMENTOS POR DIRECCIONES
	//quiza hay que ordenar por la base del segmento que apunta
	printf("Lista de segmentos actual\n");	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
	}


	list_sort(segmentos_memoria,ordenar_direcciones_de_memoria); // No se si esta bien usada la funcion, lo voy a preguntar en soporte pero dejo la idea
	printf("Segmentos ordenados\n");	
	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
	}
	//ordenar_segmentos(); // No se si esta bien usada la funcion, lo voy a preguntar en soporte pero dejo la idea
	//-----------------------------------------------
	//t_list* lista_de_ocupados = list_create();
	printf("Limpio segmentos vacios\n");	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
		/*
		if(!auxiliar->libre){
			list_add(lista_de_ocupados,auxiliar);
		}
		else{
			list_remove(segmentos_memoria,i);
			i--;
		}
		  */
		if(auxiliar->libre){
			list_remove(segmentos_memoria,i);
			i--;
			printf("Eliminando segmento %i\n",auxiliar->numero_segmento);			
		}
	}
	
	int offset = 0;
	void* inicio_segmento_libre;
	uint32_t tamanio_acumulado = 0;
	uint32_t tamanio_segmento_libre = 0;
	printf("Moviendo segmentos\n");
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Pasando segmento %d de %i a %d \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,memoria_principal + offset);
		//printf("Pasando segmento %d desde %i a %i \n",);
		//pthread_mutex_lock(segmento_auxiliar->mutex_segmento);
		memcpy(memoria_principal + offset, auxiliar->inicio_segmento, auxiliar->tamanio_segmento);
		//pthread_mutex_unlock(segmento_auxiliar->mutex_segmento);
		//segmento_auxiliar->base = memoria_principal + offset;
		auxiliar->inicio_segmento = memoria_principal + offset;
		tamanio_acumulado += auxiliar->tamanio_segmento;
		offset += auxiliar->tamanio_segmento;
	}
	
	inicio_segmento_libre = memoria_principal + tamanio_acumulado; // Si rompe, castear el inicio de auxiliar como uint32_t
	tamanio_segmento_libre = mi_ram_hq_configuracion->TAMANIO_MEMORIA - tamanio_acumulado; //No se si va el +1, para dami no va, lo borro

	t_segmento_de_memoria* segmento_libre = malloc(sizeof(t_segmento_de_memoria));
	segmento_libre->libre = true;
	segmento_libre->inicio_segmento = inicio_segmento_libre;
	segmento_libre->tamanio_segmento = tamanio_segmento_libre;
	segmento_libre->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(segmento_libre->mutex_segmento,NULL);

	list_add(segmentos_memoria,segmento_libre); // Al final de la ordenada le agrego el segmento libre
	//segmentos_memoria = lista_de_ocupados; // Actualizamos la lista de segmentos global con la nueva que ya esta ordenada
	printf("Segmentos luego de compactar\n");
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
	}
}

bool ordenar_direcciones_de_memoria(void* p1, void* p2){
	t_segmento_de_memoria* segmento1 = p1;
	t_segmento_de_memoria* segmento2 = p2;
		
	return ((int)segmento1->inicio_segmento) < ((int)segmento2->inicio_segmento);
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
void recorrer_pcb(t_segmento_de_memoria * pcb){
	uint32_t pid;
	//pthread_mutex_lock(pcb->mutex_segmento);
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	//pthread_mutex_unlock(pcb->mutex_segmento);
	printf ("Patota pid = %i\n",pid);
}
void recorrer_tareas(t_segmento_de_memoria * tareas){
	//pthread_mutex_lock(tareas->mutex_segmento);
	char* auxiliar = malloc (tareas->tamanio_segmento);
	memcpy(auxiliar, tareas->inicio_segmento, tareas->tamanio_segmento);
	//pthread_mutex_unlock(tareas->mutex_segmento);
	//quiza hay que recorrer distinto pq frena en el primer /0
	
	printf ("Lista de tareas: \n");
	printf ("%s\n",auxiliar);
	//printf ("%s\n",auxiliar+1);
	//printf ("%c\n",*auxiliar);
	for(int i = 0;i<tareas->tamanio_segmento -1;i++){
		//printf ("%c\n",*(auxiliar+i));
		 if( *(auxiliar+i) == '\0'){
			printf ("%s\n",auxiliar+i+1);
		}

	}
}
void recorrer_tcb(t_list * tripulantes_list){
	for(int i = 0;i < tripulantes_list->elements_count;i++){
		t_segmento_de_memoria * tripulante = list_get(tripulantes_list,i);
		//pthread_mutex_lock(tripulante->mutex_segmento);
		
		uint32_t tid = 0;
		char estado_actual;
		uint32_t posX = 0;
		uint32_t posY = 0;
		uint32_t tareaActual = 0;
		uint32_t patotaActual = 0;

		int offset = 0;
		memcpy(&tid, tripulante->inicio_segmento + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&estado_actual, tripulante->inicio_segmento + offset, sizeof(char));
		offset = offset + sizeof(char);
		memcpy(&posX, tripulante->inicio_segmento + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&posY, tripulante->inicio_segmento + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&tareaActual, tripulante->inicio_segmento + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		memcpy(&patotaActual, tripulante->inicio_segmento + offset, sizeof(uint32_t));
		offset = offset + sizeof(uint32_t);
		
		//pthread_mutex_unlock(tripulante->mutex_segmento);
		printf ("Tripulante tid %i, estado %c, posicion %d;%d, tarea %i, patota %i\n",tid,estado_actual,posX,posY,tareaActual,patotaActual);
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
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_pcb->numero_segmento,patota->segmento_pcb->inicio_segmento,patota->segmento_pcb->tamanio_segmento);
		//recorrer_tareas_dump(pid,patota->segmento_tarea);
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_tarea->numero_segmento,patota->segmento_tarea->inicio_segmento,patota->segmento_tarea->tamanio_segmento);
		pthread_mutex_lock(patota->mutex_segmentos_tripulantes);
		recorrer_tcb_dump(pid,patota->segmentos_tripulantes);
		pthread_mutex_unlock(patota->mutex_segmentos_tripulantes);
	}
}

uint32_t obtener_patota_memoria(t_segmento_de_memoria * pcb){
	uint32_t pid;
	//pthread_mutex_lock(pcb->mutex_segmento);
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	//pthread_mutex_unlock(pcb->mutex_segmento);
	return pid;
}

void recorrer_pcb_dump(t_segmento_de_memoria* pcb){
	uint32_t pid;
	//pthread_mutex_lock(pcb->mutex_segmento);
	
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	//pthread_mutex_unlock(pcb->mutex_segmento);
	printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,pcb->numero_segmento,pcb->inicio_segmento,pcb->tamanio_segmento);
}
void recorrer_tareas_dump(uint32_t pid,t_segmento_de_memoria* tareas){
	printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tareas->numero_segmento,tareas->inicio_segmento,tareas->tamanio_segmento);
}
void recorrer_tcb_dump(uint32_t pid,t_list* tripulantes){
	for(int i = 0;i < tripulantes->elements_count;i++){
		t_segmento_de_memoria * tripulante = list_get(tripulantes,i);
		printf ("Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tripulante->numero_segmento,tripulante->inicio_segmento,tripulante->tamanio_segmento);
	}
}


//Cuando un FRAME no tiene un proceso imprime 0, los estados son 1: Libre 0: ocupado, capaz hay que cambiar esto
 void imprimir_dump_paginacion(){
	printf ("--------------------------------------------------------------------------\n");printf ("--------------------------------------------------------------------------\n");
	char * time =  temporal_get_string_time("%d/%m/%y %H:%M:%S");
	printf ("Dump: %s \n",time);

	for(int i=0; i<frames->elements_count; i++){
		t_frame_en_memoria* frame = list_get(frames,i);
		printf("Marco: %i\t Estado: %i\t Proceso: %i\t Pagina: %i \n",i,frame->libre,frame->pid_duenio,frame->indice_pagina);
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
char * obtener_proxima_tarea(char * tareas,uint32_t id_tarea,uint32_t size_tareas){
	int n_tarea = 0;
	for(int i = 0;i<size_tareas-1;i++){
		if(n_tarea == id_tarea){
			//char * aux = malloc (strlen(tareas+i));
			//strcpy(aux,tareas+i);
			return tareas+i;
		}
		if(*(tareas+i) == '\0'){
			n_tarea++;
		}
	}
	
	return tareas+strlen(tareas);
}

void actualizarTareaActual(t_tabla_de_segmento* auxiliar_patota,uint32_t tripulante_tid){
	pthread_mutex_lock(auxiliar_patota->mutex_segmentos_tripulantes);
	for(int i = 0;i < auxiliar_patota->segmentos_tripulantes->elements_count;i++){
		t_segmento_de_memoria * tripulante = list_get(auxiliar_patota->segmentos_tripulantes,i);
		uint32_t tid = 0;
		
		//pthread_mutex_lock(tripulante->mutex_segmento);
		memcpy(&tid, tripulante->inicio_segmento, sizeof(uint32_t));
		if(tid == tripulante_tid){			
			int offset = sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t) + sizeof(uint32_t);
			uint32_t tareaActual;
			memcpy(&tareaActual, tripulante->inicio_segmento + offset, sizeof(uint32_t));
			tareaActual++;
			memcpy(tripulante->inicio_segmento + offset,&tareaActual , sizeof(uint32_t));
			
		}
		//pthread_mutex_unlock(tripulante->mutex_segmento);
		
	}
	pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);
	
	return;
}


uint32_t calcular_memoria_libre(){
	t_segmento_de_memoria* iterador;
	uint32_t libre = 0;
	for(int i=0; i<segmentos_memoria->elements_count;i++){
		iterador = list_get(segmentos_memoria,i);
		//hacer mutex
		//rompe si el segmento esta libre pq no tiene el mutex inicializado
		if(iterador->libre){
			libre = libre + iterador->tamanio_segmento; 
		}
		
	}
	return libre;
}