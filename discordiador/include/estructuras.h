#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <commons/collections/list.h>
#include <semaphore.h>
#include "shared_utils.h"

// A TODO PONERLE UN TRI o UN DIS al inicio para no confundir con los de serializacion

typedef struct {
    int x;
    int y;
} Posicion;

typedef enum{
    DISPONIBLE,
    EN_CURSO,
    TERMINADA
} dis_estado_tarea;

typedef struct {
    dis_estado_tarea estado_tarea;
    char *nombre_tarea;
	int parametro;
	int pos_x;
	int pos_y;
	int tiempo;
    bool es_de_sabotaje;
} dis_tarea;

typedef struct {
    Posicion pos;
    estado estado; // estado : de los shared_utils.h
    int id;
    int id_patota;
    sem_t sem_tri;
    dis_tarea* tarea_actual;
} dis_tripulante;

typedef struct {
    int id_patota;
    t_list* list_tareas;
} dis_patota;


#endif