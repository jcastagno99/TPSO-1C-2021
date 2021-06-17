
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
#define MAX_CLIENTS 128


//------------Estructuras------------

typedef struct 
{
	char* PUNTO_MONTAJE;
	int PUERTO;
	int TIEMPO_SINCRONIZACION;
	char* IP_DISCORDIADOR;
	int PUERTO_DISCORDIADOR;
}i_mongo_store_config;

typedef enum{
	RECURSO, BITACORA
}tipo_archivo;

//------------Variables Globales------------

i_mongo_store_config* i_mongo_store_configuracion;
t_log* logger_i_mongo_store;
int socket_escucha;
char* ruta_superbloque;
char* ruta_blocks;
char* carpeta_files;
char* carpeta_bitacoras;
void* superbloque;
void* blocks;



//------------Firmas de funciones------------

i_mongo_store_config* leer_config_i_mongo_store(char*);
t_log* iniciar_logger_i_mongo_store(char*);

void* esperar_conexion(int);
int iniciar_servidor_i_mongo_store(int);
void crear_hilo_para_manejar_suscripciones(t_list*,int);
void* manejar_suscripciones_i_mongo_store(int*);

//----------------Filesystem----------------
void inicializar_filesystem(uint32_t,uint32_t);
void inicializar_superbloque(uint32_t,uint32_t);
void inicializar_blocks(uint32_t,uint32_t);
void crear_directorio(char* carpeta);
void no_pude_abrir_archivo(char*);
void no_pude_mapear_archivo(char*);
void inicializar_rutas(char* montaje);
bool existe_archivo(char* archivo);
void escribir_archivo(char* archivo, char * escritura, tipo_archivo tipo);
void quitar_de_archivo(char* archivo,char* escritura);
void borrar_archivo(char* archivo);
char* todo_el_archivo(char* archivo);
void crear_archivo_metadata(char* ruta, tipo_archivo tipo, char caracter_llenado);
void liberar_bloque(int numero_bloque);
void ocupar_bloque(int numero_bloque);
int get_block_size();
int get_block_amount();
int get_primer_bloque_libre();
//--------------Armado de char*-----------------
char* a_mayusc_primera_letra(char* palabra);
char* itoa_propio(uint32_t entero);
int conseguir_ultimo_bloque(t_config* llave_valor, int cant_bloques);
int encontrar_anterior_barra_cero(char* ultimo_bloque, int block_size);
//--------------Sincronizacion----------
pthread_t hilo_sincronizacion;
void* sincronizar(void* tamanios);


#endif /* I_MONGO_STORE_LIB_H */
