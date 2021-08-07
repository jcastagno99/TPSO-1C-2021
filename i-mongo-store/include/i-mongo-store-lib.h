
#ifndef I_MONGO_STORE_LIB_H
#define I_MONGO_STORE_LIB_H
#define _GNU_SOURCE
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
#include <fcntl.h>
#include <shared_utils.h>
#include <ctype.h>
#include <dirent.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#define MAX_CLIENTS 128


//------------Estructuras------------

typedef struct 
{
	char* PUNTO_MONTAJE;
	int PUERTO;
	int TIEMPO_SINCRONIZACION;
	char* IP_DISCORDIADOR;
	int PUERTO_DISCORDIADOR;
	t_list* POSICIONES_SABOTAJE;
	int INDICE_ULTIMO_SABOTAJE;
}i_mongo_store_config;

typedef enum{
	RECURSO, BITACORA
}tipo_archivo;

typedef enum
{
	GENERAR,
	CONSUMIR,
	DESCARTAR,
	SABOTEAR,
	OTRA
}operaciones;

typedef struct{
	uint32_t tid;
	pthread_mutex_t* mutex_bitacora;
}bitacora_trip_mutex;

//------------Variables Globales------------

i_mongo_store_config* i_mongo_store_configuracion;
t_log* logger_i_mongo_store;
int socket_escucha;
char* ruta_superbloque;
char* ruta_blocks;
char* carpeta_files;
char* carpeta_bitacoras;
char* carpeta_md5;
char* ruta_info_blocks;
char* ruta_info_blocks_aux;
void* superbloque;
void* blocks;
void* blocks_mapeado;
pthread_mutex_t superbloque_bitarray_mutex;
int fd_bloques;
int fd_superbloques;
t_list* mutex_bitacoras;
pthread_mutex_t mutex_lista_bitacoras;
t_list* lista_hilos;
uint32_t* tamanios_global;
pthread_mutex_t mutex_oxigeno;
pthread_mutex_t mutex_comida;
pthread_mutex_t mutex_basura;
//------------Firmas de funciones------------

i_mongo_store_config* leer_config_i_mongo_store(char*);
t_log* iniciar_logger_i_mongo_store(char*);

void* esperar_conexion(int);
int iniciar_servidor_i_mongo_store(int);
void crear_hilo_para_manejar_suscripciones(t_list*,int);
void* manejar_suscripciones_i_mongo_store(int*);

//----------------Filesystem----------------
void inicializar_filesystem(char*);
void inicializar_superbloque(uint32_t,uint32_t);
void inicializar_blocks(uint32_t,uint32_t);
bool crear_directorio(char* carpeta);
void no_pude_abrir_archivo(char*);
void no_pude_mapear_archivo(char*);
void inicializar_rutas(char* montaje);
bool existe_archivo(char* archivo);
int escribir_archivo(char* archivo, char * escritura, tipo_archivo tipo, uint32_t tid);
int quitar_de_archivo(char* archivo,char* escritura);
int borrar_archivo(char* archivo);
int registrar_comienzo_tarea(char* tarea, uint32_t tid);
int consumir_recurso(char* recurso, int cantidad);
int generar_recurso(char* recurso, int cantidad,uint32_t tid);
int descartar_recurso(char* recurso);
char* todo_el_archivo(char* archivo, uint32_t tid);
void crear_archivo_metadata(char* ruta, tipo_archivo tipo, char caracter_llenado);
void liberar_bloque(int numero_bloque);
void ocupar_bloque(int numero_bloque);
int get_block_size();
int get_block_amount();
int get_primer_bloque_libre();
char* setear_nuevos_blocks(t_config* config, int cant_bloques_actual);
void inicializar_superbloque_existente(uint32_t* block_size_ref, uint32_t* block_amount_ref);
void inicializar_blocks_existente(uint32_t block_size, uint32_t block_amount);
void vaciar_bloque(int numero_bloque);

//--------------Armado de char*-----------------
char* a_mayusc_primera_letra(char* palabra);
char* itoa_propio(uint32_t entero);
int conseguir_bloque(t_config * llave_valor, int cant_bloques, int indice);
int encontrar_anterior_barra_cero(char* ultimo_bloque, int block_size);
operaciones conseguir_operacion(char* tarea);
char* conseguir_recurso(char* tarea);
int conseguir_parametros(char* tarea);
void liberar_lista_string(char **lista);
void armar_lista_de_posiciones(char* posiciones);
char* agregar_a_lista_blocks(t_config* config, uint32_t nro_bloque);
char* armar_string_para_MD5(t_config* llave_valor);
char* obtener_MD5(char* string_a_hashear, t_config* llave_valor);
//--------------Sincronizacion----------
pthread_t hilo_sincronizacion;
void* sincronizar(void* tamanios);
pthread_mutex_t* buscar_mutex_con_tid(uint32_t tid);
void agregar_a_lista_bitacoras_si_es_necesario(uint32_t tid);
bool esta_el_tripulante(uint32_t tid);
void bloquear_recurso_correspondiente(char* recurso);
void desbloquear_recurso_correspondiente(char* recurso);
//--------------Sabotajes-------------
posicion get_proximo_sabotaje_y_avanzar_indice();
int realizar_fsck();
bool reparar_block_count_saboteado(char *file_path);
bool reparar_sabotaje_blocks_en_archivo(char *file_path);
bool sabotaje_block_count();
bool reparar_sabotaje_superbloque_block_count(uint32_t nueva_cant);
bool sabotaje_superbloque();
bool sabotaje_blocks();
bool sabotaje_bitmap();
bool sabotaje_size();
bool estoy_saboteado;
bool es_el_bloque_corrupto(int nro_bloque);
int bloque_corrupto;
void cargar_bitmap_temporal(char *full_path, int *bitmap_temporal);
int get_block_amount_aux();
void armar_nuevos_blocks_sin_bloque_con_indice(char** lista_bloques, t_config* archivo, int indice_bloque);
#endif /* I_MONGO_STORE_LIB_H */