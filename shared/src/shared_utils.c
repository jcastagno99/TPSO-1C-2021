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

//Las funciones con una p adelante son las cuales los parametros son primitivas
void* pserializar_nuevo_tripulante(uint32_t tid, uint32_t posx, uint32_t posy, uint32_t pid){
	nuevo_tripulante tripulante;
	tripulante.tid = tid;
	tripulante.pos_x = posx;
	tripulante.pos_y = posy;
	tripulante.pid = pid;
	return serializar_nuevo_tripulante(tripulante);
}

void* serializar_nuevo_tripulante(nuevo_tripulante tripulante){
	int offset = 0;
	void* stream = malloc(4*sizeof(uint32_t));
	memcpy(stream+offset,&tripulante.tid,sizeof(uint32_t));
	offset+= sizeof(uint32_t);	
	memcpy(stream+offset,&tripulante.pos_y,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&tripulante.pos_x,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&tripulante.pid,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}

void* pserializar_respuesta_ok_fail(char caracter){
	if(caracter == 'O'|| caracter == 'o'){
		return serializar_respuesta_ok_fail(RESPUESTA_OK);
	}else{
		return serializar_respuesta_ok_fail(RESPUESTA_FAIL);
	}
}


void* serializar_respuesta_ok_fail(respuesta_ok_fail resultado){
	int offset = 0;
	void* stream = malloc(sizeof(respuesta_ok_fail));
	memcpy(stream+offset,&resultado, sizeof(respuesta_ok_fail));
	offset+=sizeof(respuesta_ok_fail);
	return stream;
}

void* pserializar_tid(uint32_t tid){
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream,&tid,sizeof(uint32_t));
	return stream;	
}

// void* pserializar_tarea(char* nombre, uint32_t cantidad_parametros, uint32_t parametro, uint32_t posx, uint32_t posy, uint32_t tiempo){
// 	char * nueva_tarea;
// 	/*nueva_tarea.nombre_tarea = nombre;
// 	nueva_tarea.cantidad_parametro = cantidad_parametros;
// 	nueva_tarea.parametro = parametro;
// 	nueva_tarea.pos_x = posx;
// 	nueva_tarea.pos_y = posy;
// 	nueva_tarea.tiempo = tiempo;
// 	*/
// 	return serializar_tarea(nueva_tarea,0);
// }

void* serializar_tarea(char* tarea,uint32_t longitud_nombre){
	int offset = 0;
	void* stream = malloc(sizeof(uint32_t)+longitud_nombre);
	memcpy(stream+offset,&longitud_nombre,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,tarea,longitud_nombre);
	return stream;
	
}

void* pserializar_respuesta_verificar_finalizacion(char okfail, char sino){
	respuesta_ok_fail res;
	resultado_verificacion siono;
	if(okfail == 'O' || okfail == 'o'){
		res = RESPUESTA_OK;
	}else{
		res = RESPUESTA_FAIL;
	}
	if(sino == 'S' || sino == 's'){
		siono = RESPUESTA_SI;
	}else{
		siono = RESPUESTA_NO;
	}
	return serializar_respuesta_verificar_finalizacion(res,siono);
}

void* serializar_respuesta_verificar_finalizacion(respuesta_ok_fail res, resultado_verificacion siono){
	void* stream = malloc(sizeof(respuesta_ok_fail)+sizeof(resultado_verificacion));
	int offset = 0;
	memcpy(stream + offset, &res, sizeof(respuesta_ok_fail));
	offset += sizeof(respuesta_ok_fail);
	memcpy(stream+offset, &siono, sizeof(resultado_verificacion));
	offset+= sizeof(resultado_verificacion);
	return stream;
}

void* pserializar_posicion(uint32_t posx, uint32_t posy){
	posicion pos;
	pos.pos_x = posx;
	pos.pos_y = posy;
	return serializar_posicion(pos);
}

