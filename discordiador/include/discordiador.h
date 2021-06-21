#ifndef DISCORDIADOR_H
#define DISCORDIADOR_H

#define MAX_CLIENTS 128

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// identificadores
int id_patota;
int id_tcb;
int grado_de_multiprocesamiento;
int procesadores_disponibles;
bool planificacion_pausada;
int conexion_mi_ram_hq;

// Planificacion
sem_t sem_planificador_a_corto_plazo;
sem_t sem_planificador_a_largo_plazo;

// Mutex
pthread_mutex_t mutex_cola_iniciar_patota;
pthread_mutex_t mutex_cola_de_new;
pthread_mutex_t mutex_cola_de_ready;
pthread_mutex_t mutex_cola_de_exec;
pthread_mutex_t mutex_tarea;
sem_t sem_contador_cola_iniciar_patota;
sem_t sem_contador_cola_de_new;

//Colas
t_list *cola_de_new;
t_list *cola_de_ready;
t_list *cola_de_exec;

t_list *lista_de_patotas;
t_queue *cola_de_iniciar_patotas;

t_list *list_total_tripulantes;

t_log *iniciar_logger(void);
t_config *leer_config(void);
void leer_consola(t_log *);
void leer_consola_prueba(t_log *);
void paquete(int);
void terminar_programa();
void *esperar_conexiones(void *);
int iniciar_servidor_discordiador(int);
void crear_hilo_para_manejar_suscripciones(t_list *, int);
void manejar_suscripciones_discordiador(int *);
void catch_sigint_signal();
void inicializar_discordiador();

void *planificador_a_corto_plazo();
void *planificador_a_largo_plazo();
void agregar_elemento_a_cola(t_list *, pthread_mutex_t *, void *);
void *quitar_primer_elemento_de_cola(t_list *, pthread_mutex_t *);

#endif /* DISCORDIADOR_H */
