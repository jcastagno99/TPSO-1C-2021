#include "i-mongo-store-lib.h"
#include <stdlib.h>

i_mongo_store_config *leer_config_i_mongo_store(char *path)
{
	t_config *config_aux = config_create(path);
	i_mongo_store_config *config_i_mongo_store_aux = malloc(sizeof(i_mongo_store_config));

	config_i_mongo_store_aux->PUNTO_MONTAJE = config_get_string_value(config_aux, "PUNTO_MONTAJE");
	config_i_mongo_store_aux->PUERTO = config_get_int_value(config_aux, "PUERTO");
	config_i_mongo_store_aux->TIEMPO_SINCRONIZACION = config_get_int_value(config_aux, "TIEMPO_SINCRONIZACION");
	config_i_mongo_store_aux->IP_DISCORDIADOR = config_get_string_value(config_aux, "IP_DISCORDIADOR");
	config_i_mongo_store_aux->PUERTO_DISCORDIADOR = config_get_int_value(config_aux, "PUERTO_SABOTAJE_DISCORDIADOR");
	return config_i_mongo_store_aux;
	// No hace falta liberar memoria
}

t_log *iniciar_logger_i_mongo_store(char *path)
{

	t_log *log_aux = log_create(path, "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	return log_aux;
}

void *esperar_conexion(int puerto)
{
	t_list *hilos = list_create();
	int socket_escucha = iniciar_servidor_i_mongo_store(puerto);
	int socket_i_mongo_store;
	log_info(logger_i_mongo_store, "SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_i_mongo_store = accept(socket_escucha, &cliente, &len);
		if (socket_i_mongo_store > 0)
		{
			log_info(logger_i_mongo_store, "NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_i_mongo_store);
		}
		else
		{
			log_error(logger_i_mongo_store, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
		}
	} while (1);
}

int iniciar_servidor_i_mongo_store(int puerto)
{
	int socket_v;
	int val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(puerto);

	socket_v = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_v < 0)
	{
		char *error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket_v, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket_v, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		return EXIT_FAILURE;
	}
	if (listen(socket_v, MAX_CLIENTS) < 0)
	{
		return EXIT_FAILURE;
	}

	return socket_v;
}

void crear_hilo_para_manejar_suscripciones(t_list *lista_hilos, int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_create(&hilo_conectado, NULL, (void *)manejar_suscripciones_i_mongo_store, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);
}

//Socket envio es para enviar la respuesta, de momento no lo uso
void *manejar_suscripciones_i_mongo_store(int *socket_envio)
{
	log_info(logger_i_mongo_store, "Se creó el hilo correctamente");
	t_paquete *paquete = recibir_paquete(*socket_envio);
	void* stream = paquete->stream;
	switch(paquete->codigo_operacion){
		case GENERAR_RECURSO :{
			operacion_recurso op_c_rec = deserializar_operacion_recurso(stream);
			op_c_rec.recurso = a_mayusc_primera_letra(op_c_rec.recurso);
			char* escritura = NULL;
			if(op_c_rec.cantidad > 0);
				escritura = malloc(op_c_rec.cantidad + 1);
			char* archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(op_c_rec.recurso) + 1);
			memcpy(archivo, carpeta_files, strlen(carpeta_files)+1);
			strcat(archivo, op_c_rec.recurso);
			strcat(archivo, ".ims");
			for (int i = 0; i<op_c_rec.cantidad;i++){
				escritura[i] = op_c_rec.recurso[0];
			}
				escribir_archivo(archivo, escritura);
		}
		
		case CONSUMIR_RECURSO :{
			operacion_recurso op_c_rec = deserializar_operacion_recurso(stream);
			op_c_rec.recurso = a_mayusc_primera_letra(op_c_rec.recurso);
			char* escritura = NULL;
			if(op_c_rec.cantidad > 0);
				escritura = malloc(op_c_rec.cantidad + 1);
			char* archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(op_c_rec.recurso) + 1);
			memcpy(archivo, carpeta_files, strlen(carpeta_files)+1);
			strcat(archivo, op_c_rec.recurso);
			strcat(archivo, ".ims");
			for (int i = 0; i<op_c_rec.cantidad;i++){
				escritura[i] = op_c_rec.recurso[0];
			}
				quitar_de_archivo(archivo, escritura);
		}
		case VACIAR_RECURSO :{
			char* rec = deserializar_recurso(stream);
			rec = a_mayusc_primera_letra(rec);
			char* archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(rec) + 1);
			memcpy(archivo, carpeta_files, strlen(carpeta_files)+1);
			strcat(archivo, rec);
			strcat(archivo, ".ims");
			borrar_archivo(archivo);
		}
		case REGISTRAR_MOVIMIENTO : {
			movimiento_tripulante trip_y_movs = deserializar_movimiento_tripulante(stream);
			char* posX_vieja = itoa_propio(trip_y_movs.pos_x_vieja);
			char* posY_vieja = itoa_propio(trip_y_movs.pos_y_vieja);
			char* posX_nueva = itoa_propio(trip_y_movs.pos_x_nueva);
			char* posY_nueva = itoa_propio(trip_y_movs.pos_y_nueva);
			char* tripulante_id = itoa_propio(trip_y_movs.tid);
			char* frase1 = "El tripulante: ";
			char* frase2 = "se movió desde la posición X: ";
			char* frase3 = "e Y: ";
			char* frase4 = "a la posición nueva X: ";
			char* frase5 = "y nueva Y: ";
			char* frase = malloc(strlen(posX_vieja)+strlen(posY_vieja)+strlen(posX_nueva)+strlen(posY_nueva)
			+ strlen(tripulante_id)+strlen(frase1)+strlen(frase2)+strlen(frase3)+strlen(frase4)+strlen(frase5)+1);
			frase[0] = '\0'; //creo que con esto me evito el memcpy
			strcat(frase,frase1);
			strcat(frase,tripulante_id);
			strcat(frase,frase2);
			strcat(frase,posX_vieja);
			strcat(frase,frase3);
			strcat(frase,posY_vieja);
			strcat(frase,frase4);
			strcat(frase,posX_nueva);
			strcat(frase,frase5);
			strcat(frase,posY_nueva);
			char* archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
			memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras)+1);
			strcat(archivo, "Tripulante");
			strcat(archivo, tripulante_id);
			strcat(archivo, ".ims");
			escribir_archivo(archivo,frase);
		}
		case REGISTRAR_COMIENZO_TAREA : {
			tripulante_con_tarea trip_c_tarea = deserializar_tripulante_con_tarea(stream);
		}
		case REGISTRAR_FIN_TAREA : {
			tripulante_con_tarea trip_c_tarea =deserializar_tripulante_con_tarea(stream);
		}
		case REGISTRAR_MOVIMIENTO_A_SABOTAJE : {
			movimiento_tripulante trip_c_tarea = deserializar_movimiento_tripulante(stream);
		}
		case REGISTRAR_SABOTAJE_RESUELTO : {
			uint32_t tid;
			memcpy(&tid,stream,sizeof(uint32_t));
		}
	}
	liberar_paquete(paquete);
	close(*socket_envio);
	free(socket_envio);
	return NULL;
}

