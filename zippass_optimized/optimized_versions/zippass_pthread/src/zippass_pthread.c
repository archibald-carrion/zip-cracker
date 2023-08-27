// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <stdio.h>
#include <unistd.h>
#include <zip.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#include "shared_data.h"
#include "private_data.h"
#include "internal_data.h"


char** copy_subarray(char** base_array, uint64_t starting_index,
  uint64_t ending_index);
void* find_passwords(void* parameter);
char* find_password_threads(private_data_t* private_data,
  shared_data_t* shared_data, char* actual_password, char* zip_file_path,
  uint64_t thread_quantity, uint64_t thread_id);
char* find_password(shared_data_t* shared_data, char* actual_password,
  char* zip_file_path);
int32_t check_password(const char* password, struct zip* zip_file);
int create_temporal_dir(char* path);
int remove_temporal_dir(char* path);
int create_temporal_file(char* path_original_file, char* tmp_file_path);
int remove_temporal_file(char* tmp_file_path);
void* create_combinations(void* parameter);
void* create_combinations_multiple_sizes(void* parameter);
void generate_combinations_rec(char *current_combination,
  uint64_t currentIndex, char* zip_file_path, internal_data_t* internal_data);

/**
 * @brief main function is the entry point of the program.
 * @return the program return a integer value, a 0 on success.
 */
int main(int argc, char *argv[]) {
  shared_data_t shared_data = {0};
  char** zip_files_paths = NULL;
  uint64_t paths_quantity = 1;
  // path_quantity is the quantity of zip files that must be solved
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

  // printf("Quantity of threads: %"PRIu64 "\n", thread_quantity);

  // scanf used to scan the first 2 datas given by the user
  scanf("%ms %"SCNu64"", &shared_data.alphabet
    , &shared_data.password_lenght);

  shared_data.alphabet_lenght = strlen(shared_data.alphabet);
  // shared_data.actual_password = malloc(shared_data.password_lenght + 1);
  // memset(shared_data.actual_password, 0, shared_data.password_lenght + 1);
  // SIZE_LIMIT is used for security reasons
  const uint64_t SIZE_LIMIT = 256;
  // path is used as a temporary buffer for storing the given paths
  char path[SIZE_LIMIT];
  // fgets(path, sizeof(path), stdin);

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

  // private_data
  const uint64_t THREAD_QUANTITY = thread_quantity;
  private_data_t private_datas[THREAD_QUANTITY];
  // find the password of each zip file, a mapping algorithm by blocks
  // is used to optimaly shared the task between threads.
  uint64_t block_size = paths_quantity / thread_quantity;
  uint64_t rest_excess = paths_quantity - block_size*thread_quantity;

  // addional quantity of path, this variable will be decreased
  // for each execution of the loop, the rest_excess (n) first threads
  // will execute 1 addional task

  uint64_t thread_counter = 0;
  // used to check how many thread are already created
  uint64_t zip_counter = 0;
  // used to check the quantity of path already assigned

  // loop used to map the zip files between the given threads
  while (thread_counter != thread_quantity && zip_counter != paths_quantity) {
    if (rest_excess == 0) {
      private_datas[thread_counter].passwords =
        malloc(block_size * sizeof(char*));
      private_datas[thread_counter].paths = copy_subarray(zip_files_paths,
        zip_counter, zip_counter+block_size);
      private_datas[thread_counter].paths_quantity = block_size;
      private_datas[thread_counter].shared_data = &shared_data;
      private_datas[thread_counter].thread_quantity = 1;
      zip_counter = zip_counter + block_size;
    } else {
      if (block_size == 0) {  // case where there is more threads than zipfiles
        private_datas[thread_counter].passwords =
          malloc(sizeof(char*));
        private_datas[thread_counter].paths = copy_subarray(zip_files_paths,
          zip_counter, zip_counter+1);
        private_datas[thread_counter].paths_quantity = 1;
        private_datas[thread_counter].shared_data = &shared_data;
        private_datas[thread_counter].thread_quantity = 1;
        ++zip_counter;

      } else {
        private_datas[thread_counter].passwords =
          malloc((block_size+1) * sizeof(char*));
        private_datas[thread_counter].paths = copy_subarray(zip_files_paths,
          zip_counter, zip_counter+block_size+1);
        private_datas[thread_counter].paths_quantity = block_size+1;
        private_datas[thread_counter].shared_data = &shared_data;
        private_datas[thread_counter].thread_quantity = 1;
        zip_counter = zip_counter + block_size + 1;
      }
      --rest_excess;
    }
    ++thread_counter;
  }
  const uint64_t THREAD_COUNTER_0 = thread_counter;
  uint32_t internal_threads[THREAD_COUNTER_0];
  memset(internal_threads, 0, sizeof(internal_threads));

  if (thread_quantity != thread_counter) {
    uint32_t remaining_threads = thread_quantity - thread_counter;
    if (remaining_threads/thread_counter != 0) {
      uint32_t excess_buffer = remaining_threads%thread_counter;
      for (size_t i = 0; i < thread_counter; ++i) {
        if (excess_buffer == 0) {
          internal_threads[i] = remaining_threads/thread_counter;
        } else {
          internal_threads[i] = remaining_threads/thread_counter + 1;
          --excess_buffer;
        }
      }
    } else {
      for (size_t i =0; i <remaining_threads; ++i) {
        internal_threads[i] = 1;
      }
    }
  }

  char * path_folder = "tests/paths/";
  create_temporal_dir(path_folder);

  clock_t start, end;
  double cpu_time_used;
  start = clock();

  // loop used to create the threads
  const uint64_t THREAD_COUNTER_1 = thread_counter;
  pthread_t threads[THREAD_COUNTER_1];
  for (uint64_t i = 0; i < thread_counter; ++i) {
    private_datas[i].thread_quantity =
      private_datas[i].thread_quantity + internal_threads[i];
    private_datas[i].thread_id = i;
    pthread_create(&threads[i], NULL, find_passwords,
      &private_datas[i]);
  }
  // loop used to wait for the threads to finish executing
  for (uint64_t i = 0; i < thread_counter; ++i) {
    pthread_join(threads[i], NULL);
  }

  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("Execution time: %.2f seconds\n", cpu_time_used);

  remove_temporal_dir(path_folder);

  // print all the password stored in each private_data
  for (uint64_t i = 0; i < thread_counter; ++i) {
    for (uint64_t j = 0; j < private_datas[i].paths_quantity; ++j) {
      printf("%s %s\n", private_datas[i].paths[j],
        private_datas[i].passwords[j]);
    }
  }

  for (uint64_t i = 0; i < thread_counter; ++i) {
    free_private_data(&private_datas[i]);
  }

  free(zip_files_paths);
  free_shared_data(&shared_data);

  // loop to free all the private_data
  return 0;
}

