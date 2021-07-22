#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <commons/collections/list.h>
#include <semaphore.h>
#include "shared_utils.h"

// A TODO PONERLE UN TRI o UN DIS al inicio para no confundir con los de serializacion

typedef enum
{
    DISPONIBLE,
    EN_CURSO,
    TERMINADA
} dis_estado_tarea;

typedef struct
{
    dis_estado_tarea estado_tarea;
    char *nombre_tarea;
    int parametro;
    int pos_x;
    int pos_y;
    int tiempo_restante;
    int tiempo_total;
} dis_tarea;

typedef struct
{
    int pos_x;
    int pos_y;
    estado estado; // estado : de los shared_utils.h
    int id;
    int id_patota;
    sem_t sem_tri;
    dis_tarea *tarea_actual;
    pthread_t threadid;
    sem_t procesador;
    bool hice_ciclo_inicial_tarea_es;
    int tareas_realizadas;
    bool expulsado;
    pthread_mutex_t mutex_expulsado;
} dis_tripulante;

typedef struct
{
    int id_patota;
    int cantidad_de_tareas;
} dis_patota;

#endif