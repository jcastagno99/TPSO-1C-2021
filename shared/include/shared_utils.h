#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <pthread.h>

typedef enum
{
	INICIALIZAR_PATOTA,
	INICIALIZAR_TRIPULANTE,
	EXPULSAR_TRIPULANTE,
	ACTUALIZAR_ESTADO,
	OBTENER_ESTADO,
	VERIFICAR_FINALIZACION,
	OBTENER_TAREA,
	ACTUALIZAR_UBICACION,
	OBTENER_UBICACION,
	INICIAR_SABOTAJE,
	REGISTRAR_MOVIMIENTO,
	REGISTRAR_COMIENZO_TAREA,
	REGISTRAR_FIN_TAREA,
	REGISTRAR_MOVIMIENTO_A_SABOTAJE,
	REGISTRAR_SABOTAJE_RESUELTO,
	RESPUESTA_INICIALIZAR_PATOTA,
	RESPUESTA_INICIALIZAR_TRIPULANTE,
	RESPUESTA_EXPULSAR_TRIPULANTE,
	RESPUESTA_ACTUALIZAR_ESTADO,
	RESPUESTA_OBTENER_ESTADO,
	RESPUESTA_VERIFICAR_FINALIZACION,
	RESPUESTA_OBTENER_TAREA,
	RESPUESTA_ACTUALIZAR_UBICACION,
	RESPUESTA_OBTENER_UBICACION,
	RESPUESTA_INICIAR_SABOTAJE,
	RESPUESTA_REGISTRAR_MOVIMIENTO,
	RESPUESTA_REGISTRAR_COMIENZO_TAREA,
	RESPUESTA_REGISTRAR_FIN_TAREA,
	RESPUESTA_REGISTRAR_MOVIMIENTO_A_SABOTAJE,
	RESPUESTA_REGISTRAR_SABOTAJE_RESUELTO
} op_code;

typedef enum
{
	RESPUESTA_OK,
	RESPUESTA_FAIL
} respuesta_ok_fail;

typedef enum
{
	RESPUESTA_SI,
	RESPUESTA_NO

} resultado_verificacion;

typedef enum
{
	NEW,
	READY,
	EXEC,
	BLOCKED_E_S,
	BLOCKED_SABOTAJE,
	EXIT,
	ERROR
} estado;

typedef struct
{
	op_code codigo_operacion;
	uint32_t size;
	void *stream;
} t_paquete;

typedef struct
{
	uint32_t pid;
	t_list *tareas;
} pid_con_tareas;

typedef struct
{
	uint32_t tid;
	uint16_t pos_x;
	uint16_t pos_y;
	uint32_t pid;
} nuevo_tripulante;

typedef struct
{
	char *nombre_tarea;
	uint16_t cantidad_parametro;
	uint16_t parametro;
	uint16_t pos_x;
	uint16_t pos_y;
	uint16_t tiempo;
} tarea;

typedef struct
{
	respuesta_ok_fail respuesta;
	resultado_verificacion resultado;
} respuesta_verificar_finalizacion;

typedef struct
{
	uint32_t pos_x;
	uint32_t pos_y;
}posicion;

typedef struct
{
	uint32_t tid;
	uint16_t pos_x;
	uint16_t pos_y;
}tripulante_y_posicion;

typedef struct
{
	uint32_t tid;
	uint16_t pos_x_vieja;
	uint16_t pos_y_vieja;
	uint16_t pos_x_nueva;
	uint16_t pos_y_nueva;
}actualizar_posicion_tripulante;

typedef struct
{
	uint32_t tid;
	tarea tarea;
}tripulante_con_tarea;



int crear_conexion(char *ip, char *puerto);
void *serializar_paquete(t_paquete *paquete, int bytes);
t_paquete *deserializar_paquete(op_code *opcode, uint32_t *size, void *stream);

#endif
