#include "i-mongo-store-lib.h"
#include <stdlib.h>

i_mongo_store_config *leer_config_i_mongo_store(char *path)
{
	t_config *config_aux = config_create(path);
	i_mongo_store_config *config_i_mongo_store_aux = malloc(sizeof(i_mongo_store_config));
	config_i_mongo_store_aux->PUNTO_MONTAJE = malloc(strlen(config_get_string_value(config_aux, "PUNTO_MONTAJE"))+1);
	memcpy(config_i_mongo_store_aux->PUNTO_MONTAJE, config_get_string_value(config_aux, "PUNTO_MONTAJE"),strlen(config_get_string_value(config_aux, "PUNTO_MONTAJE"))+1);
	config_i_mongo_store_aux->PUERTO = config_get_int_value(config_aux, "PUERTO");
	config_i_mongo_store_aux->TIEMPO_SINCRONIZACION = config_get_int_value(config_aux, "TIEMPO_SINCRONIZACION");
	config_i_mongo_store_aux->IP_DISCORDIADOR = malloc(strlen(config_get_string_value(config_aux, "IP_DISCORDIADOR"))+1);
	memcpy(config_i_mongo_store_aux->IP_DISCORDIADOR, config_get_string_value(config_aux, "IP_DISCORDIADOR"),strlen(config_get_string_value(config_aux, "IP_DISCORDIADOR"))+1);
	config_i_mongo_store_aux->PUERTO_DISCORDIADOR = config_get_int_value(config_aux, "PUERTO_SABOTAJE_DISCORDIADOR");
	config_destroy(config_aux);
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
			crear_hilo_para_manejar_suscripciones(lista_hilos, socket_i_mongo_store);
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
	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo_conectado, &tattr, (void *)manejar_suscripciones_i_mongo_store, socket_hilo);
}

//Socket envio es para enviar la respuesta, de momento no lo uso
void *manejar_suscripciones_i_mongo_store(int *socket_envio)
{
	log_info(logger_i_mongo_store, "Se creó el hilo correctamente");
	t_paquete *paquete = recibir_paquete(*socket_envio);
	void *stream = paquete->stream;
	op_code codigo_respuesta;
	uint32_t tamanio_respuesta;
	void *stream_respuesta;
	switch (paquete->codigo_operacion)
	{
	case OPERAR_SOBRE_TAREA:
	{
		tripulante_con_tarea tct = deserializar_tripulante_con_tarea(stream);
		agregar_a_lista_bitacoras_si_es_necesario(tct.tid);
		char* tarea_aux = malloc(strlen(tct.tarea)+1);
		memcpy(tarea_aux,tct.tarea,strlen(tct.tarea)+1);
		int resultado1 = registrar_comienzo_tarea(tarea_aux,tct.tid);
		int resultado2;
		operaciones operacion_sobre_archivo = conseguir_operacion(tct.tarea);
		if(operacion_sobre_archivo != 3){		
			char* recurso = conseguir_recurso(tct.tarea);
			int cant_unidades = conseguir_parametros(tct.tarea);
			switch(operacion_sobre_archivo){
				case GENERAR:
					resultado2 = generar_recurso(recurso,cant_unidades,tct.tid);
				break;
				case CONSUMIR:
					resultado2 = consumir_recurso(recurso,cant_unidades);
				break;
				case DESCARTAR:
					resultado2 = descartar_recurso(recurso);
				case OTRA:
					resultado2 = 1;
				break;
			}
			free(recurso);
		}else{
			resultado2 = 1;
			}
		respuesta_ok_fail resultado;
		if(resultado1 == 1 && resultado2 == 1){
			resultado = RESPUESTA_OK;
		} else{
			resultado = RESPUESTA_FAIL;
		}
		codigo_respuesta = RESPUESTA_OPERAR_SOBRE_TAREA;
		stream_respuesta = serializar_respuesta_ok_fail(resultado);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
		free(tct.tarea);
		

	}
	break;

	case REGISTRAR_MOVIMIENTO:
	{
		movimiento_tripulante trip_y_movs = deserializar_movimiento_tripulante(stream);
		agregar_a_lista_bitacoras_si_es_necesario(trip_y_movs.tid);
		char *posX_vieja = itoa_propio(trip_y_movs.pos_x_vieja);
		char *posY_vieja = itoa_propio(trip_y_movs.pos_y_vieja);
		char *posX_nueva = itoa_propio(trip_y_movs.pos_x_nueva);
		char *posY_nueva = itoa_propio(trip_y_movs.pos_y_nueva);
		char *tripulante_id = itoa_propio(trip_y_movs.tid);
		char *frase1 = "El tripulante: ";
		char *frase2 = " se movio desde la posicion X: ";
		char *frase3 = " e Y: ";
		char *frase4 = " a la posicion nueva X: ";
		char *frase5 = " y nueva Y: ";
		char *frase = malloc(strlen(posX_vieja) + strlen(posY_vieja) + strlen(posX_nueva) + strlen(posY_nueva) + strlen(tripulante_id) + strlen(frase1) + strlen(frase2) + strlen(frase3) + strlen(frase4) + strlen(frase5) + 1 + 1);
		frase[0] = '\0'; //creo que con esto me evito el memcpy
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, posX_vieja);
		strcat(frase, frase3);
		strcat(frase, posY_vieja);
		strcat(frase, frase4);
		strcat(frase, posX_nueva);
		strcat(frase, frase5);
		strcat(frase, posY_nueva);
		strcat(frase, "\n");
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA, trip_y_movs.tid);
		free(tripulante_id);
		free(posX_vieja);
		free(posY_vieja);
		free(posX_nueva);
		free(posY_nueva);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_MOVIMIENTO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_FIN_TAREA:
	{
		tripulante_con_tarea trip_c_tarea = deserializar_tripulante_con_tarea(stream);
		agregar_a_lista_bitacoras_si_es_necesario(trip_c_tarea.tid);
		char *frase1 = "El tripulante: ";
		char *frase2 = " ha finalizado la tarea: ";
		char *tripulante_id = itoa_propio(trip_c_tarea.tid);
		char *frase = malloc(strlen(frase1) + strlen(frase2) + strlen(tripulante_id) + strlen(trip_c_tarea.tarea) + 1 + 1);
		frase[0] = '\0';
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, trip_c_tarea.tarea);
		strcat(frase, "\n");
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA, trip_c_tarea.tid);
		free(trip_c_tarea.tarea);
		free(tripulante_id);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_FIN_TAREA;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_MOVIMIENTO_A_SABOTAJE:
	{
		movimiento_tripulante trip_y_movs = deserializar_movimiento_tripulante(stream);
		agregar_a_lista_bitacoras_si_es_necesario(trip_y_movs.tid);
		char *posX_vieja = itoa_propio(trip_y_movs.pos_x_vieja);
		char *posY_vieja = itoa_propio(trip_y_movs.pos_y_vieja);
		char *posX_nueva = itoa_propio(trip_y_movs.pos_x_nueva);
		char *posY_nueva = itoa_propio(trip_y_movs.pos_y_nueva);
		char *tripulante_id = itoa_propio(trip_y_movs.tid);
		char *frase1 = "El tripulante: ";
		char *frase2 = " se movio desde la posicion X: ";
		char *frase3 = " e Y: ";
		char *frase4 = " a la posicion nueva X: ";
		char *frase5 = " y nueva Y: ";
		char *frase6 = " para solucionar el sabotaje existente";
		char *frase = malloc(strlen(posX_vieja) + strlen(posY_vieja) + strlen(posX_nueva) + strlen(posY_nueva) + strlen(tripulante_id) + strlen(frase1) + strlen(frase2) + strlen(frase3) + strlen(frase4) + strlen(frase5) + strlen(frase6) + 1 + 1);
		frase[0] = '\0'; //creo que con esto me evito el memcpy
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, posX_vieja);
		strcat(frase, frase3);
		strcat(frase, posY_vieja);
		strcat(frase, frase4);
		strcat(frase, posX_nueva);
		strcat(frase, frase5);
		strcat(frase, posY_nueva);
		strcat(frase, frase6);
		strcat(frase, "\n");
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA, trip_y_movs.tid);
		free(tripulante_id);
		free(posX_vieja);
		free(posY_vieja);
		free(posX_nueva);
		free(posY_nueva);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_MOVIMIENTO_A_SABOTAJE;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_SABOTAJE_RESUELTO:
	{
		uint32_t tid;
		memcpy(&tid, stream, sizeof(uint32_t));
		agregar_a_lista_bitacoras_si_es_necesario(tid);
		char *frase1 = "El sabotaje ha sido resuelto por el tripulante: ";
		char *tripulante_id = itoa_propio(tid);
		char *frase = malloc(strlen(frase1) + strlen(tripulante_id) + 1 + 1);
		frase[0] = '\0';
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, "\n");
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA, tid);
		free(tripulante_id);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_SABOTAJE_RESUELTO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case OBTENER_BITACORA:
	{
		uint32_t tid;
		memcpy(&tid, stream, sizeof(uint32_t));
		agregar_a_lista_bitacoras_si_es_necesario(tid);
		char *tripulante_id = itoa_propio(tid);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		FILE* fd = fopen(archivo, "r+");
		char *contenido;
		if (fd == NULL && errno == ENOENT)
		{
			log_error(logger_i_mongo_store, "Se quiso leer la bitacora del tripulante %d, pero la misma no existe", tid);
			contenido = "BITACORA NO EXISTENTE";
		} else {
			contenido = todo_el_archivo(archivo,tid);
		}
		uint32_t tamanio = strlen(contenido) + 1;
		tamanio_respuesta = tamanio + sizeof(uint32_t);
		stream_respuesta = malloc(tamanio + sizeof(uint32_t));
		memcpy(stream_respuesta, &tamanio, sizeof(uint32_t));
		memcpy(stream_respuesta + sizeof(uint32_t), contenido, tamanio);
		codigo_respuesta = RESPUESTA_OBTENER_BITACORA;
		free(tripulante_id);
		free(archivo);
		free(contenido);
		fclose(fd);
	}

	default:
		//no me rompas las bolainas
		break;
	}
	liberar_paquete(paquete);
	t_paquete *paquete_respuesta = malloc(sizeof(t_paquete));
	paquete_respuesta->codigo_operacion = codigo_respuesta;
	paquete_respuesta->size = tamanio_respuesta;
	paquete_respuesta->stream = stream_respuesta;
	void *a_enviar = serializar_paquete(paquete_respuesta, paquete_respuesta->size + sizeof(op_code) + sizeof(size_t));
	send(*socket_envio, a_enviar, tamanio_respuesta + sizeof(op_code) + sizeof(size_t), 0);
	free(a_enviar);
	close(*socket_envio);
	free(socket_envio);
	return NULL;
}

