
#ifndef MI_RAM_HQ_LIB_H
#define MI_RAM_HQ_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>

#define MAX_CLIENTS 128


//------------Estructuras------------

typedef struct 
{
	int TAMANIO_MEMORIA;
	char* ESQUEMA_MEMORIA;
	int TAMANIO_PAGINA;
	int TAMANIO_SWAP;
	char* PATH_SWAP;
	char* ALGORITMO_REEMPLAZO;
	int PUERTO;
	
}mi_ram_hq_config;

//------------Variables Globales------------

mi_ram_hq_config* mi_ram_hq_configuracion;
t_log* logger_ram_hq;
int socket_escucha;
void* memoria_principal;
void* memoria_swap;


//------------Firmas de funciones------------

mi_ram_hq_config* leer_config_mi_ram_hq(char*);
t_log* iniciar_logger_mi_ram_hq(char*);

void* esperar_conexion(int);
int iniciar_servidor_mi_ram_hq(int);
void crear_hilo_para_manejar_suscripciones(t_list*,int);
void* manejar_suscripciones_mi_ram_hq(int*);


#endif /* MI_RAM_HQ_LIB_H */
