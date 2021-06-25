#include "discordiador.h"

int main(void)
{
	signal(SIGINT, catch_sigint_signal);

	//inicializacion del modulo
	inicializar_discordiador();

	// ORDENAR LA destruccion de los logs y el config
	// terminar_programa(conexion_mi_ram_hq, conexion_i_mongo_store, logger, config);
}

void cargar_config()
{
	// Grado de multiprocesamiento
	char *multitask = config_get_string_value(config, "GRADO_MULTITAREA");
	grado_de_multiprocesamiento = atoi(multitask);
	// Conexiones
	ip_mi_ram_hq = config_get_string_value(config, "IP_MI_RAM_HQ");
	puerto_mi_ram_hq = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	ip_i_mongo_store = config_get_string_value(config, "IP_I_MONGO_STORE");
	puerto_i_mongo_store = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	// RR
	char *q = config_get_string_value(config, "QUANTUM");
	quantum_rr = atoi(q);
	// RETARDO
	char *retardo = config_get_string_value(config, "RETARDO_CICLO_CPU");
	tiempo_retardo_ciclo_cpu = atoi(retardo);
	// Puerto de escucha para los sabotajes
	char *puerto_escucha_sabotaje = config_get_string_value(config, "PUERTO_ESCUCHA_SABOTAJE");
	puerto_escucha = atoi(puerto_escucha_sabotaje);
	// Algoritmo
	algoritmo = config_get_string_value(config, "ALGORITMO");
}

void inicializar_discordiador()
{
	logger = log_create("cfg/discordiador.log", "discordiador.log", 1, LOG_LEVEL_INFO);
	config = config_create("cfg/discordiador.config");

	cargar_config();
	// [TODO] Planificacion
	sem_init(&sem_para_detener_hilos, 0, 0);

	// [TODO] inicializar colas/listas y semaforos
	cola_de_new = list_create();
	cola_de_ready = list_create();
	cola_de_exec = list_create();
	lista_de_patotas = list_create();
	list_total_tripulantes = list_create();
	cola_de_iniciar_patotas = queue_create();
	cola_de_block = list_create();

	pthread_mutex_init(&mutex_cola_iniciar_patota, NULL);
	pthread_mutex_init(&mutex_cola_de_ready, NULL);
	pthread_mutex_init(&mutex_cola_de_new, NULL);
	pthread_mutex_init(&mutex_cola_de_exec, NULL);
	pthread_mutex_init(&mutex_cola_de_block, NULL);
	pthread_mutex_init(&mutex_tarea, NULL);

	sem_init(&sem_contador_cola_iniciar_patota, 0, 0);
	sem_init(&sem_contador_cola_de_new, 0, 0);
	sem_init(&sem_contador_cola_de_block, 0, 0);
	sem_init(&sem_contador_cola_de_ready, 0, 0);
	sem_init(&operacion_entrada_salida_finalizada, 0, 0);

	// [TODO] Inicializacion de VAR GLOBALES
	id_patota = 1;
	id_tcb = 1;
	planificacion_pausada = true;
	cantidad_hilos_pausables = 3 + grado_de_multiprocesamiento;
	/* Quiero que se pauseen:
	1- El hilo Planificador a corto plazo
	2- El hilo planificador a Largo plazo
	3- El hilo que gestiona la operacion de E/S
	4- Cada procesador
	*/

	sem_init(&procesadores_disponibles, 0, grado_de_multiprocesamiento);

	/******************************************
	***			𝗖𝗢𝗡𝗘𝗫𝗜𝗢𝗡𝗘𝗦			   ***
	******************************************/
	//conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);


	pthread_t hilo_server;
	pthread_create(&hilo_server, NULL, esperar_conexiones, NULL);

	// HILO CREACION PATOTAS ---------------------------------------------
	pthread_t hilo_creacion_patota;
	pthread_create(&hilo_creacion_patota, NULL, gestion_patotas_a_memoria, (void *)logger);

	// HILO CONSOLA ------------------------------------------------------
	pthread_t hilo_consola;
	pthread_create(&hilo_consola, NULL, (void *)leer_consola, (void *)logger);

	// Hilo ejecutando planificador a corto plazo  -------------------------
	pthread_t hilo_planificador_a_corto_plazo;
	pthread_create(&hilo_planificador_a_corto_plazo, NULL, planificador_a_corto_plazo, NULL);

	// Hilo ejecutando planificador a largo plazo  -------------------------
	pthread_t hilo_planificador_a_largo_plazo;
	pthread_create(&hilo_planificador_a_largo_plazo, NULL, (void *)planificador_a_largo_plazo, NULL);

	// Hilo ejecutando la Cola de Bloqueado  -------------------------
	pthread_t hilo_cola_de_block;
	pthread_create(&hilo_cola_de_block, NULL, (void *)ejecutar_tripulantes_bloqueados, NULL);

	// Hilo ejecutando la Cola de Exec  -------------------------
	pthread_t hilo_cola_de_exec;
	if (string_equals_ignore_case(algoritmo, "RR"))
	{
		for (int i = 0; i < grado_de_multiprocesamiento; i++)
		{
			int *temp = (int *)malloc(sizeof(int));
			*temp = i;
			pthread_create(&hilo_cola_de_exec, NULL, (void *)procesar_tripulante_rr, (void *)temp);
		}
		log_info(logger, "[ Discordiador ] Algoritmo: %s. Quantum: %i. Grado de multiprocesamiento: %i.", algoritmo, quantum_rr, grado_de_multiprocesamiento);
	}
	else
	{
		for (int i = 0; i < grado_de_multiprocesamiento; i++)
		{
			int *temp = (int *)malloc(sizeof(int));
			*temp = i;
			pthread_create(&hilo_cola_de_exec, NULL, (void *)procesar_tripulante_fifo, (void *)temp);
		}
		log_info(logger, "[ Discordiador ] Algoritmo: %s. Grado de multiprocesamiento: %i", algoritmo, grado_de_multiprocesamiento);
	}
	pthread_join(hilo_consola, NULL);

}