void inicializar_filesystem(char* ruta_bloques_config)
{
	uint32_t block_size;
	uint32_t block_amount;
	uint32_t *tamanios = malloc(2 * sizeof(uint32_t));
	if(crear_directorio(i_mongo_store_configuracion->PUNTO_MONTAJE)){
		t_config* config = config_create(ruta_bloques_config);
		block_size = config_get_int_value(config,"BLOCK_SIZE");
		block_amount = config_get_int_value(config,"BLOCK_COUNT");
		config_destroy(config);
		log_info(logger_i_mongo_store, "El directorio %s no existia. Creando", i_mongo_store_configuracion->PUNTO_MONTAJE);
		inicializar_superbloque(block_size, block_amount);
		inicializar_blocks(block_size, block_amount);
		crear_directorio(carpeta_files);
		crear_directorio(carpeta_bitacoras);
		crear_directorio(carpeta_md5);
		log_info(logger_i_mongo_store, "FILESYSTEM INICIALIZADO DE 0 CON EXITO");

	}else{
		log_info(logger_i_mongo_store, "El directorio %s ya existia. Iniciando con FS existente", i_mongo_store_configuracion->PUNTO_MONTAJE);
		inicializar_superbloque_existente(&block_size,&block_amount);
		inicializar_blocks_existente(block_size,block_amount);
	}
	tamanios[0] = 2 * sizeof(uint32_t) + block_amount/8 + 1;
	tamanios[1] = block_amount * block_size;
	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
	pthread_create(&hilo_sincronizacion, &tattr, sincronizar, (void *)tamanios);
	tamanios_global = tamanios;
}

void inicializar_superbloque_existente(uint32_t* block_size_ref, uint32_t* block_amount_ref){
	uint32_t* tamanio_ref = malloc(sizeof(uint32_t));
	uint32_t* cant_ref = malloc(sizeof(uint32_t));
	int res1;
	fd_superbloques = open(ruta_superbloque, O_RDWR, (mode_t)0777);
	if (fd_superbloques == -1)
	{
		no_pude_abrir_archivo(ruta_superbloque);
		exit(-1);
	}
	res1 = read(fd_superbloques,tamanio_ref,sizeof(uint32_t));
	if( res1 == sizeof(uint32_t)){
		log_info(logger_i_mongo_store,"El tamanio existente de los bloques es de %d bytes",*tamanio_ref);
	} else{
		log_error(logger_i_mongo_store, "No se pudo leer el tamanio de los bloques. Se esperaban leer %d bytes y se leyeron %d", sizeof(uint32_t), res1);
	}
	int res2 = pread(fd_superbloques ,cant_ref,sizeof(uint32_t), sizeof(uint32_t));
	if( res2 == sizeof(uint32_t)){
		log_info(logger_i_mongo_store,"la cantidad existente de los bloques es de %d bytes",*cant_ref);
	} else{
		log_error(logger_i_mongo_store, "No se pudo leer la cantidad de bloques. Se esperaban leer %d bytes y se leyeron %d", sizeof(uint32_t), res2);
	}
	*block_size_ref = *tamanio_ref;
	*block_amount_ref = *cant_ref;
	free(tamanio_ref);
	free(cant_ref);
	superbloque = mmap(NULL, 2 * sizeof(uint32_t) + (*block_amount_ref)/8 + 1, PROT_WRITE | PROT_READ, MAP_SHARED, fd_superbloques, 0);
	if (superbloque == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_superbloque);
		exit(-1);
	}
}

