
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
#include <shared_utils.h>

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

typedef struct
{
	uint32_t pid;
	uint32_t direccion_tareas;
	t_list* segmentos;
	pthread_mutex_t mutex_segmentos;

}t_tabla_de_segmento;

typedef struct
{
	void* base;
	void* offset;
	t_tripulante_en_memoria* tripulante;

}t_segmento;

typedef struct
{
	uint32_t tid;
	char estado;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t proxima_instruccion;
	uint32_t direccion_pcb;

}t_tripulante_en_memoria;

//------------Variables Globales------------

mi_ram_hq_config* mi_ram_hq_configuracion;
t_log* logger_ram_hq;
int socket_escucha;
void* memoria_principal;
void* memoria_swap;

t_list* patotas;
pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_swap;


//------------Firmas de funciones------------

mi_ram_hq_config* leer_config_mi_ram_hq(char*);
t_log* iniciar_logger_mi_ram_hq(char*);
void crear_estructura_administrativa();

void* esperar_conexion(int);
int iniciar_servidor_mi_ram_hq(int);
void crear_hilo_para_manejar_suscripciones(t_list*,int);
void* manejar_suscripciones_mi_ram_hq(int*);


respuesta_ok_fail iniciar_patota(pid_con_tareas);
respuesta_ok_fail iniciar_tripulante(nuevo_tripulante);
respuesta_ok_fail actualizar_ubicacion(tripulante_y_posicion);
tarea obtener_proxima_tarea(uint32_t);
respuesta_ok_fail expulsar_tripulante(uint32_t);
estado obtener_estado(uint32_t);
posicion obtener_ubicacion(uint32_t);




#endif /* MI_RAM_HQ_LIB_H */
