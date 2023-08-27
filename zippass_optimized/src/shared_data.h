// Copyright 2023 Archibald Emmanuel Carrion Claeys
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>
#include <pthread.h>
#include "queue.h"

/// @brief shared_data_t is a struct used to
///  share data between multiple functions.
typedef struct {
  char* path_folder;
  // path_folder is the path of the zip file folder were there is all the
  // temporary files
  char* path_zip_file;
  // path_zip_file is the path of the zip file to crack, the program never
  // tries to modify it, instead uses a copied version of it stored in
  // path_folder
  uint64_t is_password_found;
  // is_password_found is a flag that indicates if the password was found
  queue_t* queue;
  // queue is a queue that stores the passwords to try, it's the consuming
  // queue of the consumers and the producing queue of the producer
  pthread_mutex_t mutex;
  // mutex is a mutex used to protect the shared data
  char** password;
  // password is a pointer to a string that stores the password found, if the
  // password is not found it's NULL
  uint64_t password_length;
  // password_length is the maximum length of the password to be found
} shared_data_t;
#endif