void inicializar_superbloque(uint32_t block_size, uint32_t block_amount)
{
	fd_superbloques = open(ruta_superbloque, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd_superbloques == -1)
	{
		no_pude_abrir_archivo(ruta_superbloque);
		exit(-1);
	}
	ftruncate(fd_superbloques, 2 * sizeof(uint32_t) + block_amount/8 + 1);
	superbloque = mmap(NULL, 2 * sizeof(uint32_t) + block_amount/8 + 1, PROT_WRITE | PROT_READ, MAP_SHARED, fd_superbloques, 0);
	if (superbloque == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_superbloque);
		exit(-1);
	}
	memcpy(superbloque, &block_size, sizeof(uint32_t));
	memcpy(superbloque + sizeof(uint32_t), &block_amount, sizeof(uint32_t));
	char vector_disponibilidad[block_amount/8];
	for (int i = 0; i < block_amount/8; i++)
	{
		vector_disponibilidad[i] = 0;
	}
	t_bitarray *bitarray = bitarray_create_with_mode(vector_disponibilidad, sizeof(vector_disponibilidad), MSB_FIRST);
	memcpy(superbloque + 2 * sizeof(uint32_t), bitarray->bitarray, sizeof(vector_disponibilidad));
	msync(superbloque, 2 * sizeof(uint32_t) + block_amount/8 +1, 0);
	char fin_archivo = '\0';
	memcpy(superbloque + 2 * sizeof(uint32_t)+ sizeof(vector_disponibilidad)+1,&fin_archivo,sizeof(char));
	log_info(logger_i_mongo_store,"El tamanio de los bloques es de %d bytes",block_size);
	log_info(logger_i_mongo_store,"la cantidad de los bloques es de %d bytes",block_amount);
	bitarray_destroy(bitarray);
}

void inicializar_blocks(uint32_t block_size, uint32_t block_amount)
{
	fd_bloques = open(ruta_blocks, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd_bloques == -1)
	{
		no_pude_abrir_archivo(ruta_blocks);
	}
	fallocate(fd_bloques, 0, 0, block_size * block_amount);
	blocks = mmap(NULL, block_size * block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd_bloques, 0);
	if (blocks == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_blocks);
		exit(-1);
	}
}

void inicializar_blocks_existente(uint32_t block_size, uint32_t block_amount)
{
	fd_bloques = open(ruta_blocks, O_RDWR, (mode_t)0777);
	if (fd_bloques == -1)
	{
		no_pude_abrir_archivo(ruta_blocks);
	}
	blocks = mmap(NULL, block_size * block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd_bloques, 0);
	if (blocks == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_blocks);
		exit(-1);
	}
}

