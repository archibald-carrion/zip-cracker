// Copyright 2023 Archibald Emmanuel Carrion Claeys
#ifndef INTERNAL_DATA_H
#define INTERNAL_DATA_H

#include <pthread.h>
#include "shared_data.h"
#include "private_data.h"

/**
 * @brief internal_data_t is a struct used to
  store the private data for internal threads, the ones that work
  with the same zip files.
 */
typedef struct {
  shared_data_t* shared_data;
  /** @brief shared_data is a pointer to a shared_data_t, it's used
  to shared between the threads the shared_data. */
  private_data_t* private_data;
  /** @brief private_data is a pointer to a  private_data_t, it's used
  to shared between the sub-threads data like the mutex. */
  char* password;
  /** @brief password is a variable used to store the password
    found by the thread, by default the variable contians a 0 if not found. */
  char* path;
  /** @brief paths is a char* used to store the path to
    the zip file that the thread must solve. */
  uint64_t * is_password_found;
  /** @brief is_password_found is a integer used as a booleand to store
    0 if the password is not found, and 1 if the password is found. */
  uint64_t size_combination;
  /** @brief size_combination is a integer used to store the size of the
    the combination that the program must create. */
  } internal_data_t;
#endif
