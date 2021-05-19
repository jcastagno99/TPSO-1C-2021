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

void crear_estructura_administrativa()
{
	patotas = list_create();
	if (strcmp(mi_ram_hq_configuracion->ESQUEMA_MEMORIA, "SEGMENTACION"))
	{
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

respuesta_ok_fail iniciar_patota(pid_con_tareas patota_con_tareas)
{
	respuesta_ok_fail respuesta = RESPUESTA_OK;
	//Es necesario bloquear la lista de patotas? no se modifican luego de inicializarlas
	t_tabla_de_segmento* patota = buscar_patota(patota_con_tareas.pid); //TODO
	if(patota){
		log_error(logger_ram_hq,"La patota de pid %i ya existe",patota_con_tareas.pid);
		respuesta = RESPUESTA_FAIL;
	}
	//TODO: usar un semaforo
	t_segmento_de_memoria* segmento_a_usar = buscar_segmento_para(patota_con_tareas.pid); //En un inicio first fit, cuando actualicen enunciado lo modifico
	if(!segmento_a_usar){
		//TODO: compactar
		segmento_a_usar = buscar_segmento_para(patota_con_tareas.pid); //Esperar respuesta a consulta en github
	}
	return respuesta;
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