bool crear_directorio(char *carpeta)
{
	int res = mkdir(carpeta, (mode_t)0777);
	if (res == -1)
	{
		return 0;
	}
	else{
		return 1;
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

char *a_mayusc_primera_letra(char *palabra)
{
	for (int i = 0; palabra[i]; i++)
	{
		if (i == 0)
		{
			palabra[i] = toupper(palabra[i]);
		}
		else
		{
			palabra[i] = tolower(palabra[i]);
		}
	}
	return palabra;
}

bool existe_recurso(char *recurso)
{
	char *archivo = malloc(strlen(carpeta_files) + strlen(recurso) + strlen(".ims") + 1);
	memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
	strcat(archivo, recurso);
	strcat(archivo, ".ims");

	DIR *dir = opendir(archivo);
	if (dir)
	{
		closedir(dir);
		return 1;
	}
	else if (ENOENT == errno)
	{
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir pero no existe", archivo);
		return 0;
	}
	else
	{
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir. Existe pero hubo otro fallo", archivo);
		return 1;
	}
	return 0;
}

void *sincronizar(void *tamanio)
{ //como si fuera un array
	while (1)
	{
		uint32_t *tamanios = (uint32_t *)tamanio;
		msync(superbloque, tamanios[0], 0);
		msync(blocks, tamanios[1], 0);
		log_info(logger_i_mongo_store, "Se sincronizaron Blocks y SUperbloques");
		sleep(i_mongo_store_configuracion->TIEMPO_SINCRONIZACION);
	}
}

char *itoa_propio(uint32_t entero)
{
	int aux;
	int digitos;
	char convertido;
	digitos = 0;
	aux = entero;
	if (entero == 0)
	{
		char *unCero = malloc(2);
		unCero[0] = entero + '0';
		unCero[1] = '\0';
		return unCero;
	}
	while (aux > 0)
	{
		aux = aux / 10;
		digitos += 1;
	}
	char *cadena = malloc(digitos + 1);
	cadena[digitos] = '\0';
	while (entero)
	{
		convertido = entero % 10 + '0';
		cadena[digitos - 1] = convertido;
		digitos--;
		entero = entero / 10;
	}
	return cadena;
}

int escribir_archivo(char *ruta, char *contenido, tipo_archivo tipo, uint32_t tid)
{
	pthread_mutex_t* el_mutex;
	if(tipo == BITACORA){
		el_mutex = buscar_mutex_con_tid(tid);
		pthread_mutex_lock(el_mutex);
	}
	log_info(logger_i_mongo_store,"por escribir %s en %s", contenido,ruta);
	FILE *archivo;
	archivo = fopen(ruta, "r+");
	if (archivo == NULL && errno == ENOENT)
	{
		crear_archivo_metadata(ruta, tipo, contenido[0]);
		log_warning(logger_i_mongo_store, "Se creo el archivo %s", ruta);
	}
	else if (archivo == NULL && errno != ENOENT)
	{
		fclose(archivo);
		log_error(logger_i_mongo_store, "El archivo %s existe, pero no se pudo abrir", ruta);
		if(tipo == BITACORA){
		pthread_mutex_unlock(el_mutex);
		}
		return -1;
	}

	if(archivo != NULL){
		fclose(archivo);
	}
	t_config *llave_valor = config_create(ruta);
	int cant_bloques = config_get_int_value(llave_valor, "BLOCK_COUNT");
	int longitud_restante = strlen(contenido);
	int caracteres_llenados = 0;
	int block_size = get_block_size();
	if (cant_bloques != 0)
	{
		int nro_ultimo_bloque = conseguir_bloque(llave_valor, cant_bloques, cant_bloques - 1);
		char *ultimo_bloque = malloc(block_size);
		memcpy(ultimo_bloque, blocks + (nro_ultimo_bloque * block_size), block_size);
		int ultima_posicion_escrita = encontrar_anterior_barra_cero(ultimo_bloque, block_size); //Devuelve la posición del ultimo caracter anterior al barra cero
		if((ultimo_bloque[block_size-1] == '\0' && ultima_posicion_escrita == block_size - 2) || ultima_posicion_escrita < block_size - 2){ //CHEQUEO EL CASO EXTREMO EN EL QUE EL
		// BLOQUE YA ESTABA COMPLETAMENTE LLENO OSEA NO HABIA BARRA CERO
			for (int j = 1; (j + ultima_posicion_escrita) < block_size && longitud_restante > 0; j++)
			{ //Arranco desde el \0
				ultimo_bloque[j + ultima_posicion_escrita] = contenido[j - 1];
				longitud_restante--;
				caracteres_llenados++;
			}
			log_info(logger_i_mongo_store, "Se han llenado %d caracteres en el bloque %d, asignados al archivo %s",
				 caracteres_llenados, nro_ultimo_bloque, ruta);
			if (ultima_posicion_escrita + caracteres_llenados + 1 < block_size)
			{ //Me sobró bloque, pongo barra cero
				ultimo_bloque[ultima_posicion_escrita + caracteres_llenados + 1] = '\0';
			}
			memcpy(blocks + (nro_ultimo_bloque * block_size), ultimo_bloque, block_size);
			int tamanio_viejo = config_get_int_value(llave_valor,"SIZE");
			char* tamanio_nuevo = itoa_propio(tamanio_viejo + caracteres_llenados);
			config_set_value(llave_valor,"SIZE",tamanio_nuevo);
			if(tipo == RECURSO){
				char* string_md5 = armar_string_para_MD5(llave_valor);
				char* md5 = obtener_MD5(string_md5,llave_valor);
				config_set_value(llave_valor, "MD5",md5);
				free(md5);
			}

			free(ultimo_bloque);
			free(tamanio_nuevo);
			config_save(llave_valor);
		 }else{
			free(ultimo_bloque);
		}

	}
	while (longitud_restante > 0)
	{
		int caracteres_llenados_viejos = caracteres_llenados;
		pthread_mutex_lock(&superbloque_bitarray_mutex);
		int primer_bloque_libre = get_primer_bloque_libre();
		ocupar_bloque(primer_bloque_libre);
		pthread_mutex_unlock(&superbloque_bitarray_mutex);
		char *escritura = agregar_a_lista_blocks(llave_valor,primer_bloque_libre);
		config_set_value(llave_valor, "BLOCKS", escritura);
		int cant_bloques_vieja = config_get_int_value(llave_valor, "BLOCK_COUNT");
		cant_bloques_vieja++;
		char *cant_bloques_vieja_aux = itoa_propio(cant_bloques_vieja);
		config_set_value(llave_valor, "BLOCK_COUNT", cant_bloques_vieja_aux);
		char *bloque_a_llenar = malloc(block_size);
		memcpy(bloque_a_llenar, blocks + (primer_bloque_libre * block_size), block_size);
		if (longitud_restante > block_size)
		{
			memcpy(bloque_a_llenar, contenido + caracteres_llenados, block_size);
			longitud_restante -= block_size;
			caracteres_llenados += block_size;
		}
		else
		{
			int ultimo_byte = longitud_restante;
			memcpy(bloque_a_llenar, contenido + caracteres_llenados, longitud_restante);
			if(longitud_restante != block_size)
				bloque_a_llenar[ultimo_byte] = '\0';
			longitud_restante = 0;
			caracteres_llenados += ultimo_byte;
		}
		memcpy(blocks + primer_bloque_libre * block_size, bloque_a_llenar, block_size);
		free(bloque_a_llenar);
		log_info(logger_i_mongo_store, "Se han llenado %d caracteres en el bloque %d, asignados al archivo %s, TOTAL: %d",
				 caracteres_llenados - caracteres_llenados_viejos, primer_bloque_libre, ruta,caracteres_llenados);
		int size_viejo = config_get_int_value(llave_valor,"SIZE");
		char* nuevos_caracteres_llenados = itoa_propio(caracteres_llenados - caracteres_llenados_viejos + size_viejo);
		config_set_value(llave_valor,"SIZE",nuevos_caracteres_llenados);
		if(tipo == RECURSO){
			char* string_md5 = armar_string_para_MD5(llave_valor);
			char* md5 = obtener_MD5(string_md5,llave_valor);
			log_warning(logger_i_mongo_store,"valor md5: %s", md5);
			config_set_value(llave_valor, "MD5",md5);
			free(md5);
		}
		int cant_llaves = config_keys_amount(llave_valor);
		if(cant_llaves == 0)
			log_error(logger_i_mongo_store, "GUARDANDO CONFIG VACIO. Caso iteracion de bloques. Tamanio: %d . Cant bloques: %d", nuevos_caracteres_llenados, cant_bloques_vieja);
		config_save(llave_valor);
		free(escritura);
		free(cant_bloques_vieja_aux);
		free(nuevos_caracteres_llenados);
	}
	log_info(logger_i_mongo_store, "Se ha finalizado la operación de escritura del archivo %s y de los bloques correspondientes", ruta);
	config_destroy(llave_valor);
	if(tipo == BITACORA){
		pthread_mutex_unlock(el_mutex);
	}
	return 1;
}

int quitar_de_archivo(char *ruta, char *contenido)
{
	FILE *archivo;
	archivo = fopen(ruta, "r+");
	if (archivo == NULL && errno == ENOENT)
	{
		log_error(logger_i_mongo_store, "Se quiso borrar %s, pero no existe el archivo %s", contenido, ruta);
		return -1;
	}
	else if (archivo == NULL && errno != ENOENT)
	{
		log_error(logger_i_mongo_store, "El archivo %s existe, pero no se pudo abrir", ruta);
	}
	t_config *llave_valor = config_create(ruta);
	int cant_bloques = config_get_int_value(llave_valor, "BLOCK_COUNT");
	int cant_bloques_actual = cant_bloques;
	int longitud_restante = strlen(contenido);
	int caracteres_vaciados = 0;
	int block_size = get_block_size();
	char *tipo_recurso = config_get_string_value(llave_valor, "CARACTER_LLENADO");
	int nro_ultimo_bloque = conseguir_bloque(llave_valor, cant_bloques, cant_bloques - 1);
	if(nro_ultimo_bloque == -1){
		log_error(logger_i_mongo_store, "El recurso %c no tenia asignado ningun byte ni bloque. Abortando operacion",contenido[0]);
		config_destroy(llave_valor);
		return 0;
	}
	char *ultimo_bloque = malloc(block_size);
	memcpy(ultimo_bloque, blocks + (nro_ultimo_bloque * block_size), block_size);
	int ultima_posicion_escrita = encontrar_anterior_barra_cero(ultimo_bloque, block_size);
	int caracteres_disponibles_para_consumir = ((cant_bloques - 1) * block_size) + ultima_posicion_escrita + 1;
	int tamanio_original = config_get_int_value(llave_valor, "SIZE");
	fclose(archivo);
	//CASO 1: No hay suficientes caracteres
	if (longitud_restante > caracteres_disponibles_para_consumir && longitud_restante > 0)
	{
		log_info(logger_i_mongo_store, "Se quitaran mas recursos de los existentes en la operación de consumir %d unidades de recurso %s. Habian %d Vaciando archivo...",
				  longitud_restante, tipo_recurso, caracteres_disponibles_para_consumir);
		char** bloques = config_get_array_value(llave_valor,"BLOCKS");
		int nro_bloque;
		pthread_mutex_lock(&superbloque_bitarray_mutex);
				  	for (int i = 0; i<cant_bloques;i++){
						nro_bloque = atoi(bloques[i]);
						vaciar_bloque(nro_bloque);
						liberar_bloque(nro_bloque);
						free(bloques[i]);
					  }
		pthread_mutex_unlock(&superbloque_bitarray_mutex);
		config_set_value(llave_valor,"SIZE","0");
		config_set_value(llave_valor,"BLOCK_COUNT","0");
		config_set_value(llave_valor,"BLOCKS","[]");
		config_set_value(llave_valor,"MD5","");
		config_save(llave_valor);
		free(bloques);
		free(ultimo_bloque);
		config_destroy(llave_valor);
		return 1;
		}
	else
	{
		//CASO 2: No es necesario liberar ningun bloque
		if (longitud_restante < ultima_posicion_escrita)
		{
			for(int k= ultima_posicion_escrita; longitud_restante != 0; k--){
				ultimo_bloque[k] = '\0';
				longitud_restante--;
				caracteres_vaciados++;
			}
			log_info(logger_i_mongo_store, "Se borraron %d unidades de %c en el bloque %d", caracteres_vaciados, contenido[0],nro_ultimo_bloque);
			memcpy(blocks + (nro_ultimo_bloque * block_size), ultimo_bloque, block_size);
			char* nuevo_size = itoa_propio(tamanio_original - caracteres_vaciados);
			config_set_value(llave_valor,"SIZE",nuevo_size);
			char* string_md5 = armar_string_para_MD5(llave_valor);
			char* md5 = obtener_MD5(string_md5,llave_valor);
			config_set_value(llave_valor, "MD5",md5);
			config_save(llave_valor);
			free(ultimo_bloque);
			free(nuevo_size);
			free(md5);
		}
		//CASO 3: Hay que liberar el ultimo bloque minimamente
		else
		{
			for(int j = ultima_posicion_escrita; j>= 0; j--){
				ultimo_bloque[j] = '\0';
				longitud_restante--;
				caracteres_vaciados++;
			}
			log_info(logger_i_mongo_store, "Se borraron %d unidades de %c en el bloque %d", caracteres_vaciados, contenido[0],nro_ultimo_bloque);
			pthread_mutex_lock(&superbloque_bitarray_mutex);
			liberar_bloque(nro_ultimo_bloque);
			pthread_mutex_unlock(&superbloque_bitarray_mutex);
			cant_bloques_actual--;
			memcpy(blocks + (block_size) * nro_ultimo_bloque, ultimo_bloque, block_size);
			int indice_ultimo_bloque_sin_vaciar = conseguir_bloque(llave_valor,cant_bloques,cant_bloques_actual-1);
			while (longitud_restante >= block_size)
			{
				vaciar_bloque(indice_ultimo_bloque_sin_vaciar);
				caracteres_vaciados += block_size;
				log_info(logger_i_mongo_store, "Se consumieron %d unidades del recurso %c , vaciando el bloque %d", block_size, contenido[0], indice_ultimo_bloque_sin_vaciar);			
				pthread_mutex_lock(&superbloque_bitarray_mutex);
				liberar_bloque(indice_ultimo_bloque_sin_vaciar);
				pthread_mutex_unlock(&superbloque_bitarray_mutex);
				cant_bloques_actual--;
				indice_ultimo_bloque_sin_vaciar--;
				longitud_restante -= block_size;
			}
			//CASO 3.1: longitud_restante es 0 y cant_bloques_actual es 0. Esto es porque arriba me fije si habia lugar suficiente y si llegue aca es porque lo hay.
			if (cant_bloques_actual == 0)
			{
				log_info(logger_i_mongo_store, "De casualidad habian %d unidades del recurso %s y se consumieron exactamente %d. Sin bloques", caracteres_disponibles_para_consumir,
						 tipo_recurso, caracteres_vaciados);
				config_set_value(llave_valor, "BLOCK_COUNT", "0");
				config_set_value(llave_valor, "BLOCKS", "[]");
				config_set_value(llave_valor, "SIZE","0");
				config_set_value(llave_valor, "MD5", "");
				config_save(llave_valor);
				free(ultimo_bloque);
			}
			//CASO 3.2: Ya libere todos los bloques que habia que liberar. Ahora toca quitar los caracteres restantes del que seria el ultimo bloque ahora.
			else
			{
				nro_ultimo_bloque = conseguir_bloque(llave_valor, cant_bloques, cant_bloques_actual - 1);
				memcpy(ultimo_bloque, blocks + (block_size * nro_ultimo_bloque), block_size);
				ultima_posicion_escrita = encontrar_anterior_barra_cero(ultimo_bloque, block_size);
				int longitud_restante_antes_de_iterar = longitud_restante;

				if (longitud_restante != 0){
					for(int l = block_size - 1; l>=block_size-longitud_restante_antes_de_iterar; l--){
					ultimo_bloque[l] = '\0';
					longitud_restante--;
					caracteres_vaciados++;
					}
					memcpy(blocks + (nro_ultimo_bloque * block_size), ultimo_bloque, block_size);
					log_info(logger_i_mongo_store, "Se borraron %d unidades del recurso %s con exito. Se liberaron %d bloques", longitud_restante_antes_de_iterar, tipo_recurso,
						 cant_bloques - cant_bloques_actual);
				}
				char *nueva_cant_bloques = itoa_propio(cant_bloques_actual);
				char *nuevo_size = itoa_propio(tamanio_original - caracteres_vaciados);
				config_set_value(llave_valor,"SIZE",nuevo_size);
				char* nuevos_blocks = setear_nuevos_blocks(llave_valor, cant_bloques_actual);
				config_set_value(llave_valor, "BLOCK_COUNT", nueva_cant_bloques);
				config_set_value(llave_valor, "BLOCKS", nuevos_blocks);
				char* string_md5 = armar_string_para_MD5(llave_valor);
				char* md5 = obtener_MD5(string_md5,llave_valor);
				config_set_value(llave_valor, "MD5",md5);
				config_save(llave_valor);
				free(ultimo_bloque);
				free(nueva_cant_bloques);
				free(nuevo_size);
				free(nuevos_blocks);
				free(md5);
			}
		}
	}
	config_destroy(llave_valor);
	return 1;
}

int borrar_archivo(char *archivo)
{	
	FILE* fd = fopen(archivo,"r+");
	if(fd != NULL){
		t_config *llave_valor = config_create(archivo);
		char **bloques = config_get_array_value(llave_valor, "BLOCKS");
		int cant_bloques = config_get_int_value(llave_valor, "BLOCK_COUNT");
		int nro_bloque;
		pthread_mutex_lock(&superbloque_bitarray_mutex);
		for (int i = 0; i<cant_bloques;i++){
			nro_bloque = atoi(bloques[i]);
			vaciar_bloque(nro_bloque);
			liberar_bloque(nro_bloque);
			free(bloques[i]);
		}
		pthread_mutex_unlock(&superbloque_bitarray_mutex);
		free(bloques);
		config_destroy(llave_valor);
	}	
	int resultado_borrar = unlink(archivo);
	if(resultado_borrar == 0){
		log_info(logger_i_mongo_store, "Se borro el archivo de metadata: %s", archivo);
	}else if(errno == ENOENT){
		log_info(logger_i_mongo_store, "Se intento borrar el archivo de metadata: %s, pero no existia", archivo);
	}
	else{
		log_error(logger_i_mongo_store, "Se quiso borrar el archivo %s, pero ocurrio un error inesperado",archivo);
	}
/*	config_set_value(llave_valor,"BLOCKS","[]");
	char* el_cero = itoa_propio(0);
	config_set_value(llave_valor,"BLOCK_COUNT",el_cero);
	free(el_cero);
	config_save_in_file(llave_valor,archivo);*/
	return 1;
}

char *todo_el_archivo(char *archivo, uint32_t tid)
{
	pthread_mutex_t* el_mutex = buscar_mutex_con_tid(tid);
	pthread_mutex_lock(el_mutex);
	t_config *llave_valor = config_create(archivo);
	char **bloques = config_get_array_value(llave_valor, "BLOCKS");
	int cant_bloques = config_get_int_value(llave_valor, "BLOCK_COUNT");
	int nro_bloque;
	int block_size = get_block_size();
	int nro_ultimo_bloque = conseguir_bloque(llave_valor, cant_bloques, cant_bloques - 1);
	char *ultimo_bloque = malloc(block_size);
	memcpy(ultimo_bloque, blocks + (nro_ultimo_bloque * block_size), block_size);
	int ultima_posicion_escrita = encontrar_anterior_barra_cero(ultimo_bloque, block_size);
	char* texto = malloc(cant_bloques * block_size + ultima_posicion_escrita + 2);
	texto[0] = '\0';
	char* un_bloque_lleno;
	for (int i = 0; i<cant_bloques-1;i++){
		nro_bloque = atoi(bloques[i]);
		un_bloque_lleno = malloc(block_size+1);
		un_bloque_lleno[block_size] = '\0';
		memcpy(un_bloque_lleno,blocks + (nro_bloque * block_size), block_size);
		strcat(texto, un_bloque_lleno);
		free(un_bloque_lleno);
	}
	strcat(texto,ultimo_bloque);
	free(ultimo_bloque);
	liberar_lista_string(bloques);
	config_destroy(llave_valor);
	pthread_mutex_unlock(el_mutex);
	return texto;
}

void crear_archivo_metadata(char *ruta, tipo_archivo tipo, char caracter_llenado)
{
	FILE *archivo = fopen(ruta, "w+");
	fclose(archivo);
	t_config *f = config_create(ruta);
	config_set_value(f, "SIZE", "0");
	config_set_value(f, "BLOCK_COUNT", "0");
	config_set_value(f, "BLOCKS", "[]");
	char *car_llenado = malloc(sizeof(char)+1);
	car_llenado[0] = caracter_llenado;
	car_llenado[1] = '\0';
	if (tipo == RECURSO)
	{
		config_set_value(f, "CARACTER_LLENADO", car_llenado);
		config_set_value(f, "MD5", "");
	}
	config_save(f);
	config_destroy(f);
	free(car_llenado);
}

int conseguir_bloque(t_config *llave_valor, int cant_bloques, int indice)
{
	char **bloques = config_get_array_value(llave_valor, "BLOCKS");
	if(cant_bloques != 0){
		char *retorno = bloques[indice];
		for (int i = 0; i < cant_bloques; i++)
		{
			if (i != indice)
				free(bloques[i]);
		}
		free(bloques);
		int retorno_posta = atoi(retorno);
		free(retorno);
		return retorno_posta;
	}else{
		free(bloques);
		return -1;
	}

}

int encontrar_anterior_barra_cero(char *ultimo_bloque, int block_size)
{
	int i;
	bool encontrado = 0;
	for (i = 0; i < block_size && !encontrado; i++)
	{
		if (ultimo_bloque[i] == '\0')
		{
			encontrado = 1;
		}
	}
	return i - 1 - 1;
}

int get_block_size()
{
	uint32_t bs;
	memcpy(&bs, superbloque, sizeof(uint32_t));
	return bs;
}

int get_block_amount()
{
	uint32_t ba;
	memcpy(&ba, superbloque + sizeof(uint32_t), sizeof(uint32_t));
	return ba;
}

int get_primer_bloque_libre()
{
	int cant_bloques = get_block_amount();
	t_bitarray *bit_array = bitarray_create_with_mode(superbloque + 2*sizeof(uint32_t),cant_bloques/8,MSB_FIRST);
	bool ocupado = 1;
	int i;
	for (i = 0; i < bitarray_get_max_bit(bit_array) && ocupado; i++)
	{
		ocupado = bitarray_test_bit(bit_array, i);
	}
	bitarray_destroy(bit_array);
	if (ocupado == 0)
	{
		return i-1;
	}
	else
	{
		log_error(logger_i_mongo_store,"NO HAY MAS BLOQUES LIBRES");
		return -1;
	}
}

void liberar_bloque(int numero_bloque)
{
	int cant_bloques = get_block_amount();
	t_bitarray *bit_array = bitarray_create_with_mode(superbloque + 2*sizeof(uint32_t),cant_bloques/8,MSB_FIRST);
	bitarray_clean_bit(bit_array, numero_bloque);
	log_info(logger_i_mongo_store,"Se libero el bloque %d", numero_bloque);
	free(bit_array);
}

void ocupar_bloque(int numero_bloque)
{
	int cant_bloques = get_block_amount();
	t_bitarray *bit_array = bitarray_create_with_mode(superbloque + 2*sizeof(uint32_t),cant_bloques/8,MSB_FIRST);
	bitarray_set_bit(bit_array, numero_bloque);
	log_info(logger_i_mongo_store, "Se ocupo el bloque %d",numero_bloque);
	free(bit_array);
}

char* setear_nuevos_blocks(t_config* config, int cant_bloques_actual){
	int cant_bloques_vieja = config_get_int_value(config,"BLOCK_COUNT");
	char** blocks_como_arrays = config_get_array_value(config,"BLOCKS");
	int memoria_a_alocar_numeros = 0;
	for(int i = 0; i<cant_bloques_actual;i++){
		memoria_a_alocar_numeros += strlen(blocks_como_arrays[i]);
	}
	char* blocks_nuevos = malloc(2 * sizeof(char) + memoria_a_alocar_numeros + cant_bloques_actual - 1 + 1 ); // 2 por los corchetes, cant_bloques_actual -1 para las comas, 1 por el barra cero
	blocks_nuevos[0] = '[';
	blocks_nuevos[2 * sizeof(char) + memoria_a_alocar_numeros + cant_bloques_actual - 1] = '\0';
	int posicion_a_escribir_proximo_numero = 1;
	uint32_t proximo_numero;
	char* prox_numero_como_string;
	for(int j = 0; j<cant_bloques_actual;j++){
		proximo_numero = atoi(blocks_como_arrays[j]);
		prox_numero_como_string = itoa_propio(proximo_numero);
		memcpy(blocks_nuevos + posicion_a_escribir_proximo_numero, prox_numero_como_string, strlen(prox_numero_como_string));
		blocks_nuevos[posicion_a_escribir_proximo_numero + strlen(prox_numero_como_string)] = ',';
		posicion_a_escribir_proximo_numero = posicion_a_escribir_proximo_numero + strlen(prox_numero_como_string) + 1;
		free(prox_numero_como_string);
	}
	blocks_nuevos[2 * sizeof(char) + memoria_a_alocar_numeros + cant_bloques_actual - 2] = ']';
	for(int k = 0; k < cant_bloques_vieja; k++){
		free(blocks_como_arrays[k]);
	}
	free(blocks_como_arrays);
	return blocks_nuevos;
}

int generar_recurso(char* recurso, int cantidad, uint32_t tid){
		recurso = a_mayusc_primera_letra(recurso);
		char *escritura = NULL;
		if (cantidad > 0){
			escritura = malloc(cantidad + 2);
			char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(recurso) + 1);
			memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
			strcat(archivo,recurso);
			strcat(archivo, ".ims");
			for (int i = 0; i < cantidad; i++)
			{
				escritura[i] = recurso[0];
			} 
			escritura[cantidad] = '\0';
			escribir_archivo(archivo, escritura, RECURSO, tid);
			free(archivo);
			free(escritura);
		}
		return 1;
}

int descartar_recurso(char* rec){
		rec = a_mayusc_primera_letra(rec);
		char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(rec) + 1);
		memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
		strcat(archivo, rec);
		strcat(archivo, ".ims");
		borrar_archivo(archivo);
		free(archivo);
		return 1;
}