void inicializar_filesystem(uint32_t block_size, uint32_t block_amount)
{
	crear_directorio(i_mongo_store_configuracion->PUNTO_MONTAJE);
	inicializar_superbloque(block_size, block_amount);
	inicializar_blocks(block_size, block_amount);
	crear_directorio(carpeta_files);
	crear_directorio(carpeta_bitacoras);
	log_info(logger_i_mongo_store, "FILESYSTEM INICIALIZADO DE 0 CON EXITO");
}

void inicializar_superbloque(uint32_t block_size, uint32_t block_amount)
{
	int fd = open(ruta_superbloque, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd == -1)
	{
		no_pude_abrir_archivo(ruta_superbloque);
		exit(-1);
	}
	ftruncate(fd,2 * sizeof(uint32_t) + block_amount);
	superbloque = mmap(NULL, 2 * sizeof(uint32_t) + block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (superbloque == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_superbloque);
		exit(-1);
	}
	memcpy(superbloque, &block_size, sizeof(uint32_t));
	memcpy(superbloque + sizeof(uint32_t), &block_amount, sizeof(uint32_t));
	int offset = 2 * sizeof(uint32_t);
	bool estado = 0;
	for (int i = 0; i < block_amount; i++)
	{
		memcpy(superbloque + offset, &estado, sizeof(bool));
		offset += sizeof(bool);
	}
	msync(superbloque, 2 * sizeof(uint32_t) + block_amount, 0);
	struct stat *algo = malloc(sizeof(struct stat));
	int resultado = fstat(fd, algo);
	if(resultado ==-1){
		log_error(logger_i_mongo_store, "no anduvo fstat");
	}
	else{
		log_info(logger_i_mongo_store,"cant bloques: %d, tam bloques: %d, ",algo->st_blocks,algo->st_blksize);
	}
}

void inicializar_blocks(uint32_t block_size, uint32_t block_amount)
{
	int fd = open(ruta_blocks, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd == -1)
	{
		no_pude_abrir_archivo(ruta_blocks);
	}
	fallocate(fd, 0, 0, block_size * block_amount);
	blocks = mmap(NULL, block_size * block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (blocks == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_blocks);
		exit(-1);
	
	}
}

void crear_directorio(char *carpeta){
	int res = mkdir(carpeta, (mode_t)0777);
	if (res == -1)
	{
		log_error(logger_i_mongo_store, "No pude crear la carpeta %s", carpeta);
		exit(-1);
	}
}

void no_pude_abrir_archivo(char *ruta_archivo)
{
	log_error(logger_i_mongo_store, "No se pudo abrir el archivo %s", ruta_archivo);
}

void no_pude_mapear_archivo(char *ruta_archivo)
{
	log_error(logger_i_mongo_store, "No se pudo mapear el archivo %s", ruta_archivo);
}

char* a_mayusc_primera_letra(char* palabra){
	for(int i = 0; palabra[i]; i++){
		if(i == 0){
			palabra[i] = toupper(palabra[i]);
		}
		else{
			palabra[i] = tolower(palabra[i]);
		}
	}
	return palabra;
}

bool existe_recurso(char* recurso){
	char* archivo = malloc(strlen(carpeta_files)+strlen(recurso)+strlen(".ims")+1);
	memcpy(archivo, carpeta_files, strlen(carpeta_files)+1);
	strcat(archivo,recurso);
	strcat(archivo,".ims");

	DIR* dir = opendir(archivo);
	if(dir){
		closedir(dir);
		return 1;
	} else if (ENOENT == errno){
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir pero no existe", archivo);
		return 0;
	} else{
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir. Existe pero hubo otro fallo", archivo);
		return 1;
	}
	return 0;
}