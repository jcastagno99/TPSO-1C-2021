#include "mi-ram-hq-lib.h"
#include <stdlib.h>

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
	t_paquete *paquete = recibir_paquete(*socket_hilo);
	if (strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		switch (paquete->codigo_operacion)
		{
		case INICIAR_PATOTA:
		{
			pid_con_tareas patota_con_tareas = deserializar_pid_con_tareas(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de INICIAR_PATOTA
			respuesta_ok_fail resultado = iniciar_patota_segmentacion(patota_con_tareas);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_PATOTA, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case INICIAR_TRIPULANTE:
		{
			nuevo_tripulante nuevo_tripulante = deserializar_nuevo_tripulante(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de INICIAR_TRIPULANTE
			respuesta_ok_fail resultado = iniciar_tripulante_segmentacion(nuevo_tripulante);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_INICIAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case ACTUALIZAR_UBICACION:
		{
			tripulante_y_posicion tripulante_y_posicion = deserializar_tripulante_y_posicion(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de ACTUALIZAR_UBICACION
			respuesta_ok_fail resultado = actualizar_ubicacion_segmentacion(tripulante_y_posicion);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_ACTUALIZAR_UBICACION, sizeof(respuesta_ok_fail), respuesta);
			break;
		}
		case OBTENER_PROXIMA_TAREA:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_PROXIMA_TAREA
			tarea proxima_tarea = obtener_proxima_tarea_segmentacion(tripulante_pid); //Aca podemos agregar un control, tal vez si falla enviar respuesta_ok_fail...
			void *respuesta = serializar_tarea(proxima_tarea);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_PROXIMA_TAREA, sizeof(tarea), respuesta);
			break;
		}
		case EXPULSAR_TRIPULANTE:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_PROXIMA_TAREA
			respuesta_ok_fail resultado = expulsar_tripulante_segmentacion(tripulante_pid);
			void *respuesta = serializar_respuesta_ok_fail(resultado);
			enviar_paquete(*socket_hilo, RESPUESTA_EXPULSAR_TRIPULANTE, sizeof(respuesta_ok_fail), respuesta);
		}
		case OBTENER_ESTADO:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			estado estado = obtener_estado_segmentacion(tripulante_pid);
			void *respuesta = serializar_estado(estado);
			enviar_paquete(*socket_hilo, RESPUESTA_OBTENER_ESTADO, sizeof(estado), respuesta);
			break;
		}
		case OBTENER_UBICACION:
		{
			uint32_t tripulante_pid = deserializar_pid(paquete->stream);
			//TODO : Armar la funcion que contiene la logica de OBTENER_UBICACION
			posicion posicion = obtener_ubicacion_segmentacion(tripulante_pid);
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
	else if (strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
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

	return NULL;
}

void crear_estructuras_administrativa()
{
	patotas = list_create();
	mutex_init(&mutex_patotas,NULL);
	mutex_init(&mutex_memoria,NULL);
	mutex_init(&mutex_swap,NULL);

	if (strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
		numero_segmento_global = 0;
		t_segmento_de_memoria* segmento;
		segmento->inicio_segmento = memoria_principal;
		segmento->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		segmento->libre = true;
		list_add(segmentos_memoria,segmento);
		
	}
	else if (strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "PAGINACION"))
	{
		//crear_estructuras_administrativas_paginacion;
	}
	else
	{
		log_error(logger_ram_hq, "El esquema de memoria no es soportado por el sistema");
	}
}

//------------------------------------------MANEJO DE LOGICA DE MENSAJES-------------------------------------

//Falta hacer la funcion de compactacion
//Falta eliminar el elemento de la t_list cuando la operacion se rechace, necesito hacer funciones iterativas para encontrar el indice y usar list_remove...
//Es necesario "compactar" segmentos libres contiguos o puedo delegarlo a cuando se llame al algoritmo de compactacion general

respuesta_ok_fail iniciar_patota_segmentacion(pid_con_tareas patota_con_tareas)
{
	pthread_mutex_lock(&mutex_patotas);
	t_tabla_de_segmento* patota = buscar_patota(patota_con_tareas.pid);
	if(patota){
		pthread_mutex_unloc(&mutex_patotas);
		log_error(logger_ram_hq,"La patota de pid %i ya existe, solicitud rechazada",patota_con_tareas.pid);
		list_destroy_and_destroy_elements(patota_con_tareas.tareas,free);
		return RESPUESTA_FAIL;
	}
	pthread_mutex_unloc(&mutex_patotas);
	
	patota = malloc(sizeof(t_tabla_de_segmento));
	patota->segmento_pcb = NULL;
	patota->segmento_tarea = NULL;
	patota->segmentos_tripulantes = list_create();
	patota->mutex_segmentos_tripulantes = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(patota->mutex_segmentos_tripulantes,NULL);
	pthread_mutex_lock(&mutex_patotas);
	list_add(patotas,patota);
	pthread_mutex_unlock(&mutex_patotas);

	pthread_mutex_lock(&mutex_memoria);
	t_segmento_de_memoria* segmento_a_usar_pcb = buscar_segmento_pcb();
	if(!segmento_a_usar_pcb){
		//TODO: compactar
		log_info(logger_ram_hq,"No encontre un segmento disponible para el PCB de PID: &i , voy a compactar",patota_con_tareas.pid);
		segmento_a_usar_pcb = buscar_segmento_pcb();
		if(!segmento_a_usar_pcb){
			pthread_mutex_unloc(&mutex_memoria);
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar el PCB aun luego de compactar, solicitud rechazada");
			list_destroy_and_destroy_elements(patota_con_tareas.tareas,free);
			free(patota->mutex_segmentos_tripulantes);
			free(patota);
			//Eliminar el elemento de la lista, necesito su indice...
			return RESPUESTA_FAIL;
		}
	}
	pthread_mutex_unloc(&mutex_memoria);

	log_info(logger_ram_hq,"Encontre un segmento para el PCB de PID: &i , empieza en: &i ",patota_con_tareas.pid,(int) segmento_a_usar_pcb->inicio_segmento); 
	t_segmento* segmento_pcb = malloc(sizeof(t_segmento));
	segmento_pcb->base = segmento_a_usar_pcb->inicio_segmento;
	segmento_pcb->tamanio = segmento_a_usar_pcb->tamanio_segmento;
	segmento_pcb->numero_segmento = numero_segmento_global;
	numero_segmento_global++;
	segmento_pcb->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(segmento_pcb->mutex_segmento,NULL);
	patota->segmento_pcb = segmento_pcb;
	
	pthread_mutex_lock(&mutex_memoria);
	t_segmento_de_memoria* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas.tareas); 
	if(!segmento_a_usar_tareas){
		log_info(logger_ram_hq,"No encontre un segmento disponible para las tareas, voy a compactar");
		//TODO: compactar
		t_segmento_de_memoria* segmento_a_usar_tareas = buscar_segmento_tareas(patota_con_tareas.tareas); 
		if(!segmento_a_usar_tareas){
			pthread_mutex_unlock(&mutex_memoria);
			log_error(logger_ram_hq,"No hay espacio en la memoria para almacenar las tareas aun luego de compactar, solicitud rechazada");
			free(segmento_pcb->mutex_segmento);
			free(segmento_pcb);
			list_remove_and_destroy_elements(patota->segmentos_tripulantes,free);
			free(patota->mutex_segmentos_tripulantes);
			free(patota);
			segmento_a_usar_pcb->libre = true; // Necesario compactar con los segmentos contiguos?
			return RESPUESTA_FAIL;
		}
	}
	pthread_mutex_unlock(&mutex_memoria);

	log_info(logger_ram_hq,"Encontre un segmento para las tareas, empieza en: &i ",(int) segmento_a_usar_pcb->inicio_segmento);
	t_segmento* segmento_tarea = malloc(sizeof(t_segmento));
	segmento_tarea->base = segmento_a_usar_tareas->inicio_segmento;
	segmento_tarea->tamanio = segmento_a_usar_tareas->tamanio_segmento;
	segmento_tarea->numero_segmento = numero_segmento_global;
	numero_segmento_global++;
	segmento_tarea->mutex_segmento = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_initi(segmento_tarea->mutex_segmento);
	patota->segmento_tarea = segmento_tarea;
	log_info(logger_ram_hq,"Las estructuras administrativas fueron creadas y los segmentos asignados, procedo a cargar la informacion a memoria");

	uint32_t direccion_logica_tareas = 0; //TODO: Calcular la direccion logica real
	//Los semaforos se toman dentro de las funciones:
	cargar_pcb_en_segmento(patota_con_tareas.pid,direccion_logica_tareas,segmento_pcb);
	cargar_tareas_en_segmento(patota_con_tareas.tareas,segmento_tarea);

	return RESPUESTA_OK;
}

respuesta_ok_fail iniciar_tripulante(nuevo_tripulante tripulante)
{
	respuesta_ok_fail respuesta;
	//TODO
	respuesta = RESPUESTA_FAIL;
	return respuesta;
}
respuesta_ok_fail actualizar_ubicacion(tripulante_y_posicion tripulante_con_posicion)
{
	respuesta_ok_fail respuesta;
	//TODO
	return respuesta;
}
tarea obtener_proxima_tarea(uint32_t tripulante_pid)
{
	tarea proxima_tarea;
	return proxima_tarea;
}
respuesta_ok_fail expulsar_tripulante(uint32_t tripulante_pid)
{
	respuesta_ok_fail respuesta;
	//TODO
	return respuesta;
}
estado obtener_estado(uint32_t tripulante_pid)
{
	estado estado_obtenido;
	//TODO
	return estado_obtenido;
}
posicion obtener_ubicacion(uint32_t tripulante_pid)
{
	posicion ubicacion_obtenida;
	//TODO
	return ubicacion_obtenida;
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

//TODO : ACTUALIZAR LOS CONDICIONALES CUANDO SE ACTUALIZE EL ARCHIVO DE CONFIGURACIÓN CON EL ALGORITMO DE SEGMENTACIÓN (BF/FF)

t_segmento_de_memoria* buscar_segmento_pcb(){
	t_segmento_de_memoria* iterador;
	t_segmento_de_memoria* auxiliar;
	if(strcmp("mi_ram_hq_configuracion->ALGORITMO_DE_SEGMENTACION","FIRST_FIT")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			iterador = list_get(segmentos_memoria,i);
			if(iterador->tamanio_segmento >= 2*(sizeof(uint32_t)) && iterador->libre){
				auxiliar->inicio_segmento = iterador->inicio_segmento;
				auxiliar->tamanio_segmento = 2*(sizeof(uint32_t)); 
				auxiliar->libre = false;
				iterador->inicio_segmento += 2*(sizeof(uint32_t));
				iterador->tamanio_segmento -= 2*(sizeof(uint32_t));
				return auxiliar;
			}
		}
	}
	else if(strcmp("mi_ram_hq_configuracion->ALGORITMO_DE_SEGMENTACION","BEST_FIT")){ 
		t_segmento_de_memoria* vencedor;
		vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			auxiliar = list_get(segmentos_memoria,i);
			if((auxiliar->tamanio_segmento >= 2*(sizeof(uint32_t))) && (auxiliar->tamanio_segmento < vencedor->tamanio_segmento) && auxiliar->libre){
				auxiliar->libre = false;
				vencedor = auxiliar;
			}
		}
		return vencedor;
	}
	return NULL;
};

