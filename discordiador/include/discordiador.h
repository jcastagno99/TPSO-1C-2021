#ifndef DISCORDIADOR_H
#define DISCORDIADOR_H

#define MAX_CLIENTS 128

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/temporal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "shared_utils.h"
#include "consola.h"

t_log *logger;
t_config *config;

// CONFIG
int tiempo_retardo_ciclo_cpu;
char *ip_mi_ram_hq;
char *puerto_mi_ram_hq;
char *ip_i_mongo_store;
char *puerto_i_mongo_store;
//int conexion_mi_ram_hq; no existe una global se crea una local por conexion
int quantum_rr;
int puerto_escucha;
char *algoritmo;
int grado_de_multiprocesamiento;

// Identificadores
int id_patota;
int id_tcb;

// Planificacion
bool planificacion_pausada;
bool sabotaje;
int pos_sab_x;
int pos_sab_y;
int duracion_sabotaje;

// Mutex
pthread_mutex_t mutex_cola_iniciar_patota;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_cola_de_ready;
pthread_mutex_t mutex_cola_de_exec;
pthread_mutex_t mutex_cola_de_block;
pthread_mutex_t mutex_tarea;

// Contadores
sem_t sem_contador_cola_iniciar_patota;
sem_t sem_contador_cola_de_new;
sem_t sem_contador_cola_de_block;
sem_t sem_contador_cola_de_ready;
sem_t operacion_entrada_salida_finalizada;
sem_t procesadores_disponibles;
sem_t sem_sabotaje;
sem_t sem_sabotaje_fin;
sem_t sem_planificador_a_largo_plazo;
sem_t sem_hilo_de_bloqueados;

// Colas
t_list *cola_de_new;
t_list *cola_de_ready;
t_list *cola_de_exec;
t_list *cola_de_block;
t_list *lista_de_patotas;
t_queue *cola_de_iniciar_patotas;
t_list *list_total_tripulantes;
t_list *hilos_pausables;

void leer_consola(t_log *);
void leer_consola_prueba(t_log *);
void paquete(int);
void terminar_programa();
void *esperar_conexiones();
int iniciar_servidor_discordiador(int);
void crear_hilo_para_manejar_suscripciones(int);
void manejar_suscripciones_discordiador(int *);
void catch_sigint_signal();
void inicializar_discordiador();
void cargar_config();
void pausar_planificacion();
void iniciar_planificacion();

void *planificador_a_largo_plazo();
void agregar_elemento_a_cola(t_list *, pthread_mutex_t *, void *);
void *quitar_primer_elemento_de_cola(t_list *, pthread_mutex_t *);
void loggear_estado_de_cola(t_list *, char *, char *);
void *procesar_tripulante_fifo(void *);
void *procesar_tripulante_rr();
void *ejecutar_tripulantes_bloqueados();
void chequear_planificacion_pausada(sem_t *, int);
void manejar_sabotaje(int);
int calcular_distancia(int, int, int, int);
dis_tripulante *minimum(dis_tripulante *, dis_tripulante *);

///void iterate_distancias(dis_tripulante *);

bool ordenar_por_tid(dis_tripulante *, dis_tripulante *);
void bloqueo_por_sabotaje(dis_tripulante *);
void enviar_a_ready(dis_tripulante *);
void vaciar_lista(t_list *);
dis_tripulante *seleccionar_tripulante_a_ejecutar();
void atender_interrupciones(sem_t *, int, dis_tripulante *);
bool fue_expulsado(dis_tripulante *);
void tripulante_expulsado(dis_tripulante *);
int chequear_expulsion_de_tripulante(t_list *, pthread_mutex_t *);

// MENSAJES PARA IMONGO
void notificar_atencion_sabotaje_imongo(int, char *);
void notificar_fin_sabotaje_imongo(int);

// Funciones de serializacion
void actualizar_estado_miriam(int,estado);
void liberar_listas();

#endif /* DISCORDIADOR_H */