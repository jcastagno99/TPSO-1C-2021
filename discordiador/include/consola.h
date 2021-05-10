#ifndef CONSOLA_H
#define CONSOLA_H

#include <stdlib.h> // [atoi] esta incluido
#include <stdio.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>
#include "discordiador.h"

typedef enum
{
    INIT_PATOTA,
    LIST_CREWS,
    EJECT_CREW,
    INIT_PLANIFICATION,
    PAUSE_PLANIFICATION,
    GET_BINNACLE,
    NO_VALIDO
} Comando_Discordiador;

Comando_Discordiador obtener_comando(char *);

void liberar_lista_string(char **lista);

bool posicion_validas_tripulantes(char **str_split);

void validacion_sintactica(char *text);

uint32_t crear_tareas_enviar_patota_test();

void crear_tripulante_test(uint32_t, int);

#endif