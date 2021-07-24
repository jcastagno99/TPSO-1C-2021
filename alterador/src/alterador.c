#include <commons/config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    printf("Ingrese la corrupcion de superbloque a realizar. 1: cant bloques. 2: bitmap \n");
    uint32_t opcion;
    scanf("%d",&opcion);
    if(opcion == 1){
      t_config* llave_valor = config_create(argv[1]);
    char* punto_montaje = config_get_string_value(llave_valor,"PUNTO_MONTAJE");
     char* ruta_superbloque = malloc(strlen(punto_montaje)+ strlen("/Superbloque.ims")+1);
     ruta_superbloque[0] = '\0';
     memcpy(ruta_superbloque,punto_montaje,strlen(punto_montaje)+1);
    strcat(ruta_superbloque, "/Superbloque.ims");
    printf("Ingrese cantidad de bloques nueva, corrompiendo superbloque.ims \n");
    uint32_t nuevo_block_amount;
    scanf("%d",&nuevo_block_amount);
    FILE* f = fopen(ruta_superbloque,"r+");
     fseek(f,4,SEEK_SET);
     fwrite(&nuevo_block_amount,sizeof(uint32_t),1,f);
     fclose(f);
     free(ruta_superbloque);
     config_destroy(llave_valor);
     return 0;
    }
    else{
    t_config* llave_valor = config_create(argv[1]);
    char* punto_montaje = config_get_string_value(llave_valor,"PUNTO_MONTAJE");
     char* ruta_superbloque = malloc(strlen(punto_montaje)+ strlen("/Superbloque.ims")+1);
     memcpy(ruta_superbloque,punto_montaje,strlen(punto_montaje));
    strcat(ruta_superbloque, "/Superbloque.ims");
    FILE* f = fopen(ruta_superbloque,"r+");
     fseek(f,8,SEEK_SET);
     
    }

}