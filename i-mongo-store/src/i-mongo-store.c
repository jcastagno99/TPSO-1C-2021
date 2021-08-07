#include "i-mongo-store-lib.h"
void handler_sabotaje(int signal)
{
	estoy_saboteado = 1;
	bloque_corrupto = -1;
	printf("\033[1;33mSabotaje detectado. Enviando información a Discordiador...\033[0m\n");
	char* puerto = itoa_propio(i_mongo_store_configuracion->PUERTO_DISCORDIADOR);
	int conexion_discordiador = crear_conexion(i_mongo_store_configuracion ->IP_DISCORDIADOR, puerto);
	posicion nueva_pos = get_proximo_sabotaje_y_avanzar_indice();
	log_warning(logger_i_mongo_store, "Sabotaje detectado en %i %i", nueva_pos.pos_x, nueva_pos.pos_y);
	void *stream = pserializar_posicion(nueva_pos.pos_x,nueva_pos.pos_y);
	uint32_t size_paquete = 2 * sizeof(uint32_t);
	enviar_paquete(conexion_discordiador, INICIAR_SABOTAJE, size_paquete, stream);
	printf("\033[1;33mSabotaje enviado exitosamente!\033[0m\n");
	t_paquete *respuesta = recibir_paquete(conexion_discordiador);
	respuesta_ok_fail rta = deserializar_respuesta_ok_fail(respuesta->stream);
	log_info(logger_i_mongo_store,"Se recibio la respuesta a iniciar sabotaje %d",rta);
	liberar_paquete(respuesta);
	free(puerto);
	close(conexion_discordiador);
}

void handler_sigint(int signal){
	pthread_mutex_destroy(&mutex_oxigeno);
	pthread_mutex_destroy(&mutex_comida);
	pthread_mutex_destroy(&mutex_basura);
	pthread_mutex_destroy(&mutex_lista_bitacoras);
	log_destroy(logger_i_mongo_store);
	pthread_mutex_destroy(&superbloque_bitarray_mutex);
	int tam_bloques = get_block_size();
	int cant_bloques = get_block_amount(); 
	munmap(superbloque,2*sizeof(uint32_t)+(cant_bloques/8) + 1);
	munmap(blocks_mapeado, tam_bloques*cant_bloques);
	free(blocks);
	list_destroy_and_destroy_elements(i_mongo_store_configuracion->POSICIONES_SABOTAJE,free);
	free(i_mongo_store_configuracion->PUNTO_MONTAJE);
	free(i_mongo_store_configuracion->IP_DISCORDIADOR);
	free(i_mongo_store_configuracion);
	free(ruta_superbloque);
	free(ruta_blocks);
	free(carpeta_files);
	free(carpeta_bitacoras);
	free(carpeta_md5);
	free(ruta_info_blocks);
	free(ruta_info_blocks_aux);
	close(fd_bloques);
	close(fd_superbloques);
	bitacora_trip_mutex* un_trip;
	for(int i = 0; i< mutex_bitacoras->elements_count; i++){
		un_trip = list_get(mutex_bitacoras,i);
		pthread_mutex_destroy(un_trip->mutex_bitacora);
		free(un_trip->mutex_bitacora);
		free(un_trip);
	}
	list_destroy(lista_hilos);
	list_destroy(mutex_bitacoras);
	free(tamanios_global);
	exit(0);
}
int main(int argc, char *argv[])
{
	estoy_saboteado = 0;
	signal(SIGUSR1, handler_sabotaje);
	signal(SIGINT, handler_sigint);
	system("rm cfg/i-mongo-store.log");
	pthread_mutex_init(&mutex_lista_bitacoras,NULL);
	pthread_mutex_init(&mutex_oxigeno,NULL);
	pthread_mutex_init(&mutex_comida,NULL);
	pthread_mutex_init(&mutex_basura,NULL);
	mutex_bitacoras = list_create();
	lista_hilos = list_create();
	pthread_mutex_init(&superbloque_bitarray_mutex, NULL); //creo que este SI es necesario
	i_mongo_store_configuracion = leer_config_i_mongo_store(argv[1]);
	armar_lista_de_posiciones(argv[1]);
	i_mongo_store_configuracion->INDICE_ULTIMO_SABOTAJE = 0;
	logger_i_mongo_store = iniciar_logger_i_mongo_store(argv[2]);
	char *borro_polus = argv[4];
	if (strcmp(borro_polus, "1") == 0)
	{
		system("rm -r /home/utnso/polus");
	}
	inicializar_rutas(i_mongo_store_configuracion->PUNTO_MONTAJE);
	inicializar_filesystem(argv[3]);
	esperar_conexion(i_mongo_store_configuracion->PUERTO);
	pthread_join(hilo_sincronizacion, NULL);
}

void inicializar_rutas(char *montaje)
{
	ruta_superbloque = malloc(strlen(montaje) + strlen("/Superbloque.ims") + 1);
	memcpy(ruta_superbloque, montaje, strlen(montaje) + 1);
	strcat(ruta_superbloque, "/Superbloque.ims");
	ruta_blocks = malloc(strlen(montaje) + strlen("/Blocks.ims") + 1);
	memcpy(ruta_blocks, montaje, strlen(montaje) + 1);
	strcat(ruta_blocks, "/Blocks.ims");
	carpeta_files = malloc(strlen(montaje) + strlen("/Files/") + 1);
	memcpy(carpeta_files, montaje, strlen(montaje) + 1);
	strcat(carpeta_files, "/Files/");
	carpeta_bitacoras = malloc(strlen(montaje) + strlen("/Bitacoras/") + 1);
	memcpy(carpeta_bitacoras, montaje, strlen(montaje) + 1);
	strcat(carpeta_bitacoras, "/Bitacoras/");
	carpeta_md5 = malloc(strlen(montaje)+ strlen("/MD5/")+1);
	memcpy(carpeta_md5,montaje, strlen(montaje)+1);
	strcat(carpeta_md5,"/MD5/");
	ruta_info_blocks = malloc(strlen(montaje) + strlen("/Infoblocks.ims")+1);
	memcpy(ruta_info_blocks, montaje, strlen(montaje)+1);
	strcat(ruta_info_blocks, "/Infoblocks.ims");
	ruta_info_blocks_aux = malloc(strlen(montaje) + strlen("/Infoblocks_aux.ims")+1);
	memcpy(ruta_info_blocks_aux, montaje, strlen(montaje)+1);
	strcat(ruta_info_blocks_aux, "/Infoblocks_aux.ims");
}