// Copyright 2023 Archibald Emmanuel Carrion Claeys
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>

/**
 * @brief shared_data_t is a struct used to
  share data between multiple functions.
 */
typedef struct {
  char* alphabet;
  /** @brief alphabet is the set of chars that can be used in the password. */
  uint64_t password_lenght;
  /** @brief password_lenght is the maximum length of the password. */
  uint64_t alphabet_lenght;
  /** @brief size_alphabet is the size of the alphabet. */
} shared_data_t;

void free_shared_data(shared_data_t* shared_data);

#endif
