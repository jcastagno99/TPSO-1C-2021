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