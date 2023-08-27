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
  uint64_t size_alphabet;
  /** @brief size_alphabet is the size of the alphabet. */
  char* actual_password;
  /** @brief actual_password is a variable used to store the password
    found by the program, once a new password found for another
    zip file, the current password stored is lost. */
} shared_data_t;

void free_shared_data(shared_data_t* shared_data);

#endif