int consumir_recurso(char* recurso, int cantidad){
		recurso = a_mayusc_primera_letra(recurso);
		char *escritura = NULL;
		if (cantidad > 0){
			escritura = malloc(cantidad + 1);
			char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(recurso) + 1);
			memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
			strcat(archivo, recurso);
			strcat(archivo, ".ims");
			for (int i = 0; i < cantidad; i++)
			{
				escritura[i] = recurso[0];
			}
			escritura[cantidad] = '\0';
			quitar_de_archivo(archivo, escritura);
			free(archivo);
			free(escritura);
			}
		return 1;
}

int registrar_comienzo_tarea(char* tarea, uint32_t tid){
		log_info(logger_i_mongo_store, "Se recibio la tarea %s", tarea);
		char *frase1 = "El tripulante: ";
		char *frase2 = " ha comenzado con la tarea: ";
		char *tripulante_id = itoa_propio(tid);
		char *frase = malloc(strlen(frase1) + strlen(frase2) + strlen(tripulante_id) + strlen(tarea) + 1 +1); //uno por el \0, otro por el \n
		memcpy(frase,frase1,strlen(frase1)+1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, tarea);
		strcat(frase, "\n");
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1); 
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA, tid);
		free(tarea);
		free(tripulante_id);
		free(frase);
		free(archivo);
		return 1;
}

