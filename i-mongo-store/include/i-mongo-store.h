#ifndef I_MONGO_STORE_H
#define I_MONGO_STORE_H

#include "i-mongo-store-lib.h"
// Funciones Auxiliares
bool reparar_sabotaje_block_count_en_archivo(char *);
bool reparar_sabotaje_md5_en_archivo(char *);
bool reparar_sabotaje_size_en_archivo(char *);
void cargar_bitmap_temporal(char *, int *);

// Funciones de sabotaje
bool sabotaje_block_count();
bool sabotaje_superbloque();
bool sabotaje_md5();
bool sabotaje_size();
bool sabotaje_bitmap();

#endif /* I_MONGO_STORE_H */