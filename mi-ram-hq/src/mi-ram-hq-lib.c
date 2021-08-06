#include "mi-ram-hq-lib.h"
#include <stdlib.h>

pthread_mutex_t mutex_frames;
pthread_mutex_t mutex_no_reemplazar;
pthread_mutex_t mutex_mover_trip;
pthread_mutex_t mutex_dummy;
pthread_mutex_t mutex_accion;
char* accion;
int socket_accion;

void crear_estructuras_administrativas()
{
	patotas = list_create();
	segmentos_memoria = list_create();
	pthread_mutex_init(&mutex_tabla_patotas,NULL);
	pthread_mutex_init(&mutex_tabla_de_segmentos,NULL);
	pthread_mutex_init(&mutex_memoria,NULL);
	pthread_mutex_init(&mutex_swap,NULL);
	pthread_mutex_init(&mutex_iniciar_patota,NULL);
	pthread_mutex_init(&mutex_busqueda_patota,NULL);
	pthread_mutex_init(&mutex_lru,NULL);
	pthread_mutex_init(&mutex_dump,NULL);
	pthread_mutex_init(&mutex_frames,NULL);
	pthread_mutex_init(&mutex_no_reemplazar,NULL);
	pthread_mutex_init(&mutex_mover_trip,NULL);
	pthread_mutex_init(&mutex_dummy,NULL);
	pthread_mutex_init(&mutex_accion,NULL);
	

	crear_mapa ();
	signal(SIGUSR2, sighandlerDump);
	//signal(SIGINT,sighandlerLiberarMemoria); 
	if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		log_info(logger_ram_hq,"Creando estructuras administrativas para esquema de memoria: Segmentacion");
		numero_segmento_global = 0;
		t_segmento* segmento = malloc(sizeof(t_segmento));
		segmento->inicio_segmento =  memoria_principal;
		segmento->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		segmento->libre = true;
		segmento->mutex_segmento = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(segmento->mutex_segmento,NULL);
		log_info(logger_ram_hq,"Se reservaron %i bytes de memoria que comienzan en %i",mi_ram_hq_configuracion->TAMANIO_MEMORIA,memoria_principal);
		list_add(segmentos_memoria,segmento);
		signal(SIGUSR1, sighandlerCompactar); //10
		
	}
	else if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
	{
		log_info(logger_ram_hq,"Creando estructuras administrativas para esquema de memoria: Paginacion \n");
		frames = list_create();
		int cantidad_frames = mi_ram_hq_configuracion->TAMANIO_MEMORIA / mi_ram_hq_configuracion->TAMANIO_PAGINA;
		int offset = 0;
		for(int i =0; i<cantidad_frames; i++){
			t_frame_en_memoria* un_frame = malloc(sizeof(t_frame_en_memoria));
			un_frame->inicio = memoria_principal + offset;
			un_frame->libre = 1;
			un_frame->pagina_a_la_que_pertenece = NULL;
			un_frame->pid_duenio = 0;
			un_frame->indice_pagina = 0;
			un_frame->mutex = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(un_frame->mutex,NULL);
			offset += mi_ram_hq_configuracion->TAMANIO_PAGINA;
			list_add(frames,un_frame);
			log_info(logger_ram_hq,"Cree el frame %i, inicia en %i \n",i,un_frame->inicio);
		}
		inicializar_swap();
		puntero_lista_frames_clock = 0;
		historial_uso_paginas = list_create();
	}
	else
	{
		log_error(logger_ram_hq, "El esquema de memoria no es soportado por el sistema");
	}
	
}

void inicializar_swap(){
	log_info(logger_ram_hq,"Levantando el area de SWAP \n");
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
	log_info(logger_ram_hq,"Creando estructuras administrativas de SWAP: \n");
	frames_swap = list_create();
	int cantidad_frames_swap = mi_ram_hq_configuracion->TAMANIO_SWAP / mi_ram_hq_configuracion->TAMANIO_PAGINA;
	int offset_swap = 0;
	for (int i=0; i<cantidad_frames_swap; i++){
		t_frame_en_swap* frame_swap = malloc(sizeof(t_frame_en_swap));
		frame_swap->inicio = memoria_swap + offset_swap;
		frame_swap->libre = 1;
		frame_swap->mutex = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(frame_swap->mutex,NULL);
		offset_swap += mi_ram_hq_configuracion->TAMANIO_PAGINA;
		list_add(frames_swap,frame_swap);
		//log_info(logger_ram_hq,"Cree el frame de SWAP %i, comienza en %i \n",i,frame_swap->inicio);
	}
}

mi_ram_hq_config *leer_config_mi_ram_hq(char *path)
{
	config_aux = config_create(path);
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
	system("rm cfg/mi-ram-hq.log");
	//Al iniciarlizar ese parametro en 0, el logger no escribe por consola y le deja ese espacio al mapa
	t_log *log_aux = log_create(path, "MI-RAM-HQ", 0, LOG_LEVEL_INFO);
	return log_aux;
}