/**
 * @brief copy_subarray function is a function used to copy a part of 
 * an array into another array of the right size, then the function return
 * this new array.
 * @param base_array base_array is the array on wich the new array is based
 * @param starting_index starting_index is the first index of the base array
 * that must be copied
 * @param ending_index ending index is the index of the last elemento of the
 * base array that must be copied
 * @return the program return a char** to the new subarray.
 */
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


/**
 * @brief find_passwords function is used to choose if the find_password
 * function or the find_password_thread function must be used.
 * This way the program can easily deal between the different cases of
 * execution that could occur depending on the quantity of threads and zip
 * files given.
 * The function is used as the first function for each thread.
 * @param parameter parameter is a void*, the only parameter received by this
 * function, it contains all the parameters needed to comeplete the given task. 
 */
void* find_passwords(void* parameter) {
  private_data_t* private_data = (private_data_t*) parameter;
  // case where the thread must solve only one zip file
  if (private_data->paths_quantity == 1) {
    char* password_n =
      malloc(private_data->shared_data->password_lenght * sizeof(char));
    uint64_t size_password =
      private_data->shared_data->password_lenght * sizeof(char);
    memset(password_n, 0, size_password);
    find_password_threads(private_data, private_data->shared_data, password_n,
      private_data->paths[0], private_data->thread_quantity,
      private_data->thread_id);
    private_data->passwords[0] = password_n;

  } else {  //  case where the thread must solve multiple zip files
    for (uint64_t i = 0; i < private_data->paths_quantity; ++i) {
      char* password_n =
        malloc(private_data->shared_data->password_lenght * sizeof(char));
      uint64_t size_password =
        private_data->shared_data->password_lenght * sizeof(char);
      memset(password_n, 0, size_password);
      find_password(private_data->shared_data,
        password_n, private_data->paths[i]);
      private_data->passwords[i] = password_n;
    }
  }

  return NULL;
}

