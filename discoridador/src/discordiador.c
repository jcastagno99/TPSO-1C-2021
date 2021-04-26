/*
 * main.c
 *
 *  Created on: 28 feb. 2019
 *      Author: utnso
 */

#include "discordiador.h"

int main(void)
{ 
	
	/*---------------------------------------------------PARTE 2-------------------------------------------------------------*/
	int conexion;
	char* ip;
	char* puerto;
	//char* valor;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();

	//Loggear "soy un log"
	//log_info(logger,"soy un log");

	config = leer_config();

	//asignar valor de config a la variable valor
	//valor = config_get_string_value(config,"CLAVE");

	//Loggear valor de config
	//log_info(logger,valor);
	//leer_consola(logger);


	/*---------------------------------------------------PARTE 3-------------------------------------------------------------*/

	//antes de continuar, tenemos que asegurarnos que el servidor esté corriendo porque lo necesitaremos para lo que sigue.

	//crear conexion

	ip = config_get_string_value(config,"IP_MI_RAM_HQ");

	puerto = config_get_string_value(config,"PUERTO_MI_RAM_HQ");

	conexion = crear_conexion(ip,puerto);

	//enviar CLAVE al servirdor

	//char* clave;

	//clave = config_get_string_value(config,"CLAVE");

	//enviar_mensaje(clave,conexion);

	//t_paquete* paquete = crear_paquete();

	//llenar_paquete(paquete,logger);
	//enviar_paquete(paquete,conexion);
	//free(paquete->buffer->stream);
	//free(paquete->buffer);
	//free(paquete);

	//paquete(conexion);

	terminar_programa(conexion, logger, config);
	
}


t_log* iniciar_logger(void)
{
	return log_create("/home/utnso/workspace/tp-2021-1c-S-quito-de-Oviedo/discoridador/cfg/discordiador.log","discordiador.log",1,LOG_LEVEL_INFO);
}

t_config* leer_config(void)
{
	return config_create("/home/utnso/workspace/tp-2021-1c-S-quito-de-Oviedo/discoridador/cfg/discordiador.config");
}

void leer_consola(t_log* logger)
{
	char* leido;
	//El primero te lo dejo de yapa
	leido = readline(">");
	while(*leido != '\0'){

		log_info(logger,leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);

}

void llenar_paquete(t_paquete* paquete,t_log* logger)
{
	//Ahora toca lo divertido!
	log_info(logger,"Bienvenido! por favor ingrese los mensajes que quiera enviar al servidor: ");
	char* leido;
	leido = readline(">");
	while(*leido != '\0'){

		log_info(logger,"Voy a cargar al paquete el siguiente valor: %s",leido);
		agregar_a_paquete(paquete,leido,strlen(leido)+1);
		free(leido);
		leido = readline(">");
	}

	free(leido);
	

}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	//Y por ultimo, para cerrar, hay que liberar lo que utilizamos (conexion, log y config) con las funciones de las commons y del TP mencionadas en el enunciado
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
