#ifndef FUNCIONES_DE_CONSOLA_H
#define FUNCIONES_DE_CONSOLA_H

#include "estructuras.h"
#include "discordiador.h" // Al terner esto automaticamente borro lo de arriba

// Carga la patota
void *gestion_patotas_a_memoria();
void iniciar_patota(char **);
void iniciar_patota_local(pid_con_tareas_y_tripulantes *patota_full);
t_list * armar_tareas_para_enviar(char *);

// HILO TRIPULANTE
void ejecucion_tripulante(int);
dis_tarea* pedir_tarea_miriam(uint32_t );
void realizar_operacion(dis_tarea *, dis_tripulante *);

void notificar_movimiento_a_miram(dis_tripulante *trip);

void listar_tripulantes();

void mover_tri_hacia_tarea(dis_tarea *, dis_tripulante *);
bool tarea_es_de_entrada_salida(dis_tarea *);
void chequear_tarea_terminada(dis_tarea *, dis_tripulante *);
void chequear_tripulante_finalizado(dis_tripulante *);
bool chequear_tripulante_expulsado(dis_tripulante *);

// FUNCIONES DE MENSAJERIA
void guardar_movimiento_en_imongo(dis_tripulante *, int, int);
void *serializar_trip_con_char(uint32_t, char *);
void notificar_inicio_tarea_imongo(int, dis_tarea *);
void notificar_fin_tarea_imongo(int, char *);

// FUNCIONES SECUNDARIAS
void liberar_lista_string(char **);
int longitud_lista_string(char **);
void destroy_pid_con_tareas_y_tripulantes(pid_con_tareas_y_tripulantes *);
void iterador_listar_tripulantes(dis_tripulante *);

// Funciones de FREE
void free_distarea(dis_tarea*);
void obtener_bitacora(int);

#endif