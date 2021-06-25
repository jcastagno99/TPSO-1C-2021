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

t_list *crear_tareas_global(char *);

void iniciar_patota_global(char **);
void iniciar_patota_local(pid_con_tareas_y_tripulantes *patota_full);

void ejecucion_tripulante(int);
void listar_tripulantes();
void *gestion_patotas_a_memoria();
void expulsar_tripulante(int);

int longitud_lista_string(char **);
dis_tarea *pedir_tarea(int,int);
char * pedir_tarea_miriam(uint32_t);
void realizar_operacion(dis_tarea *, dis_tripulante *);

void destroy_pid_con_tareas_y_tripulantes(pid_con_tareas_y_tripulantes *);
void iterador_listar_tripulantes(dis_tripulante *);

t_list * armar_tareas_para_enviar(char *);

void mover_tri_hacia_tarea(dis_tarea *, dis_tripulante *);
bool tarea_es_de_entrada_salida(dis_tarea *);
void chequear_tarea_terminada(dis_tarea *, dis_tripulante *);
void chequear_tripulante_finalizado(dis_tripulante *);
bool chequear_tripulante_expulsado(dis_tripulante *);

#endif