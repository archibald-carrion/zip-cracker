// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <stdlib.h>
#include "shared_data.h"

/**
 * @brief free_shared_data is used to free the memory allocated in the heap
  memory segment for the given shared_data_t.
 * @param shared_data shared_data is the struct that the function must free
 */
void free_shared_data(shared_data_t* shared_data) {
  free(shared_data->actual_password);
  free(shared_data->alphabet);
}