void *esperar_conexion(int puerto)
{
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
			log_info(logger_ram_hq, "NUEVA CONEXIÓN! Socket id: %i",socket_mi_ram_hq);
			crear_hilo_para_manejar_suscripciones(socket_mi_ram_hq);
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

void crear_hilo_para_manejar_suscripciones(int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_attr_t atributo_hilo;
	pthread_attr_init(&atributo_hilo);
	pthread_attr_setdetachstate(&atributo_hilo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo_conectado, &atributo_hilo, (void *)manejar_suscripciones_mi_ram_hq, socket_hilo);
	//pthread_join(&hilo_conectado,NULL);
	return;
}

void *manejar_suscripciones_mi_ram_hq(int *socket_hilo)
{
	//pthread_mutex_lock(&mutex_dummy);
	log_info(logger_ram_hq, "Se creó el hilo correctamente para la conexion %i",*socket_hilo);
	t_paquete *paquete = recibir_paquete(*socket_hilo);
	//pthread_mutex_lock(&mutex_falopa);
	if (!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		switch (paquete->codigo_operacion)
		{
		case INICIAR_PATOTA:
		{

			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Comenzando deserializacion del mensaje",*socket_hilo);
			pid_con_tareas_y_tripulantes_miriam patota_con_tareas_y_tripulantes = deserializar_pid_con_tareas_y_tripulantes(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Iniciando proceso de segmentacion de datos",*socket_hilo);
			pthread_mutex_lock(&mutex_iniciar_patota);
			respuesta_ok_fail resultado = iniciar_patota_segmentacion(patota_con_tareas_y_tripulantes,*socket_hilo);
			pthread_mutex_unlock(&mutex_iniciar_patota);
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			free(patota_con_tareas_y_tripulantes.tareas);
			list_destroy_and_destroy_elements(patota_con_tareas_y_tripulantes.tripulantes,free);
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Comenzando deserializacion del mensaje",*socket_hilo);
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Iniciando proceso de actualizacion de datos",*socket_hilo);
			respuesta_ok_fail resultado = actualizar_ubicacion_segmentacion(tripulante_y_posicion,*socket_hilo);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Comenzando deserializacion del mensaje",*socket_hilo);
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Iniciando proceso de obtencion de datos",*socket_hilo);
			char* proxima_tarea = obtener_proxima_tarea_segmentacion(tripulante_tid,*socket_hilo); 
			uint32_t long_tarea = strlen(proxima_tarea)+1;
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Devolviendo tarea obtenida: %s (de longitud %i)",*socket_hilo,proxima_tarea,long_tarea);
			void *respuesta = serializar_tarea(proxima_tarea,long_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, long_tarea+sizeof(uint32_t), respuesta);
			free(proxima_tarea);
			break;
		}
		case EXPULSAR_TRIPULANTE:
		{
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Comenzando deserializacion del mensaje",*socket_hilo);
			uint32_t tripulante_tid = deserializar_tid(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Iniciando proceso borrado de datos",*socket_hilo);
			respuesta_ok_fail resultado = expulsar_tripulante_segmentacion(tripulante_tid,*socket_hilo);
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_EXPULSAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_ESTADO:
		{
			uint32_t tripulante_tid;
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Comenzando deserializacion del mensaje",*socket_hilo);
			estado estado_nuevo = deserializar_estado_tcb (paquete->stream,&tripulante_tid);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Iniciando proceso de actualizacion de datos",*socket_hilo);
			respuesta_ok_fail resultado = actualizar_estado_segmentacion(tripulante_tid,estado_nuevo,*socket_hilo);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_ESTADO, sizeof(respuesta_ok_fail), respuesta);
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
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Comenzando deserializacion del mensaje",*socket_hilo);
			pid_con_tareas_y_tripulantes_miriam pid_con_tareas_y_tripulantes = deserializar_pid_con_tareas_y_tripulantes(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Iniciando proceso de paginacion de datos",*socket_hilo);
			pthread_mutex_lock(&mutex_iniciar_patota);
			patota_stream_paginacion patota_con_tareas_y_tripulantes = orginizar_stream_paginacion(pid_con_tareas_y_tripulantes);
			respuesta_ok_fail resultado = iniciar_patota_paginacion(patota_con_tareas_y_tripulantes);
			pthread_mutex_unlock(&mutex_iniciar_patota);
			pthread_mutex_lock(&mutex_accion);
			socket_accion = *socket_hilo;
			accion = "INICIAR_PATOTA";
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Comenzando deserializacion del mensaje",*socket_hilo);
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Iniciando proceso de actualizacion de datos",*socket_hilo);
			respuesta_ok_fail resultado = actualizar_ubicacion_paginacion(tripulante_y_posicion);
			pthread_mutex_lock(&mutex_accion);
			socket_accion = *socket_hilo;
			accion = "ACTUALIZAR_UBICACION";
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Comenzando deserializacion del mensaje",*socket_hilo);
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Iniciando proceso de obtencion de datos",*socket_hilo);
			char * proxima_tarea = obtener_proxima_tarea_paginacion(tripulante_pid);
			pthread_mutex_lock(&mutex_accion);
			socket_accion = *socket_hilo;
			accion = "OBTENER_PROXIMA_TAREA";
			uint32_t long_tarea = strlen(proxima_tarea)+1;
			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Devolviendo tarea obtenida: %s (de longitud %i)",*socket_hilo,proxima_tarea,long_tarea);
			void *respuesta = serializar_tarea(proxima_tarea,long_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, long_tarea+sizeof(uint32_t), respuesta);
			free(proxima_tarea);
			break;
		}
		case EXPULSAR_TRIPULANTE:
		{
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Comenzando deserializacion del mensaje",*socket_hilo);
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Iniciando proceso borrado de datos",*socket_hilo);
			respuesta_ok_fail resultado = expulsar_tripulante_paginacion(tripulante_pid);
			pthread_mutex_lock(&mutex_accion);
			socket_accion = *socket_hilo;
			accion = "EXPULSAR_TRIPULANTE";
			log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_EXPULSAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_ESTADO:
		{
			uint32_t tripulante_tid;
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Comenzando deserializacion del mensaje",*socket_hilo);
			estado estado_nuevo = deserializar_estado_tcb (paquete->stream,&tripulante_tid);
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Iniciando proceso de actualizacion de datos tid %i",*socket_hilo,tripulante_tid);
			respuesta_ok_fail resultado = actualizar_estado_paginacion(tripulante_tid,estado_nuevo,*socket_hilo);
			pthread_mutex_lock(&mutex_accion);
			socket_accion = *socket_hilo;
			accion = "ACTUALIZAR_ESTADO";
			log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Resultado %i (0 ok 1 fail)",*socket_hilo,resultado);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_ESTADO, sizeof(respuesta_ok_fail), respuesta);
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
		imprimir_dump_paginacion(logger_ram_hq,"");
		//sighandlerDump(1);
		
	} 
	else if(!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION")){
		//imprimir_dump();
	}
	log_info(logger_ram_hq,"Cerrando socket %i",*socket_hilo);
	liberar_paquete(paquete);
	close(*socket_hilo);
    free(socket_hilo);
	//pthread_mutex_unlock(&mutex_falopa);
	//pthread_mutex_unlock(&mutex_dummy);
	pthread_exit(NULL);
	return NULL;
}

//------------------------------------------MANEJO DE LOGICA DE MENSAJES-------------------------------------

respuesta_ok_fail iniciar_patota_segmentacion(pid_con_tareas_y_tripulantes_miriam patota_con_tareas_y_tripulantes,int socket)
{
	t_segmentos_de_patota* patota = buscar_patota(patota_con_tareas_y_tripulantes.pid);
	if(patota){
		pthread_mutex_unlock(&mutex_tabla_patotas);
		log_error(logger_ram_hq,"Socket %i, INICIAR_PATOTA: La patota de pid %i ya existe, solicitud rechazada",socket,patota_con_tareas_y_tripulantes.pid);
		return RESPUESTA_FAIL;
	}
	
	//ver si entra todo, sino devolver fail
	uint32_t memoria_libre = calcular_memoria_libre();
	// tiene la longitud de todas las tareas + pid + cantidad de tripulantes * (estado + tid + tarea_actual + posx + posy + patota_direccion_virtual)
	uint32_t memoria_necesaria = patota_con_tareas_y_tripulantes.longitud_palabra + 2*(sizeof(uint32_t)) + patota_con_tareas_y_tripulantes.tripulantes->elements_count * (sizeof(uint32_t)*5 + sizeof(char));

	if(memoria_libre < memoria_necesaria){
		log_error(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No hay lugar para guardar la patota %i",socket,patota_con_tareas_y_tripulantes.pid);
		return RESPUESTA_FAIL;
	}

	//inicializo tabla de segmentos para esta patota
	patota = malloc(sizeof(t_segmentos_de_patota));
	patota->segmento_pcb = NULL;
	patota->segmento_tarea = NULL;
	patota->segmentos_tripulantes = list_create();
	patota->mutex_segmentos_tripulantes = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_segmentos_tripulantes,NULL);
	
	//Busco segmento para el pcb
	t_segmento* segmento_a_usar_pcb = buscar_segmento_pcb();
	if(!segmento_a_usar_pcb){
		//TODO: compactar
		log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No encontre un segmento disponible para el PCB de PID: %i , voy a compactar",socket,patota_con_tareas_y_tripulantes.pid);
		compactar_memoria();
		segmento_a_usar_pcb = buscar_segmento_pcb();
		//no deberia pasar
		if(!segmento_a_usar_pcb){
			log_error(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No hay espacio en la memoria para almacenar el PCB aun luego de compactar, solicitud rechazada. FAIL",socket);
			return RESPUESTA_FAIL;
		}
	}

	//agrego segmento pcb a mi patota (tabla de segmentos)
	log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Encontre un segmento para el PCB de PID: %i , empieza en: %i ",socket,patota_con_tareas_y_tripulantes.pid,(int) segmento_a_usar_pcb->inicio_segmento); 
	patota->segmento_pcb = segmento_a_usar_pcb;
	
	//busco segmento para las tareas
	t_segmento* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.longitud_palabra); 
	if(!segmento_a_usar_tareas){
		log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No encontre un segmento disponible para las tareas, voy a compactar",socket);
		compactar_memoria();
		segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas_y_tripulantes.longitud_palabra); 
		//no deberia pasar
		if(!segmento_a_usar_tareas){
			log_error(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No hay espacio en la memoria para almacenar las tareas aun luego de compactar, solicitud rechazada. FAIL",socket);
			return RESPUESTA_FAIL;
		}
	}

	log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Encontre un segmento para las tareas, empieza en: %i ",socket,(int) segmento_a_usar_tareas->inicio_segmento);
	
	//agrego segmento tareas a mi patota
	patota->segmento_tarea = segmento_a_usar_tareas;

	for(int i=0; i<patota_con_tareas_y_tripulantes.tripulantes->elements_count; i++){
		t_segmento* segmento_a_usar_tripulante = buscar_segmento_tcb();
		if(!segmento_a_usar_tripulante){
			log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No encontre un segmento disponible para el tripulante numero: %i, voy a compactar",socket,i+1);
			compactar_memoria();
			segmento_a_usar_tripulante = buscar_segmento_tcb();
			//no deberia pasar
			if(!segmento_a_usar_tripulante){
				log_error(logger_ram_hq,"Socket %i, INICIAR_PATOTA: No hay suficiente espacio para almacenar al tripulante numero: %i de la patota, solicitud rechazada. FAIL",socket, i+1);
				return RESPUESTA_FAIL;
			}
		}	
		log_info(logger_ram_hq,"Socket %i, INICIAR_PATOTA: Encontre un segmento para el tripulante numero: %i , empieza en: %i ",socket,i+1,(int) segmento_a_usar_tripulante->inicio_segmento);
		
		//agrego segmento tripulante a la lista de tripulantes de la patota (tabla de segmentos)
		list_add(patota->segmentos_tripulantes,segmento_a_usar_tripulante);
		
		nuevo_tripulante_sin_pid* tripulante = list_get(patota_con_tareas_y_tripulantes.tripulantes,i);
		
		crear_tripulante_mapa(tripulante->tid,tripulante->pos_x,tripulante->pos_y);
		
		cargar_tcb_sinPid_en_segmento(tripulante, segmento_a_usar_tripulante,patota_con_tareas_y_tripulantes.pid);
	}

	uint32_t direccion_logica_tareas = 0; //TODO: Calcular la direccion logica real
	//Los semaforos se toman dentro de las funciones:
	cargar_pcb_en_segmento(patota_con_tareas_y_tripulantes.pid,direccion_logica_tareas,patota->segmento_pcb);
	//hace falta longitud palabra?
	cargar_tareas_en_segmento(patota_con_tareas_y_tripulantes.tareas,patota_con_tareas_y_tripulantes.longitud_palabra,patota->segmento_tarea);
	
	//agrego patota a mi lista de patotas (agrego mi tabla de segmentos a la tabla de procesos)
	list_add(patotas,patota);
	
	return RESPUESTA_OK;
}


respuesta_ok_fail 
iniciar_patota_paginacion(patota_stream_paginacion patota_con_tareas_y_tripulantes)
{
	uint32_t pid = 0;
	memcpy(&pid,patota_con_tareas_y_tripulantes.stream,sizeof(uint32_t));
	pthread_mutex_lock(&mutex_tabla_patotas);
	t_tabla_de_paginas* patota = buscar_patota_paginacion(pid);
	if(patota){
		pthread_mutex_unlock(&mutex_tabla_patotas);
		log_error(logger_ram_hq,"La patota de pid %i ya existe, solicitud rechazada",pid);
		free(patota_con_tareas_y_tripulantes.stream);
		return RESPUESTA_FAIL;
	}	

	patota = malloc(sizeof(t_tabla_de_paginas));
	patota->mutex_tabla_paginas = malloc(sizeof(pthread_mutex_t));
	patota->mutex_tripulantes_vivos = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_tabla_paginas,NULL);
	pthread_mutex_init(patota->mutex_tripulantes_vivos,NULL);
	patota->paginas = list_create();
	patota->tareas = patota_con_tareas_y_tripulantes.tareas;
	patota->cantidad_tripulantes = patota_con_tareas_y_tripulantes.cantidad_tripulantes;
	patota->contador_tripulantes_vivos = patota_con_tareas_y_tripulantes.cantidad_tripulantes;
	patota->id_patota = pid;

	float cant = patota_con_tareas_y_tripulantes.tamanio_patota / mi_ram_hq_configuracion->TAMANIO_PAGINA;
	int cantidad_paginas_a_usar = ceilf(cant);

	int contador_frames_libres = 0;

	pthread_mutex_lock(&mutex_frames);
	for(int i=0;i<frames->elements_count; i++){
		t_frame_en_memoria* frame_aux = list_get(frames,i);
		if(frame_aux->libre)
			contador_frames_libres++;
	}
	pthread_mutex_unlock(&mutex_frames);
	
	for(int i=0; i<frames_swap->elements_count;i++){
		t_frame_en_swap* frame_aux = list_get(frames_swap,i);
		if(frame_aux->libre)
			contador_frames_libres++;
	}
	
	if(cantidad_paginas_a_usar > contador_frames_libres){
		log_error(logger_ram_hq,"No hay espacio suficiente para swapear las paginas necesarias para cargar la nueva patota, la solicitud sera denegada");
		pthread_mutex_unlock(&mutex_tabla_patotas);
		pthread_mutex_destroy(patota->mutex_tabla_paginas);
		free(patota->mutex_tabla_paginas);
		list_destroy(patota->paginas);
		while(patota->tareas->elements_count){
			tarea_ram* auxiliar_tarea = list_get(patota->tareas,0);
			free(auxiliar_tarea->tarea);
			free(auxiliar_tarea);
			list_remove(patota->tareas,0);
		}
		list_destroy(patota->tareas);
		free(patota);
		return RESPUESTA_FAIL;
	}

	//pthread_mutex_lock(&mutex_tabla_patotas);
	list_add(patotas,patota);
	//pthread_mutex_unlock(&mutex_tabla_patotas);

	for(int i=0; i<patota->cantidad_tripulantes; i++){
		uint32_t tid = 0;
		uint32_t pos_x = 0;
		uint32_t pos_y = 0;
		int offset = 8 + patota_con_tareas_y_tripulantes.tamanio_tareas + (i * 21);
		memcpy(&tid,patota_con_tareas_y_tripulantes.stream + offset,4);
		offset += 5;
		memcpy(&pos_x,patota_con_tareas_y_tripulantes.stream + offset, 4);
		offset += 4;
		memcpy(&pos_y,patota_con_tareas_y_tripulantes.stream + offset, 4);
		crear_tripulante_mapa(tid,pos_x,pos_y);
	}

	t_frame_en_memoria* frame;
	int bytes_ya_escritos = 0;
	int bytes_que_faltan = patota_con_tareas_y_tripulantes.tamanio_patota;
	int offset = 0;
	uint32_t id_pagina = 0;
	pthread_mutex_lock(patota->mutex_tabla_paginas);
	for(int i=0; i<cantidad_paginas_a_usar; i++){
		frame = buscar_frame_libre();
		if(!frame){
			if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
				frame = sustituir_LRU();
			}
			if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
				frame = sustituir_CLOCK();
			}
		}
		pthread_mutex_lock(frame->mutex);
		t_pagina* pagina = malloc(sizeof(t_pagina));
		pagina->id_pagina = id_pagina;
		pagina->inicio_memoria = frame->inicio;
		pagina->frame_swap = NULL;
		pagina->mutex_pagina = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(pagina->mutex_pagina,NULL);
		pagina->presente = 1;
		pagina->fue_modificada = 0;
		pagina->uso = 1;
		if(i == cantidad_paginas_a_usar - 1){
			pagina->bytes_usados = bytes_que_faltan;
		}
		else{
			pagina->bytes_usados = mi_ram_hq_configuracion->TAMANIO_PAGINA;
		}
		list_add(patota->paginas,pagina);
		frame->pagina_a_la_que_pertenece = pagina;
		frame->libre = 0;
		// -------------------------------------------- ESTO ES PARA EL DUMP -------------------------------------------------------------
		frame->pid_duenio = pid;
		frame->indice_pagina = id_pagina;
		id_pagina ++;
		// -------------------------------------------------------------------------------------------------------------------------------
		t_pagina_y_frame* pagina_y_frame = malloc(sizeof(t_pagina_y_frame));
		pagina_y_frame->pagina = pagina;
		pagina_y_frame->frame = frame;
		pthread_mutex_lock(&mutex_lru);
		list_add(historial_uso_paginas,pagina_y_frame);
		pthread_mutex_unlock(&mutex_lru);
		//---------------------------------------------------------------------------------------------------------------------------------
		memcpy(frame->inicio,patota_con_tareas_y_tripulantes.stream + offset,minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,bytes_que_faltan));
		bytes_que_faltan -= minimo_entre(mi_ram_hq_configuracion->TAMANIO_PAGINA,(patota_con_tareas_y_tripulantes.tamanio_patota - bytes_ya_escritos));
		bytes_ya_escritos = (patota_con_tareas_y_tripulantes.tamanio_patota - bytes_que_faltan);
		offset = bytes_ya_escritos;
		pthread_mutex_unlock(frame->mutex);
	}

	pthread_mutex_unlock(patota->mutex_tabla_paginas);
	pthread_mutex_unlock(&mutex_tabla_patotas);

	log_info(logger_ram_hq,"Las estructuras administrativas fueron creadas correctamente");

	
	free(patota_con_tareas_y_tripulantes.stream);
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


respuesta_ok_fail actualizar_ubicacion_segmentacion(tripulante_y_posicion tripulante_con_posicion,int socket){
	t_segmentos_de_patota* auxiliar_patota;
	t_segmento* segmento_tripulante_auxiliar;
	pthread_mutex_lock(&mutex_tabla_patotas);
	log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Buscando el tripulante %d",socket,tripulante_con_posicion.tid);
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar_patota = list_get(patotas,i);
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			pthread_mutex_lock(&mutex_tabla_de_segmentos);
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			uint32_t tid_aux = 0;
			pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar -> inicio_segmento,sizeof(uint32_t)); 
			if(tid_aux == tripulante_con_posicion.tid){
				
				//obtengo posicion actual
				uint32_t pos_x = 0;
				uint32_t pos_y = 0;
				uint32_t offset_x = sizeof(uint32_t) + sizeof(char);
				uint32_t offset_y = offset_x + sizeof(uint32_t);
				
				pthread_mutex_lock(&mutex_memoria);	
				memcpy(&pos_x,segmento_tripulante_auxiliar -> inicio_segmento + offset_x,sizeof(uint32_t)); 
				memcpy(&pos_y,segmento_tripulante_auxiliar -> inicio_segmento + offset_y,sizeof(uint32_t)); 
				pthread_mutex_unlock(&mutex_memoria);	
				
				direccion direc = obtener_direccion_movimiento_mapa(tripulante_con_posicion.pos_x,tripulante_con_posicion.pos_y,pos_x,pos_y);	
				mover_tripulante_mapa(obtener_caracter_mapa(tripulante_con_posicion.tid),direc);
				
				//obtengo pid para informar
				uint32_t pid;
				pthread_mutex_lock(&mutex_memoria);	
				pthread_mutex_lock(auxiliar_patota ->segmento_pcb -> mutex_segmento);	
				memcpy(&pid,auxiliar_patota ->segmento_pcb-> inicio_segmento,sizeof(uint32_t)); 
				pthread_mutex_unlock(auxiliar_patota ->segmento_pcb -> mutex_segmento);	
				pthread_mutex_unlock(&mutex_memoria);	

				log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: Encontre al tripulante %d en el segmento #%i en memoria perteneciente a la patota #%i, posicion actual %d %d",socket,tripulante_con_posicion.tid,segmento_tripulante_auxiliar -> numero_segmento,pid,tripulante_con_posicion.pos_x,tripulante_con_posicion.pos_y);
				
				pthread_mutex_lock(&mutex_memoria);	
				memcpy(segmento_tripulante_auxiliar -> inicio_segmento + offset_x,&tripulante_con_posicion.pos_x,sizeof(uint32_t));
				memcpy(segmento_tripulante_auxiliar -> inicio_segmento + offset_y,&tripulante_con_posicion.pos_y,sizeof(uint32_t));
				pthread_mutex_unlock(&mutex_memoria);	
				pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);	
				
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_tabla_patotas);
				return RESPUESTA_OK;
			}
			pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);	
		}
		pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
	}
	log_error(logger_ram_hq,"Socket %i, ACTUALIZAR_UBICACION: No se encontro el tripulante %d en memoria",socket,tripulante_con_posicion.tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);	

	return RESPUESTA_FAIL;
}


respuesta_ok_fail actualizar_ubicacion_paginacion(tripulante_y_posicion tripulante_con_posicion){
	pthread_mutex_lock(&mutex_tabla_patotas);
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tripulante_con_posicion.tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);
	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tripulante_con_posicion.tid);
		return RESPUESTA_FAIL;
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tripulante_con_posicion.tid);
	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i=0; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	tamanio_tareas -= 1;
	primer_indice += tamanio_tareas;
	double indice_auxiliar = primer_indice;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_auxiliar); 
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	inicio_tcb* tcb = buscar_inicio_tcb(tripulante_con_posicion.tid,patota,indice_auxiliar,offset_entero);
	double nuevo_indice = 0;
	int i = 0;
	while(!tcb->pagina && i<patota->cantidad_tripulantes){
		primer_indice += 5*sizeof(uint32_t) + sizeof(char);
		offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA,&nuevo_indice);
		offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
		free(tcb);
		tcb = buscar_inicio_tcb(tripulante_con_posicion.tid,patota,nuevo_indice,offset_entero);
		i++;
	}
	
	double indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (i * (5*sizeof(uint32_t) + sizeof(char))) + sizeof(uint32_t) + sizeof(char);
	double offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	int offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	pthread_mutex_lock(&mutex_no_reemplazar);

	traerme_todo_el_tcb_a_memoria(tcb,patota);

	uint32_t pos_x = leer_un_uint32_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,patota);
	escribir_un_uint32_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,tripulante_con_posicion.pos_x,patota);

	indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (i * (5*sizeof(uint32_t) + sizeof(char))) + sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t);
	offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;

	uint32_t pos_y = leer_un_uint32_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,patota);
	escribir_un_uint32_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,tripulante_con_posicion.pos_y,patota);
	pthread_mutex_unlock(&mutex_no_reemplazar);

	direccion direc = obtener_direccion_movimiento_mapa(tripulante_con_posicion.pos_x,tripulante_con_posicion.pos_y,pos_x,pos_y);
	mover_tripulante_mapa(obtener_caracter_mapa(tripulante_con_posicion.tid),direc);
	
	free(tcb);
	return RESPUESTA_OK;
}

uint32_t leer_un_uint32_a_partir_de_indice(double indice, int offset, t_tabla_de_paginas* patota){
	uint32_t dato;
	t_pagina* auxiliar_pagina = list_get(patota->paginas,indice);
	int tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset;
	int bytes_escritos = 0;
	int bytes_desplazados_escritura = 0;
	int offset_lectura = 0;
	while(bytes_escritos < 4){
		if(tamanio_disponible_pagina < 4){
			memcpy(&dato + offset_lectura,auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura,tamanio_disponible_pagina);
			bytes_escritos += tamanio_disponible_pagina;
			bytes_desplazados_escritura += tamanio_disponible_pagina;
			offset_lectura += tamanio_disponible_pagina;
			tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA;
			auxiliar_pagina = list_get(patota->paginas, indice + 1);
			offset = 0;
		}
		else{
			memcpy(&dato + offset_lectura, auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura, 4 - bytes_escritos);
			bytes_escritos = 4;
		}
	}

	return dato;
}