operaciones conseguir_operacion(char* tarea){
	char** operacion = string_split(tarea, "_");
		if(strcmp(operacion[0],"GENERAR")==0){
			liberar_lista_string(operacion);
			return GENERAR;
		}	
		if(strcmp(operacion[0],"CONSUMIR")==0){
			liberar_lista_string(operacion);
			return CONSUMIR;
		}	
		if(strcmp(operacion[0],"DESCARTAR")==0){
			liberar_lista_string(operacion);
			return DESCARTAR;
		}
		else{
			liberar_lista_string(operacion);
			return OTRA;
		}
}	

char* conseguir_recurso(char* tarea){
	char** str_spl = string_split(tarea," ");
	char** recurso = string_split(str_spl[0],"_");
	char* rec_ret = malloc(strlen(recurso[1])+1);
	memcpy(rec_ret,recurso[1],strlen(recurso[1])+1);
	liberar_lista_string(str_spl);
	liberar_lista_string(recurso);
	return rec_ret;
}

int conseguir_parametros(char* tarea){
	char** str_spl = string_split(tarea," ");
	char** parametros = string_split(str_spl[1],";");
	int unidades = atoi(parametros[0]);
	liberar_lista_string(str_spl);
	liberar_lista_string(parametros);
	return unidades;
}

