
#ifndef MI_RAM_HQ_LIB_H
#define MI_RAM_HQ_LIB_H

#include <math.h>
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
#include <commons/temporal.h>
#include <shared_utils.h>
#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>


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
	char* CRITERIO_SELECCION;
	int PUERTO;
	
}mi_ram_hq_config;

//t_list * tabla_de_segmentos
typedef struct
{
	bool libre;
	void* inicio_segmento;
	uint32_t tamanio_segmento;
	uint32_t numero_segmento;
	pthread_mutex_t* mutex_segmento;

}t_segmento; //segmento

//t_list * patotas 
typedef struct
{
	t_segmento * segmento_pcb;
	t_segmento* segmento_tarea;
	t_list* segmentos_tripulantes;
	pthread_mutex_t* mutex_segmentos_tripulantes;

}t_segmentos_de_patota; //patota

typedef struct{

	void* inicio_memoria;
	void* inicio_swap;
	uint32_t id_pagina;
	uint8_t presente;
	uint8_t fue_modificada;
	uint8_t uso;
	pthread_mutex_t* mutex_pagina;

}t_pagina;

typedef struct{

	t_list* paginas;
	t_list* tareas;
	int cantidad_tripulantes;
	int id_patota;
	pthread_mutex_t* mutex_tabla_paginas;

}t_tabla_de_paginas;

typedef struct{
	t_pagina* pagina;
	int indice;
	int offset;

}inicio_tcb;

typedef struct{

	void* inicio;
	t_pagina* pagina_a_la_que_pertenece;
	pthread_mutex_t* mutex;
	bool libre;
	uint32_t pid_duenio;
	uint32_t indice_pagina; 

}t_frame_en_memoria;

typedef struct{
	t_pagina* pagina;
	t_frame_en_memoria* frame;

}t_pagina_y_frame;

//------------Variables Globales------------

mi_ram_hq_config* mi_ram_hq_configuracion;
t_log* logger_ram_hq;
int socket_escucha;
void* memoria_principal;
void* memoria_swap;

t_list* patotas;

pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_tabla_patotas;
pthread_mutex_t mutex_tabla_de_segmentos;
pthread_mutex_t mutex_iniciar_patota;
pthread_mutex_t mutex_swap;

t_list* historial_uso_paginas;
int puntero_lista_frames_clock;

t_list* segmentos_memoria;

uint32_t numero_segmento_global;

t_list* frames;
int offset_swap;

//------------Firmas de funciones------------

mi_ram_hq_config* leer_config_mi_ram_hq(char*);
t_log* iniciar_logger_mi_ram_hq(char*);
void crear_estructuras_administrativas();

void* esperar_conexion(int);
int iniciar_servidor_mi_ram_hq(int);
void crear_hilo_para_manejar_suscripciones(t_list*,int);
void* manejar_suscripciones_mi_ram_hq(int*);

// Funciones principales del manejo de mensajes
respuesta_ok_fail iniciar_patota_segmentacion(pid_con_tareas_y_tripulantes_miriam,int);
respuesta_ok_fail iniciar_patota_paginacion(patota_stream_paginacion);
respuesta_ok_fail actualizar_ubicacion_segmentacion(tripulante_y_posicion,int);
respuesta_ok_fail actualizar_ubicacion_paginacion(tripulante_y_posicion);
char* obtener_proxima_tarea_segmentacion(uint32_t,int);
char* obtener_proxima_tarea_paginacion(uint32_t);
respuesta_ok_fail expulsar_tripulante_segmentacion(uint32_t,int);
respuesta_ok_fail expulsar_tripulante_paginacion(uint32_t);

//Busqueda de datos en memoria
t_segmentos_de_patota* buscar_patota(uint32_t);
t_tabla_de_paginas* buscar_patota_paginacion(uint32_t);
t_tabla_de_paginas* buscar_patota_con_tid_paginacion(uint32_t);
t_segmentos_de_patota* buscar_patota_con_tid(uint32_t ,int);
uint32_t obtener_patota_memoria(t_segmento*);
char * obtener_proxima_tarea(char *,uint32_t,uint32_t);

//Buscar segmentos libres en memoria
t_segmento* buscar_segmento_tareas(uint32_t);
t_segmento* buscar_segmento_pcb();
t_segmento* buscar_segmento_tcb();

//Buscar paginas en memoria
t_list* buscar_cantidad_frames_libres(int);
inicio_tcb* buscar_inicio_tcb(uint32_t,t_tabla_de_paginas*,double, int);

//Buscar frames
t_frame_en_memoria* buscar_frame_libre();
t_list* buscar_cantidad_frames_libres(int);
int buscar_frame_y_pagina_con_tid_y_pid(int,int);


//Escribir en memoria un segmento
void cargar_pcb_en_segmento(uint32_t,uint32_t,t_segmento*);
void cargar_tareas_en_segmento(char* ,uint32_t, t_segmento* );
void cargar_tcb_en_segmento(uint32_t,estado,uint32_t,uint32_t,uint32_t,uint32_t,t_segmento*);
void cargar_tcb_sinPid_en_segmento(nuevo_tripulante_sin_pid*,t_segmento*, uint32_t);

//Escribir datos en paginacion
void escribir_un_uint32_a_partir_de_un_indice(double,int,uint32_t,t_tabla_de_paginas*);
void escribir_un_char_a_partir_de_indice(double,int,char,t_tabla_de_paginas*);

//Actualizar datos en memoria
void actualizarTareaActual(t_segmentos_de_patota* ,uint32_t,int );
respuesta_ok_fail actualizar_estado_segmentacion(uint32_t ,estado ,int);
respuesta_ok_fail actualizar_estado_paginacion(uint32_t, estado, int);


//Compactacion
void compactar_memoria();
bool ordenar_direcciones_de_memoria(void* p1, void* p2);

//Swap
void inicializar_swap();
t_frame_en_memoria* sustituir_LRU();
t_frame_en_memoria* sustituir_CLOCK();
void actualizar_pagina(t_pagina*);
t_frame_en_memoria* iterar_clock_sobre_frames(int);


//Otros paginacion
int minimo_entre(int, int);
void escribir_un_uint32_a_partir_de_indice(double,int,uint32_t,t_tabla_de_paginas*);

//Otros segmentacion
uint32_t calcular_memoria_libre(void);
void swap(t_segmento **,t_segmento **);

//Otros compartidos
char obtener_char_estado (estado);

//Funciones del dump
void imprimir_dump(void);
void recorrer_pcb_dump(t_segmento*);
void recorrer_tareas_dump(uint32_t,t_segmento*);
void recorrer_tcb_dump(uint32_t,t_list*);
void imprimir_dump_paginacion(void);



//Funciones armadas para debuggear
void funcion_test_memoria(uint32_t);
void recorrer_pcb(t_segmento * );
void recorrer_tareas(t_segmento * );
void recorrer_tcb(t_list * );

#endif /* MI_RAM_HQ_LIB_H */