//PENDIENTE EL -1 QUE COMENTO ABAJO
t_tabla_de_paginas* buscar_patota_con_tid_paginacion(uint32_t tid){
	pthread_mutex_lock(&mutex_busqueda_patota);

	t_tabla_de_paginas* auxiliar;
	tarea_ram* tarea_aux;
	t_pagina* pagina_aux;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		double indice_tripulantes = 2 * sizeof(uint32_t);
		uint32_t tid_aux = 0;
		for(int j=0; j<auxiliar->tareas->elements_count; j++){
			tarea_aux = list_get(auxiliar->tareas,j);
			indice_tripulantes += tarea_aux->tamanio;
		}
		indice_tripulantes -= 1; //No se porque se suma uno de mas a cuando lo cargo en INICIAR_PATOTA, tengo que trackear esto


		for(int k=0; k<auxiliar->cantidad_tripulantes; k++){

			int bytes_leidos = 0;
			double indice_a_primer_posicion = (indice_tripulantes + k * 21) / mi_ram_hq_configuracion->TAMANIO_PAGINA; //21 es lo que ocupa un TCB
			double offset_pagina = modf(indice_a_primer_posicion,&indice_a_primer_posicion);
			int offset_pagina_entero = offset_pagina * mi_ram_hq_configuracion->TAMANIO_PAGINA;

			pagina_aux = list_get(auxiliar->paginas,indice_a_primer_posicion);

			pthread_mutex_lock(pagina_aux->mutex_pagina);
			int bytes_a_leer = sizeof(uint32_t);
			int bytes_escritos = 0;
			int espacio_disponible_en_pagina = (mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_pagina_entero);
			int espacio_leido = 0;
			while(bytes_leidos < 4){
				if(bytes_a_leer > espacio_disponible_en_pagina && pagina_aux->presente){
					memcpy(&tid_aux + espacio_leido,pagina_aux->inicio_memoria + offset_pagina_entero,espacio_disponible_en_pagina);
					espacio_leido += espacio_disponible_en_pagina;
					bytes_escritos = espacio_disponible_en_pagina;
					bytes_leidos = bytes_leidos + espacio_disponible_en_pagina;
					espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
					pthread_mutex_unlock(pagina_aux->mutex_pagina);
					pagina_aux = list_get(auxiliar->paginas,indice_a_primer_posicion+1);
					pthread_mutex_lock(pagina_aux->mutex_pagina);
					offset_pagina_entero = 0;
				}
				else if (bytes_a_leer <= espacio_disponible_en_pagina && pagina_aux->presente){
					memcpy(&tid_aux + espacio_leido,pagina_aux->inicio_memoria + offset_pagina_entero,sizeof(uint32_t) - bytes_escritos);
					bytes_leidos = bytes_leidos + sizeof(uint32_t) - bytes_escritos;
					pthread_mutex_unlock(pagina_aux->mutex_pagina);
					if(tid_aux == tid){
						pthread_mutex_unlock(&mutex_busqueda_patota);
						return auxiliar;
					}
				}
				else if(bytes_a_leer > espacio_disponible_en_pagina && !(pagina_aux->presente)){
					memcpy(&tid_aux + espacio_leido,pagina_aux->frame_swap->inicio + offset_pagina_entero,espacio_disponible_en_pagina);
					bytes_leidos = bytes_leidos + espacio_disponible_en_pagina;
					espacio_leido += espacio_disponible_en_pagina;
					bytes_escritos = espacio_disponible_en_pagina;
					espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
					pthread_mutex_unlock(pagina_aux->mutex_pagina);
					pagina_aux = list_get(auxiliar->paginas,indice_a_primer_posicion+1);
					pthread_mutex_lock(pagina_aux->mutex_pagina);
					offset_pagina = 0;
				}
				else if(bytes_a_leer <= espacio_disponible_en_pagina && !(pagina_aux->presente)){
					memcpy(&tid_aux + espacio_leido,pagina_aux->frame_swap->inicio + offset_pagina_entero,sizeof(uint32_t) - bytes_escritos);
					bytes_leidos = bytes_leidos + sizeof(uint32_t) - bytes_escritos;
					pthread_mutex_unlock(pagina_aux->mutex_pagina);
					if(tid_aux == tid){
						pthread_mutex_unlock(&mutex_busqueda_patota);
						return auxiliar;
					}
				}
				//pthread_mutex_unlock(pagina_aux->mutex_pagina);
			}

		}
		
	}
	pthread_mutex_unlock(&mutex_busqueda_patota);
	return NULL;
}

inicio_tcb* buscar_inicio_tcb(uint32_t tid,t_tabla_de_paginas* patota,double indice, int offset_pagina){
	pthread_mutex_lock(patota->mutex_tabla_paginas);
	inicio_tcb* retornar = malloc(sizeof(inicio_tcb));
	t_pagina* auxiliar = list_get(patota->paginas,indice);
	t_pagina* pagina_retorno = list_get(patota->paginas,indice);
	uint32_t tid_aux = 0;
	uint32_t tid_aux_posta = 0;
	int espacio_leido = 0;	
	int bytes_a_leer = sizeof(uint32_t);
	int offset_original = offset_pagina;
	while(espacio_leido < 4){
		//auxiliar = list_get(patota->paginas,indice);
		//pagina_retorno = list_get(patota->paginas,indice);
		pthread_mutex_lock(auxiliar->mutex_pagina);
		int espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_pagina;
		if(bytes_a_leer > espacio_disponible_en_pagina && auxiliar->presente){
			memcpy(&tid_aux + espacio_leido, auxiliar->inicio_memoria + offset_pagina, espacio_disponible_en_pagina);
			espacio_leido += espacio_disponible_en_pagina;
			espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
			offset_pagina = 0;
			pthread_mutex_unlock(auxiliar->mutex_pagina);
			//pagina_retorno = list_get(patota->paginas,indice);
			auxiliar = list_get(patota->paginas, indice + 1);
		}
		else if(bytes_a_leer <= espacio_disponible_en_pagina && auxiliar->presente) {
			memcpy(&tid_aux + espacio_leido,auxiliar->inicio_memoria + offset_pagina,sizeof(uint32_t)-espacio_leido);
			memcpy(&tid_aux_posta,&tid_aux,4);
			espacio_leido += 4;
			if(tid_aux_posta == tid){
				pthread_mutex_unlock(auxiliar->mutex_pagina);
				pthread_mutex_unlock(patota->mutex_tabla_paginas);
				retornar->indice = indice;
				retornar->offset = offset_original;
				retornar->pagina = pagina_retorno;
				return retornar;
			}
		}
		else if(bytes_a_leer > espacio_disponible_en_pagina && !(auxiliar->presente)){
			memcpy(&tid_aux + espacio_leido, auxiliar->frame_swap->inicio + offset_pagina, espacio_disponible_en_pagina);
			espacio_leido += espacio_disponible_en_pagina;
			espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
			offset_pagina = 0;
			pthread_mutex_unlock(auxiliar->mutex_pagina);
			//pagina_retorno = list_get(patota->paginas,indice);
			auxiliar = list_get(patota->paginas, indice + 1);
		}
		else if(bytes_a_leer <= espacio_disponible_en_pagina && !(auxiliar->presente)){
			memcpy(&tid_aux + espacio_leido,auxiliar->frame_swap->inicio + offset_pagina,sizeof(uint32_t)-espacio_leido);
			memcpy(&tid_aux_posta,&tid_aux,4);
			espacio_leido += 4;
			if(tid_aux_posta == tid){
				pthread_mutex_unlock(auxiliar->mutex_pagina);
				pthread_mutex_unlock(patota->mutex_tabla_paginas);
				retornar->indice = indice;
				retornar->offset = offset_original;
				retornar->pagina = pagina_retorno;
				return retornar;
			}
		}
	}
	pthread_mutex_unlock(auxiliar->mutex_pagina);
	pthread_mutex_unlock(patota->mutex_tabla_paginas);
	retornar->indice = 0;
	retornar->offset = 0;
	retornar->pagina = NULL;
	return retornar;
}

void escribir_un_uint32_a_partir_de_indice(double indice, int offset, uint32_t dato, t_tabla_de_paginas* patota){

	t_pagina* auxiliar_pagina = list_get(patota->paginas,indice);
	int tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset;
	int bytes_escritos = 0;
	int offset_lectura = 0;

	t_frame_en_memoria* frame;

	while(bytes_escritos < 4){
		pthread_mutex_lock(auxiliar_pagina->mutex_pagina);
		if(!auxiliar_pagina->presente){
			frame = buscar_frame_libre();
			if(!frame){
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
					frame = sustituir_LRU();
				}
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
					frame = sustituir_CLOCK();
				}
			}
			pthread_mutex_lock(frame->mutex);
			auxiliar_pagina->presente = 1;
			auxiliar_pagina->inicio_memoria = frame->inicio;
			frame->pagina_a_la_que_pertenece = auxiliar_pagina;
			frame->indice_pagina = auxiliar_pagina->id_pagina;
			frame->pid_duenio = patota->id_patota;
			frame->libre = 0;
			pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
			memcpy(frame->inicio,auxiliar_pagina->frame_swap->inicio,mi_ram_hq_configuracion->TAMANIO_PAGINA);
			auxiliar_pagina->frame_swap->libre = 1;
			pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
			t_pagina_y_frame* frame_y_pagina = malloc(sizeof(t_pagina_y_frame));
			frame_y_pagina->frame = frame;
			frame_y_pagina->pagina = auxiliar_pagina;
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		else if(auxiliar_pagina->presente){
			int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar_pagina->id_pagina,patota->id_patota);
			pthread_mutex_lock(&mutex_lru);
			t_pagina_y_frame* frame_y_pagina = list_remove(historial_uso_paginas,indice);
			pthread_mutex_unlock(&mutex_lru);
			frame = frame_y_pagina->frame;
			pthread_mutex_lock(frame->mutex);
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		
		}
		if(tamanio_disponible_pagina < 4){
			memcpy(auxiliar_pagina->inicio_memoria + offset,&dato + offset_lectura,tamanio_disponible_pagina);
			bytes_escritos += tamanio_disponible_pagina;
			offset_lectura += tamanio_disponible_pagina;
			tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA;
			auxiliar_pagina->fue_modificada = 1;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			auxiliar_pagina = list_get(patota->paginas,indice+1);
			offset = 0;
		}
		else{
			memcpy(auxiliar_pagina->inicio_memoria + offset ,&dato + offset_lectura, 4 - bytes_escritos);
			auxiliar_pagina->fue_modificada = 1;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			int escritura = 4 - bytes_escritos;
			bytes_escritos += escritura;
		}
		pthread_mutex_unlock(frame->mutex);
	}

}

t_frame_en_memoria* buscar_frame_libre(){
	pthread_mutex_lock(&mutex_memoria);
	t_frame_en_memoria* auxiliar;
	pthread_mutex_lock(&mutex_frames);
	
	for(int i=0; i<frames->elements_count; i++){
		auxiliar = list_get(frames,i);
		pthread_mutex_lock(auxiliar->mutex);
		if(auxiliar->libre){
			pthread_mutex_unlock(auxiliar->mutex);
			pthread_mutex_unlock(&mutex_frames);
			pthread_mutex_unlock(&mutex_memoria);
			return auxiliar;
		}
	  pthread_mutex_unlock(auxiliar->mutex);
	}
	pthread_mutex_unlock(&mutex_frames);
	pthread_mutex_unlock(&mutex_memoria);
	return NULL;
}

char * obtener_proxima_tarea_paginacion(uint32_t tripulante_tid)
{	
	//pthread_mutex_lock(&mutex_dummy);
	pthread_mutex_lock(&mutex_tabla_patotas);
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tripulante_tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);
	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tripulante_tid);
		//pthread_mutex_unlock(&mutex_dummy);
		return "RESPUESTA_FAIL";
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tripulante_tid);

	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i=0; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	tamanio_tareas -= 1;
	primer_indice += tamanio_tareas;
	double indice_auxiliar = 0;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_auxiliar);
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	inicio_tcb* tcb = buscar_inicio_tcb(tripulante_tid,patota,indice_auxiliar,offset_entero);
	double nuevo_indice = 0;
	int i = 0;	
	while(!tcb->pagina && i<patota->cantidad_tripulantes){
		primer_indice += 5*sizeof(uint32_t) + sizeof(char);
		offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA,&nuevo_indice);
		offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
		free(tcb);
		tcb = buscar_inicio_tcb(tripulante_tid,patota,nuevo_indice,offset_entero);
		i++;
	}

	double indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (i * (5*sizeof(uint32_t) + sizeof(char))) + 3 * sizeof(uint32_t) + sizeof(char);
	double offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	int offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;


	int espacio_leido = 0;
	uint32_t indice_proxima_tarea = 0;
	t_frame_en_memoria* frame;
	
	pthread_mutex_lock(&mutex_no_reemplazar);
	traerme_todo_el_tcb_a_memoria(tcb,patota);
	free(tcb);

	int offset_pagina_entero = offset_posicion_entero;
	int bytes_escritos = 0;
	int k = indice_de_posicion;
	int bytes_a_leer = sizeof(uint32_t); 
	int espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_posicion_entero;
	while(espacio_leido < 4){
		t_pagina* auxiliar = list_get(patota->paginas,k);
		pthread_mutex_lock(auxiliar->mutex_pagina);
		if(!auxiliar->presente){
			frame = buscar_frame_libre();
			if(!frame){	
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
					frame = sustituir_LRU();
				}
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
					frame = sustituir_CLOCK();
				}
			}
			pthread_mutex_lock(frame->mutex);
			auxiliar->presente = 1;
			auxiliar->inicio_memoria = frame->inicio;
			auxiliar->uso = 1;
			frame->pagina_a_la_que_pertenece = auxiliar;
			frame->indice_pagina = auxiliar->id_pagina;
			frame->pid_duenio = patota->id_patota;
			frame->libre = 0;
			pthread_mutex_lock(auxiliar->frame_swap->mutex);
			memcpy(frame->inicio,auxiliar->frame_swap->inicio,mi_ram_hq_configuracion->TAMANIO_PAGINA);
			auxiliar->frame_swap->libre = 1;
			pthread_mutex_unlock(auxiliar->frame_swap->mutex);
			t_pagina_y_frame* frame_y_pagina = malloc(sizeof(t_pagina_y_frame));
			frame_y_pagina->frame = frame;
			frame_y_pagina->pagina = auxiliar;
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		else if(auxiliar->presente){
			int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar->id_pagina,patota->id_patota);
			pthread_mutex_lock(&mutex_lru);
			t_pagina_y_frame* frame_y_pagina = list_remove(historial_uso_paginas,indice);
			pthread_mutex_unlock(&mutex_lru);	
			frame = frame_y_pagina->frame;
			pthread_mutex_lock(frame->mutex);
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		if(bytes_a_leer > espacio_disponible_en_pagina){
			memcpy(&indice_proxima_tarea + espacio_leido,auxiliar->inicio_memoria + offset_pagina_entero,espacio_disponible_en_pagina);
			espacio_leido += espacio_disponible_en_pagina;
			bytes_a_leer -= espacio_disponible_en_pagina;
			bytes_escritos = espacio_disponible_en_pagina;
			espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
			offset_pagina_entero = 0;
			auxiliar->uso = 1;
			k++;
		}
		else{
			memcpy(&indice_proxima_tarea + espacio_leido,auxiliar->inicio_memoria + offset_pagina_entero,sizeof(uint32_t) - bytes_escritos);
			espacio_leido += sizeof(uint32_t) - bytes_escritos;
			auxiliar->uso = 1;
			//offset_pagina_entero = 0;
		}
		pthread_mutex_unlock(auxiliar->mutex_pagina);
		pthread_mutex_unlock(frame->mutex);
 	}	
	escribir_un_uint32_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,indice_proxima_tarea + 1,patota);

	char* proxima_tarea;

	if(indice_proxima_tarea + 1 > patota->tareas->elements_count){
		log_info(logger_ram_hq,"El tripulante de tid: %i ya cumplio con todas sus tareas, no se actualizara la direccion logica",tripulante_tid);
		proxima_tarea = malloc(4);
		strcpy(proxima_tarea,"");
		expulsar_tripulante_paginacion(tripulante_tid);
		pthread_mutex_unlock(&mutex_no_reemplazar);
		//pthread_mutex_unlock(&mutex_dummy);
		return proxima_tarea;
	}


