#ifndef FUNCIONES_DE_CONSOLA_H
#define FUNCIONES_DE_CONSOLA_H

//#include "discordiador.h"    CREARE MIS PROPIAS ESTRUCTURAS EN STRUC LUEGO MACHEARE CON LAS DE SERIALIZACION
/*
#include <stdlib.h> 
#include <stdio.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
*/

#include "estructuras.h"
#include "discordiador.h" // Al terner esto automaticamente borro lo de arriba

t_list * armar_tareas_para_enviar(char *);
t_list *crear_tareas_local(char*);
t_list *crear_tareas_global(char *);

void iniciar_patota_global(char **);
void iniciar_patota_local(t_list* , t_list* ,char **str_split);

void ejecucion_tripulante(int);
void listar_tripulantes();
void *gestion_patotas_a_memoria();

#endif