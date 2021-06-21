#include "discordiador.h"


int main(void)
{
	signal(SIGINT, catch_sigint_signal);

	//inicializacion del modulo
	inicializar_discordiador();

	// ORDENAR LA destruccion de los logs y el config
	// terminar_programa(conexion_mi_ram_hq, conexion_i_mongo_store, logger, config);
}

void inicializar_discordiador()
{
	logger = iniciar_logger();
	config = leer_config();

	// [TODO] Planificacion
	sem_init(&sem_planificador_a_corto_plazo, 0, 0);
	sem_init(&sem_planificador_a_largo_plazo, 0, 0);

	// [TODO] inicializar colas/listas y semaforos
	cola_de_new = list_create();
	cola_de_ready = list_create();
	cola_de_exec = list_create();
	lista_de_patotas = list_create();
	list_total_tripulantes = list_create();
	cola_de_iniciar_patotas = queue_create();

	pthread_mutex_init(&mutex_cola_iniciar_patota, NULL);
	pthread_mutex_init(&mutex_cola_de_ready, NULL);
	pthread_mutex_init(&mutex_cola_de_new, NULL);
	pthread_mutex_init(&mutex_cola_de_exec, NULL);
	pthread_mutex_init(&mutex_tarea, NULL);

	sem_init(&sem_contador_cola_iniciar_patota, 0, 0);
	sem_init(&sem_contador_cola_de_new, 0, 0);

	// [TODO] Inicializacion de VAR GLOBALES
	id_patota = 1;
	id_tcb=1;
	grado_de_multiprocesamiento = 1;
	procesadores_disponibles = grado_de_multiprocesamiento;
	planificacion_pausada = false;

	//CONFIG
	ip_mi_ram_hq = config_get_string_value(config, "IP_MI_RAM_HQ");
	puerto_mi_ram_hq = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	ip_i_mongo_store = config_get_string_value(config, "IP_I_MONGO_STORE");
	puerto_i_mongo_store = config_get_string_value(config, "PUERTO_I_MONGO_STORE");


	conexion_mi_ram_hq = crear_conexion(ip_mi_ram_hq, puerto_mi_ram_hq);
	//pthread_t hilo_server;
	//pthread_create(&hilo_server, NULL, esperar_conexiones, &puerto);

	// HILO CONSOLA ------------------------------------------------------
	pthread_t hilo_consola;
	pthread_create(&hilo_consola, NULL, (void *)leer_consola, (void *)logger);
	
	pthread_join(hilo_consola, NULL);
}

t_log *iniciar_logger(void)
{
	return log_create("cfg/discordiador.log", "discordiador.log", 1, LOG_LEVEL_INFO);
}

t_config *leer_config(void)
{
	return config_create("cfg/discordiador.config");
}

// SERVIDOR QUE ESPERA UN SABOTAJE --------------
void *esperar_conexiones(void *arg)
{
	int puerto = *((int *)arg);
	t_list *hilos = list_create();
	int socket_escucha = iniciar_servidor_discordiador(puerto);
	int socket_cliente;
	log_info(logger, "SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_cliente = accept(socket_escucha, &cliente, &len);
		if (socket_cliente > 0)
		{
			log_info(logger, "NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_cliente);
		}
		else
		{
			log_error(logger, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
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
	log_info(logger, "Se creó el hilo correctamente. Socket fd %i", *socket_hilo);
	//LLENAME
	//CON AGUA
	close(*socket_hilo);
	free(socket_hilo);
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

		//struct_prueba* una_prueba = malloc(sizeof(struct_prueba));
		//una_prueba->tamanio = strlen(leido)+1;
		//una_prueba->contenido = leido;
		//enviar_paquete(conexion_mi_ram_hq,PRUEBA,una_prueba->tamanio,una_prueba->contenido);
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
		if(planificacion_pausada)
    		sem_wait(&sem_planificador_a_corto_plazo);

		sem_wait(&sem_planificador_a_corto_plazo);
		log_info(logger, "Planificador a corto plazo:  Cola ready: %i", list_size(cola_de_ready));
		log_info(logger, "Ejecutando cosas...");

		// 1- Paso elementos a ready mientras se pueda
		if (list_size(cola_de_ready) && procesadores_disponibles)
		{
			//Si hay elementos en ready y hay procesadores disponibles, pasar de ready a exec
			dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_ready, &mutex_cola_de_ready);
			tripulante->estado=EXEC;
			agregar_elemento_a_cola(cola_de_exec, &mutex_cola_de_exec, tripulante);
			log_info(logger, "Agregado exitosamente! Ahora la cantidad en ready es %i y en exec, %i", list_size(cola_de_ready), list_size(cola_de_exec));
		}

		// 2- Ejecuto las colas según el algoritmo configurado
		// llename
		// con agua
		sleep(4);
		sem_post(&sem_planificador_a_corto_plazo);

		//[se va a repetir muchas veces :C]
		//log_info(logger, "Planificador a corto plazo: Cola ready: %i", list_size(cola_de_ready));

		sleep(1); //MAGIA NEGRA PARA QUE ANDE
				  //este sleep es para que el semaforo no sea inmediatamente tomado por este hilo y le de tiempo al
				  //hilo que pone en pausa la planificacion. Puede ser un sleep de menor tamaño claramente, es
				  //solo para bloquear al hilo por un momento
	}
	return NULL;
}

void *planificador_a_largo_plazo()
{
	while (1)
	{
		if(planificacion_pausada)
    		sem_wait(&sem_planificador_a_largo_plazo);
		
		//si hay tripulantes enviados a NEW y la planificación está, actuará.
		sem_wait(&sem_planificador_a_largo_plazo);
		sem_wait(&sem_contador_cola_de_new);
		log_info(logger, "Planificador a largo plazo: Cola new: %i", list_size(cola_de_new));
		log_info(logger, "Pasando elementos de NEW --------> READY");
		dis_tripulante *tripulante = (dis_tripulante *)quitar_primer_elemento_de_cola(cola_de_new, &mutex_cola_de_new);
		tripulante->estado=READY;
		agregar_elemento_a_cola(cola_de_ready, &mutex_cola_de_ready, tripulante);
		log_info(logger, "Planificador a largo plazo: quedaron %i elementos", list_size(cola_de_new));
		sleep(2);
		//en este punto, habrá elementos en la cola NEW y la planificacion estará activada.
		sem_post(&sem_planificador_a_largo_plazo);

		sleep(1); // CREO YA NO IRIRA
		//este sleep es para que el semaforo no sea inmediatamente tomado por este hilo y le de tiempo al
		//hilo que pone en pausa la planificacion. Puede ser un sleep de menor tamaño claramente, es
		//solo para bloquear al hilo por un momento
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
//---------- Funciones para ver el estado de las colas ---------
//LLENAME
//CON AGUA
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