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
#include <commons/string.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

typedef enum
{
	INICIAR_PATOTA,
	INICIAR_TRIPULANTE,
	EXPULSAR_TRIPULANTE,
	ACTUALIZAR_ESTADO,
	OBTENER_ESTADO,
	VERIFICAR_FINALIZACION,
	OBTENER_PROXIMA_TAREA,
	ACTUALIZAR_UBICACION,
	OBTENER_UBICACION,
	INICIAR_SABOTAJE,
	OBTENER_BITACORA,
	REGISTRAR_MOVIMIENTO,
	REGISTRAR_FIN_TAREA,
	REGISTRAR_MOVIMIENTO_A_SABOTAJE,
	REGISTRAR_SABOTAJE_RESUELTO,
	RESPUESTA_INICIAR_PATOTA,
	RESPUESTA_INICIAR_TRIPULANTE,
	RESPUESTA_EXPULSAR_TRIPULANTE,
	RESPUESTA_ACTUALIZAR_ESTADO,
	RESPUESTA_OBTENER_ESTADO,
	RESPUESTA_VERIFICAR_FINALIZACION,
	RESPUESTA_OBTENER_PROXIMA_TAREA,
	RESPUESTA_ACTUALIZAR_UBICACION,
	RESPUESTA_OBTENER_UBICACION,
	RESPUESTA_INICIAR_SABOTAJE,
	RESPUESTA_REGISTRAR_MOVIMIENTO,
	RESPUESTA_REGISTRAR_FIN_TAREA,
	RESPUESTA_REGISTRAR_MOVIMIENTO_A_SABOTAJE,
	RESPUESTA_REGISTRAR_SABOTAJE_RESUELTO,
	RESPUESTA_OBTENER_BITACORA,
	OPERAR_SOBRE_TAREA,
	RESPUESTA_OPERAR_SOBRE_TAREA,
	PRUEBA,
	SIN_CONEXION,

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
	EXPULSADO,
	ERROR
} estado;

typedef struct
{
	op_code codigo_operacion;
	size_t size;
	void *stream;
} t_paquete;

typedef struct
{
	uint32_t pid;
	t_list *tareas;
} pid_con_tareas;

//-----------------------------------------MIRIAM-----------------------------------------------------------------

typedef struct
{
	char* tarea;
	int tamanio; //CUENTA EL /0
}tarea_ram;

typedef struct
{
	uint32_t pid;
	t_list *tareas;
	t_list *tripulantes;
}pid_con_tareas_y_tripulantes;

typedef struct
{
	uint32_t pid;
	uint32_t longitud_palabra;
	char *tareas;
	t_list *tripulantes;
}pid_con_tareas_y_tripulantes_miriam;


typedef struct 
{
	uint32_t tid;
	uint32_t pos_x;
	uint32_t pos_y;	
}nuevo_tripulante_sin_pid;

typedef struct{

	void* stream;
	uint32_t tamanio_tareas;
	float tamanio_patota;
	t_list* tareas;
	int cantidad_tripulantes;
}patota_stream_paginacion;



//---------------------------------------------------------------------------------------------------------------------

typedef struct
{
	uint32_t tid;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t pid;
} nuevo_tripulante;

typedef struct
{
	char *nombre_tarea;
	uint32_t cantidad_parametro;
	uint32_t parametro;
	uint32_t pos_x;
	uint32_t pos_y;
	uint32_t tiempo;
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
	uint32_t pos_x;
	uint32_t pos_y;
}tripulante_y_posicion;

typedef struct
{
	uint32_t tid;
	uint32_t pos_x_vieja;
	uint32_t pos_y_vieja;
	uint32_t pos_x_nueva;
	uint32_t pos_y_nueva;
}movimiento_tripulante;

typedef struct
{
	uint32_t tid;
	char* tarea;
}tripulante_con_tarea;

typedef struct{
	char* recurso;
	uint32_t cantidad;
}operacion_recurso;
//------------------------------------------FIRMAS DE FUNCIONES------------------------------------------

