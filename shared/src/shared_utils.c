#include "shared_utils.h"

//--------------------------------SERIALIZADORES------------------------------------

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * paquete_listo = malloc(bytes);
	int desplazamiento = 0;

	memcpy(paquete_listo + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(paquete_listo + desplazamiento, &(paquete->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(paquete_listo + desplazamiento, paquete->stream, paquete->size);
	desplazamiento+= paquete->size;

	free(paquete->stream);
	free(paquete);

	return paquete_listo;
}

/*void* serializar_pid_con_tareas(pid_con_tareas* pid_con_tareas)
{
	int cantidad_elementos_lista  = pid_con_tareas->tareas->elements_count;
	tarea* auxiliar;
	int tamanio_total_nombre = 0;
	for(int i=0; i<cantidad_elementos_lista; i++){
		auxiliar = list_get(pid_con_tareas->tareas,i);
		tamanio_total_nombre += strlen(auxiliar->nombre_tarea) + 1;
	}
	void* mensaje_listo = malloc(4+cantidad_elementos_lista*(10)+tamanio_total_nombre);
	int desplazamiento = 0;

	memcpy(mensaje_listo + desplazamiento, pid_con_tareas->pid,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	uint16_t tamanio_nombre_tarea;
	for(int i = 0; i<cantidad_elementos_lista; i++){
		auxiliar = list_get(pid_con_tareas->tareas,i);
		tamanio_nombre_tarea = strlen(auxiliar->nombre_tarea) +1;
		memcpy(mensaje_listo + desplazamiento,&(tamanio_nombre_tarea),(sizeof(uint16_t)));
		desplazamiento += sizeof(uint16_t);
		memcpy(mensaje_listo + desplazamiento,auxiliar->nombre_tarea,tamanio_nombre_tarea);
		desplazamiento += tamanio_nombre_tarea;
		memcpy(mensaje_listo + desplazamiento, &auxiliar->cantidad_parametro,sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->parametro,sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->pos_x,sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->pos_y,sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->tiempo,sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);
		free(auxiliar->nombre_tarea);
	}
	list_destroy_and_destroy_elements(pid_con_tareas->tareas,free);
	return mensaje_listo;
}*/

//---------------------------------DESERIALIZADORES---------------------------------------------

t_paquete* deserializar_paquete(op_code* opcode, uint32_t* size, void* stream)
{
	t_paquete* paquete_listo = malloc(sizeof(t_paquete));
	paquete_listo->stream = malloc(*size);
	int desplazamiento = 0; 

	memcpy(&(paquete_listo->codigo_operacion),opcode,sizeof(op_code));
	desplazamiento += sizeof(op_code);
	memcpy(&(paquete_listo->size),size,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete_listo->stream,stream,paquete_listo->size);

	free(opcode);
	free(size);
	free(stream);

	return paquete_listo;
}

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		printf("error\n");

	freeaddrinfo(server_info);

	return socket_cliente;
}

//-----------------------------------------MANEJO DE PAQUETES------------------------------------------------------------

//Primero se recibe el op_code
//Acuerdo para que el SIZE del paquete solo toma encuenta el SIZE del STREAM (no toma ninguno de los UINT)
//Esta funcion deja el paquete listo para enviar a deserializar (lo que seria el void*)
t_paquete* recibir_paquete(int socket){
	t_paquete * paquete = malloc(sizeof(t_paquete));
	int res = recv(socket,&paquete->codigo_operacion,sizeof(op_code),0);
	printf("el codigo de operacion es: %i ", paquete->codigo_operacion);
	if (res <= 0 ){
		close(socket);
		free(paquete);
		return error(res);
	}
	

	res = recv(socket,&paquete->size,sizeof(uint32_t),0);
	printf("el size del paquete es: %i ",paquete->size);
	if(res <= 0){
		close(socket);
		free(paquete);
		return error(res);
	}

	paquete->stream = malloc(paquete->size);
	res = recv(socket,paquete->stream,paquete->size,0);


	if(res <= 0){
		close(socket);
		free(paquete);
		free(paquete->stream);
		return error(res);
	}

	
	return paquete;
}

int enviar_paquete(int socket, op_code op_code,uint32_t size, void* stream){
	t_paquete* paquete = crear_paquete(op_code,size,stream);
	int res = send(socket, &(paquete->codigo_operacion) , sizeof(op_code), 0);

	if(res >= 0){
		void* buffer = malloc(paquete->size);
		memcpy(buffer,&paquete->size,sizeof(uint32_t));
		memcpy(buffer + sizeof(uint32_t),paquete->stream,size);
		res = send(socket,buffer,paquete->size + sizeof(uint32_t),0);
		if(res <0){
			perror("ERROR ENVIANDO MENSAJE");
			res = -errno;
		}
		free(buffer);
	}else{
		perror("ERROR ENVIANDO MENSAJE");
		res = -errno;
	}
	liberar_paquete(paquete);

	return res;

}

t_paquete* crear_paquete(op_code op_code, uint32_t size, void* stream){
	t_paquete* paquete = (t_paquete*)malloc(sizeof(t_paquete));
	paquete->codigo_operacion = op_code;
	paquete->size = size;
	paquete->stream = malloc(size);
	
	memcpy(paquete->stream,stream,size);

	free(stream);

	return paquete;
}

t_paquete* error(int res){
	op_code op_code = res == 0? SIN_CONEXION : ERROR;
	int32_t err = -errno;
	return crear_paquete(op_code,sizeof(int32_t),&err);
}

void liberar_paquete(t_paquete* paquete){
	if(paquete != NULL){
		if(paquete->stream != NULL){
			free(paquete->stream);
		}
		free(paquete);
	}
}


char* deserializar_prueba(void* stream){
	uint32_t size = 5;
	char* auxiliar = malloc(size);
	memcpy(auxiliar,stream,size);
	return auxiliar;
}