/**
 * @brief find_password_threads function is used to create every single
 * combination of password posible, it solve the problem using a brute force 
 * approach, using parallel programmgin to reduce the execution time.
 * The function generates all possible passwords using the given alphabet and
 * for every password, the program try the password using the check_password 
 * function, to know if the password is valid or not.
 * @param shared_data shared_data is a pointer to an instance of the
 * shared_data_t struct.
 * @param actual_password actual_password is a parameter used to store the
 *  password found (or not) by the subroutine
 * @param zip_file_path zip_file_path is the path of the given zip file
 * that must be unlocked.
 * @param thread_quantity thread_quantity is an integer parameter that contains
 * the quantity of threads that the program can use.
 * @param thread_id thread_id is a unique integer value that allow the
 * program to recognize this thread from others, it's used to create the
 * copies of the zip file
 * @return If a valid password is found, it is stored in 
 * `shared_data->actual_password`  * and returned. 
 * Else the function return an empty string.
 */
char* find_password_threads(private_data_t* private_data,
  shared_data_t* shared_data, char* actual_password, char* zip_file_path,
  uint64_t thread_quantity, uint64_t thread_id) {
  uint32_t usefull_threads = thread_quantity;
  if (thread_quantity > shared_data->password_lenght) {
    usefull_threads = shared_data->password_lenght;
  }

  //  create usefull_threads of copy of zip file
  char **buffer_array_paths = malloc(usefull_threads*sizeof(char*));
  // loop used to create the named for the files
  for (uint64_t i = 0; i < usefull_threads; ++i) {
    uint32_t size_i = 0;
    uint32_t size_thread_id = 0;

    uint64_t i_buffer = i;
    if (i_buffer == 0) {
      size_i = 1;
    } else {
      while (i_buffer > 0) {
        i_buffer/=10;
        ++size_i;
      }
    }

    uint64_t id_buffer = i;
    if (i_buffer == 0) {
      size_thread_id = 1;
    } else {
      while (id_buffer > 0) {
        id_buffer/=10;
        ++size_thread_id;
      }
    }

    uint32_t size_new_file = 2 + size_thread_id + size_i;
    buffer_array_paths[i] = malloc(size_new_file);
    snprintf(buffer_array_paths[i], size_new_file,
      "%"PRIu64"_%"PRIu64, thread_id, i);
  }

  char **array_paths = malloc(usefull_threads*sizeof(char*));
  // loop used to create the paths to the files
  for (uint64_t i = 0; i < usefull_threads; ++i) {
    int64_t size_path = sizeof("tests/paths/") + sizeof(buffer_array_paths[i]);
    array_paths[i] = malloc(size_path);
    snprintf(array_paths[i], size_path, "tests/paths/%s",
      buffer_array_paths[i]);
  }

  for (uint64_t i = 0; i < usefull_threads; ++i) {
    create_temporal_file(zip_file_path, array_paths[i]);
  }

  char** passwords = malloc(sizeof(char*) * usefull_threads);

  // allocate the memory for the passwords and set them as 0
  uint64_t is_found = 0;
  for (uint32_t i = 0; i < usefull_threads; ++i) {
    passwords[i] = malloc(shared_data->password_lenght);
    memset(passwords[i], 0, shared_data->password_lenght);
    // create the threads and pass them the struct
  }

  pthread_mutex_init(&(private_data->mutex), NULL);
  // loop used to create the threads
  const uint32_t USEFULL_THREADS = usefull_threads;
  pthread_t i_threads[USEFULL_THREADS];
  internal_data_t internal_datas[USEFULL_THREADS];
  if (thread_quantity >= shared_data->password_lenght) {
    for (uint64_t i = 0; i < usefull_threads; ++i) {
      internal_datas[i].shared_data = shared_data;
      internal_datas[i].private_data = private_data;
      internal_datas[i].password = passwords[i];
      internal_datas[i].path = array_paths[i];
      internal_datas[i].is_password_found = &is_found;
      internal_datas[i].size_combination =
        internal_datas[i].shared_data->password_lenght-i;
      pthread_create(&i_threads[i], NULL, create_combinations,
        &internal_datas[i]);
    }
  } else {
    for (uint64_t i = 0; i < usefull_threads-1; ++i) {
      internal_datas[i].shared_data = shared_data;
      internal_datas[i].private_data = private_data;
      internal_datas[i].password = passwords[i];
      internal_datas[i].path = array_paths[i];
      internal_datas[i].is_password_found = &is_found;
      internal_datas[i].size_combination =
        internal_datas[i].shared_data->password_lenght-i;
      pthread_create(&i_threads[i], NULL, create_combinations,
        &internal_datas[i]);
    }
    uint64_t i = usefull_threads-1;
    internal_datas[i].shared_data = shared_data;
    internal_datas[i].private_data = private_data;
    internal_datas[i].password = passwords[i];
    internal_datas[i].path = array_paths[i];
    internal_datas[i].is_password_found = &is_found;
    internal_datas[i].size_combination =
      internal_datas[i].shared_data->password_lenght-(i);
    pthread_create(&i_threads[i], NULL,
      create_combinations_multiple_sizes,
      &internal_datas[i]);
  }

  // loop used to wait for the threads to finish
  // executing & copy the result to actual_password
  for (uint64_t i = 0; i < usefull_threads; ++i) {
    pthread_join(i_threads[i], NULL);
    if (passwords[i][0] != '\0') {
      snprintf(actual_password, shared_data->password_lenght + 1,
        "%s", passwords[i]);
    }
  }

  pthread_mutex_destroy(&(private_data->mutex));

  for (uint64_t i = 0; i < usefull_threads; ++i) {
    remove_temporal_file(array_paths[i]);
  }

  for (uint32_t i = 0; i < usefull_threads; ++i) {
    free(array_paths[i]);
  }
  free(array_paths);
  for (uint32_t i = 0; i < usefull_threads; ++i) {
    free(buffer_array_paths[i]);
  }
  free(buffer_array_paths);
  for (uint32_t i = 0; i < usefull_threads; ++i) {
    free(passwords[i]);
  }
  free(passwords);
  return actual_password;
}

