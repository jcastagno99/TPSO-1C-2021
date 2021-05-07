

#ifndef DISCORDIADOR_H
#define DISCORDIADOR_H

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "shared_utils.h"

int conexion_mi_ram_hq;

//tarea tarea1;
//tarea tarea2;

t_log* iniciar_logger(void);
t_config* leer_config(void);
void leer_consola(t_log*);
void leer_consola_prueba(t_log*);
void paquete(int);
void terminar_programa(int,int, t_log*, t_config*);

#endif /* DISCORDIADOR_H */