//---------------------------------LEE LA TAREA DESDE MEMORIA A PARTIR DEL INDICE Y TRAE A MEMORIA TODAS LAS PAGINAS QUE LA CONTENGAN--------------------
	
	tarea_ram* tarea_contador;
	tarea_ram* tarea_especifica = list_get(patota->tareas,indice_proxima_tarea);
	proxima_tarea = malloc(tarea_especifica->tamanio);
	strcpy(proxima_tarea,"");
	t_pagina* auxiliar_pagina;
	double tamanio_hasta_tarea = 2*(sizeof(uint32_t));
	for(int i=0; i<indice_proxima_tarea; i++){
		tarea_contador = list_get(patota->tareas,i);
		tamanio_hasta_tarea += tarea_contador->tamanio;
	}

	double indice_de_tarea = tamanio_hasta_tarea / mi_ram_hq_configuracion->TAMANIO_PAGINA;
	double offset_hasta_tarea = modf(indice_de_tarea,&indice_de_tarea);
	int offset_hasta_tarea_entero = offset_hasta_tarea * mi_ram_hq_configuracion->TAMANIO_PAGINA;

	int bytes_de_tarea = tarea_especifica->tamanio;
	int bytes_leidos_tarea = 0;
	int indice_auxiliar_tarea = indice_de_tarea;
	espacio_disponible_en_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_hasta_tarea_entero;
	offset_pagina_entero = offset_hasta_tarea_entero;

	while(bytes_leidos_tarea < bytes_de_tarea){
		auxiliar_pagina = list_get(patota->paginas,indice_auxiliar_tarea);
		pthread_mutex_lock(auxiliar_pagina->mutex_pagina);
		if(!auxiliar_pagina->presente){
			frame = buscar_frame_libre();
			if(!frame){
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
					frame = sustituir_LRU();
				}
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
					frame = sustituir_CLOCK();
				}
			}
			pthread_mutex_lock(frame->mutex);
			auxiliar_pagina->presente = 1;
			auxiliar_pagina->inicio_memoria = frame->inicio;
			auxiliar_pagina->uso = 1;
			frame->pagina_a_la_que_pertenece = auxiliar_pagina;
			frame->indice_pagina = auxiliar_pagina->id_pagina;
			frame->pid_duenio = patota->id_patota;
			frame->libre = 0;
			pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
			memcpy(frame->inicio,auxiliar_pagina->frame_swap->inicio,mi_ram_hq_configuracion->TAMANIO_PAGINA);
			auxiliar_pagina->frame_swap->libre = 1;
			pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
			t_pagina_y_frame* frame_y_pagina = malloc(sizeof(t_pagina_y_frame));
			frame_y_pagina->frame = frame;
			frame_y_pagina->pagina = auxiliar_pagina;
			
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		else if(auxiliar_pagina->presente){
			int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar_pagina->id_pagina,patota->id_patota);

			pthread_mutex_lock(&mutex_lru);
			t_pagina_y_frame* frame_y_pagina = list_remove(historial_uso_paginas,indice);
			pthread_mutex_unlock(&mutex_lru);
			
			frame = frame_y_pagina->frame;
			pthread_mutex_lock(frame->mutex);

			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		if((bytes_de_tarea - bytes_leidos_tarea) > espacio_disponible_en_pagina){
			memcpy(proxima_tarea + bytes_leidos_tarea,auxiliar_pagina->inicio_memoria + offset_pagina_entero,espacio_disponible_en_pagina);
			bytes_leidos_tarea += espacio_disponible_en_pagina;
			espacio_disponible_en_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;
			offset_pagina_entero = 0;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			indice_auxiliar_tarea++;
		}
		else{
			memcpy(proxima_tarea + bytes_leidos_tarea,auxiliar_pagina->inicio_memoria + offset_pagina_entero,bytes_de_tarea - bytes_leidos_tarea);
			bytes_leidos_tarea = bytes_de_tarea;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
		}

	 pthread_mutex_unlock(frame->mutex);
	} 
	
	proxima_tarea[tarea_especifica->tamanio - 1] = '\0';
	pthread_mutex_unlock(&mutex_no_reemplazar);
	//pthread_mutex_unlock(&mutex_dummy);
	return proxima_tarea;
}

char* obtener_proxima_tarea_segmentacion(uint32_t tripulante_tid, int socket)
{
	t_segmento* tripulante_aux;
	uint32_t tid_aux = 0;
	t_segmentos_de_patota* auxiliar_patota = buscar_patota_con_tid(tripulante_tid,socket);
	if(!auxiliar_patota){
		log_error(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: El tripulante %d no esta dado de alta en ninguna patota", socket,tripulante_tid);
		char * ret = malloc (15);
		strcpy(ret,"RESPUESTA_FAIL");
		return ret;
	}
	//obtengo las tareas de la patota
	char* tareas = malloc(auxiliar_patota->segmento_tarea->tamanio_segmento);
	strcpy(tareas,"");

	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(auxiliar_patota->segmento_tarea->mutex_segmento);
	memcpy(tareas,auxiliar_patota->segmento_tarea->inicio_segmento,auxiliar_patota->segmento_tarea->tamanio_segmento); //Me traigo todo el contenido del segmento de tareas
	pthread_mutex_unlock(auxiliar_patota->segmento_tarea->mutex_segmento);
	pthread_mutex_unlock(&mutex_memoria);
	
	//obtengo el numero de tarea que le corresponde al tripulante
	uint32_t id_tarea = -1;
	
	pthread_mutex_lock(auxiliar_patota->mutex_segmentos_tripulantes);
	for(int i=0; i<auxiliar_patota->segmentos_tripulantes->elements_count; i++){
		tripulante_aux = list_get(auxiliar_patota->segmentos_tripulantes,i);

		pthread_mutex_lock(&mutex_memoria);
		pthread_mutex_lock(tripulante_aux->mutex_segmento);
		memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
		pthread_mutex_unlock(tripulante_aux->mutex_segmento);
		pthread_mutex_unlock(&mutex_memoria);
	

		if(tid_aux == tripulante_tid){
			
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&id_tarea,tripulante_aux->inicio_segmento + 3*(sizeof(uint32_t)) + 1, sizeof(uint32_t));			
			pthread_mutex_unlock(tripulante_aux->mutex_segmento);
			pthread_mutex_unlock(&mutex_memoria);

			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Tripulante %i tarea actual #%i",socket,tid_aux,id_tarea);
			pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);

			//este porque uso el tamaño del segmento
			pthread_mutex_lock(auxiliar_patota->segmento_tarea->mutex_segmento);		
			strcpy(tareas,obtener_proxima_tarea(tareas,id_tarea,auxiliar_patota->segmento_tarea->tamanio_segmento));
			pthread_mutex_unlock(auxiliar_patota->segmento_tarea->mutex_segmento);		

			if(!strlen(tareas)){
				//Borro tripulante de la lista de tripulantes de la patota
				tripulante_aux->libre = true;
				list_remove(auxiliar_patota->segmentos_tripulantes,i);

				pthread_mutex_lock(&mutex_mover_trip);
				item_borrar(nivel,obtener_caracter_mapa(tid_aux));
				nivel_gui_dibujar(nivel);
				pthread_mutex_unlock(&mutex_mover_trip);
					

				if(! auxiliar_patota->segmentos_tripulantes->elements_count){
					// si la patota esta vacia la elimino
					uint32_t pid  = 0;
					pthread_mutex_lock(&mutex_memoria);
					pthread_mutex_lock(auxiliar_patota->segmento_pcb->mutex_segmento);
					memcpy(&pid,auxiliar_patota->segmento_pcb->inicio_segmento,sizeof(uint32_t));
					pthread_mutex_unlock(auxiliar_patota->segmento_pcb->mutex_segmento);
					pthread_mutex_unlock(&mutex_memoria);

					borrar_patota(pid);
					list_destroy(auxiliar_patota->segmentos_tripulantes);
					pthread_mutex_destroy(auxiliar_patota->mutex_segmentos_tripulantes);
					free(auxiliar_patota->mutex_segmentos_tripulantes);
					auxiliar_patota->segmento_pcb->libre = true;
					auxiliar_patota->segmento_tarea->libre = true;
					free(auxiliar_patota);

					return tareas;
				};

			}
			break;	
		}
	}

	pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);

	if(id_tarea == -1){
		log_error(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: No encontre id tarea, %i",socket, id_tarea);
	}
	
	actualizarTareaActual(auxiliar_patota,tripulante_tid,socket);

	return tareas;
}

respuesta_ok_fail expulsar_tripulante_paginacion(uint32_t tripulante_tid)
{
	pthread_mutex_lock(&mutex_tabla_patotas);
	pthread_mutex_lock(&mutex_mover_trip);
	item_borrar(nivel,obtener_caracter_mapa(tripulante_tid));
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mover_trip);
	
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tripulante_tid);

	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tripulante_tid);
		return RESPUESTA_FAIL;
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tripulante_tid);

	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i=0; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	tamanio_tareas -= 1;
	primer_indice += tamanio_tareas;
	double indice_auxiliar = 0;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_auxiliar);
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	inicio_tcb* tcb = buscar_inicio_tcb(tripulante_tid,patota,indice_auxiliar,offset_entero);
	double nuevo_indice = 0;
	int i = 0;
	while(!tcb->pagina && i<patota->cantidad_tripulantes){
		primer_indice += 5*sizeof(uint32_t) + sizeof(char);
		offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA,&nuevo_indice);
		offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
		free(tcb);
		tcb = buscar_inicio_tcb(tripulante_tid,patota,nuevo_indice,offset_entero);
		i++;
	}
	t_pagina* auxiliar_pagina; 
	int indice_pagina = tcb->indice;
	int offset_tcb = tcb->offset;
	int total_tripulante_leido = 21;
	while(total_tripulante_leido > 0){
		auxiliar_pagina = list_get(patota->paginas,indice_pagina);
		if(total_tripulante_leido + offset_tcb > mi_ram_hq_configuracion->TAMANIO_PAGINA){
			int bytes_a_liberar = (mi_ram_hq_configuracion->TAMANIO_PAGINA - offset_tcb);
			auxiliar_pagina->bytes_usados -= bytes_a_liberar;
			log_info(logger_ram_hq,"La pagina %i del proceso %i libera %i bytes, le quedan %i bytes",auxiliar_pagina->id_pagina,patota->id_patota,bytes_a_liberar,auxiliar_pagina->bytes_usados);
			if(auxiliar_pagina->bytes_usados == 0 && auxiliar_pagina->presente){
				int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar_pagina->id_pagina,patota->id_patota);
				pthread_mutex_lock(&mutex_lru);
				t_pagina_y_frame* auxiliar_frame = list_remove(historial_uso_paginas,indice);
				pthread_mutex_unlock(&mutex_lru);
				t_frame_en_memoria* frame = auxiliar_frame->frame;
				frame->libre = 1;
				auxiliar_pagina->presente = 0;
				free(auxiliar_frame);
				log_info(logger_ram_hq,"La pagina %i del proceso %i fue achurada y su frame fue liberado\n",auxiliar_pagina->id_pagina,patota->id_patota);
			}
			else if(auxiliar_pagina->bytes_usados == 0 && !auxiliar_pagina->presente){
				pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
				auxiliar_pagina->frame_swap->libre = 1;
				pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
				log_info(logger_ram_hq,"La pagina %i del proceso %i fue achurada pero estaba en SWAP, se libera el espacio que ocupaba\n",auxiliar_pagina->id_pagina,patota->id_patota);
			}
			total_tripulante_leido -= bytes_a_liberar;
			offset_tcb = 0;
			indice_pagina ++;
		}
		else{
			auxiliar_pagina->bytes_usados -= total_tripulante_leido;
			log_info(logger_ram_hq,"La pagina %i del proceso %i libera %i bytes, le quedan %i bytes",auxiliar_pagina->id_pagina,patota->id_patota,total_tripulante_leido,auxiliar_pagina->bytes_usados);
			if(auxiliar_pagina->bytes_usados == 0 && auxiliar_pagina->presente){
				int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar_pagina->id_pagina,patota->id_patota);
				pthread_mutex_lock(&mutex_lru);
				t_pagina_y_frame* auxiliar_frame = list_remove(historial_uso_paginas,indice);
				pthread_mutex_unlock(&mutex_lru);
				t_frame_en_memoria* frame = auxiliar_frame->frame;
				frame->libre = 1;
				auxiliar_pagina->presente = 0;
				free(auxiliar_frame);
				log_info(logger_ram_hq,"La pagina %i del proceso %i fue achurado y su frame fue liberado\n",auxiliar_pagina->id_pagina,patota->id_patota);
			}
			else if(auxiliar_pagina->bytes_usados == 0 && !auxiliar_pagina->presente){
				pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
				auxiliar_pagina->frame_swap->libre = 1;
				pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
				log_info(logger_ram_hq,"La pagina %i del proceso %i fue achurada pero estaba en SWAP, se libera el espacio que ocupaba\n",auxiliar_pagina->id_pagina,patota->id_patota);
			}
			total_tripulante_leido = 0;
			offset_tcb = 0;
			indice_pagina ++;
		}	
	}
	free(tcb);
	pthread_mutex_lock(patota->mutex_tripulantes_vivos);
	patota->contador_tripulantes_vivos --;
	pthread_mutex_unlock(patota->mutex_tripulantes_vivos);
	if(patota->contador_tripulantes_vivos == 0){
		log_info(logger_ram_hq,"Todos los tripulantes de la patota %i estan muertos, se procede a eliminar la patota y liberar los frames ocupados",patota->id_patota);
		while(patota->paginas->elements_count){
			t_pagina* pagina_aux = list_get(patota->paginas,0);
			if(pagina_aux->presente){ 	
				int indice = buscar_frame_y_pagina_con_tid_pid(pagina_aux->id_pagina,patota->id_patota);
				pthread_mutex_lock(&mutex_lru);
				t_pagina_y_frame* auxiliar_frame = list_remove(historial_uso_paginas,indice);
				pthread_mutex_unlock(&mutex_lru);
				t_frame_en_memoria* frame = auxiliar_frame->frame;
				pthread_mutex_lock(frame->mutex);
				frame->libre = 1;
				pthread_mutex_unlock(frame->mutex);
				log_info(logger_ram_hq,"Liberando el frame correspondiente a la pagina %i del proceso %i",pagina_aux->id_pagina,patota->id_patota);
				pagina_aux->presente = 0;
				free(auxiliar_frame);
			}
			pthread_mutex_destroy(pagina_aux->mutex_pagina);
			free(pagina_aux->mutex_pagina);
			free(pagina_aux);
			list_remove(patota->paginas,0);
		}
		list_destroy(patota->paginas);
		while(patota->tareas->elements_count){
			tarea_ram* auxiliar_tarea = list_get(patota->tareas,0);
			free(auxiliar_tarea->tarea);
			free(auxiliar_tarea);
			list_remove(patota->tareas,0);
		}
		list_destroy(patota->tareas);
		pthread_mutex_destroy(patota->mutex_tabla_paginas);
		free(patota->mutex_tabla_paginas);
		pthread_mutex_destroy(patota->mutex_tripulantes_vivos);
		free(patota->mutex_tripulantes_vivos);
		int indice_patota = obtener_indice_patota(patota->id_patota);
		list_remove(patotas,indice_patota);
		free(patota);
		
	}
	pthread_mutex_unlock(&mutex_tabla_patotas);
	return RESPUESTA_OK;
}