/**
 * @brief find_password function is used to create every single combination
 * of password posible, it solve the problem using a brute force approach.
 * The function generates all possible passwords using the given alphabet and
 * for every password, the program try the password using the check_password 
 * function, to know if the password is valid or not.

 * @param shared_data shared_data is a pointer to an instance of the
 * shared_data_t struct.
 * @param zip_file_path zip_file_path is the path of the given zip file
 * that must be unlocked.
 * @return If a valid password is found, it is stored in 
 * `shared_data->actual_password`  * and returned. 
 * Else the function return an empty string.
 */
char* find_password(shared_data_t* shared_data,
  char* actual_password, char* zip_file_path) {
  int64_t m = shared_data->password_lenght;
  int64_t n = shared_data->alphabet_lenght;
  const uint32_t PASSWORD_SIZE = m + 1;
  char password[PASSWORD_SIZE];
  int64_t i, j, k;

      struct zip* zip_file = NULL;
    zip_file = zip_open(zip_file_path, 0, NULL);
    if (zip_file == NULL) {
      printf("Error opening zip file: %s\n", zip_file_path);
    }

  for (i = 1; i <= m; i++) {
    memset(password, 0, sizeof(password));  // clear the password buffer
    for (j = 0; j < i; j++) {
      password[j] = shared_data->alphabet[0];
      // initialize the password with the first character of the alphabet
    }

    while (password[0] != '\0') {
      int32_t is_password_valid = check_password(password, zip_file);
      if (is_password_valid) {
        snprintf(actual_password,
          shared_data->password_lenght + 1, "%s", password);
        zip_close(zip_file);
        return actual_password;
        // break;
      }
      k = i - 1;

      while (k >= 0 && password[k] == shared_data->alphabet[n-1]) {
        k--;
      }

      if (k < 0) {
        break;
      }

      for (j = 0; j < n; j++) {
        if (shared_data->alphabet[j] > password[k]) {
          password[k] = shared_data->alphabet[j];
          break;
        }
      }

      for (j = k+1; j < i; j++) {
        password[j] = shared_data->alphabet[0];
      }
    }
  }
  zip_close(zip_file);
  return "";
}

