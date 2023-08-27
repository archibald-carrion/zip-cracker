// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zip.h>
#include <time.h>

#include "shared_data.h"
#include "queue.h"

void produce(shared_data_t* shared_data, char* alphabet);
void wait_consumers(pthread_t* threads, uint64_t thread_quantity);
void* consume(void* parameter);
void create_consumers(shared_data_t* shared_data, uint64_t thread_quantity,
  pthread_t* threads);

int32_t check_password(const char* password, struct zip* zip_file,
  pthread_mutex_t* mutex);
char* find_password(shared_data_t* shared_data, char* actual_password,
  char* zip_file_path);
char** copy_subarray(char** base_array, uint64_t starting_index,
  uint64_t ending_index);
int create_temporal_dir(char* path);
int remove_temporal_dir(char* path);
int create_temporal_file(char* path_original_file, char* tmp_file_path);
int remove_temporal_file(char* tmp_file_path);

/// @brief main function is the entry point of the program.
/// @return the program return a integer value, a 0 on success.
int main(int argc, char *argv[]) {
  char** zip_files_paths = NULL;
  uint64_t paths_quantity = 1;
  char* alphabet = NULL;
  uint64_t password_lenght = 0;
  uint64_t thread_quantity = 0;
  thread_quantity = sysconf(_SC_NPROCESSORS_ONLN);

  // condition used to check if a command line parameter is given or not
  if (argc >= 2) {
    thread_quantity = atoi(argv[1]);
    if (thread_quantity == 0) {
      fprintf(stderr, " Error: invalid argument '%s'\n", argv[1]);
      return 1;
    }
  }

  // scanf used to scan the first 2 datas given by the user
  scanf("%ms %"SCNu64"", &alphabet
    , &password_lenght);


  const uint64_t SIZE_LIMIT = 256;
  // path is used as a temporary buffer for storing the given paths
  char path[SIZE_LIMIT];

  // while loop used to scan all the zip files'path given by the user
  while (fgets(path, sizeof(path), stdin) != NULL) {
    if (path[0] == '\n' || path[1] == '\n') {
      // separation line in the input data
      continue;
    }
    zip_files_paths = realloc(zip_files_paths, sizeof(char*)*paths_quantity);
    zip_files_paths[paths_quantity-1] = malloc(SIZE_LIMIT);
    sscanf(path, "%255s", zip_files_paths[paths_quantity-1]);

    ++paths_quantity;
  }
  --paths_quantity;

  char * path_temporary_folder = "tests/paths/";
  create_temporal_dir(path_temporary_folder);

  // array used to store the passwords of each zip file

  // array of threads
  const uint64_t THREAD_QUANTITY = thread_quantity;
  pthread_t threads[THREAD_QUANTITY];
  // queue of produced passwords
  queue_t* queue = queue_create();
  // array of the found password
  char** passwords;
  passwords = malloc(sizeof(char*)*paths_quantity);
  for (uint64_t i = 0; i < paths_quantity; ++i) {
    passwords[i] = NULL;
  }
  // instance of shared_data_t used to communicate between threads
  shared_data_t shared_data = {0};
  shared_data.path_folder = path_temporary_folder;
  pthread_mutex_init(&shared_data.mutex, NULL);
  shared_data.password = NULL;
  shared_data.path_zip_file = NULL;
  shared_data.is_password_found = 0;
  shared_data.queue = queue;
  shared_data.password_length = password_lenght;
  // set the pointer to the queue to the instance

  // for each path, call create_consumers function, then call produce function
  // copy the password from the shared_data to passwords[i]
  clock_t start, end;
  double cpu_time_used;
  start = clock();

  for (uint64_t i = 0; i < paths_quantity; ++i) {
    shared_data.path_zip_file = zip_files_paths[i];
    shared_data.password = &passwords[i];
    shared_data.is_password_found = 0;
    shared_data.queue->finished = 0;
    // create_consumers
    create_consumers(&shared_data, thread_quantity, threads);
    // produce
    produce(&shared_data, alphabet);
    // join the threads
    wait_consumers(threads, thread_quantity);
    // empty the queue
    queue_empty(queue);
  }

  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("Execution time: %.2f seconds\n", cpu_time_used);

  queue_destroy(queue);
  pthread_mutex_destroy(&shared_data.mutex);

  // print all the passwords
  for (uint64_t i = 0; i < paths_quantity; ++i) {
    if (passwords[i] != NULL) {
      printf("%s %s\n", zip_files_paths[i], passwords[i]);
    } else {
      printf("%s\n", zip_files_paths[i]);
    }
  }

  for (uint64_t i = 0; i < paths_quantity; ++i) {
    if (passwords[i] != NULL) {
      free(passwords[i]);
    }
  }
  free(passwords);

  // free the array of shared_data_t
  for (uint64_t i = 0; i < paths_quantity; ++i) {
    free(zip_files_paths[i]);
  }
  free(zip_files_paths);
  remove_temporal_dir(path_temporary_folder);
  free(alphabet);
  return 0;
}