int obtener_indice_patota(uint32_t id_patota){
	t_tabla_de_paginas* iterador;
	for(int i=0; i<patotas->elements_count; i++){
		iterador = list_get(patotas,i);
		if(iterador->id_patota == id_patota){
			return i;
		}
	}
	return -1;
}

respuesta_ok_fail expulsar_tripulante_segmentacion(uint32_t tid,int socket)
{
	t_segmentos_de_patota* patota_aux;
	t_segmento* tripulante_aux;
	uint32_t tid_aux = 0;
	log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Buscando el tripulante %d",socket,tid);
	pthread_mutex_lock(&mutex_tabla_patotas);
	for(int i=0 ; i<patotas->elements_count; i++){
		//itero cada patota
		patota_aux = list_get(patotas,i);
		//bloqueo segmentos de tripulantes de esa patota
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			//recorro todos los segmentos de tripulantes de la patota
			pthread_mutex_lock(&mutex_tabla_de_segmentos);
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			
			//bloqueo al tripulante
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			//obtengo su tid
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));

			pthread_mutex_unlock(tripulante_aux->mutex_segmento);
			pthread_mutex_unlock(&mutex_memoria);

			if(tid_aux == tid){

				//encontre al tripulante, lo borro de memoria
				//bloque memoria
				pthread_mutex_lock(&mutex_memoria);
				pthread_mutex_lock(&mutex_tabla_de_segmentos);
				
				pthread_mutex_lock(tripulante_aux->mutex_segmento);
				tripulante_aux->libre = true;
				pthread_mutex_unlock(tripulante_aux->mutex_segmento);

				pthread_mutex_lock(&mutex_mover_trip);
				item_borrar(nivel,obtener_caracter_mapa(tid));
				nivel_gui_dibujar(nivel);
				pthread_mutex_unlock(&mutex_mover_trip);
				
				//obtengo su pid para informarlo
				uint32_t pid = 0;
				pthread_mutex_lock(patota_aux->segmento_pcb->mutex_segmento);
				memcpy(&pid,patota_aux->segmento_pcb->inicio_segmento,sizeof(uint32_t));
				pthread_mutex_unlock(patota_aux->segmento_pcb->mutex_segmento);
				
				log_info(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: Encontre el segmento #%i perteneciente al tripulante en %d de la patota #%i, procedo a liberarlo",socket,tripulante_aux->numero_segmento, tid_aux,pid);
				
				//Borro tripulante de la lista de tripulantes de la patota
				list_remove(patota_aux->segmentos_tripulantes,j);
				
				
				if(! patota_aux->segmentos_tripulantes->elements_count){
					// si la patota esta vacia la elimino
					list_destroy(patota_aux->segmentos_tripulantes);
					pthread_mutex_destroy(patota_aux->mutex_segmentos_tripulantes);
					free(patota_aux->mutex_segmentos_tripulantes);
					patota_aux->segmento_pcb->libre = true;
					patota_aux->segmento_tarea->libre = true;
					free(patota_aux);
					list_remove(patotas,i);
					
				};
				
				pthread_mutex_unlock(&mutex_tabla_de_segmentos);
				pthread_mutex_unlock(&mutex_memoria);
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_tabla_patotas);
				
				
				return RESPUESTA_OK;
			}			
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
	}
	log_error(logger_ram_hq,"Socket %i, EXPULSAR_TRIPULANTE: No se pudo encontrar el tripulante %d en memoria, solicitud rechazada",socket, tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);
				
	return RESPUESTA_FAIL;
}

respuesta_ok_fail actualizar_estado_paginacion(uint32_t tid, estado estado,int socket){

	char char_estado = obtener_char_estado(estado);

	pthread_mutex_lock(&mutex_tabla_patotas);
	t_tabla_de_paginas* patota = buscar_patota_con_tid_paginacion(tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);
	if(!patota){
		log_error(logger_ram_hq,"No existe patota a la que pertenezca el tripulante de tid: %i",tid);
		return RESPUESTA_FAIL;
	}
	log_info(logger_ram_hq,"Encontre la patota a la que pertenece el tripulante de tid: %i",tid);

	double primer_indice = 2*sizeof(uint32_t);
	int tamanio_tareas = 0;
	tarea_ram* auxiliar_tarea;
	for (int i=0; i<patota->tareas->elements_count; i++){
		auxiliar_tarea = list_get(patota->tareas,i);
		tamanio_tareas += auxiliar_tarea->tamanio;
	}
	tamanio_tareas -= 1; // -1 POR EL ERROR EN EL TAMANIO DE LAS TAREAS
	primer_indice += tamanio_tareas;
	double indice_auxiliar = 0;
	double offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_auxiliar);
	int offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
	inicio_tcb* tcb = buscar_inicio_tcb(tid,patota,indice_auxiliar,offset_entero);
	double nuevo_indice = 0;
	int i = 0;
	while(!tcb->pagina && i<patota->cantidad_tripulantes){
		primer_indice += 5*sizeof(uint32_t) + sizeof(char);
		offset = modf(primer_indice / mi_ram_hq_configuracion->TAMANIO_PAGINA,&nuevo_indice);
		offset_entero = offset * mi_ram_hq_configuracion->TAMANIO_PAGINA;
		free(tcb);
		tcb = buscar_inicio_tcb(tid,patota,nuevo_indice,offset_entero);

		i++;
	}


	double indice_de_posicion = 2*sizeof(uint32_t) + tamanio_tareas + (i * (5*sizeof(uint32_t) + sizeof(char))) + sizeof(uint32_t);
	double offset_posicion = modf(indice_de_posicion / mi_ram_hq_configuracion->TAMANIO_PAGINA, &indice_de_posicion);
	int offset_posicion_entero = offset_posicion * mi_ram_hq_configuracion->TAMANIO_PAGINA;

	pthread_mutex_lock(&mutex_no_reemplazar);
	traerme_todo_el_tcb_a_memoria(tcb,patota);
	escribir_un_char_a_partir_de_indice(indice_de_posicion,offset_posicion_entero,char_estado,patota);
	pthread_mutex_unlock(&mutex_no_reemplazar);

	free(tcb);
	return RESPUESTA_OK;
}

void traerme_todo_el_tcb_a_memoria(inicio_tcb* tcb, t_tabla_de_paginas* patota){

	t_pagina* auxiliar_pagina = list_get(patota->paginas,tcb->indice);
	int tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - tcb->offset;
	int contador = 0;
	int indice = tcb->indice;

	t_frame_en_memoria* frame;

	while(contador < 21){
		pthread_mutex_lock(auxiliar_pagina->mutex_pagina);
		if(!auxiliar_pagina->presente){
			frame = buscar_frame_libre();
			if(!frame){
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
					frame = sustituir_LRU();
				}
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
					frame = sustituir_CLOCK();
				}
			}
			pthread_mutex_lock(frame->mutex);
			auxiliar_pagina->presente = 1;
			auxiliar_pagina->inicio_memoria = frame->inicio;
			auxiliar_pagina ->uso = 1;
			frame->pagina_a_la_que_pertenece = auxiliar_pagina;
			frame->indice_pagina = auxiliar_pagina->id_pagina;
			frame->pid_duenio = patota->id_patota;
			frame->libre = 0;
			pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
			memcpy(frame->inicio,auxiliar_pagina->frame_swap->inicio,mi_ram_hq_configuracion->TAMANIO_PAGINA);
			auxiliar_pagina->frame_swap->libre = 1;
			pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
			t_pagina_y_frame* frame_y_pagina = malloc(sizeof(t_pagina_y_frame));
			frame_y_pagina->frame = frame;
			frame_y_pagina->pagina = auxiliar_pagina;
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
			pthread_mutex_unlock(frame->mutex);
		}
		if(tamanio_disponible_pagina < 21){
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			contador += (21-tamanio_disponible_pagina);
			auxiliar_pagina = list_get(patota->paginas,indice + 1);
			indice ++;
			tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA;
		}
		else if(tamanio_disponible_pagina >= 21){
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			contador += 21;
		}
	}
}

void escribir_un_char_a_partir_de_indice(double indice,int offset,char dato,t_tabla_de_paginas* patota){

	t_pagina* auxiliar_pagina = list_get(patota->paginas,indice);
	int tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - offset;
	int bytes_escritos = 0;
	int bytes_desplazados_escritura = 0;
	int offset_lectura = 0;

	t_frame_en_memoria* frame;

	while(bytes_escritos < 1){
		pthread_mutex_lock(auxiliar_pagina->mutex_pagina);
		if(!auxiliar_pagina->presente){
			frame = buscar_frame_libre();
			if(!frame){
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
					frame = sustituir_LRU();
				}
				if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"CLOCK")){
					frame = sustituir_CLOCK();
				}
			}
			pthread_mutex_lock(frame->mutex);
			auxiliar_pagina->presente = 1;
			auxiliar_pagina->inicio_memoria = frame->inicio;
			auxiliar_pagina->uso = 1;
			frame->pagina_a_la_que_pertenece = auxiliar_pagina;
			frame->indice_pagina = auxiliar_pagina->id_pagina;
			frame->pid_duenio = patota->id_patota;
			frame->libre = 0;
			pthread_mutex_lock(auxiliar_pagina->frame_swap->mutex);
			memcpy(frame->inicio,auxiliar_pagina->frame_swap->inicio,mi_ram_hq_configuracion->TAMANIO_PAGINA);
			auxiliar_pagina->frame_swap->libre = 1;
			pthread_mutex_unlock(auxiliar_pagina->frame_swap->mutex);
			t_pagina_y_frame* frame_y_pagina = malloc(sizeof(t_pagina_y_frame));
			frame_y_pagina->frame = frame;
			frame_y_pagina->pagina = auxiliar_pagina;
			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		else if(auxiliar_pagina->presente){
			int indice = buscar_frame_y_pagina_con_tid_pid(auxiliar_pagina->id_pagina,patota->id_patota);

			pthread_mutex_lock(&mutex_lru);
			t_pagina_y_frame* frame_y_pagina = list_remove(historial_uso_paginas,indice);
			pthread_mutex_unlock(&mutex_lru);
			frame = frame_y_pagina->frame;
			pthread_mutex_lock(frame->mutex);

			pthread_mutex_lock(&mutex_lru);
			list_add(historial_uso_paginas,frame_y_pagina);
			pthread_mutex_unlock(&mutex_lru);
		}
		if(tamanio_disponible_pagina < 1){
			memcpy(auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura,&dato + offset_lectura,tamanio_disponible_pagina);
			bytes_escritos += tamanio_disponible_pagina;
			bytes_desplazados_escritura += tamanio_disponible_pagina;
			offset_lectura += tamanio_disponible_pagina;
			tamanio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA;
			auxiliar_pagina->fue_modificada = 1;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			auxiliar_pagina = list_get(patota->paginas,indice+1);
		}
		else{
			memcpy(auxiliar_pagina->inicio_memoria + offset + bytes_desplazados_escritura,&dato + offset_lectura, 1 - bytes_escritos);
			auxiliar_pagina->fue_modificada = 1;
			auxiliar_pagina->uso = 1;
			pthread_mutex_unlock(auxiliar_pagina->mutex_pagina);
			int escritura = 1 - bytes_escritos;
			bytes_escritos += escritura;
		}
		pthread_mutex_unlock(frame->mutex);
	}

}

respuesta_ok_fail actualizar_estado_segmentacion(uint32_t tid,estado est,int socket)
{
	t_segmentos_de_patota* patota_aux;
	t_segmento* tripulante_aux;
	uint32_t tid_aux = 0;
	
	char estado = obtener_char_estado(est);

	pthread_mutex_lock(&mutex_tabla_patotas);
	log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Buscando el tripulante %d",socket,tid);
	for(int i=0; i<patotas->elements_count; i++){
		patota_aux = list_get(patotas,i);
		pthread_mutex_lock(patota_aux->mutex_segmentos_tripulantes);
		for(int j=0; j<patota_aux->segmentos_tripulantes->elements_count; j++){
			pthread_mutex_lock(&mutex_tabla_de_segmentos);
			tripulante_aux = list_get(patota_aux->segmentos_tripulantes,j);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(tripulante_aux->mutex_segmento);
			memcpy(&tid_aux,tripulante_aux->inicio_segmento,sizeof(uint32_t));
			pthread_mutex_unlock(tripulante_aux->mutex_segmento);
			pthread_mutex_unlock(&mutex_memoria);
			
			

			if(tid_aux == tid){
				//Busco pid para informarlo
				uint32_t pid = 0;
				pthread_mutex_lock(&mutex_memoria);
				pthread_mutex_lock(patota_aux->segmento_pcb->mutex_segmento);
				memcpy(&pid,patota_aux->segmento_pcb->inicio_segmento,sizeof(uint32_t));
				pthread_mutex_unlock(patota_aux->segmento_pcb->mutex_segmento);
				pthread_mutex_unlock(&mutex_memoria);

				pthread_mutex_lock(&mutex_memoria);
				pthread_mutex_lock(tripulante_aux->mutex_segmento);
			
				log_info(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: Encontre al tripulante %d en memoria. Segmento #%i perteneciente a la patota %i. Se actualiza el estado a %c",socket, tid,tripulante_aux->numero_segmento,pid,estado);
				memcpy(tripulante_aux->inicio_segmento + sizeof(uint32_t),&estado,sizeof(char));

				pthread_mutex_unlock(tripulante_aux->mutex_segmento);
				pthread_mutex_unlock(&mutex_memoria);			
				pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_tabla_patotas);
				return RESPUESTA_OK;
			}
			
		}
		pthread_mutex_unlock(patota_aux->mutex_segmentos_tripulantes);

	}
	log_error(logger_ram_hq,"Socket %i, ACTUALIZAR_ESTADO: No se pudo encontrar el tripulante %d en memoria, solicitud rechazada",socket, tid);
	pthread_mutex_unlock(&mutex_tabla_patotas);
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

//-----------------------------------------------------------FUNCIONES DE BUSQUEDA DE SEGMENTOS--------------------------------------

t_tabla_de_paginas* buscar_patota_paginacion(uint32_t pid){
	pthread_mutex_lock(&mutex_busqueda_patota);
	t_tabla_de_paginas* auxiliar;
	t_pagina* auxiliar_pagina;
	uint32_t pid_auxiliar = 0;
	uint32_t espacio_disponible_pagina = mi_ram_hq_configuracion->TAMANIO_PAGINA - 0; // 0 es la posicion donde arranca el dato que estoy buscando dentro de la pagina
	uint32_t espacio_a_leer = 4;
	uint32_t offset = 0;
	uint32_t espacio_leido = 0;
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar = list_get(patotas,i);
		for(int j=0; j<auxiliar->paginas->elements_count; j++){
			auxiliar_pagina = list_get(auxiliar->paginas,j);
			if(espacio_a_leer > espacio_disponible_pagina && auxiliar_pagina->presente){
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->inicio_memoria + offset,espacio_disponible_pagina);
				espacio_leido += espacio_disponible_pagina;
				espacio_disponible_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;

			}
			else if(espacio_a_leer <= espacio_disponible_pagina && auxiliar_pagina->presente){
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->inicio_memoria + offset,espacio_a_leer - espacio_leido);
				if(pid_auxiliar == pid){
					pthread_mutex_unlock(&mutex_busqueda_patota);
					return auxiliar;
				}
				else{
					break;
				}
			}
			else if(espacio_a_leer > espacio_disponible_pagina && !auxiliar_pagina->presente){
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->frame_swap->inicio + offset,espacio_disponible_pagina);
				espacio_leido += espacio_disponible_pagina;
				espacio_disponible_pagina =  mi_ram_hq_configuracion->TAMANIO_PAGINA;

			}
			else if(espacio_a_leer <= espacio_disponible_pagina && !auxiliar_pagina->presente){
				memcpy(&pid_auxiliar + espacio_leido,auxiliar_pagina->frame_swap->inicio + offset,espacio_a_leer - espacio_leido);
				if(pid_auxiliar == pid){
					pthread_mutex_unlock(&mutex_busqueda_patota);
					return auxiliar;
				}
				else{
					break;
				}
			}
		}

	}
	pthread_mutex_unlock(&mutex_busqueda_patota);
	return NULL;
}

