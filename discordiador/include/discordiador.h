#ifndef DISCORDIADOR_H
#define DISCORDIADOR_H

#define MAX_CLIENTS 128

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "shared_utils.h"

int conexion_mi_ram_hq;
t_log *logger;
t_config *config;

//tarea tarea1;
//tarea tarea2;

t_log *iniciar_logger(void);
t_config *leer_config(void);
void leer_consola(t_log *);
void leer_consola_prueba(t_log *);
void paquete(int);
void terminar_programa(int, int, t_log *, t_config *);
void *esperar_conexiones(int);
int iniciar_servidor_discordiador(int);
void crear_hilo_para_manejar_suscripciones(t_list *, int);
void *manejar_suscripciones_discordiador(int *);

#endif /* DISCORDIADOR_H */
