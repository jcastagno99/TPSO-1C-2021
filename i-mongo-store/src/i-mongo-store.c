#include "i-mongo-store-lib.h"

bool reparar_sabotaje_block_count_en_archivo(char *file_path){
	char *full_path = string_from_format("/home/utnso/polus/Files/%s", file_path);
	t_config *archivo = config_create(full_path);
	char **test = config_get_array_value(archivo, "BLOCKS");
	int cantidad_real_de_blocks = 0;
	while (test[cantidad_real_de_blocks])
		cantidad_real_de_blocks++;
	
	int v = config_get_int_value(archivo, "BLOCK_COUNT");
	
	if(v == cantidad_real_de_blocks){
		config_destroy(archivo);
		return false;
	}else{
		log_error(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Block Count detectado en %s . Corrigiendo...", file_path);
		config_set_value(archivo, "BLOCK_COUNT", string_itoa(cantidad_real_de_blocks));
		int t = config_get_int_value(archivo, "BLOCK_COUNT");
		config_save(archivo);
		config_destroy(archivo);
		log_warning(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Block_Count corregido exitosamente!");
		return true;
	}

}

bool reparar_sabotaje_md5_en_archivo(char *file_path){
	char *full_path = string_from_format("/home/utnso/polus/Files/%s", file_path);
	t_config *archivo = config_create(full_path);
	printf(logger_i_mongo_store, "[ I-mongo ] Analizando el archivo %s...", file_path);
	char *string_a_hashear = armar_string_para_MD5(archivo);
	char *hash = obtener_MD5(string_a_hashear, archivo);
	char *md5 = config_get_string_value(archivo, "MD5");

	if(string_equals_ignore_case(md5, hash)){
		printf("[ I-mongo ] El archivo %s no está saboteado.", file_path);
		config_destroy(archivo);
		return false;

	}else{
		log_error(logger_i_mongo_store, "[ I-Mongo ] Sabotaje MD5 detectado en %s . Corrigiendo...", file_path);
		int block_size = get_block_size();
		int size = config_get_int_value(archivo, "SIZE");
		char **array_blocks = config_get_array_value(archivo, "BLOCKS");
		int block_count = config_get_int_value(archivo, "BLOCK_COUNT");
		char *caracter = config_get_string_value(archivo, "CARACTER_LLENADO");
		int res;
		int i = 0;
		char rellenar_con_esto[32];

		while(i<block_count){
			int block = atoi(array_blocks[i]);
			if(i == block_count-1){
				//Acá estaría en el último bloque por lo tanto hay fragmentación interna
				for (int j = 0; j < block_size; j++)
				{
					if(size > 0){
						rellenar_con_esto[j]=caracter[0];
						size--;
					}else{
						rellenar_con_esto[j]='\0';
					}
				}
			}else{//Si no estoy en el último bloque, quiere decir que no tengo fragmentación interna
			// asi que lleno todo el bloque de una
				size -= block_size;
				for (int j = 0; j < block_size; j++)
				{
					rellenar_con_esto[j]=caracter[0];
				}
			}
			memcpy(blocks + (block * block_size), rellenar_con_esto, block_size);
			i++;
		}
		config_save(archivo);
		config_destroy(archivo);
		log_warning(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Block_Count corregido exitosamente!");
		return true;
	}
}

bool reparar_sabotaje_size_en_archivo(char *file_path){
	char *full_path = string_from_format("/home/utnso/polus/Files/%s", file_path);
	t_config *archivo = config_create(full_path);
	// Primero tengo que leer todos los bloques y contar cuantos caracteres de llenado tengo
	// Luego tengo que compararlo con el size que tengo actualmente para ver si está corrupto
	// Si lo está, piso ese valor con lo que conté
	int block_size = get_block_size();
	char **array_blocks = config_get_array_value(archivo, "BLOCKS");
	int block_count = config_get_int_value(archivo, "BLOCK_COUNT");
	char *caracter = config_get_string_value(archivo, "CARACTER_LLENADO");
	int size_posiblemente_corrupto = config_get_int_value(archivo, "SIZE");
	int contador_de_caracteres_escritos = 0;

	for (int i = 0; i < block_count; i++)
	{
		if(i == block_count - 1){
			//voy a leer hasta el \0 ya que voy a tener fragmentación interna
			int bloque_actual = atoi(array_blocks[i]);
			char *buffer = malloc(block_size);
			memcpy(buffer, blocks + (bloque_actual * block_size), block_size);
			int j=0;
			while (buffer[j] != '\0')
			{
				contador_de_caracteres_escritos++;
				j++;
			}
			free(buffer);
		}else
			contador_de_caracteres_escritos += block_size;
	}

	if(size_posiblemente_corrupto == contador_de_caracteres_escritos){
		config_destroy(archivo);
		return false;
	}else{
		log_error(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Size detectado en %s . Corrigiendo...", file_path);
		config_set_value(archivo, "SIZE", string_itoa(contador_de_caracteres_escritos));
		config_save(archivo);
		config_destroy(archivo);
		log_warning(logger_i_mongo_store, "[ I-Mongo ] Sabotaje Size corregido exitosamente!");
		return true;
	}
}

void cargar_bitmap_temporal(char *full_path, int *bitmap_temporal){
	t_config *archivo = config_create(full_path);
	char **used_blocks = config_get_array_value(archivo, "BLOCKS");
	int size_array = config_get_int_value(archivo, "BLOCK_COUNT");
				
	for (int i = 0; i<size_array; i++)
	{
		int block = atoi(used_blocks[i]);
		bitmap_temporal[block] = 1;
	}		
	config_destroy(archivo);
}

bool sabotaje_block_count(){
	log_warning(logger_i_mongo_store, "[ I-Mongo ] Detectando sabotaje Block Count...");
	DIR *d;
	struct dirent *dir;
	d = opendir("/home/utnso/polus/Files");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				if(reparar_sabotaje_block_count_en_archivo(dir->d_name))
					return true;
			}
		}
		closedir(d);
	}
	log_warning(logger_i_mongo_store, "[ I-Mongo ] No se detecto el sabotaje Block Count.");
	return false;
}

bool sabotaje_superbloque(){
	return false;
}

bool sabotaje_size(){
	log_warning(logger_i_mongo_store, "[ I-Mongo ] Detectando sabotaje Size...");
	DIR *d;
	struct dirent *dir;
	d = opendir("/home/utnso/polus/Files");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				if(reparar_sabotaje_size_en_archivo(dir->d_name))
					return true;
			}
		}
		closedir(d);
	}
	log_warning(logger_i_mongo_store, "[ I-Mongo ] No se detecto el sabotaje Size...");
	return false;
}