t_list* buscar_cantidad_frames_libres(int cantidad){
	
	t_list* frames_a_usar = list_create();
	t_frame_en_memoria* auxiliar;
	int contador = 0;
	pthread_mutex_lock(&mutex_frames);
	
	for(int i=0; i<frames->elements_count; i++){
		if(contador >= cantidad){
			pthread_mutex_unlock(&mutex_frames);
			return frames_a_usar;
		}
		auxiliar = list_get(frames,i);
		if(auxiliar->libre){
			auxiliar->libre = false;
			list_add(frames_a_usar,auxiliar);
			contador ++;
		}
	}
	pthread_mutex_unlock(&mutex_frames);
	return frames_a_usar;
}

t_segmentos_de_patota* buscar_patota(uint32_t pid){
	t_segmentos_de_patota* auxiliar;
	uint32_t pid_auxiliar = 0;
	pthread_mutex_lock(&mutex_tabla_patotas);
	for(int i=0; i<patotas->elements_count; i++){
		pthread_mutex_lock(&mutex_tabla_de_segmentos);
		auxiliar = list_get(patotas,i);
		pthread_mutex_unlock(&mutex_tabla_de_segmentos);

		pthread_mutex_lock(&mutex_memoria);
		pthread_mutex_lock(auxiliar->segmento_pcb->mutex_segmento);
		memcpy(&pid_auxiliar,auxiliar->segmento_pcb->inicio_segmento,sizeof(uint32_t));
		pthread_mutex_unlock(auxiliar->segmento_pcb->mutex_segmento);
		pthread_mutex_unlock(&mutex_memoria);
		
		if(pid_auxiliar == pid){
			pthread_mutex_unlock(&mutex_tabla_patotas);
			return auxiliar;
		}
	}
	pthread_mutex_unlock(&mutex_tabla_patotas);
	return NULL;
}

t_segmentos_de_patota* buscar_patota_con_tid(uint32_t tid,int socket){
	t_segmentos_de_patota* auxiliar_patota;
	t_segmento* segmento_tripulante_auxiliar;
	log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Buscando patota del tripulante %i",socket,tid);
	pthread_mutex_lock(&mutex_tabla_patotas);
	for(int i=0; i<patotas->elements_count; i++){
		auxiliar_patota = list_get(patotas,i);
		pthread_mutex_lock(auxiliar_patota -> mutex_segmentos_tripulantes);
		for(int j=0; j<auxiliar_patota->segmentos_tripulantes->elements_count; j++){
			uint32_t tid_aux = 0;
			uint32_t pid_aux = 0;
			

			pthread_mutex_lock(&mutex_tabla_de_segmentos);
			segmento_tripulante_auxiliar = list_get(auxiliar_patota->segmentos_tripulantes,j);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
		
			pthread_mutex_lock(&mutex_memoria);

			pthread_mutex_lock(segmento_tripulante_auxiliar -> mutex_segmento);
			memcpy(&tid_aux,segmento_tripulante_auxiliar->inicio_segmento,sizeof(uint32_t));
			pthread_mutex_unlock(segmento_tripulante_auxiliar -> mutex_segmento);

			pthread_mutex_lock(auxiliar_patota->segmento_pcb->mutex_segmento);
			memcpy(&pid_aux,auxiliar_patota->segmento_pcb->inicio_segmento,sizeof(uint32_t));
			pthread_mutex_unlock(auxiliar_patota->segmento_pcb->mutex_segmento);
			
			pthread_mutex_unlock(&mutex_memoria);
			
			if(tid_aux == tid){
				pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
				pthread_mutex_unlock(&mutex_tabla_patotas);
				log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Patota correspondiente al tid: %d encontrada en la patota %i",socket,tid,pid_aux);
				return auxiliar_patota;
			}
		}
		pthread_mutex_unlock(auxiliar_patota -> mutex_segmentos_tripulantes);
	}
	pthread_mutex_unlock(&mutex_tabla_patotas);
	log_error(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: No se encontro una patota con el tid: %d asociada",socket,tid);
	return NULL;
}

t_segmento* buscar_segmento_pcb(){
	t_segmento* iterador;
	t_segmento* auxiliar = malloc(sizeof(t_segmento));
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			pthread_mutex_lock(iterador->mutex_segmento);
			if(iterador->tamanio_segmento >= 2*(sizeof(uint32_t)) && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = 2*(sizeof(uint32_t)); 
				auxiliar->libre = false;
				auxiliar->numero_segmento = numero_segmento_global;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
				numero_segmento_global++;
				list_add_in_index(segmentos_memoria,i,auxiliar);
				iterador->inicio_segmento += 2*(sizeof(uint32_t));
				iterador->tamanio_segmento -= 2*(sizeof(uint32_t));
				pthread_mutex_unlock(iterador->mutex_segmento);
				pthread_mutex_unlock(&mutex_tabla_de_segmentos);
				return auxiliar;
			}
			pthread_mutex_unlock(iterador->mutex_segmento);
		} 
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){ 
		t_segmento* vencedor = NULL;
		//vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		int index_vencedor = 0;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			
			pthread_mutex_lock(iterador->mutex_segmento);
			if((iterador->tamanio_segmento >= 2*(sizeof(uint32_t))) && (iterador->libre)){
				if(!vencedor){
					vencedor = iterador;
					index_vencedor = i;
				}
				else if(iterador->tamanio_segmento < vencedor->tamanio_segmento){
					vencedor = iterador;
					index_vencedor = i;
				}
			}
			
			pthread_mutex_unlock(iterador->mutex_segmento);
		}

		if(vencedor){
			pthread_mutex_lock(vencedor->mutex_segmento);
		
			auxiliar->inicio_segmento = vencedor->inicio_segmento;
			auxiliar->tamanio_segmento = 2*(sizeof(uint32_t)); 
			auxiliar->libre = false;
			auxiliar->numero_segmento = numero_segmento_global;
			auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(auxiliar->mutex_segmento,NULL);
			numero_segmento_global++;
			list_add_in_index(segmentos_memoria,index_vencedor,auxiliar);
			
			vencedor->inicio_segmento += 2*(sizeof(uint32_t));
			vencedor->tamanio_segmento -= 2*(sizeof(uint32_t));
			pthread_mutex_unlock(vencedor->mutex_segmento);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			
			return auxiliar;
		}
		
	}
	free(auxiliar);
	pthread_mutex_unlock(&mutex_tabla_de_segmentos);
	return NULL;
};

t_segmento* buscar_segmento_tareas(uint32_t tamanio_tareas){	
	t_segmento* iterador;
	t_segmento* auxiliar = malloc(sizeof(t_segmento));
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			pthread_mutex_lock(iterador->mutex_segmento);
			if(iterador->tamanio_segmento >= tamanio_tareas && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = tamanio_tareas; 
				auxiliar->libre = false;
				auxiliar->numero_segmento = numero_segmento_global;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
				numero_segmento_global++;
				list_add_in_index(segmentos_memoria,i,auxiliar);
				iterador->inicio_segmento += tamanio_tareas;
				iterador->tamanio_segmento -= tamanio_tareas;
				pthread_mutex_unlock(iterador->mutex_segmento);
				pthread_mutex_unlock(&mutex_tabla_de_segmentos);
				return auxiliar;
			}
			pthread_mutex_unlock(iterador->mutex_segmento);
		}
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){
		t_segmento* vencedor = NULL;
		int index_vencedor = 0;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			pthread_mutex_lock(iterador->mutex_segmento);
			if((iterador->tamanio_segmento >= tamanio_tareas) && (iterador->libre)){
				if(!vencedor){
					vencedor = iterador;
					index_vencedor = i;
				}
				else if(iterador->tamanio_segmento < vencedor->tamanio_segmento){
					vencedor = iterador;
					index_vencedor = i;
				}
			}
			pthread_mutex_unlock(iterador->mutex_segmento);
		}
		
		if(vencedor){

			pthread_mutex_lock(vencedor->mutex_segmento);
			
			auxiliar->inicio_segmento = vencedor->inicio_segmento;
			auxiliar->tamanio_segmento = tamanio_tareas; 
			auxiliar->libre = false;
			auxiliar->numero_segmento = numero_segmento_global;
			auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(auxiliar->mutex_segmento,NULL);
			numero_segmento_global++;
			list_add_in_index(segmentos_memoria,index_vencedor,auxiliar);
			vencedor->inicio_segmento += tamanio_tareas;
			vencedor->tamanio_segmento -= tamanio_tareas;
			
			pthread_mutex_unlock(vencedor->mutex_segmento);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			
			return auxiliar;
		
		}
	}
	pthread_mutex_unlock(&mutex_tabla_de_segmentos);
	free(auxiliar);
	return NULL;
};

t_segmento* buscar_segmento_tcb(){
	t_segmento* iterador;
	t_segmento* auxiliar = malloc(sizeof(t_segmento));
	uint32_t size_tcb = sizeof(uint32_t)*5 + sizeof(char);
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"FF")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			pthread_mutex_lock(iterador->mutex_segmento);
			if(iterador->tamanio_segmento >= size_tcb && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = size_tcb; 
				auxiliar->libre = false;
				auxiliar->numero_segmento = numero_segmento_global;
				numero_segmento_global++;
				auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(auxiliar->mutex_segmento,NULL);
				list_add_in_index(segmentos_memoria,i,auxiliar);
				iterador->inicio_segmento += size_tcb;
				iterador->tamanio_segmento -= size_tcb;
				pthread_mutex_unlock(iterador->mutex_segmento);
				pthread_mutex_unlock(&mutex_tabla_de_segmentos);
				return auxiliar;
			}
			pthread_mutex_unlock(iterador->mutex_segmento);
		}
	}
	else if(!strcmp(mi_ram_hq_configuracion->CRITERIO_SELECCION,"BF")){ 
		t_segmento* vencedor = NULL;
		int index_vencedor = 0;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			pthread_mutex_lock(iterador->mutex_segmento);
			if((iterador->tamanio_segmento >= size_tcb) && (iterador->libre)){
				if(!vencedor){
					vencedor = iterador;
					index_vencedor = i;
				}
				else if(iterador->tamanio_segmento < vencedor->tamanio_segmento){
					vencedor = iterador;
					index_vencedor = i;
				}
				
			}
			pthread_mutex_unlock(iterador->mutex_segmento);
		}

		if(vencedor){

			pthread_mutex_lock(vencedor->mutex_segmento);	
			
			auxiliar->inicio_segmento = vencedor->inicio_segmento;
			auxiliar->tamanio_segmento = size_tcb; 
			auxiliar->libre = false;
			auxiliar->numero_segmento = numero_segmento_global;
			numero_segmento_global++;
			auxiliar->mutex_segmento = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(auxiliar->mutex_segmento,NULL);
			list_add_in_index(segmentos_memoria,index_vencedor,auxiliar);
			vencedor->inicio_segmento += size_tcb;
			vencedor->tamanio_segmento -= size_tcb;
			
			pthread_mutex_unlock(vencedor->mutex_segmento);
			pthread_mutex_unlock(&mutex_tabla_de_segmentos);
			return auxiliar;
		}
	}

	pthread_mutex_unlock(&mutex_tabla_de_segmentos);
	free(auxiliar);
	return NULL;
};
//----------------------------------------------CARGAR SEGMENTOS A MEMORIA-------------------------------------------------------------------------