/// @brief wait_consumers function is used to wait for all the consumers to
/// finish execution, by calling pthread_join on each thread
/// @param threads threads is an array of pthreads
/// @param thread_quantity thread_quantity is the number of threads
void wait_consumers(pthread_t* threads, uint64_t thread_quantity) {
  for (uint64_t i = 0; i < thread_quantity; ++i) {
    pthread_join(threads[i], NULL);
  }
}

/// @brief create_consumers function is used to create all the consumers wich
/// will be used to consume the passwords from the queue, they all are pthreads
/// @param shared_data shared_data is a pointer to the shared_data_t instance
/// @param thread_quantity thread_quantity is the number of threads to
/// be created
void create_consumers(shared_data_t* shared_data, uint64_t thread_quantity,
  pthread_t* threads) {
  // create all the threads
  for (uint64_t i = 0; i < thread_quantity; ++i) {
    pthread_create(&threads[i], NULL, consume, shared_data);
  }
}

/// @brief produce function is used to produce all the passwords and put them
/// in the producing queue
/// @param shared_data shared_data is a pointer to the shared_data_t instance
/// @param alphabet alphabet is a pointer to the alphabet string
void produce(shared_data_t* shared_data, char* alphabet) {
  pthread_mutex_lock(&shared_data->mutex);
  int64_t m = shared_data->password_length;
  pthread_mutex_unlock(&shared_data->mutex);

  int64_t n = strlen(alphabet);
  const uint32_t PASSWORD_SIZE = m + 1;
  char password[PASSWORD_SIZE];
  int64_t i, j, k;

  for (i = 1; i <= m; i++) {
    memset(password, 0, sizeof(password));  // clear the password buffer
    for (j = 0; j < i; j++) {
      password[j] = alphabet[0];
      // initialize the password with the first character of the alphabet
    }

    while (password[0] != '\0') {
      int64_t is_password_valid = 0;
      pthread_mutex_lock(&shared_data->mutex);
      is_password_valid = shared_data->is_password_found;
      char* new_password = malloc(PASSWORD_SIZE);
      snprintf(new_password, PASSWORD_SIZE, "%s", password);
      // enqueue the password to the producing queue
      enqueue(shared_data->queue, new_password);
      pthread_mutex_unlock(&shared_data->mutex);

      if (is_password_valid) {
        return;
      }
      k = i - 1;

      while (k >= 0 && password[k] == alphabet[n-1]) {
        k--;
      }

      if (k < 0) {
        break;
      }

      for (j = 0; j < n; j++) {
        if (alphabet[j] > password[k]) {
          password[k] = alphabet[j];
          break;
        }
      }

      for (j = k+1; j < i; j++) {
        password[j] = alphabet[0];
      }
    }
  }

  pthread_mutex_lock(&shared_data->mutex);
  // no more passwords to be produce
  // the finished bool is switch to true
  shared_data->queue->finished = 1;
  pthread_mutex_unlock(&shared_data->mutex);
}

/// @brief consume function is used to consume all the passwords from the
/// producing queue, and try to unzip the zip file with the password
/// @param parameter parameter is a pointer to the shared_data_t instance
/// @return void* void* is a pointer to the result of the thread, in this case
/// the thread returns NULL
void* consume(void* parameter) {
  shared_data_t* shared_data = (shared_data_t*) parameter;

  pthread_t thread_id = pthread_self();
  // Create the string
  char* path_general = "tests/paths/zip_file_";
  char string_id[20];
  // sprintf(string_id, "%lu", thread_id);
  snprintf(string_id, sizeof(string_id), "%lu", thread_id);
  // own_file is the path to the copied zip file
  char* own_file = malloc(strlen(path_general) + strlen(string_id) + 1);
  if (own_file == NULL) {
    fprintf(stderr, "Error allocating memory\n");
    exit(EXIT_FAILURE);
  }
  // snprintf(own_file, sizeof(own_file), " %s%s", path_general, string_id);
  strcpy(own_file, path_general);  // NOLINT
  strcat(own_file, string_id);     // NOLINT

  // create a copy of the zip file in the folder
  pthread_mutex_lock(&shared_data->mutex);
  create_temporal_file(shared_data->path_zip_file, own_file);
  // zip_file is the zip file to be unziped, we open it in this function this
  // way so the check_password function doesn't need to reopen it each time
  struct zip* zip_file = NULL;
  zip_file = zip_open(own_file, 0, NULL);
  pthread_mutex_unlock(&shared_data->mutex);
  if (zip_file == NULL) {
    printf("Error opening zip file: %s\n", own_file);
    remove_temporal_file(own_file);
    free(own_file);
    return NULL;
  }

  char* password = NULL;

  // consume while the password is not found or there is no more passwords
  while (1) {
    pthread_mutex_lock(&shared_data->mutex);
    // password was already found
    if (shared_data->is_password_found) {
      zip_close(zip_file);
      remove_temporal_file(own_file);
      free(own_file);
      pthread_mutex_unlock(&shared_data->mutex);
      return NULL;
    } else {
      if (!is_empty(shared_data->queue)) {
        password = dequeue(shared_data->queue);
      } else {  // consuming queue is empty
        if (shared_data->queue->finished == 1) {
          // no more passwords to consume
          zip_close(zip_file);
          remove_temporal_file(own_file);
          free(own_file);
          pthread_mutex_unlock(&shared_data->mutex);
          return NULL;
        } else {  // wait until the producing queue is not empty
          pthread_mutex_unlock(&shared_data->mutex);
          continue;
        }
      }
    }
    pthread_mutex_unlock(&shared_data->mutex);

    int is_valid = check_password(password, zip_file, &shared_data->mutex);
    // case were the password is valid, must be stored in the shared_data
    // instance and return
    if (is_valid) {
      pthread_mutex_lock(&shared_data->mutex);
      *(shared_data->password) = password;
      shared_data->is_password_found = 1;
      zip_close(zip_file);
      remove_temporal_file(own_file);
      pthread_mutex_unlock(&shared_data->mutex);
      free(own_file);
      return NULL;
    }
    free(password);
  }
}