void armar_lista_de_posiciones(char* ruta_config){
	i_mongo_store_configuracion->POSICIONES_SABOTAJE = list_create();
	t_config* config = config_create(ruta_config);
	char *p = config_get_string_value(config,"POSICIONES_SABOTAJE");
    char**  magio = string_get_string_as_array(p);
	posicion* una_posicion;
    // PROCESO PARA RECORRER- INCLUYE LIBERACION DE MEMORIA
    int i = 0;
    while (magio[i] != NULL)
    {
		una_posicion = malloc(sizeof(posicion));
		if(i == 0){
			char* primer_elemento = magio[0];
			primer_elemento[0] = primer_elemento[1];
			primer_elemento[1] = primer_elemento[2];
			primer_elemento[2] = primer_elemento[3];
			primer_elemento[3] = '\0';
		}
        char **posiciones = string_split(magio[i], "|");
        una_posicion->pos_x = atoi(posiciones[0]);
        una_posicion->pos_y = atoi(posiciones[1]);
        list_add(i_mongo_store_configuracion->POSICIONES_SABOTAJE, una_posicion);
        liberar_lista_string(posiciones);
        i++;
    }
    liberar_lista_string(magio);
	config_destroy(config);
}

void liberar_lista_string(char **lista)
{
    int i = 0;
    while (lista[i] != NULL)
    {
        free(lista[i]);
        i++;
    }
    free(lista);
}

posicion get_proximo_sabotaje_y_avanzar_indice(){
	posicion* posicion_a_retornar = (list_get(i_mongo_store_configuracion->POSICIONES_SABOTAJE,i_mongo_store_configuracion->INDICE_ULTIMO_SABOTAJE));
	if(i_mongo_store_configuracion->INDICE_ULTIMO_SABOTAJE < (i_mongo_store_configuracion->POSICIONES_SABOTAJE->elements_count)-1){
		i_mongo_store_configuracion->INDICE_ULTIMO_SABOTAJE++;
	}else{
		i_mongo_store_configuracion->INDICE_ULTIMO_SABOTAJE = 0;
	}
	return *posicion_a_retornar;
}