void cargar_pcb_en_segmento(uint32_t pid, uint32_t direccion_logica_tareas, t_segmento* segmento){
	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(segmento->mutex_segmento);
	int offset = 0;
	memcpy(segmento->inicio_segmento + offset, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(segmento->inicio_segmento + offset, &direccion_logica_tareas, sizeof(uint32_t));
	pthread_mutex_unlock(&mutex_memoria);
	pthread_mutex_unlock(segmento->mutex_segmento);
	
}

//Aclaración: Esta función asume que las tareas incluyen un /0 al final de la cadena de char*
void cargar_tareas_en_segmento(char* tareas,uint32_t longitud_palabra, t_segmento* segmento){
	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(segmento->mutex_segmento);
	memcpy(segmento->inicio_segmento, tareas, longitud_palabra);
	pthread_mutex_unlock(&mutex_memoria);
	pthread_mutex_unlock(segmento->mutex_segmento);
}

void cargar_tcb_sinPid_en_segmento(nuevo_tripulante_sin_pid* tripulante, t_segmento* segmento, uint32_t pid){
	
	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(segmento->mutex_segmento);
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
	pthread_mutex_unlock(&mutex_memoria);
	pthread_mutex_unlock(segmento->mutex_segmento);
}


//-------------------------------------------------------------------------FUNCION DE COMPACTACION-----------------------------------------------------

void swap(t_segmento **aux1,t_segmento **aux2){
	t_segmento* temp = malloc(sizeof(t_segmento));
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



void compactar_memoria(){
	/*
	t_segmento* auxiliar;
	auxiliar = list_get(segmentos_memoria,0);
	int i;
	//TODO: ORDENAR LISTA DE SEGMENTOS POR DIRECCIONES
	//quiza hay que ordenar por la base del segmento que apunta
	printf("Lista de segmentos actual\n");	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		printf("Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
	}

	 */

	//NO OLVIDAR QUE CADA ACCESO A SEGMENTO LO BLOQUEE
	//wait lista de segmentos y cada segmento en si
	log_info(logger_ram_hq,"Iniciando compactacion");
	log_info(logger_ram_hq,"Ordenando segmentos\n");	
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	t_segmento* auxiliar;
	int i;
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		pthread_mutex_lock(auxiliar->mutex_segmento);
	}
	list_sort(segmentos_memoria,ordenar_direcciones_de_memoria); // No se si esta bien usada la funcion, lo voy a preguntar en soporte pero dejo la idea
	log_info(logger_ram_hq,"Segmentos ordenados\n");	

	log_info(logger_ram_hq,"Limpio segmentos vacios\n");	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		log_info(logger_ram_hq,"Segmento %d inicio en %i libre %d tamanio %d  \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,auxiliar->libre,auxiliar->tamanio_segmento);
		if(auxiliar->libre){
			list_remove(segmentos_memoria,i);
			i--;
			log_info(logger_ram_hq,"Eliminando segmento %i\n",auxiliar->numero_segmento);			
		}
	}
	
	int offset = 0;
	void* inicio_segmento_libre;
	uint32_t tamanio_acumulado = 0;
	uint32_t tamanio_segmento_libre = 0;
	log_info(logger_ram_hq,"Moviendo segmentos\n");
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		log_info(logger_ram_hq,"Pasando segmento %d de %i a %d \n",auxiliar->numero_segmento,auxiliar->inicio_segmento,memoria_principal + offset);
		memcpy(memoria_principal + offset, auxiliar->inicio_segmento, auxiliar->tamanio_segmento);
		auxiliar->inicio_segmento = memoria_principal + offset;
		tamanio_acumulado += auxiliar->tamanio_segmento;
		offset += auxiliar->tamanio_segmento;
	}
	
	inicio_segmento_libre = memoria_principal + tamanio_acumulado; // Si rompe, castear el inicio de auxiliar como uint32_t
	tamanio_segmento_libre = mi_ram_hq_configuracion->TAMANIO_MEMORIA - tamanio_acumulado; //No se si va el +1, para dami no va, lo borro

	t_segmento* segmento_libre = malloc(sizeof(t_segmento));
	segmento_libre->libre = true;
	segmento_libre->inicio_segmento = inicio_segmento_libre;
	segmento_libre->tamanio_segmento = tamanio_segmento_libre;
	segmento_libre->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(segmento_libre->mutex_segmento,NULL);
	pthread_mutex_lock(segmento_libre->mutex_segmento);

	list_add(segmentos_memoria,segmento_libre); // Al final de la ordenada le agrego el segmento libre
	
	for(i=0; i<segmentos_memoria->elements_count; i++){
		auxiliar = list_get(segmentos_memoria,i);
		pthread_mutex_unlock(auxiliar->mutex_segmento);
	}
	pthread_mutex_unlock(&mutex_tabla_de_segmentos);
	
}

bool ordenar_direcciones_de_memoria(void* p1, void* p2){
	t_segmento* segmento1 = p1;
	t_segmento* segmento2 = p2;
		
	return ((int)segmento1->inicio_segmento) < ((int)segmento2->inicio_segmento);
}

void recorrer_pcb(t_segmento * pcb){
	uint32_t pid = 0;
	pthread_mutex_lock(pcb ->mutex_segmento);
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	log_info(logger_ram_hq,"Patota pid = %i\n",pid);
	pthread_mutex_unlock(pcb ->mutex_segmento);
}
void recorrer_tareas(t_segmento * tareas){
	pthread_mutex_lock(tareas ->mutex_segmento);
	
	char* auxiliar = malloc (tareas->tamanio_segmento);

	strcpy(auxiliar,"");

	memcpy(auxiliar, tareas->inicio_segmento, tareas->tamanio_segmento);
	
	log_info(logger_ram_hq,"Lista de tareas: \n");
	log_info(logger_ram_hq,"%s\n",auxiliar);
	//printf ("%s\n",auxiliar+1);
	//printf ("%c\n",*auxiliar);
	for(int i = 0;i<tareas->tamanio_segmento -1;i++){
		//printf ("%c\n",*(auxiliar+i));
		 if( *(auxiliar+i) == '\0'){
			log_info(logger_ram_hq,"%s\n",auxiliar+i+1);
		}

	}
	free(auxiliar);
	pthread_mutex_unlock(tareas ->mutex_segmento);
}
void recorrer_tripulante(t_segmento * tripulante){
	uint32_t tid = 0;
	char estado_actual;
	uint32_t posX = 0;
	uint32_t posY = 0;
	uint32_t tareaActual = 0;
	uint32_t patotaActual = 0;

	pthread_mutex_lock(tripulante ->mutex_segmento);
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
	log_info(logger_ram_hq,"Tripulante tid %i, estado %c, posicion %d;%d, tarea %i, patota %i\n",tid,estado_actual,posX,posY,tareaActual,patotaActual);
	pthread_mutex_unlock(tripulante ->mutex_segmento);
}