// SERVIDOR QUE ESPERA UN SABOTAJE --------------
void *esperar_conexiones()
{
	t_list *hilos = list_create();
	//[TODO] borrar esto que nunca se uso ni se va a usar, ademas se crea acá dentor de esta funcion y no va a poder
	// ser referenciado desde ningun lugar.
	int socket_escucha = iniciar_servidor_discordiador(puerto_escucha);
	int socket_cliente;
	log_info(logger, "[ Discordiador ] SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto_escucha);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_cliente = accept(socket_escucha, &cliente, &len);
		if (socket_cliente > 0)
		{
			log_info(logger, "[ Discordiador ] NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_cliente);
		}
		else
		{
			log_error(logger, "[ Discordiador ] ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
		}
	} while (1);
}

int iniciar_servidor_discordiador(int puerto)
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
	pthread_create(&hilo_conectado, NULL, (void *)manejar_suscripciones_discordiador, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);
}

void manejar_suscripciones_discordiador(int *socket_hilo)
{
	int fd = *socket_hilo;
	free(socket_hilo);
	log_info(logger, "[ Discordiador ] Atendiendo sabotaje. Socket fd %i", fd);
	//LLENAME
	//CON AGUA
	//aca manejar el sabotaje
	close(fd);
}
// FIN SERVIDOR PARA SABOTAJES ---------------------------------

void leer_consola(t_log *logger)
{
	char *leido;
	leido = readline(">");
	while (*leido != '\0')
	{
		log_info(logger, leido);
		validacion_sintactica(leido); // VALIDACION SINTACTICA

		add_history(leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);
}

//----------- PLANIFICADORES--------------
void *planificador_a_corto_plazo()
{
	while (1)
	{
		chequear_planificacion_pausada();
		sem_wait(&sem_contador_cola_de_ready);
		sem_wait(&procesadores_disponibles);
		//Si hay elementos en ready y hay procesadores disponibles, pasar de ready a exec
		loggear_estado_de_cola(cola_de_ready, "Planificador a corto plazo", "Ready antes del ciclo");
		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_ready, &mutex_cola_de_ready);
		tripulante->estado = EXEC;
		respuesta_ok_fail respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
		agregar_elemento_a_cola(cola_de_exec, &mutex_cola_de_exec, tripulante);
		sem_post(&sem_contador_cola_de_exec);
		loggear_estado_de_cola(cola_de_ready, "Planificador a corto plazo", "Ready despues del ciclo");

		sleep(2);
	}
	return NULL;
}

void *planificador_a_largo_plazo()
{
	while (1)
	{
		chequear_planificacion_pausada();
		//si hay tripulantes enviados a NEW y la planificación está, actuará.
		sem_wait(&sem_contador_cola_de_new);
		loggear_estado_de_cola(cola_de_new, "Planificador a largo plazo", "New antes del ciclo");
		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_new, &mutex_cola_de_new);
		tripulante->estado = READY;
		respuesta_ok_fail respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
		agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
		sem_post(&sem_contador_cola_de_ready);
		sem_post(&(tripulante->sem_tri));
		loggear_estado_de_cola(cola_de_new, "Planificador a largo plazo", "New despues del ciclo");
	}
	return NULL;
}

