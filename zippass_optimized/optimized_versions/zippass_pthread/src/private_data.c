// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <stdlib.h>
#include "private_data.h"

/**
  * @brief free_private_data is used to free the memory allocated in the heap
  * memory segment for the given private_data_t.
  * @param private_data private_data is the struct that the function must free
*/
void free_private_data(private_data_t* private_data) {
  for (uint64_t i = 0; i < private_data->paths_quantity; ++i) {
    free(private_data->passwords[i]);
    free(private_data->paths[i]);
  }
  free(private_data->paths);
  free(private_data->passwords);
  // free the passwords and paths
}