void imprimir_dump(t_log* log_dump,char * time){
	
	
	
	log_info(log_dump,"--------------------------------------------------------------------------\n");log_info(log_dump,"--------------------------------------------------------------------------\n");
	log_info(log_dump,"Dump: %s \n",time);
	
	//free(time);
	for(int i = 0;i < patotas->elements_count;i++){
		t_segmentos_de_patota* patota = list_get(patotas,i);
		uint32_t pid = obtener_patota_memoria(patota->segmento_pcb);
		//recorrer_pcb_dump(pid,patota->segmento_pcb);
		log_info(log_dump,"Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_pcb->numero_segmento,patota->segmento_pcb->inicio_segmento,patota->segmento_pcb->tamanio_segmento);
		//recorrer_tareas_dump(pid,patota->segmento_tarea);
		log_info(log_dump,"Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,patota->segmento_tarea->numero_segmento,patota->segmento_tarea->inicio_segmento,patota->segmento_tarea->tamanio_segmento);
		pthread_mutex_lock(patota->mutex_segmentos_tripulantes);
		recorrer_tcb_dump(pid,patota->segmentos_tripulantes,log_dump);
		pthread_mutex_unlock(patota->mutex_segmentos_tripulantes);
	}

	log_info(log_dump,"--------------------------------------------------------------------------\n");log_info(log_dump,"--------------------------------------------------------------------------\n");
	log_info(log_dump,"Dump alternativo: %s \n",time);
	
	for(int i = 0;i < segmentos_memoria->elements_count;i++){
		t_segmento* segmento = list_get(segmentos_memoria,i);
		pthread_mutex_lock(segmento->mutex_segmento);
		log_info(log_dump,"Segmento: %i\t Inicio: %i\t Tam: %ib Libre: %d\n",segmento->numero_segmento,segmento->inicio_segmento,segmento->tamanio_segmento,segmento->libre);
		pthread_mutex_unlock(segmento->mutex_segmento);
		
	}
}

uint32_t obtener_patota_memoria(t_segmento * pcb){
	uint32_t pid = 0;
	//pthread_mutex_lock(pcb->mutex_segmento);
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	//pthread_mutex_unlock(pcb->mutex_segmento);
	return pid;
}

void recorrer_pcb_dump(t_segmento* pcb){
	uint32_t pid = 0;
	//pthread_mutex_lock(pcb->mutex_segmento);
	
	memcpy(&pid, pcb->inicio_segmento, sizeof(uint32_t));
	//pthread_mutex_unlock(pcb->mutex_segmento);
	log_info(logger_ram_hq,"Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,pcb->numero_segmento,pcb->inicio_segmento,pcb->tamanio_segmento);
}
void recorrer_tareas_dump(uint32_t pid,t_segmento* tareas){
	log_info(logger_ram_hq,"Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tareas->numero_segmento,tareas->inicio_segmento,tareas->tamanio_segmento);
}
void recorrer_tcb_dump(uint32_t pid,t_list* tripulantes,t_log * log_dump){
	for(int i = 0;i < tripulantes->elements_count;i++){
		t_segmento * tripulante = list_get(tripulantes,i);
		log_info(log_dump,"Proceso: %i\t Segmento: %i\t Inicio: %i\t Tam: %i b\n",pid,tripulante->numero_segmento,tripulante->inicio_segmento,tripulante->tamanio_segmento);
	}
}


//Cuando un FRAME no tiene un proceso imprime 0, los estados son 1: Libre 0: ocupado, capaz hay que cambiar esto
 void imprimir_dump_paginacion(t_log* log_dump,char * time){
	pthread_mutex_lock(&mutex_dump);
	log_info(log_dump,"--------------------------------------------------------------------------");log_info(log_dump,"--------------------------------------------------------------------------\n");
	log_info(log_dump,"Dump: %s \n",time);
	log_info(log_dump,"Dump correspondiente a la accion %s, del socket %i \n",accion,socket_accion);
	pthread_mutex_unlock(&mutex_accion);
	pthread_mutex_lock(&mutex_frames);
	for(int i=0; i<frames->elements_count && i<10; i++)	{
		t_frame_en_memoria* frame = list_get(frames,i);
		pthread_mutex_lock(frame->mutex);
		int bit_uso;
		if(!frame->libre){
			bit_uso = frame->pagina_a_la_que_pertenece->uso;
		}
		else{
			bit_uso = -1;
		}
		log_info(log_dump,"Marco: %i\t Estado: %i\t Proceso: %i\t Pagina: %i \t	Bit de uso: %i",i,frame->libre,frame->pid_duenio,frame->indice_pagina,bit_uso);
		pthread_mutex_unlock(frame->mutex);
	}
	pthread_mutex_unlock(&mutex_frames);

	if(!strcmp(mi_ram_hq_configuracion->ALGORITMO_REEMPLAZO,"LRU")){
		t_pagina_y_frame * auxiliar;
		pthread_mutex_lock(&mutex_lru);
		log_info(log_dump,"Imprimiendo la cola de LRU:");
		for(int i=0; i<historial_uso_paginas->elements_count && i<10; i++){
			auxiliar = list_get(historial_uso_paginas,i);
			log_info(log_dump,"Frame perteneciente a la pagina %i, del proceso %i en la posicion %i",auxiliar->frame->indice_pagina,auxiliar->frame->pid_duenio,i);
		}
		pthread_mutex_unlock(&mutex_lru);
	}
	log_info(log_dump,"--------------------------------------------------------------------------------\n");
  	pthread_mutex_unlock(&mutex_dump);
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

void actualizarTareaActual(t_segmentos_de_patota* auxiliar_patota,uint32_t tripulante_tid,int socket){
	pthread_mutex_lock(auxiliar_patota->mutex_segmentos_tripulantes);
	for(int i = 0;i < auxiliar_patota->segmentos_tripulantes->elements_count;i++){
		pthread_mutex_lock(&mutex_tabla_de_segmentos);
		t_segmento * tripulante = list_get(auxiliar_patota->segmentos_tripulantes,i);
		pthread_mutex_unlock(&mutex_tabla_de_segmentos);
		
		uint32_t tid = 0;
		
		pthread_mutex_lock(&mutex_memoria);
		pthread_mutex_lock(tripulante->mutex_segmento);
		memcpy(&tid, tripulante->inicio_segmento, sizeof(uint32_t));
		pthread_mutex_unlock(tripulante->mutex_segmento);
		pthread_mutex_unlock(&mutex_memoria);
		
		
		if(tid == tripulante_tid){			
		
			int offset = sizeof(uint32_t) + sizeof(char) + sizeof(uint32_t) + sizeof(uint32_t);
			uint32_t tareaActual = 0;
			
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(tripulante->mutex_segmento);

			memcpy(&tareaActual, tripulante->inicio_segmento + offset, sizeof(uint32_t));
			tareaActual++;
			memcpy(tripulante->inicio_segmento + offset,&tareaActual , sizeof(uint32_t));

			pthread_mutex_unlock(tripulante->mutex_segmento);
			pthread_mutex_unlock(&mutex_memoria);

			log_info(logger_ram_hq,"Socket %i, OBTENER_PROXIMA_TAREA: Tripulante %i actualizado. Tarea #%i",socket,tripulante_tid,tareaActual);
			
		}
		
	}
	pthread_mutex_unlock(auxiliar_patota->mutex_segmentos_tripulantes);
	
	return;
}


uint32_t calcular_memoria_libre(){
	t_segmento* iterador;
	uint32_t libre = 0;
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	for(int i=0; i<segmentos_memoria->elements_count;i++){
		iterador = list_get(segmentos_memoria,i);
		pthread_mutex_lock(iterador->mutex_segmento);
		if(iterador->libre){
			libre = libre + iterador->tamanio_segmento; 
		}
		pthread_mutex_unlock(iterador->mutex_segmento);
	}
	pthread_mutex_unlock(&mutex_tabla_de_segmentos);
	return libre;
}

//--------------------------------------------------Algoritmos de SWAP---------------------------------------------


 t_frame_en_memoria* sustituir_LRU(){
	log_info(logger_ram_hq,"inicio algoritmo de sustitucion LRU");
	pthread_mutex_lock(&mutex_lru);
	t_pagina_y_frame* pagina_a_quitar_con_su_frame = list_remove(historial_uso_paginas,0);
	pthread_mutex_unlock(&mutex_lru);
	pthread_mutex_lock(&mutex_frames);
	actualizar_pagina(pagina_a_quitar_con_su_frame->pagina);
	pthread_mutex_unlock(&mutex_frames);
	//pagina_a_quitar_con_su_frame->pagina->presente = 0;
	log_info(logger_ram_hq, "se ha elegido como victima a la pagina %i del proceso %i", pagina_a_quitar_con_su_frame->pagina->id_pagina,pagina_a_quitar_con_su_frame->frame->pid_duenio);
	t_frame_en_memoria* a_retornar;
	a_retornar = pagina_a_quitar_con_su_frame->frame;
	free(pagina_a_quitar_con_su_frame);
	return a_retornar;
}


void actualizar_pagina(t_pagina* pagina){

	//pthread_mutex_lock(&mutex_swap);
	pthread_mutex_lock(pagina->mutex_pagina);
	t_frame_en_swap* auxiliar = buscar_frame_swap_libre();
	if(!auxiliar){
		log_error(logger_ram_hq,"Se pidio buscar un frame en swap pero no habia ninguno disponible");
		pthread_mutex_unlock(pagina->mutex_pagina);
		//pthread_mutex_unlock(&mutex_swap);
		return;
	}
	pthread_mutex_lock(auxiliar->mutex);
	pagina->frame_swap = auxiliar;
	memcpy(pagina->frame_swap->inicio,pagina->inicio_memoria,mi_ram_hq_configuracion->TAMANIO_PAGINA);
	pagina->frame_swap->libre = 0;
	pagina->presente = 0;
	pagina->inicio_memoria = NULL;
	pthread_mutex_unlock(auxiliar->mutex);
	pthread_mutex_unlock(pagina->mutex_pagina);
	//pthread_mutex_unlock(&mutex_swap);

}


t_frame_en_memoria* sustituir_CLOCK() {
	log_info(logger_ram_hq,"inicio algoritmo de sustitucion Clock");
	t_frame_en_memoria* frame_libre = iterar_clock_sobre_frames();
	return frame_libre;
}

t_frame_en_memoria* iterar_clock_sobre_frames(){
	t_pagina_y_frame* una_pagina_con_su_frame = malloc(sizeof(t_pagina_y_frame));
	t_pagina_y_frame* pagina_con_frame_quitadas = malloc(sizeof(t_pagina_y_frame));
	pagina_con_frame_quitadas->pagina = NULL;
	t_frame_en_memoria* a_retornar;
	int indice = 0;
	int valor_original_puntero = puntero_lista_frames_clock;
	//ACA ROMPE PORQUE ACTUZLIAR PAGINA USA ESTE LOCK
	pthread_mutex_lock(&mutex_frames);
	for (int i = 0;	i < frames->elements_count && pagina_con_frame_quitadas->pagina == NULL; i++) {
		indice = i;
		if(valor_original_puntero + i >= frames->elements_count)
			indice = i + valor_original_puntero - frames->elements_count;
		else
			indice = valor_original_puntero + i;
		t_frame_en_memoria* auxiliar = list_get(frames,indice);	
		una_pagina_con_su_frame->frame = auxiliar;
		una_pagina_con_su_frame->pagina = una_pagina_con_su_frame->frame->pagina_a_la_que_pertenece;

		if (!una_pagina_con_su_frame->pagina->uso) {
			actualizar_pagina(una_pagina_con_su_frame->pagina);
			pagina_con_frame_quitadas->frame = una_pagina_con_su_frame->frame;
			pagina_con_frame_quitadas->pagina = pagina_con_frame_quitadas->frame->pagina_a_la_que_pertenece;
			log_info(logger_ram_hq, "Se ha seleccionado como victima a la pagina %i del proceso %i", pagina_con_frame_quitadas->pagina->id_pagina,pagina_con_frame_quitadas->frame->pid_duenio);
			if(indice + 1 == frames->elements_count)
				puntero_lista_frames_clock = 0;
			else
				puntero_lista_frames_clock = indice + 1;
		}else{
			una_pagina_con_su_frame->pagina->uso = 0;
		}
	}	
	if (pagina_con_frame_quitadas->pagina != NULL) {
		pthread_mutex_lock(pagina_con_frame_quitadas->pagina->mutex_pagina);
		pagina_con_frame_quitadas->pagina->presente = 0;
		pagina_con_frame_quitadas->pagina->fue_modificada = 0;
		pthread_mutex_unlock(pagina_con_frame_quitadas->pagina->mutex_pagina);
		a_retornar = pagina_con_frame_quitadas->frame;
		free(pagina_con_frame_quitadas);
		free(una_pagina_con_su_frame);
		pthread_mutex_unlock(&mutex_frames);
		return a_retornar;
	}
	else{
		a_retornar = list_get(frames,valor_original_puntero);
		log_info(logger_ram_hq, "Se ha seleccionado como victima a la pagina %i del proceso %i", a_retornar->pagina_a_la_que_pertenece->id_pagina,a_retornar->pid_duenio);
		actualizar_pagina(a_retornar->pagina_a_la_que_pertenece);
		a_retornar->pagina_a_la_que_pertenece->presente = 0;
		if(puntero_lista_frames_clock + 1 == frames->elements_count){
			puntero_lista_frames_clock = 0;
		}
		else 
			puntero_lista_frames_clock ++;
		free(pagina_con_frame_quitadas);
		free(una_pagina_con_su_frame);
		pthread_mutex_unlock(&mutex_frames);
		return a_retornar;
	}
	pthread_mutex_unlock(&mutex_frames);
}

int buscar_frame_y_pagina_con_tid_pid(int id_pagina,int id_patota){
	t_pagina_y_frame* auxiliar;
	pthread_mutex_lock(&mutex_lru);
	for(int i=0; i<historial_uso_paginas->elements_count; i++){
		auxiliar = list_get(historial_uso_paginas,i);

		if(auxiliar->frame->indice_pagina == id_pagina && auxiliar->frame->pid_duenio == id_patota){
			pthread_mutex_unlock(&mutex_lru);
			return i;
		}
	}
	pthread_mutex_unlock(&mutex_lru);
	log_error(logger_ram_hq,"Un frame marcado como presente no tiene asignado ningun frame");
	return -1;
}

t_frame_en_swap* buscar_frame_swap_libre(){
	t_frame_en_swap* auxiliar;
	//pthread_mutex_lock(&mutex_frames);
	for(int i=0; i<frames_swap->elements_count; i++){
		auxiliar = list_get(frames_swap,i);
		pthread_mutex_lock(auxiliar->mutex);
		if(auxiliar->libre){
			pthread_mutex_unlock(auxiliar->mutex);
			//pthread_mutex_unlock(&mutex_frames);
			return auxiliar;
		}
	  pthread_mutex_unlock(auxiliar->mutex);
	}
	//pthread_mutex_unlock(&mutex_frames);
	return NULL;
}

// Funciones de manejo de señales
void sighandlerCompactar(int signum) {
	log_info(logger_ram_hq,"Llego señal de compactacion, procedo a compactar");
	pthread_mutex_lock(&mutex_iniciar_patota);
	compactar_memoria();
	//imprimir_dump();
	pthread_mutex_unlock(&mutex_iniciar_patota);
}

void sighandlerDump(int signum) {
	log_info(logger_ram_hq,"Llego señal de dump");
	//funcion_test_memoria_completa();
	char * time =  temporal_get_string_time("%d-%m-%y_%H:%M:%S");
	char * path_dump = malloc (100);
	strcpy(path_dump,"cfg/DMP_");
	strcat(path_dump,time);
	strcat(path_dump,".dmp");


	if(!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA,"SEGMENTACION")){
		t_log *log_dump = log_create(path_dump, "DUMP", 0, LOG_LEVEL_INFO);
		imprimir_dump(log_dump,time);
		signal(SIGUSR2, sighandlerDump);
		log_destroy(log_dump);	
	}
	else{
		t_log *log_dump = log_create(path_dump, "DUMP", 0, LOG_LEVEL_INFO);
		imprimir_dump_paginacion(log_dump,time);
		signal(SIGUSR2, sighandlerDump);
		log_destroy(log_dump);
	}

	free(path_dump);
	free (time);

}

void funcion_test_memoria_completa (void){
	
	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(&mutex_tabla_patotas);
	int i;
	for(i = 0;i< patotas->elements_count;i++){
		t_segmentos_de_patota* patota = list_get(patotas,i);
		recorrer_pcb(patota->segmento_pcb);
		recorrer_tareas(patota->segmento_tarea);
		pthread_mutex_lock(patota->mutex_segmentos_tripulantes);
		int j;
		for(j = 0;j< patota->segmentos_tripulantes->elements_count;j++){
			t_segmento* tripulante = list_get(patota->segmentos_tripulantes,j);
			recorrer_tripulante(tripulante);
		}
		pthread_mutex_unlock(patota->mutex_segmentos_tripulantes);
	}
	pthread_mutex_unlock(&mutex_tabla_patotas);
	pthread_mutex_unlock(&mutex_memoria);
	
}

void sighandlerLiberarMemoria(int signum){
	log_info(logger_ram_hq,"Llego la señal para evacuar el modulo");
	//nivel_gui_terminar();
	//nivel_destruir(nivel);
	
	if(!strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
		explotar_la_nave();
	else
		explotar_la_nave_segmentada();
}


void explotar_la_nave(){
	log_info(logger_ram_hq,"¡Los reactores de la nave se sobrecalentaron y estan a punto de explotar! \n Liberando todos los recursos..");
	pthread_mutex_destroy(&mutex_memoria);
	pthread_mutex_destroy(&mutex_swap);
	pthread_mutex_destroy(&mutex_iniciar_patota);
	pthread_mutex_destroy(&mutex_tabla_patotas);
	pthread_mutex_destroy(&mutex_tabla_de_segmentos);
	pthread_mutex_destroy(&mutex_busqueda_patota);
	while(patotas->elements_count){
		t_tabla_de_paginas* auxiliar_patota = list_get(patotas,0);
		while(auxiliar_patota->paginas){
			t_pagina* auxiliar_pagina = list_get(auxiliar_patota->paginas,0);
			pthread_mutex_destroy(auxiliar_pagina->mutex_pagina);
			free(auxiliar_pagina->mutex_pagina);
			list_remove(auxiliar_patota->paginas,0);
		}
		list_destroy(auxiliar_patota->paginas);
		while(auxiliar_patota->tareas->elements_count){
			tarea_ram* auxiliar_tarea = list_get(auxiliar_patota->tareas,0);
			free(auxiliar_tarea->tarea);
			list_remove(auxiliar_patota->tareas,0);
		}
		list_destroy(auxiliar_patota->tareas);
		pthread_mutex_destroy(auxiliar_patota->mutex_tabla_paginas);
		free(auxiliar_patota->mutex_tabla_paginas);
		pthread_mutex_destroy(auxiliar_patota->mutex_tripulantes_vivos);
		free(auxiliar_patota->mutex_tripulantes_vivos);
		
		int indice_patota = obtener_indice_patota(auxiliar_patota->id_patota);
		list_remove(patotas,indice_patota);
		free(auxiliar_patota);
	}
	list_destroy(patotas);
	while(frames->elements_count){
		t_frame_en_memoria* auxiliar_frame = list_get(frames,0);
		pthread_mutex_destroy(auxiliar_frame->mutex);
		free(auxiliar_frame->mutex);
		list_remove(frames,0);
		free(auxiliar_frame);
	}
	list_destroy(frames);
	while(frames_swap->elements_count){
		t_frame_en_swap* auxiliar = list_get(frames_swap,0);
		pthread_mutex_destroy(auxiliar->mutex);
		free(auxiliar->mutex);
		list_remove(frames_swap,0);
		free(auxiliar);
	}
	list_destroy(frames_swap);
	pthread_mutex_lock(&mutex_lru);
	while(historial_uso_paginas->elements_count){
		t_pagina_y_frame* auxiliar_historial = list_get(historial_uso_paginas,0);
		list_remove(historial_uso_paginas,0);
		free(auxiliar_historial);
	}
	pthread_mutex_unlock(&mutex_lru);
	list_destroy(historial_uso_paginas);
	free(memoria_principal);
	munmap(memoria_swap,mi_ram_hq_configuracion->TAMANIO_SWAP);
	config_destroy(config_aux);
	free(mi_ram_hq_configuracion);
	log_destroy(logger_ram_hq);
	list_destroy(segmentos_memoria);
	exit(0);
}


void crear_mapa (){
	
	nivel_gui_inicializar();


	cols = 10;
	rows = 10;

	nivel_gui_get_area_nivel(&cols, &rows);
	
	nivel = nivel_crear("Nave");

	nivel_gui_dibujar(nivel);
}

//actualizar_ubicacion
void mover_tripulante_mapa (char simbolo,direccion dir){
	pthread_mutex_lock(&mutex_mover_trip);
	switch(dir) {
		case ABAJO:
			item_desplazar(nivel, simbolo, 0, -1);
		break;

		case ARRIBA:
			item_desplazar(nivel, simbolo, 0, 1);
		break;

		case IZQUIERDA:
			item_desplazar(nivel, simbolo, -1, 0);
		break;

		case DERECHA:
			item_desplazar(nivel, simbolo, 1, 0);
		break;
		
	}
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mover_trip);
}

//iniciar_patota
void crear_tripulante_mapa (uint32_t tid,uint32_t x,uint32_t y){
	pthread_mutex_lock(&mutex_mover_trip);
	personaje_crear(nivel, 'a'+tid-1, x, y);
	//ver si se puede sacar este assert y asi no tira ese warning
	//ASSERT_CREATE(nivel, 'a'+tid-1, trip_mem);
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mover_trip);
	return;
}

char obtener_caracter_mapa (uint32_t tid){
	char ret = ('a'+tid-1);
	return ret;
}

direccion obtener_direccion_movimiento_mapa(uint32_t pos_nueva_x,uint32_t pos_nueva_y,uint32_t pos_vieja_x,uint32_t pos_vieja_y){

	direccion direc;
	if(pos_vieja_x == pos_nueva_x){
		//posicion nueva es menor a la vieja en y -> ABAJO
		if(pos_nueva_y < pos_vieja_y)
			direc = ABAJO;
		//posicion nueva es mayor a la vieja en y -> ARRIBA
		else
			direc = ARRIBA;
	}
	else{
		//posicion nueva es menor a la vieja en x -> IZQUIERDA
		if(pos_nueva_x < pos_vieja_x)
			direc = IZQUIERDA;
		//posicion nueva es mayor a la vieja en x -> DERECHA
		else
			direc = DERECHA;
	}
	return direc;
}


void explotar_la_nave_segmentada(){
	pthread_mutex_lock(&mutex_iniciar_patota);
	pthread_mutex_lock(&mutex_memoria);
	pthread_mutex_lock(&mutex_tabla_de_segmentos);
	pthread_mutex_lock(&mutex_tabla_patotas);
	pthread_mutex_destroy(&mutex_iniciar_patota);
	pthread_mutex_destroy(&mutex_memoria);
	pthread_mutex_destroy(&mutex_tabla_de_segmentos);
	pthread_mutex_destroy(&mutex_tabla_patotas);
	
	while(patotas->elements_count){
		t_segmentos_de_patota* patota = list_get(patotas,0);
		pthread_mutex_destroy(patota->mutex_segmentos_tripulantes);
		free(patota->mutex_segmentos_tripulantes);
		list_destroy(patota->segmentos_tripulantes);
		free(patota);
		list_remove(patotas,0);
	}
	list_destroy(patotas);

	while(segmentos_memoria->elements_count){
		t_segmento* segmento = list_get(segmentos_memoria,0);
		pthread_mutex_destroy(segmento->mutex_segmento);
		free(segmento->mutex_segmento);
		free(segmento);
		list_remove(segmentos_memoria,0);
	}
	list_destroy(segmentos_memoria);
	
	config_destroy(config_aux);

	free(memoria_principal);
	free(mi_ram_hq_configuracion);
	log_destroy(logger_ram_hq);
	//config mallockeada en leer_config_mi_ram_hq --> config_mi_ram_hq_aux


	//seguro tengo que liberar cosas del mapa

	exit(0);
}

void borrar_patota(uint32_t pid){
	pthread_mutex_lock(&mutex_tabla_patotas);
	for(int i=0;i<patotas->elements_count;i++){
		t_segmentos_de_patota* auxiliar_patota = list_get(patotas,i);
		uint32_t pid_aux = 0;
		pthread_mutex_lock(&mutex_memoria);
		pthread_mutex_lock(auxiliar_patota->segmento_pcb->mutex_segmento);
		memcpy(&pid_aux,auxiliar_patota->segmento_pcb->inicio_segmento,sizeof(uint32_t));
		pthread_mutex_unlock(auxiliar_patota->segmento_pcb->mutex_segmento);
		pthread_mutex_unlock(&mutex_memoria);

		if(pid_aux == pid){
			list_remove(patotas,i);
			pthread_mutex_unlock(&mutex_tabla_patotas);
			return;
		}

	}
	
	pthread_mutex_unlock(&mutex_tabla_patotas);
	
}
