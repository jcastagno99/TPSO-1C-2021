#include <commons/config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    if(argv[2]==0){
      t_config* llave_valor = config_create(argv[1]);
    char* punto_montaje = config_get_string_value(llave_valor,"PUNTO_MONTAJE");
     char* ruta_superbloque = malloc(strlen(punto_montaje)+ strlen("/Superbloque.ims")+1);
     memcpy(ruta_superbloque,punto_montaje,strlen(punto_montaje));
    strcat(ruta_superbloque, "/Superbloque.ims");
     uint32_t nuevo_block_amount = config_get_int_value(llave_valor,"CANT_BLOQUES");
    FILE* f = fopen(ruta_superbloque,"r+");
     fseek(f,4,SEEK_SET);
     fwrite(&nuevo_block_amount,sizeof(uint32_t),1,f);
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