/// @brief copy_subarray function is a function used to copy a part of
/// an array into another array of the right size, then the function return
/// this new array.
/// @param base_array base_array is the array on wich the new array is based
/// @param starting_index starting_index is the first index of the base array
/// that must be copied
/// @param ending_index ending index is the index of the last elemento of the
/// base array that must be copied
/// @return the program return a char** to the new subarray.
char** copy_subarray(char** base_array, uint64_t starting_index,
  uint64_t ending_index) {
  uint64_t size = ending_index - starting_index;
  char** sub_array = malloc(size * sizeof(char*));
  // sub_array is the array storing the sub_division of base
  for (uint64_t i = 0; i < size; ++i) {
    // loop used to copy every path in the sub_array
    sub_array[i] = base_array[i+starting_index];
  }
  // returning the sub_array pointer to the user
  return sub_array;
}

/// @brief check_password function is used to check if a given password
/// is valid for a given encrypted zip file.
/// @param password password is a char*, it's the password that must be tested.
/// @param zip_file zip_file is a struct zip*, it's the zip file that must be
/// unziped with the password.
/// @return check_password return an integer value, if the password is valid
/// the function return a 1, in the contrary case the function return a 0;
int32_t check_password(const char* password, struct zip* zip_file,
  pthread_mutex_t * mutex) {
  int32_t is_password_valid = 0;

  const char* file_name = NULL;
  pthread_mutex_lock(mutex);
  file_name = zip_get_name(zip_file, 0, 0);
  pthread_mutex_unlock(mutex);

  if (file_name != NULL) {
    zip_file_t* file = NULL;
    pthread_mutex_lock(mutex);
    file = zip_fopen_encrypted(zip_file,
      file_name, ZIP_FL_ENC_GUESS, password);
    pthread_mutex_unlock(mutex);

    if (file != NULL) {
      char buffer[256];
      memset(buffer, 0, sizeof(buffer));
      int n = zip_fread(file, buffer, sizeof(buffer));
      buffer[n] = '\0';
      if (n > 0) {
        char* id = NULL;
        char* saveptr;
        id = strtok_r(buffer, " \t\n", &saveptr);
        if ((strcmp(id, "CI0117-23a")) == 0) {
          is_password_valid = 1;
          // printf("hello there\n");
        }
      }
      zip_fclose(file);
    }
  }

  return is_password_valid;
}

/// @brief create_temporal_dir is a function used to created a temporal
/// folder that the program will delete after using it.
/// @param path path is a char*, it's the path to the directory that must be
/// created
/// @return the function return an error message
int create_temporal_dir(char* path) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
    mkdir(path, 0700);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/// @brief create_temporal_fileis a function used to created a temporal
/// copy of a file that the program will delete after using it.
/// @param path_original_file path_original_file is a char*, it's the path
/// to the file that must be copied
/// @param tmp_file_path tmp_file_path is a char*, it's the path
/// to the file that will be created
/// @return the function return an error message
int create_temporal_file(char* path_original_file, char* tmp_file_path) {
  int error = EXIT_SUCCESS;

  FILE* original;
  FILE* temporal;

  original = fopen(path_original_file, "rb");
  temporal = fopen(tmp_file_path, "wb");

  // Tomado de: https://stackoverflow.com/a/5263102/19025248
  size_t n, m;
  unsigned char buff[8192];
  do {
    n = fread(buff, 1, sizeof buff, original);
    if (n) m = fwrite(buff, 1, n, temporal);
    else   m = 0;
  } while ((n > 0) && (n == m));
  if (m) perror("copy");

  fclose(temporal);
  fclose(original);

  return error;
}

/// @brief remove_temporal_dir is a function used to delete a temporal
/// folder created by the program
/// @param path path is a char*, it's the path to the directory that must be
/// delete, the folder must be empty to be delete
/// @return the function return an error message
int remove_temporal_dir(char* path) {
  struct stat st = {0};
  if (stat(path, &st) != -1) {
    return rmdir(path);
  }
  return EXIT_FAILURE;
}

/// @brief remove_temporal_file is a function used to delete a temporal
/// file created by the program
/// @param path path is a char*, it's the path to the file that must be
/// delete
/// @return the function return an error message
int remove_temporal_file(char* tmp_file_path) {
  return remove(tmp_file_path);
}
