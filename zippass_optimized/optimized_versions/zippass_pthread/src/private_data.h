// Copyright 2023 Archibald Emmanuel Carrion Claeys
#ifndef PRIVATE_DATA_H
#define PRIVATE_DATA_H

#include <pthread.h>
#include "shared_data.h"

/**
  * @brief private_data_t is a struct used to
  store the private data that must be accessed by only one thread.
*/
typedef struct {
  char** passwords;
  /** @brief passwords is a variable used to store the passwords
    found by the thread for each path given. */
  char** paths;
  /** @brief paths is a char** used to store all the paths to
    the zip files that the thread must solve. */
  uint64_t paths_quantity;
  /** @brief paths_quantity is an integer value used to store
    the quantity of zip files that the thread must solve.
    At the end of the execution of find_passwords, paths_quantity
    also is the quantity of passwords in passwords. */
  shared_data_t* shared_data;
  /** @brief shared_data is a pointer to a shared_data_t, it's used
  to shared between the threads the shared_data. */
  uint64_t thread_quantity;
  /** @brief thread_quantity is a integer value that contains the quantity of
    threads that the thread owning private_data can use to perform its tasks.*/
  uint64_t thread_id;
  /** @brief thread_id is a integer value that represent the numerical id
    of the thread, is used to create the copy of the zip files. */
  pthread_mutex_t mutex;
  /** @brief mutex is a pthread mutex used to modify sahred data without
    getting data race. */
} private_data_t;

void free_private_data(private_data_t* private_data);
#endif
