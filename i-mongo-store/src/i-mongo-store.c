#include "i-mongo-store-lib.h"

bool reparar_block_count_saboteado(char *file_path){
	char *full_path = string_from_format("/home/utnso/polus/Files/%s", file_path);
	t_config *archivo = config_create(full_path);
	char **test = config_get_array_value(archivo, "BLOCKS");
	int cantidad_real_de_blocks = 0;
	while (test[cantidad_real_de_blocks])
		cantidad_real_de_blocks++;
	
	int v = config_get_int_value(archivo, "BLOCK_COUNT");
	
	if(v == cantidad_real_de_blocks)
		return false;
	else{
		log_error(logger_i_mongo_store, "[ I-Mongo ] Sabotaje detectado. Corrigiendo");
		config_set_value(archivo, "BLOCK_COUNT", string_itoa(cantidad_real_de_blocks));
		int t = config_get_int_value(archivo, "BLOCK_COUNT");
		config_save(archivo);
		config_destroy(archivo);
		log_warning(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Block_Count corregido exitosamente!");
		return true;
	}

}

bool sabotaje_block_count(){
	log_warning(logger_i_mongo_store, "[ I-Mongo ] Detectando Sabotaje Block Count...");
	DIR *d;
	struct dirent *dir;
	d = opendir("/home/utnso/polus/Files");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				if(reparar_block_count_saboteado(dir->d_name))
					return true;
			}
		}
		closedir(d);
	}
	log_warning(logger_i_mongo_store, "[ I-Mongo ] No se detecto el sabotaje Block Count.");
	return false;
}

void handler_sabotaje(int signal)
{
	printf("\033[1;33mSabotaje detectado. Enviando información a Discordiador...\033[0m\n");
	char* puerto = itoa_propio(i_mongo_store_configuracion->PUERTO_DISCORDIADOR);
	int conexion_discordiador = crear_conexion(i_mongo_store_configuracion ->IP_DISCORDIADOR, puerto);
	posicion nueva_pos = get_proximo_sabotaje_y_avanzar_indice();
	void *stream = pserializar_posicion(nueva_pos.pos_x,nueva_pos.pos_y);
	uint32_t size_paquete = 2 * sizeof(uint32_t);

	if(sabotaje_block_count())
	
	//enviar_paquete(conexion_discordiador, INICIAR_SABOTAJE, size_paquete, stream);
	//printf("\033[1;33mSabotaje enviado exitosamente!\033[0m\n");
	//t_paquete *respuesta = recibir_paquete(conexion_discordiador);
	//[TODO] validar respuesta
	//printf("\033[1;32mPaquete recibido. Sabotaje finalizado! ** \033[0m\n");
	free(puerto);
	close(conexion_discordiador);
}

void handler_sigint(int signal){
	pthread_mutex_destroy(&mutex_lista_bitacoras);
	log_destroy(logger_i_mongo_store);
	pthread_mutex_destroy(&superbloque_bitarray_mutex);
	int tam_bloques = get_block_size();
	int cant_bloques = get_block_amount(); 
	munmap(superbloque,2*sizeof(uint32_t)+(cant_bloques/8) + 1);
	munmap(blocks, tam_bloques*cant_bloques);
	list_destroy_and_destroy_elements(i_mongo_store_configuracion->POSICIONES_SABOTAJE,free);
	free(i_mongo_store_configuracion->PUNTO_MONTAJE);
	free(i_mongo_store_configuracion->IP_DISCORDIADOR);
	free(i_mongo_store_configuracion);
	free(ruta_superbloque);
	free(ruta_blocks);
	free(carpeta_files);
	free(carpeta_bitacoras);
	free(carpeta_md5);
	close(fd_bloques);
	close(fd_superbloques);
	bitacora_trip_mutex* un_trip;
	for(int i = 0; i< mutex_bitacoras->elements_count; i++){
		un_trip = list_get(mutex_bitacoras,i);
		pthread_mutex_destroy(un_trip->mutex_bitacora);
		free(un_trip->mutex_bitacora);
		free(un_trip);
	}
	pthread_t* un_hilo;
	list_destroy(lista_hilos);
	list_destroy(mutex_bitacoras);
	free(tamanios_global);
	exit(0);
}
int main(int argc, char *argv[])
{
	signal(SIGUSR1, handler_sabotaje);
	signal(SIGINT, handler_sigint);
	system("rm cfg/i-mongo-store.log");
	pthread_mutex_init(&mutex_lista_bitacoras,NULL);
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
}