void* serializar_posicion(posicion pos){
	int offset = 0;
	void* stream = malloc(2*sizeof(uint32_t));
	memcpy(stream+offset,&pos.pos_x,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&pos.pos_y,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}

void* pserializar_tripulante_y_posicion(uint32_t tid, uint32_t posx, uint32_t posy){
	tripulante_y_posicion drip;
	drip.tid = tid;
	drip.pos_x = posx;
	drip.pos_y = posy;
	return serializar_tripulante_y_posicion(drip);
}

void* serializar_tripulante_y_posicion(tripulante_y_posicion trip){
	int offset = 0;
	void* stream = malloc(3*sizeof(uint32_t));
	memcpy(stream+offset, &trip.tid, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset, &trip.pos_x, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset, &trip.pos_y, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}

void* pserializar_movimiento_tripulante(uint32_t tid,uint32_t xv,uint32_t yv,uint32_t xn,uint32_t yn){
	movimiento_tripulante trip;
	trip.tid = tid;
	trip.pos_x_vieja = xv;
	trip.pos_y_vieja = yv;
	trip.pos_x_nueva = xn;
	trip.pos_y_nueva = yn;
	return serializar_movimiento_tripulante(trip);
}

void* serializar_movimiento_tripulante(movimiento_tripulante trip){
	int offset = 0;
	void* stream = malloc(5*sizeof(uint32_t));
	memcpy(stream+offset,&trip.tid,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&trip.pos_x_vieja,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&trip.pos_y_vieja,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&trip.pos_x_nueva,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,&trip.pos_y_nueva,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}
//esta funcion que tiene pp de nombre, de paso te arma tambien la tarea
/* void* ppserializar_tripulante_con_tarea(uint32_t tid, char* nombre, uint32_t cantidad_parametros, 
uint32_t parametro, uint32_t posx, uint32_t posy, uint32_t tiempo){
	tarea tarea;
	tarea.nombre_tarea = nombre;
	tarea.cantidad_parametro = cantidad_parametros;
	tarea.parametro = parametro;
	tarea.pos_x = posx;
	tarea.pos_y = posy;
	tarea.tiempo = tiempo;
	return pserializar_tripulante_con_tarea(tid,tarea);
}
Dada de baja con el ultimo cambio a mensajes de struct tarea (ahora solo se mandan char*)*/
//esta en cambio, supone que ya tenes la tarea armada
void* pserializar_tripulante_con_tarea(uint32_t tid, char* tarea){
	tripulante_con_tarea tripctarea;
	tripctarea.tarea = tarea;
	tripctarea.tid = tid;
	return serializar_tripulante_con_tarea(tripctarea);
}
//y esta, supone que tenes la estructura completa ya armada
void* serializar_tripulante_con_tarea(tripulante_con_tarea tct){
	int offset = 0;
	uint32_t longitud_nombre = strlen(tct.tarea)+1;
	void* stream = malloc(2*sizeof(uint32_t)+longitud_nombre);
	memcpy(stream+offset,&tct.tid,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(stream+offset,&longitud_nombre,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset,tct.tarea,longitud_nombre);
	offset+=longitud_nombre;
	return stream;
}

int obtener_tamanio_tareas(t_list *lista)
{
    int total = 0;
    for(int i = 0; i<lista->elements_count; i++){
        char * tarea = list_get(lista,i);
		total = total + strlen(tarea)+1;
    }
    return total;
}

void* serializar_pid_con_tareas_y_tripulantes(pid_con_tareas_y_tripulantes* patota_con_tripulantes,uint32_t * size_paquete){
	
	int cant_tripu = patota_con_tripulantes->tripulantes->elements_count;
	int tamanio_total_nombre = obtener_tamanio_tareas(patota_con_tripulantes->tareas);
	
	*size_paquete = sizeof(uint32_t)*3 + tamanio_total_nombre + cant_tripu*sizeof(uint32_t)*3;
	//  Mensaje_listo :
	/*	(los primeros 3 uint32_t) => pid, tamanio char* tareas y centinela de tripulantes
		tamanio total nombre => longitud del char * (tareas)
		(cant_tripu*sizeof(uint32_t)*3) => toda la struct estatica de los nuevo_tripulante_sin_pid 
	*/ 
	void* mensaje_listo = malloc(*size_paquete); 

	int desplazamiento = 0;

	memcpy(mensaje_listo + desplazamiento, &patota_con_tripulantes->pid,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(mensaje_listo + desplazamiento, &tamanio_total_nombre, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for(int i = 0; i<patota_con_tripulantes->tareas->elements_count; i++){
		char * auxiliar = list_get(patota_con_tripulantes->tareas,i);
		int tamanio_nombre_tarea = strlen(auxiliar) + 1;
		memcpy(mensaje_listo + desplazamiento, auxiliar,tamanio_nombre_tarea);	
		desplazamiento += tamanio_nombre_tarea;		
	}
	
	// LOS TRIPULANTES *********************************************************
	nuevo_tripulante_sin_pid* aux_trip;
	for(int i = 0; i< cant_tripu; i++){
		aux_trip = list_get(patota_con_tripulantes->tripulantes,i);

		memcpy(mensaje_listo + desplazamiento, &aux_trip->tid, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &aux_trip->pos_x, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &aux_trip->pos_y, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
	}
	uint32_t cent = 0;// el centinela ya era cero no se si se puede repetir [ISSUE]
	memcpy(mensaje_listo + desplazamiento,&cent,sizeof(uint32_t));

	return mensaje_listo;
}

//--------------**************************************************************************

void* serializar_pid_con_tareas(pid_con_tareas pid_con_tareas)
{
	int cantidad_elementos_lista  = pid_con_tareas.tareas->elements_count;
	tarea* auxiliar;
	int tamanio_total_nombre = 0;
	for(int i=0; i<cantidad_elementos_lista; i++){
		auxiliar = list_get(pid_con_tareas.tareas,i);
		tamanio_total_nombre += strlen(auxiliar->nombre_tarea) + 1;
	}
	void* mensaje_listo = malloc(sizeof(uint32_t)+cantidad_elementos_lista*(6*sizeof(uint32_t))+
	tamanio_total_nombre); //1 sizeof(uint32_t) es por la longitud de cada tarea // [EL DESERIALIZADOR ASUME QUE ES EL TAMAÑO DE TODO EL CHAR]
	int desplazamiento = 0;

	memcpy(mensaje_listo + desplazamiento, &pid_con_tareas.pid,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	uint32_t tamanio_nombre_tarea;
	for(int i = 0; i<cantidad_elementos_lista; i++){
		auxiliar = list_get(pid_con_tareas.tareas,i);
		tamanio_nombre_tarea = strlen(auxiliar->nombre_tarea) +1;
		memcpy(mensaje_listo + desplazamiento,&(tamanio_nombre_tarea),(sizeof(uint32_t)));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento,auxiliar->nombre_tarea,tamanio_nombre_tarea);
		desplazamiento += tamanio_nombre_tarea;
		memcpy(mensaje_listo + desplazamiento, &auxiliar->cantidad_parametro,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->parametro,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->pos_x,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->pos_y,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(mensaje_listo + desplazamiento, &auxiliar->tiempo,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		//free(auxiliar->nombre_tarea); dejaria afuera la liberacion de las funciones de serializacion
	}
	uint32_t centinela = 0;
	memcpy(mensaje_listo + desplazamiento,&centinela,sizeof(uint32_t));
	desplazamiento+= sizeof(uint32_t);
	//list_destroy_and_destroy_elements(pid_con_tareas.tareas,free);
	return mensaje_listo;
}

void* serializar_pid(uint32_t pid){
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream,&pid,sizeof(uint32_t));
	return stream;
}

void* serializar_estado(estado estado){
	void* stream = malloc(sizeof(estado));
	memcpy(stream,&estado,sizeof(estado));
	return stream;
}
void* serializar_estado_tcb(estado est,uint32_t tid){
	void* stream = malloc(sizeof(estado)+sizeof(uint32_t));
	memcpy(stream,&tid,sizeof(uint32_t));
	int offset = sizeof(uint32_t);
	memcpy(stream+offset,&est,sizeof(estado));
	return stream;
}


void* pserializar_operacion_recurso(char* recurso, uint32_t cantidad){
	operacion_recurso op_con_recurso;
	op_con_recurso.recurso = recurso;
	op_con_recurso.cantidad = cantidad;
	return serializar_operacion_recurso(op_con_recurso);
}

void* serializar_operacion_recurso(operacion_recurso recurso_con_cantidad){
	void* stream;
	uint32_t longitud = strlen(recurso_con_cantidad.recurso)+1;
	stream = malloc(2* sizeof(uint32_t) + longitud);
	memcpy(stream,&longitud,sizeof(uint32_t));
	int offset = sizeof(uint32_t);
	memcpy(stream+offset,recurso_con_cantidad.recurso,longitud);
	offset+= longitud;
	memcpy(stream+offset,&recurso_con_cantidad.cantidad,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}

void* serializar_recurso(char* recurso){
	uint32_t longitud = strlen(recurso)+1;
	void* stream = malloc(sizeof(uint32_t)+longitud);
	memcpy(stream,&longitud,sizeof(uint32_t));
	int offset = sizeof(uint32_t);
	memcpy(stream+offset,recurso,longitud);
	offset+= longitud;
	return stream;
}



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

nuevo_tripulante deserializar_nuevo_tripulante(void* stream){
	nuevo_tripulante tripulante;
	int offset = 0;
	memcpy(&tripulante.tid, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tripulante.pos_x, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tripulante.pos_y, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tripulante.pid, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return tripulante;
}

nuevo_tripulante_sin_pid* deserializar_nuevo_tripulante_sin_pid(void* stream){
	nuevo_tripulante_sin_pid* tripulante = malloc(sizeof(nuevo_tripulante_sin_pid));
	int offset = 0;
	memcpy(&tripulante->tid, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tripulante->pos_x, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tripulante->pos_y, stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return tripulante;
}

respuesta_ok_fail deserializar_respuesta_ok_fail(void* stream){
	respuesta_ok_fail rta;
	memcpy(&rta,stream,sizeof(respuesta_ok_fail));
	return rta;
}

char* deserializar_tarea(void* stream){
	int offset = 0;
	char* tarea;
	uint32_t longitud;
	memcpy(&longitud,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	tarea = malloc(longitud);
	memcpy(tarea,stream+offset,longitud);
	return tarea;
}

respuesta_verificar_finalizacion deserializar_respuesta_verificar_finalizacion(void* stream){
	respuesta_verificar_finalizacion rta;
	int offset = 0;
	memcpy(&rta.respuesta, stream+offset, sizeof(respuesta_ok_fail));
	offset+= sizeof(respuesta_ok_fail);
	memcpy(&rta.resultado, stream+offset, sizeof(resultado_verificacion));
	offset += sizeof(resultado_verificacion);
	return rta;
}

posicion deserializar_posicion (void* stream){
	posicion pos;
	int offset = 0;
	memcpy(&pos.pos_x, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&pos.pos_y, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return pos;
}

tripulante_y_posicion deserializar_tripulante_y_posicion(void* stream){
	tripulante_y_posicion trip;
	int offset = 0;
	memcpy(&trip.tid,stream + offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&trip.pos_x,stream+offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&trip.pos_y,stream+offset, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return trip;
}

movimiento_tripulante deserializar_movimiento_tripulante(void* stream){
	int offset = 0;
	movimiento_tripulante mov;
	memcpy(&mov.tid, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&mov.pos_x_vieja,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&mov.pos_y_vieja,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&mov.pos_x_nueva,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&mov.pos_y_nueva, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return mov;
}

tripulante_con_tarea deserializar_tripulante_con_tarea(void* stream){
	tripulante_con_tarea tct;
	int offset = 0;
	uint32_t longitud = 0;
	memcpy(&tct.tid, stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&longitud,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	tct.tarea = malloc(longitud);
	memcpy(tct.tarea,stream+offset,longitud);
	offset+= longitud;
	return tct;
}


pid_con_tareas deserializar_pid_con_tareas(void* stream){
	pid_con_tareas pct;
	pct.tareas = list_create();
	int offset = 0;
	uint32_t tamanio_palabra = 0;
	memcpy(&pct.pid,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&tamanio_palabra,stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	while(tamanio_palabra != 0){
		list_add(pct.tareas, deserializar_tarea_alt(stream+offset,tamanio_palabra));
		offset+= tamanio_palabra + 5*sizeof(uint32_t);
		memcpy(&tamanio_palabra,stream+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);
	}
	return pct;
}

pid_con_tareas_y_tripulantes_miriam deserializar_pid_con_tareas_y_tripulantes(void* stream){
	pid_con_tareas_y_tripulantes_miriam pct;
	pct.tripulantes = list_create();
	int offset = 0;
	memcpy(&pct.pid,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&pct.longitud_palabra,stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	
	pct.tareas = malloc(pct.longitud_palabra);
	memcpy(pct.tareas,stream + offset, pct.longitud_palabra);

	offset= offset+pct.longitud_palabra;
	uint32_t tid;
	memcpy(&tid,stream+offset,sizeof(uint32_t));
	while(tid != 0){
		list_add(pct.tripulantes, deserializar_nuevo_tripulante_sin_pid(stream+offset));
		offset += 3*sizeof(uint32_t);
		memcpy(&tid,stream+offset,sizeof(uint32_t));
	}
	return pct;
}

// TODO HAY UNA LECTURA INVALIDA DE 1 BYTE
patota_stream_paginacion orginizar_stream_paginacion(pid_con_tareas_y_tripulantes_miriam pct){

	patota_stream_paginacion patota;
	patota.tareas = list_create();

	int contador = 0;
	int puntero = 0;
	while(puntero < pct.longitud_palabra){
		if(*(pct.tareas + puntero) == '\0'){
			contador ++;
		}
		puntero++;
	}

	int offset_tarea = 0;
	for(int i =0; i<contador; i++){
		char* auxiliar = string_substring(pct.tareas,offset_tarea,99);
		offset_tarea += string_length(auxiliar) + 1;
		tarea_ram* una_tarea = malloc(sizeof(tarea_ram));
		una_tarea->tarea = auxiliar;
		una_tarea->tamanio = string_length(auxiliar) + 1;
		list_add(patota.tareas,una_tarea);

	}

	patota.tamanio_patota = 2 * sizeof(uint32_t) + pct.longitud_palabra - 1 + (pct.tripulantes->elements_count * (5 * sizeof(uint32_t) + sizeof(char)));
	patota.tamanio_tareas = pct.longitud_palabra - 1;
	patota.cantidad_tripulantes = pct.tripulantes->elements_count;
	void* stream = malloc(patota.tamanio_patota);

	int offset = 0;
	memcpy(stream + offset,&pct.pid,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	uint32_t dir_tareas = 0;
	memcpy(stream + offset,&dir_tareas,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset,pct.tareas,patota.tamanio_tareas);
	offset += patota.tamanio_tareas;

	nuevo_tripulante_sin_pid* aux;
	for(int i=0; i<pct.tripulantes->elements_count; i++){
		aux = list_get(pct.tripulantes,i);
		memcpy(stream + offset,&aux->tid, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		char estado = 'N';
		memcpy(stream + offset,&estado, sizeof(char));
		offset += sizeof(char);
		memcpy(stream + offset,&aux->pos_x, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset,&aux->pos_y, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		uint32_t proxima_tarea = 0;
		uint32_t dir_pcb = 0;
		memcpy(stream + offset,&proxima_tarea, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset,&dir_pcb, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}
	patota.stream = stream;
	free(pct.tareas);
	list_destroy_and_destroy_elements(pct.tripulantes,free);
	return patota;
}

tarea* deserializar_tarea_alt(void* stream,uint32_t longitud){
	int offset = 0;
	tarea* tarea = malloc(sizeof(tarea));
	tarea->nombre_tarea = malloc(longitud);
	memcpy(tarea->nombre_tarea,stream+offset,longitud);
	offset+= longitud;
	memcpy(&tarea->cantidad_parametro, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tarea->parametro, stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(&tarea->pos_x,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&tarea->pos_y,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&tarea->tiempo,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	return tarea;
}

uint32_t deserializar_pid(void* stream){
	uint32_t pid;
	memcpy(&pid,stream,sizeof(uint32_t));
	return pid;
}
uint32_t deserializar_tid(void* stream){
	uint32_t tid;
	memcpy(&tid,stream,sizeof(uint32_t));
	return tid;
}


estado deserializar_estado(void* stream){
	estado estadoRta;
	memcpy(&estadoRta,stream,sizeof(estado));
	return estadoRta;
}

operacion_recurso deserializar_operacion_recurso(void* stream){
	operacion_recurso rec_c_cantidad;
	uint32_t offset = 0;
	uint32_t longitud;
	memcpy(&longitud,stream+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	rec_c_cantidad.recurso = malloc(longitud);
	memcpy(rec_c_cantidad.recurso,stream+offset,longitud);
	offset+=longitud;
	memcpy(&rec_c_cantidad.cantidad,stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return rec_c_cantidad;
}

char* deserializar_recurso(void* stream){
	char* recurso;
	uint32_t longitud;
	uint32_t offset = 0;
	memcpy(&longitud,stream+offset,sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	recurso = malloc(longitud);
	memcpy(recurso,stream+offset,longitud);
	offset+=longitud;
	return recurso;
}

estado deserializar_estado_tcb(void* stream,uint32_t * tid){
	estado est;
	uint32_t aux_tid;
	memcpy(&aux_tid,stream,sizeof(uint32_t));
	*tid = aux_tid;
	int offset = sizeof(uint32_t);
	memcpy(&est,stream+offset,sizeof(estado));
	return est;
}
//------------------------------------------------CONEXIONES-----------------------------------------------

int crear_conexion(char *ip, char* puerto)
{
	int conn;

	struct addrinfo hints;
	struct addrinfo *server_info, *p;


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);
	for(p = server_info; p != NULL; p = p->ai_next)
	{
		conn = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		//Si socket() falla, pasa al siguiente
		if(conn == -1)
			continue;
		
		//Si connect() funciona, deja de iterar
		if(connect(conn, p->ai_addr, p->ai_addrlen) != -1)
			break;

		//Si socket() funciona pero connect() falla, cierra el socket y pasa al siguiente
		close(conn);
	}
	//printf("\033[1;33mEL RESULTADO DE CREAR LA CONEXION FUE DE %i\033[0m\n", conn);
	freeaddrinfo(server_info);
	if(conn != -1 && p != NULL)
		return conn;
	return -1;
	
	/*
	for(p = server_info; p != NULL; p = p->ai_next) {
	
		int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

		if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
			printf("error\n");

		freeaddrinfo(server_info);
		return socket_cliente;

	}
	 */
	
}

//-----------------------------------------MANEJO DE PAQUETES------------------------------------------------------------

//Primero se recibe el op_code
//Acuerdo para que el SIZE del paquete solo toma encuenta el SIZE del STREAM (no toma ninguno de los UINT)
//Esta funcion deja el paquete listo para enviar a deserializar (lo que seria el void*)
t_paquete* recibir_paquete(int socket){
	t_paquete * paquete = malloc(sizeof(t_paquete));
	int res = recv(socket,&paquete->codigo_operacion,sizeof(op_code),0);
	//printf("el codigo de operacion es: %i \n", paquete->codigo_operacion);
	if (res <= 0 ){
		close(socket);
		free(paquete);
		return error(res);
	}
	

	res = recv(socket,&paquete->size,sizeof(uint32_t),0);
	//printf("el size del paquete es: %i \n",paquete->size);
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
	void* buffer = malloc(paquete->size + sizeof(size_t) + sizeof(op_code));
	int offset = 0;
	memcpy(buffer+offset, &paquete->codigo_operacion, sizeof(op_code));
	offset+= sizeof(op_code);
	memcpy(buffer+offset, &paquete->size, sizeof(size_t));
	offset+= sizeof(size_t);
	memcpy(buffer+offset,paquete->stream, paquete->size);
	//printf("\033[1;32mEnviando paquete con CODIGO DE OPERACION %lu SIZE %lu\033[0m\n", (unsigned long)op_code, (unsigned long)size);
	int res = send(socket, buffer , sizeof(op_code) + sizeof(size_t) + paquete->size, 0);
	if(res <0){
		//printf("ERROR ENVIANDO MENSAJE %i\n", op_code);
		res = -errno;
	}
	//printf("\033[1;32mENVIE PAQUETE con CODIGO DE OPERACION %i SIZE %lu\033[0m\n", op_code, (unsigned long)size);
	free(buffer);
	//printf("\033[1;32mHICE EL PRIMER FREE con CODIGO DE OPERACION %i SIZE %lu\033[0m\n", op_code, (unsigned long)size);
	liberar_paquete(paquete);
	//printf("\033[1;32mHICE EL SEGUNDO FREE con CODIGO DE OPERACION %i SIZE %lu\033[0m\n", op_code, (unsigned long)size);
	

	return res;

}

t_paquete* crear_paquete(op_code op_code, uint32_t size, void* stream){
	t_paquete* paquete = (t_paquete*)malloc(sizeof(t_paquete));
	paquete->codigo_operacion = op_code;
	paquete->size = size;
	paquete->stream = malloc(size);
	
	memcpy(paquete->stream,stream,size);
	//printf("\033[1;31mA PUNTO DE ROMPER EN CREAR_PAQUETE OPCODE %i SIZE %lu\033[0m\n", op_code, (unsigned long)size);
	if(stream)
		free(stream);
	//printf("\033[1;31mNO ME ROMPI OPCODE %i SIZE %lu\033[0m\n", op_code, (unsigned long)size);

	return paquete;
}

t_paquete* error(int res){
	op_code op_code = res == 0? SIN_CONEXION : ERROR;
	//int32_t err = -errno;
	return crear_paquete(op_code,sizeof(int32_t),NULL);
}

void liberar_paquete(t_paquete* paquete){
	if(paquete != NULL){
		if(paquete->stream != NULL){
			free(paquete->stream);
		}
		free(paquete);
	}
}