char* agregar_a_lista_blocks(t_config* config, uint32_t nro_bloque){
	char* numero_bloque = itoa_propio(nro_bloque);
	char* blocks = config_get_string_value(config,"BLOCKS");
	char* resultado;
	if(blocks[1] == ']'){ //pregunto por lista vacia
		resultado = malloc(2*sizeof(char) + strlen(numero_bloque) + 1);
		resultado[0] = '[';
		for(int i = 0; i<strlen(numero_bloque);i++){
			resultado[1+i] = numero_bloque[i];
		}
		resultado[strlen(numero_bloque)+1] = ']';
		resultado[strlen(numero_bloque)+2] = '\0';
	}else{
		resultado = malloc(strlen(blocks)+1+strlen(numero_bloque)+1); // uno por una , el resto por la longitud del numero, y 1 por el barra cero
		for(int j = 0; j<strlen(blocks)-1;j++){
			resultado[j] = blocks[j];
		}
		resultado[strlen(blocks)-1] = ',';
		for(int k = 0; k<strlen(numero_bloque);k++){  
			resultado[strlen(blocks) + k] = numero_bloque[k];
		}
		resultado[strlen(blocks) + strlen(numero_bloque)] = ']';
		resultado[strlen(blocks) + strlen(numero_bloque) + 1] = '\0';
	}
	free(numero_bloque);
	return resultado;
}

void vaciar_bloque(int numero_bloque){
	int tamanio_bloque = get_block_size();
	char* bloque_a_vaciar = malloc(tamanio_bloque);
	for(int i = 0; i< tamanio_bloque; i++){
		bloque_a_vaciar[i] = '\0';
	}
	memcpy(blocks + (numero_bloque * tamanio_bloque),bloque_a_vaciar,tamanio_bloque);
	free(bloque_a_vaciar);
}

char* armar_string_para_MD5(t_config* llave_valor){
	int tamanio = config_get_int_value(llave_valor,"SIZE");
	char* retorno = malloc(tamanio);
	char** bloques = config_get_array_value(llave_valor,"BLOCKS");
	int cant_bloques = config_get_int_value(llave_valor,"BLOCK_COUNT");
	int tamanio_bloques = get_block_size();
	int tamanio_considerado = 0;
	int bloque_actual;
	char* bloque;
	for(int i = 0; tamanio > tamanio_considerado; i++){
		bloque_actual = atoi(bloques[i]);
		if(tamanio >= (tamanio_considerado + tamanio_bloques)){
			bloque = malloc(tamanio_bloques);
			memcpy(bloque, blocks + (bloque_actual * tamanio_bloques), tamanio_bloques);
			memcpy(retorno + tamanio_considerado, bloque, tamanio_bloques);
			tamanio_considerado += tamanio_bloques;
			free(bloque);
		}
		else{
			bloque = malloc(tamanio % tamanio_bloques);
			memcpy(bloque, blocks + (bloque_actual* tamanio_bloques), tamanio % tamanio_bloques);
			memcpy(retorno + tamanio_considerado, bloque, tamanio % tamanio_bloques);
			tamanio_considerado += (tamanio % tamanio_bloques);
			free(bloque);
		}
	}
	for(int j = 0; j<cant_bloques;j++){
		free(bloques[j]);
	}
	free(bloques);
	return retorno;
}

char* obtener_MD5(char* string_a_hashear, t_config* llave_valor){
	char* archivo = malloc(strlen(carpeta_md5) + sizeof(char) + strlen(".ims") + 1);
	memcpy(archivo, carpeta_md5, strlen(carpeta_md5)+1);
	char* caracter = config_get_string_value(llave_valor,"CARACTER_LLENADO");
	int longitud_string = config_get_int_value(llave_valor,"SIZE");
	strcat(archivo, caracter);
	strcat(archivo, ".ims");
	FILE* fd = fopen(archivo, "w+");
	char* comando = malloc(longitud_string + strlen(archivo) + strlen("echo ") + strlen(" | md5sum > ") + 1);
	comando[0] = '\0';
	char* md5sumcosito = " | md5sum > "; 
	strcat(comando, "echo ");
	memcpy(comando + strlen("echo "), string_a_hashear, longitud_string);
	memcpy(comando + strlen("echo ")+ longitud_string, md5sumcosito, strlen(md5sumcosito));
	memcpy(comando + strlen("echo ")+ longitud_string + strlen(md5sumcosito), archivo, strlen(archivo)+1);
	log_warning(logger_i_mongo_store,"para comando md5: %s", comando);
	//formato: echo <strings> | md5sum > utnso/polus/MD5/O.ims
	system(comando);
	free(comando);
	fseek(fd,0L,SEEK_END);
	int tamanio_retorno = ftell(fd);
	fseek(fd,0L,SEEK_SET);
	char* a_escribir = malloc(tamanio_retorno+1);
	fscanf(fd, "%s", a_escribir);
	free(string_a_hashear);
	fclose(fd);
	free(archivo);
	return a_escribir;
}

void agregar_a_lista_bitacoras_si_es_necesario(uint32_t tid){
	pthread_mutex_lock(&mutex_lista_bitacoras);
		if(!esta_el_tripulante(tid)){
			bitacora_trip_mutex* nuevo_trip_mutex = malloc(sizeof(bitacora_trip_mutex));
			nuevo_trip_mutex->mutex_bitacora = malloc(sizeof(pthread_mutex_t));
			nuevo_trip_mutex->tid = tid;
			pthread_mutex_init(nuevo_trip_mutex->mutex_bitacora,NULL);
			list_add(mutex_bitacoras,nuevo_trip_mutex);
		}
	pthread_mutex_unlock(&mutex_lista_bitacoras);
}

bool esta_el_tripulante(uint32_t tid){
	bitacora_trip_mutex* un_elemento;
	for(int i = 0; i<mutex_bitacoras->elements_count;i++){
		un_elemento = list_get(mutex_bitacoras,i);
		if(un_elemento->tid == tid){
			return 1;
		}
	}
	return 0;
}

pthread_mutex_t* buscar_mutex_con_tid(uint32_t tid){
	pthread_mutex_lock(&mutex_lista_bitacoras);
	bitacora_trip_mutex* un_elemento;
	for(int i = 0; i<mutex_bitacoras->elements_count;i++){
		un_elemento = list_get(mutex_bitacoras,i);
		if(un_elemento->tid == tid){
			pthread_mutex_unlock(&mutex_lista_bitacoras);
			return un_elemento->mutex_bitacora;
		}
	}
	log_error(logger_i_mongo_store, "Quise buscar el mutex para el tripulante %d, pero no lo encontre", tid);
	pthread_mutex_unlock(&mutex_lista_bitacoras);
	return NULL;
}