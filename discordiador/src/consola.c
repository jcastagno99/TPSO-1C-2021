#include "consola.h"

Comando_Discordiador obtener_comando(char *comando)
{
    if (strcmp(comando, "INICIAR_PATOTA") == 0)
        return INIT_PATOTA;
    if (strcmp(comando, "LISTAR_TRIPULANTES") == 0)
        return LIST_CREWS;
    if (strcmp(comando, "EXPULSAR_TRIPULANTE") == 0)
        return EJECT_CREW;
    if (strcmp(comando, "INICIAR_PLANIFICACION") == 0)
        return INIT_PLANIFICATION;
    if (strcmp(comando, "PAUSAR_PLANIFICACION") == 0)
        return PAUSE_PLANIFICATION;
    if (strcmp(comando, "OBTENER_BITACORA") == 0)
        return GET_BINNACLE;
    if (strcmp(comando, "q") == 0)
        return QUIT;
    return NO_VALIDO;
}

bool posicion_validas_tripulantes(char **str_split)
{
    int i = 3;
    while (str_split[i] != NULL)
    {
        char **posiciones = string_split(str_split[i], "|");

        if (posiciones[0] == NULL || posiciones[1] == NULL)
        {
            printf("Esta mal a partir de la pos %d , formato '|' no encontrado\n", i - 2);
            return false;
        }

        // Pos X Tripulante : Si es un string distinto de "0"
        if (atoi(posiciones[0]) == 0 && strcmp(posiciones[0], "0") != 0)
        {
            printf("ejx:%s | La posicion debe ser un numero\n", posiciones[0]);
            return false;
        }

        // Pos Y Tripulante : Si es un string distinto de "0"
        if (atoi(posiciones[1]) == 0 && strcmp(posiciones[1], "0") != 0)
        {
            printf("ejy:%s | La posicion debe ser un numero\n", posiciones[1]);
            return false;
        }

        liberar_lista_string(posiciones);
        i++;
    }
    return true;
}

void validacion_sintactica(char *text)
{
    char **str_split = string_split(text, " ");
    Comando_Discordiador comando = obtener_comando(str_split[0]);

    switch (comando)
    {
    //[SANTI] el nombre del enum lo puse en ingles porque en español ya estaba utilizado para los
    //op_codes
    case INIT_PATOTA:
        if (str_split[1] != NULL && str_split[2] != NULL && posicion_validas_tripulantes(str_split))
        {
            int cant_tripulantes = atoi(str_split[1]);

            if (cant_tripulantes != 0)
            {
                log_info(logger, "INICIANDO PATOTA");
                pthread_mutex_lock(&mutex_cola_iniciar_patota);
                queue_push(cola_de_iniciar_patotas, str_split);
                pthread_mutex_unlock(&mutex_cola_iniciar_patota);
                sem_post(&sem_contador_cola_iniciar_patota);
            }
            else
                log_warning(logger, "La cantidad de tripulantes debe ser un numero");
        }
        else
            log_warning(logger, "Parametros incorrectos/faltantes");
        break;

    case LIST_CREWS:
        if (str_split[1] == NULL)
        {
            printf("LISTAR_TRIPULANTES : OK\n");
            listar_tripulantes();

        }
        else
            log_warning(logger, "LISTAR_TRIPULANTES : no contiene parametros\n");
        liberar_lista_string(str_split);    
        break;

    case EJECT_CREW:

        if (str_split[1] != NULL && str_split[2] == NULL)
        {
            if (atoi(str_split[1]) != 0)
            {
                printf("EXPULSAR_TRIPULANTE : OK\n");
                // No confundir con (_local)
                expulsar_tripulante(atoi(str_split[1]));
            }
            else
                printf("EXPULSAR_TRIPULANTE : id tripulante no es un [int]\n");
        }
        else
            printf("Parametros incorrectos/faltantes\n");
        liberar_lista_string(str_split);
        break;

    case INIT_PLANIFICATION:
        printf("Posible intento de iniciar planificacion\n");
        if (str_split[1] == NULL)
        {
            iniciar_planificacion();
            log_info(logger, "INICIAR_PLANIFICACION: OK");
        }
        else
            printf("INICIAR_PLANIFICACION : no contiene parametros\n");
        liberar_lista_string(str_split);
        break;

    case PAUSE_PLANIFICATION:
        if (str_split[1] == NULL)
        {
            pausar_planificacion();
            log_info(logger, "PAUSAR_PLANIFICACION: OK");
        }
        else
            printf("PAUSAR_PLANIFICACION : no contiene parametros\n");
        liberar_lista_string(str_split);
        break;

    case GET_BINNACLE:
        printf("Posible intento de obtener bitacora\n");
        if (str_split[1] != NULL && str_split[2] == NULL)
        {
            if (atoi(str_split[1]) != 0)
            {
                printf("OBTENER_BITACORA : OK\n");
                obtener_bitacora(atoi(str_split[1]));
            }
            else
                printf("OBTENER_BITACORA : id tripulante no es un [int]\n");
        }
        else
            printf("Parametros incorrectos/faltantes\n");
        liberar_lista_string(str_split);
        break;
    case QUIT:
        terminar_programa();
    default:
        log_warning(logger, "COMANDO NO VALIDO");
        liberar_lista_string(str_split);
        break;
    }
    //liberar_lista_string(str_split);// Creo que cada uno lo debe gestionar si no ROMPE por que se pierde
}



// FUNCION CON CONEXION
void expulsar_tripulante (int tid){

    dis_tripulante *tripulante = list_get(list_total_tripulantes, tid - 1);
    
    pthread_mutex_lock(&(tripulante->mutex_expulsado));
    tripulante->expulsado = true;
    pthread_mutex_unlock(&(tripulante->mutex_expulsado));

}