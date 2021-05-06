#include "consola.h"

void liberar_lista_string(char**lista){
    int i=0;
    while (lista[i]!= NULL){
        free( lista[i]);
        i++;
    } 
    free(lista); 
}

bool posicion_validas_tripulantes(char** str_split){
    int i=3;
    while (str_split[i]!=NULL)
    {
        char**posiciones =string_split(str_split[i], "|");
        
        if (posiciones[0]==NULL || posiciones[1]==NULL){
            printf("Esta mal a partir del %d parametro, formato '|' no encontrado\n",i);            
            return false;
        }else if ( atoi(posiciones[0])==0 || atoi(posiciones[1])==0 ){
            printf("ejx:%s | ejy:%s\n",posiciones[0],posiciones[1]);
            printf("Las posiciones deben ser [int] en el parametro %d\n",i);
            return false;
        }
        liberar_lista_string(posiciones);
        i++;
    }
    return true;
}

void validacion_sintactica(char*text){
    char** str_split = string_split(text, " ");
    
    if (strcmp(str_split[0],"INICIAR_PATOTA") == 0)
    {
        printf("Posible intento de comando iniciar patota\n");
        if (str_split[1]!=NULL && str_split[2]!=NULL && posicion_validas_tripulantes(str_split)){
            if (atoi(str_split[1])!=0){                
                printf("INICIAR_PATOTA : OK\n");
                // [TODO]
            }else
                printf("INICIAR_PATOTA : id tripulante no es un [int]\n");          
        }else
            printf("Parametros incorrectos/faltantes\n");

    }else if (strcmp(str_split[0],"LISTAR_TRIPULANTES") == 0 )
    {
        printf("Posible intento de listar tripulante\n");
        if (str_split[1]==NULL){
            printf("LISTAR_TRIPULANTES : OK\n");
            // [TODO]
        }else
            printf("LISTAR_TRIPULANTES : no contiene parametros\n");
        
        
    }else if (strcmp(str_split[0],"EXPULSAR_TRIPULANTE") == 0 )
    {
        printf("Posible intento de expulsar tripulante\n");
        if (str_split[1]!=NULL &&  str_split[2]==NULL){
            if (atoi(str_split[1])!=0){
                printf("EXPULSAR_TRIPULANTE : OK\n");
                // [TODO]
            }else
                printf("EXPULSAR_TRIPULANTE : id tripulante no es un [int]\n");          
        }else
            printf("Parametros incorrectos/faltantes\n");


    }else if (strcmp(str_split[0],"INICIAR_PLANIFICACION") == 0 )
    {
        printf("Posible intento de iniciar planificacion\n");
        if (str_split[1]==NULL){
            printf("INICIAR_PLANIFICACION : OK\n");
            // [TODO]
        }else
            printf("INICIAR_PLANIFICACION : no contiene parametros\n");


    }else if (strcmp(str_split[0],"PAUSAR_PLANIFICACION") == 0 )
    {
        printf("Posible intento de pausar planificacion");
        if (str_split[1]==NULL){
            printf("PAUSAR_PLANIFICACION : OK\n");
            // [TODO]
        }else
            printf("PAUSAR_PLANIFICACION : no contiene parametros\n");


    }else if (strcmp(str_split[0],"OBTENER_BITACORA") == 0 )
    {
        printf("Posible intento de obtener bitacora\n");
        if (str_split[1]!=NULL &&  str_split[2]==NULL){
            if (atoi(str_split[1])!=0){
                printf("OBTENER_BITACORA : OK\n");
                // [TODO]
            }else
                printf("OBTENER_BITACORA : id tripulante no es un [int]\n");            
        }else
            printf("Parametros incorrectos/faltantes\n");


    }else
    {
        printf("COMANDO NO VALIDO\n");
        //[TODO]
    }
    liberar_lista_string(str_split);
}