// EL 'temp' o 'id' es el identificador del nucleo
void *procesar_tripulante_fifo(void *temp)
{
	int id = *((int *)temp);
	free(temp);
	respuesta_ok_fail respuesta;

	while (1)
	{
		sem_wait(&sem_contador_cola_de_exec);
		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_exec, &mutex_cola_de_exec);
		log_info(logger, "[ Procesador %i ] Ejecutando tripulante %i", id, tripulante->id);
		while (tripulante->estado == EXEC)
		{
			chequear_planificacion_pausada();
			sem_post(&(tripulante->sem_tri));
			sem_wait(&(tripulante->procesador));
			printf("[ Procesador %i ] 1 ciclo ejecutado\n", id);
		}
		log_info(logger, "[ Procesador %i ] Rafaga finalizada. Tripulante %i ejecutado", id, tripulante->id);
		switch (tripulante->estado)
		{
		case BLOCKED_E_S:
			respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
			agregar_elemento_a_cola(cola_de_block, &mutex_cola_de_block, tripulante);
			sem_post(&sem_contador_cola_de_block);
			break;
		case EXIT:
			respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
			
			log_info(logger, "[ Procesador %i ] El tripulante %i ha finalizado", id, tripulante->id);
			break;
		case EXPULSADO:
			log_info(logger, "[ Procesador %i ] El tripulante %i ha sido EXPULSADO", id, tripulante->id);
			break;
		default:
			log_error(logger, "[ Procesador %i ] Que verga pasó, no debería estar aca. Discordiador.c linea 281", id);
			break;
		}
		sem_post(&procesadores_disponibles);
	}
	return NULL;
}

void *procesar_tripulante_rr(void *temp)
{
	int id = *((int *)temp);
	free(temp);
	int quantum_actual = 0;
	respuesta_ok_fail respuesta;
	while (1)
	{
		sem_wait(&sem_contador_cola_de_exec);
		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_exec, &mutex_cola_de_exec);
		log_info(logger, "[ Procesador %i ] Ejecutando tripulante %i. Quantum actual: %i", id, tripulante->id, quantum_actual);
		while (quantum_actual < quantum_rr)
		{
			chequear_planificacion_pausada();
			sem_post(&(tripulante->sem_tri));
			sem_wait(&(tripulante->procesador));
			printf("[ Procesador %i ] 1 ciclo ejecutado\n", id);
			quantum_actual++;
			if (tripulante->estado != EXEC) //quiere decir que se bloqueó, o finalizó
				break;
		}
		log_info(logger, "[ Procesador %i ] Rafaga finalizada. Tripulante %i ejecutado. Quantum actual: %i", id, tripulante->id, quantum_actual);
		switch (tripulante->estado)
		{
		case BLOCKED_E_S:
			respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
			agregar_elemento_a_cola(cola_de_block, &mutex_cola_de_block, tripulante);
			sem_post(&sem_contador_cola_de_block);
			break;
		case EXIT:
			respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);			log_info(logger, "[ Procesador %i ] El tripulante %i ha finalizado", id, quantum_actual);
			break;
		case EXEC: //quiere decir que todavia no terminó de ejecutar, por lo que lo envío al final de ready
			tripulante->estado = READY;
			respuesta = actualizar_estado_miriam(tripulante->id,tripulante->estado);
			agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
			sem_post(&sem_contador_cola_de_ready);
			break;
		case EXPULSADO:
			log_info(logger, "[ Procesador %i ] El tripulante %i ha sido EXPULSADO", id, tripulante->id);
			break;
		default:
			log_error(logger, "[ Procesador %i ] No debería estar aca. Discordiador.c linea 342\n", id);
			break;
		}
		if (quantum_actual == quantum_rr)
			quantum_actual = 0;
		// lo reseteo porque si el proceso finalizo/bloqueó justo al fin de quantum
		// y yo no lo reseteo, lo que va a pasar es que el siguiente proceso que entre a exec va a ser automaticamente
		// desalojado sin ejecutar nada.
		// asi que este reset se tiene que hacer.
		sem_post(&procesadores_disponibles);
	}
	return NULL;
}