t_segmento_de_memoria* buscar_segmento_tareas(t_list* tareas){ //Las tareas son unos t_list, donde cada miembro es un char* 
	uint32_t tamanio_tareas = 0;
	char* auxiliar_tarea;
	for(int i=0; i<tareas->elements_count; i++){
		auxiliar_tarea = list_get(tareas,i);
		tamanio_tareas += strlen(auxiliar_tarea) + 1;
	}
	t_segmento_de_memoria* auxiliar;
	if(strcmp("mi_ram_hq_configuracion->ALGORITMO_DE_SEGMENTACION","FIRST_FIT")){
		for(int i=0; i<segmentos_memoria->elements_count;i++){
			auxiliar = list_get(segmentos_memoria,i);
			if(auxiliar->tamanio_segmento >= tamanio_tareas && auxiliar->libre){
				auxiliar->libre = false;
				return auxiliar;
			}
		}
	}
	else if(strcmp("mi_ram_hq_configuracion->ALGORITMO_DE_SEGMENTACION","BEST_FIT")){
		t_segmento_de_memoria* vencedor;
		vencedor->tamanio_segmento = mi_ram_hq_configuracion->TAMANIO_MEMORIA;
		for(int i=0;i<segmentos_memoria->elements_count;i++){
			auxiliar = list_get(segmentos_memoria,i);
			if((auxiliar->tamanio_segmento >= tamanio_tareas) && (auxiliar->tamanio_segmento < vencedor->tamanio_segmento) && auxiliar->libre){
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
	memcpy(segmento->base + offset, pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(segmento->base + offset, direccion_logica_tareas, sizeof(uint32_t));
	pthread_mutex_unlock(segmento->mutex_segmento);
}

//Aclaración: Esta función asume que las tareas incluyen un /0 al final de la cadena de char*
void cargar_tareas_en_segmento(t_list* tareas, t_segmento* segmento){
	pthread_mutex_lock(segmento->mutex_segmento);
	int offset = 0;
	char* auxiliar;
	for(int i=0; i<tareas->elements_count; i++){
		auxiliar = list_get(tareas,i);
		memcpy(segmento->base + offset, auxiliar, strlen(auxiliar) + 1);
		offset = strlen(auxiliar) + 1;
	}
	pthread_mutex_unlock(segmento->mutex_segmento);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------