int crear_conexion(char *ip, char *puerto);
void *serializar_paquete(t_paquete *paquete, int bytes);
t_paquete *deserializar_paquete(op_code *opcode, uint32_t *size, void *stream);
t_paquete* recibir_paquete(int socket_hilo);
int enviar_paquete(int socket, op_code op_code,uint32_t size, void* stream);
t_paquete* crear_paquete(op_code op_code, uint32_t size, void* stream);
void liberar_paquete(t_paquete* paquete);
t_paquete* error(int res);
char* deserializar_prueba(void* stream);

//serializacion y deserializacion
void* pserializar_nuevo_tripulante(uint32_t tid, uint32_t posx, uint32_t posy, uint32_t pid);
void* serializar_nuevo_tripulante(nuevo_tripulante tripulante);
void* pserializar_respuesta_ok_fail(char caracter);
void* serializar_respuesta_ok_fail(respuesta_ok_fail resultado);
void* serializar_tarea(char* tarea,uint32_t);
void* pserializar_respuesta_verificar_finalizacion(char okfail, char sino);
void* serializar_respuesta_verificar_finalizacion(respuesta_ok_fail res, resultado_verificacion siono);
void* pserializar_posicion(uint32_t posx, uint32_t posy);
void* serializar_posicion(posicion pos);
void* pserializar_tripulante_y_posicion(uint32_t tid, uint32_t posx, uint32_t posy);
void* serializar_tripulante_y_posicion(tripulante_y_posicion trip);
void* pserializar_movimiento_tripulante(uint32_t tid,uint32_t xv,uint32_t yv,uint32_t xn,uint32_t yn);
void* serializar_movimiento_tripulante(movimiento_tripulante trip);
//void* ppserializar_tripulante_con_tarea(uint32_t tid, char* nombre, uint32_t cantidad_parametros, uint32_t parametro, uint32_t posx, uint32_t posy, uint32_t tiempo);
void* pserializar_tripulante_con_tarea(uint32_t tid, char* tarea);
void* serializar_tripulante_con_tarea(tripulante_con_tarea tct);
void* serializar_pid_con_tareas(pid_con_tareas pid_con_tareas);
void* serializar_pid(uint32_t pid);
void* serializar_estado(estado estado);
void* serializar_operacion_recurso(operacion_recurso recurso_con_cantidad);
void* pserializar_operacion_recurso(char* recurso, uint32_t cantidad);
void* serializar_recurso(char* recurso);

void* serializar_pid_con_tareas_y_tripulantes(pid_con_tareas_y_tripulantes*,uint32_t *);
void* pserializar_tid(uint32_t );


nuevo_tripulante deserializar_nuevo_tripulante(void* stream);
respuesta_ok_fail deserializar_respuesta_ok_fail(void* stream);
char* deserializar_tarea(void* stream);
respuesta_verificar_finalizacion deserializar_respuesta_verificar_finalizacion(void* stream);
posicion deserializar_posicion (void* stream);
tripulante_y_posicion deserializar_tripulante_y_posicion(void* stream);
movimiento_tripulante deserializar_movimiento_tripulante(void* stream);
tripulante_con_tarea deserializar_tripulante_con_tarea(void* stream);
pid_con_tareas deserializar_pid_con_tareas(void* stream);
tarea* deserializar_tarea_alt(void* stream,uint32_t longitud);
uint32_t deserializar_pid(void* stream);
uint32_t deserializar_tid(void* stream);
estado deserializar_estado(void* stream);
operacion_recurso deserializar_operacion_recurso(void* stream);
char* deserializar_recurso(void* stream);
pid_con_tareas_y_tripulantes_miriam deserializar_pid_con_tareas_y_tripulantes(void* stream);
void* serializar_estado_tcb(estado ,uint32_t );
estado deserializar_estado_tcb(void*,uint32_t *);
patota_stream_paginacion orginizar_stream_paginacion(pid_con_tareas_y_tripulantes_miriam);

#endif