void *ejecutar_tripulantes_bloqueados()
{
	while (1)
	{
		sem_wait(&sem_contador_cola_de_block);
		dis_tripulante *trip = (dis_tripulante *)list_get(cola_de_block, 0);
		loggear_estado_de_cola(cola_de_block, "Dispositivo E/S", "block antes de procesar tripulante");
		while (trip->estado == BLOCKED_E_S)
		{
			chequear_planificacion_pausada();
			sem_post(&(trip->sem_tri));
			sem_wait(&operacion_entrada_salida_finalizada);
			printf("[ Dispositivo E/S ] 1 ciclo de E/S completado\n");
		}

		switch (trip->estado)
		{
		case READY:
			trip = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_block, &mutex_cola_de_block);
			respuesta_ok_fail respuesta = actualizar_estado_miriam(trip->id,trip->estado);
			agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, trip);
			sem_post(&sem_contador_cola_de_ready);
			break;
		case EXPULSADO:
			log_info(logger, "[ Dispositivo E/S ] El tripulante %i ha sido EXPULSADO", trip->id);
			break;
		default:
			log_info(logger, "[ Dispositivo E/S ] No debería estar aca. Discordiador.c Linea 319");
			break;
		}
		loggear_estado_de_cola(cola_de_block, "Dispositivo E/S", "block despues de procesar tripulante");
	}
	return NULL;
}

//----------- FIN PLANIFICADORES ---------

void agregar_elemento_a_cola(t_list *cola, pthread_mutex_t *mutex, void *elemento)
{
	pthread_mutex_lock(mutex);
	list_add(cola, elemento);
	pthread_mutex_unlock(mutex);
}

void *quitar_primer_elemento_de_cola(t_list *cola, pthread_mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
	void *elemento = list_remove(cola, 0);
	pthread_mutex_unlock(mutex);
	return elemento;
}

void chequear_planificacion_pausada()
{
	if (planificacion_pausada)
	{
		sem_wait(&sem_para_detener_hilos);
	}
}
//---------- Funciones para ver el estado de las colas ---------

void loggear_estado_de_cola(t_list *cola, char *thread, char *tipo_de_cola)
{
	log_info(logger, "[ %s ] Estado de la cola %s: %i elementos.", thread, tipo_de_cola, list_size(cola));
}
//---------- FIN Funciones para ver el estado de las colas ---------

void catch_sigint_signal(int signal)
{
	terminar_programa();
}

void terminar_programa()
{
	log_info(logger, "EXIT. Liberando memoria y saliendo del programa...");
	log_destroy(logger);
	config_destroy(config);
	exit(0);
}


//----------FUNCION HECHA POR DAMI PASAR A DONDE CORRESPONDA ----------//
respuesta_ok_fail actualizar_estado_miriam(int tid,estado est){
	int conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
    void *info = serializar_estado_tcb(est,tid); 
    uint32_t size_paquete = sizeof(uint32_t)+sizeof(estado);
    enviar_paquete(conexion_mi_ram_hq, ACTUALIZAR_ESTADO, size_paquete, info); 
    t_paquete *paquete_recibido = recibir_paquete(conexion_mi_ram_hq);
    respuesta_ok_fail respuesta = deserializar_respuesta_ok_fail(paquete_recibido->stream);
    printf("Recibi respuesta %i\n",respuesta);
    close(conexion_mi_ram_hq);
    //chequear respuesta
    
	return respuesta;
}