bool sabotaje_md5(){
	DIR *d;
	struct dirent *dir;
	d = opendir("/home/utnso/polus/Files");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				if(reparar_sabotaje_md5_en_archivo(dir->d_name))
					return true;
			}
		}
		closedir(d);
	}
	log_warning(logger_i_mongo_store, "[ I-Mongo ] No se detecto el sabotaje MD5.");
	return false;
}

bool sabotaje_bitmap(){
	log_warning(logger_i_mongo_store, "[ I-Mongo ] Detectando sabotaje Bitmap...");
	t_bitarray *bitarray = bitarray_create_with_mode(superbloque+2*sizeof(uint32_t), get_block_amount()/8, MSB_FIRST);
	DIR *d;
	struct dirent *dir;
	int total_block_amount = get_block_amount();
	int *bitmap_temporal = malloc(total_block_amount*sizeof(int));
	memset(bitmap_temporal, 0, total_block_amount*sizeof(int));

	d = opendir("/home/utnso/polus/Bitacoras");

	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				char *full_path = string_from_format("/home/utnso/polus/Bitacoras/%s", dir->d_name);
				cargar_bitmap_temporal(full_path, bitmap_temporal);
			}
		}
		closedir(d);
	}

	d = opendir("/home/utnso/polus/Files");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(!string_equals_ignore_case(dir->d_name, ".") && !string_equals_ignore_case(dir->d_name, "..")){
				char *full_path = string_from_format("/home/utnso/polus/Files/%s", dir->d_name);
				cargar_bitmap_temporal(full_path, bitmap_temporal);
			}
		}
		closedir(d);
	}

	for (int i = 0; i < total_block_amount; i++)
	{
		if((!bitarray_test_bit(bitarray, i)) == bitmap_temporal[i]){
			printf("Comparando %i %i\n", bitarray_test_bit(bitarray, i), bitmap_temporal[i]);
			log_error(logger_i_mongo_store, "[ I-mongo ] Sabotaje Bitmap detectado en el bloque %i. Corrigiendo...", i);
			bitarray_set_bit(bitarray, bitmap_temporal[i]);
			log_warning(logger_i_mongo_store, "[ I-mongo ] Sabotaje Bitmap corregido exitosamente!", i);
			bitarray_destroy(bitarray);
			return true;
		}
	}
	
	bitarray_destroy(bitarray);
	log_warning(logger_i_mongo_store, "[ I-Mongo ] No se detecto el sabotaje Bitmap...");
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

	if(sabotaje_block_count()) return;
	if(sabotaje_superbloque()) return;
	if(sabotaje_size()) return;
	if(sabotaje_md5()) return;
	if(sabotaje_bitmap()) return;
	
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