/**
 * @brief check_password function is used to check if a given password
 * is valid for a given encrypted zip file.
 * @param password password is a char*, it's the password that must be tested.
 * @param zip_file_path zip_file_path is a string holding the path to the
 * encrypted zip file that must be unlock.
 * @return check_password return an integer value, if the password is valid
 * the function return a 1, in the contrary case the function return a 0;
 */
int32_t check_password(const char* password, struct zip* zip_file) {
  int32_t is_password_valid = 0;
  // open the zip file
  // struct zip* zip_file = NULL;
  // zip_file = zip_open(zip_file_path, 0, NULL);
  // if (zip_file == NULL) {
  //   printf("Error opening zip file: %s\n", zip_file_path);
  // } else {
    const char* file_name = NULL;
    file_name = zip_get_name(zip_file, 0, 0);
    if (file_name != NULL) {
      zip_file_t* file = NULL;
      file = zip_fopen_encrypted(zip_file,
        file_name, ZIP_FL_ENC_GUESS, password);
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
  // }

  // Close the zip file
  // zip_close(zip_file);
  return is_password_valid;
}

/**
 * @brief create_temporal_dir is a function used to created a temporal
 * folder that the program will delete after using it.
 * @param path path is a char*, it's the path to the directory that must be
 * created
 * @return the function return an error message
 */
int create_temporal_dir(char* path) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
    mkdir(path, 0700);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

/**
 * @brief create_temporal_fileis a function used to created a temporal
 * copy of a file that the program will delete after using it.
 * @param path_original_file path_original_file is a char*, it's the path
 * to the file that must be copied
 * @param tmp_file_path tmp_file_path is a char*, it's the path
 * to the file that will be created
 * @return the function return an error message
 */
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

/**
 * @brief remove_temporal_dir is a function used to delete a temporal
 * folder created by the program
 * @param path path is a char*, it's the path to the directory that must be
 * delete, the folder must be empty to be delete
 * @return the function return an error message
 */
int remove_temporal_dir(char* path) {
  struct stat st = {0};
  if (stat(path, &st) != -1) {
    return rmdir(path);
  }
  return EXIT_FAILURE;
}

/**
 * @brief remove_temporal_file is a function used to delete a temporal
 * file created by the program
 * @param path path is a char*, it's the path to the file that must be
 * delete
 * @return the function return an error message
 */
int remove_temporal_file(char* tmp_file_path) {
  return remove(tmp_file_path);
}

/**
 * @brief generate_combinations_rec is a function used to create every single
 * combination of a given lenght composed by the letters of a given alphabet.
 * This function use recursion.
 * file created by the program
 * @param current_combination current_combination is a char* used to store the
 * actual combination created by the program.
 * @param currentIndex currentIndex is an integer value that holds an index
 * used al the moment of accesing a given cell of the current_combination
 * @param zip_file_path zip_file_path is a char* that contains the path
 * to the zip file that must be solved
 * @param internal_data internal_data is a internal_data_t* that points to
 * the instance of internal_data_t that contains the multiple important
 * elements, like the mutex used to protect the critical area, or the
 * is_password_found boolean that allow the thread to now is the password
 * was already found by another thread
 */
void generate_combinations_rec(char *current_combination, uint64_t currentIndex,
  char* zip_file_path, internal_data_t* internal_data) {
    // base case where the combination is of the correct size
    if (currentIndex == internal_data->size_combination) {
      struct zip* zip_file = NULL;
      zip_file = zip_open(zip_file_path, 0, NULL);
      if (zip_file == NULL) {
        printf("Error opening zip file: %s\n", zip_file_path);
      }
      if (check_password(current_combination, zip_file)) {
        pthread_mutex_lock(&(internal_data->private_data->mutex));
        *(internal_data->is_password_found) = 1;
        snprintf(internal_data->password, internal_data->size_combination + 1,
          "%s", current_combination);
        pthread_mutex_unlock(&(internal_data->private_data->mutex));
      }
      zip_close(zip_file);
      return;
    }

    uint64_t i = 0;
    pthread_mutex_lock(&(internal_data->private_data->mutex));
    int condition_true = *(internal_data->is_password_found) == 0;
    pthread_mutex_unlock(&(internal_data->private_data->mutex));

    while (i < internal_data->shared_data->alphabet_lenght && condition_true) {
      pthread_mutex_lock(&(internal_data->private_data->mutex));
      condition_true = *(internal_data->is_password_found) == 0;
      pthread_mutex_unlock(&(internal_data->private_data->mutex));

      current_combination[currentIndex] =
        internal_data->shared_data->alphabet[i];
      generate_combinations_rec(current_combination, currentIndex + 1,
        zip_file_path, internal_data);
      ++i;
    }
}

/**
 * @brief create_combinations is a function used as the starting point for
 * the threads that search the combinations of a given lenght.
 * @param parameter parameter is a void*, in reality the pointer given by
 * parameter points to a internal_data_t instance that contains all the data
 * necessary so that the program can execute correctly.
 */
void* create_combinations(void* parameter) {
  internal_data_t* internal_data = (internal_data_t*) parameter;
  const uint32_t PASSWORD_SIZE = internal_data->size_combination + 1;
  char* password = malloc(PASSWORD_SIZE);
  memset(password, 0, PASSWORD_SIZE);

  // call the recursive function
  generate_combinations_rec(password, 0, internal_data->path, internal_data);

  free(password);
  return NULL;
}

/**
 * @brief create_combinations_multiple_sizes function is based on the
 * find_password function, it is used to create every single combination
 * of password posible from 0 up to the maximum given lenght, it solve the
 * problem using a brute force approach.
 * The function generates all possible passwords using the given alphabet and
 * for every password, the program try the password using the check_password 
 * function, to know if the password is valid or not.
 * @param parameter parameter is a void* that points to a internal_data_t that
 * contains all the data necessary to the execution of the function
 */
void* create_combinations_multiple_sizes(void* parameter) {
  internal_data_t* internal_data = (internal_data_t*) parameter;
  int64_t m = internal_data->shared_data->password_lenght;
  int64_t n = internal_data->shared_data->alphabet_lenght;
  const uint32_t PASSWORD_SIZE = m + 1;
  char password[PASSWORD_SIZE];
  int64_t i, j, k;

    struct zip* zip_file = NULL;
    zip_file = zip_open(internal_data->path, 0, NULL);
    if (zip_file == NULL) {
      printf("Error opening zip file: %s\n", internal_data->path);
    }

  for (i = 1; i <= m && *internal_data->is_password_found == 0; i++) {
    memset(password, 0, sizeof(password));  // clear the password buffer
    for (j = 0; j < i; j++) {
      password[j] = internal_data->shared_data->alphabet[0];
      // initialize the password with the first character of the alphabet
    }


    while (password[0] != '\0' && *internal_data->is_password_found == 0) {
      int32_t is_password_valid = check_password(password, zip_file);
      if (is_password_valid) {
        pthread_mutex_lock(&(internal_data->private_data->mutex));
        snprintf(internal_data->password,
          internal_data->shared_data->password_lenght + 1, "%s", password);
        *internal_data->is_password_found = 1;
        pthread_mutex_unlock(&(internal_data->private_data->mutex));

      } else {
        k = i - 1;

        while (k >= 0 && password[k] ==
          internal_data->shared_data->alphabet[n-1]) {
          k--;
        }

        if (k < 0) {
          break;
        }

        for (j = 0; j < n; j++) {
          if (internal_data->shared_data->alphabet[j] > password[k]) {
            password[k] = internal_data->shared_data->alphabet[j];
            break;
          }
        }

        for (j = k+1; j < i; j++) {
          password[j] = internal_data->shared_data->alphabet[0];
        }
      }
    }
  }
  zip_close(zip_file);
  return NULL;
}
