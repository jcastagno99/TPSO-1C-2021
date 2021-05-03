#include "mi-ram-hq-lib.h"
#include <stdlib.h>


mi_ram_hq_config* leer_config_mi_ram_hq(char* path){
    t_config* config_aux = config_create(path);
    mi_ram_hq_config* config_mi_ram_hq_aux = malloc(sizeof(mi_ram_hq_config));

    config_mi_ram_hq_aux->TAMANIO_MEMORIA = config_get_int_value(config_aux,"TAMANIO_MEMORIA");
    config_mi_ram_hq_aux->ESQUEMA_MEMORIA = config_get_string_value(config_aux,"ESQUEMA_MEMORIA");
    config_mi_ram_hq_aux->TAMANIO_PAGINA = config_get_int_value(config_aux,"TAMANIO_PAGINA");
    config_mi_ram_hq_aux->TAMANIO_SWAP = config_get_int_value(config_aux,"TAMANIO_SWAP");
    config_mi_ram_hq_aux->PATH_SWAP = config_get_string_value(config_aux,"PATH_SWAP");
    config_mi_ram_hq_aux->ALGORITMO_REEMPLAZO = config_get_string_value(config_aux,"ALGORITMO_REEMPLAZO");
    config_mi_ram_hq_aux->PUERTO = config_get_int_value(config_aux,"PUERTO");

    return config_mi_ram_hq_aux;

}


t_log* iniciar_logger_mi_ram_hq(char* path){

    t_log* log_aux = log_create(path,"MI-RAM-HQ",1,LOG_LEVEL_INFO);
    return log_aux;
}

void* esperar_conexion(int puerto){
    t_list* hilos = list_create();
    int socket_escucha = iniciar_servidor_mi_ram_hq(puerto);
    int socket_mi_ram_hq;
        log_info(logger_ram_hq,"SERVIDOR LEVANTADO! ESCUCHANDO EN %i",puerto);
    	struct sockaddr cliente;
			socklen_t len = sizeof(cliente);
			do {
				socket_mi_ram_hq = accept(socket_escucha, &cliente, &len);
				if (socket_mi_ram_hq > 0) {
					log_info(logger_ram_hq, "NUEVA CONEXIÓN!");
					crear_hilo_para_manejar_suscripciones(hilos, socket_mi_ram_hq);
				} else {
					log_error(logger_ram_hq, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
				}
			} while (1);
}

int iniciar_servidor_mi_ram_hq(int puerto){
	int  socket_v;
	int val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr =INADDR_ANY;
	servaddr.sin_port = htons(puerto);

	socket_v = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_v < 0) {
		char* error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket_v, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket_v,(struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		return EXIT_FAILURE;
	}
	if (listen(socket_v, MAX_CLIENTS)< 0) {
		return EXIT_FAILURE;
	}

	return socket_v;
}

void crear_hilo_para_manejar_suscripciones(t_list* lista_hilos, int socket){
	int* socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_create(&hilo_conectado , NULL, (void*) manejar_suscripciones_mi_ram_hq, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);
}

void* manejar_suscripciones_mi_ram_hq(int* socket_hilo){
	log_info(logger_ram_hq,"Se creó el hilo correctamente");
	t_paquete* paquete = recibir_paquete(*socket_hilo);
	switch(paquete->codigo_operacion){
		case INICIALIZAR_TRIPULANTE: {
			break;
		}
		case ACTUALIZAR_UBICACION: {
			break;
		}
		case OBTENER_ESTADO: {
			break;
		}
		case OBTENER_UBICACION: {
			break;
		}
		case PRUEBA:{
			char* mensaje_prueba = deserializar_prueba(paquete->stream);
			log_info(logger_ram_hq,"Me llego el mensaje: %s ",mensaje_prueba);
			free(mensaje_prueba);
			break;
		}
		default: {
			break;
		}
		// faltan contemplar algunos cases
	}
    close(*socket_hilo);
	free(socket_hilo);
	liberar_paquete(paquete);
	return NULL;
}
