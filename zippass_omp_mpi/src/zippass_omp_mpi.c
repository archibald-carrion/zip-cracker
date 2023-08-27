// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <stdio.h>
#include <unistd.h>
#include <zip.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <sys/stat.h>
#include <time.h>

#include "shared_data.h"

char** copy_subarray(char** base_array, uint64_t starting_index,
  uint64_t ending_index);
void find_password(shared_data_t* shared_data, uint32_t index);
int32_t check_password(const char* password, struct zip* zip_file);
int create_temporal_dir(char* path);
int remove_temporal_dir(char* path);
int create_temporal_file(char* path_original_file, char* tmp_file_path);
int remove_temporal_file(char* tmp_file_path);
void dispatch_threads(shared_data_t* shared_data, uint64_t  paths_quantity);
void generate_combinations_rec(char *current_combination,
  uint64_t currentIndex, char* zip_file_path, uint32_t* is_password_found,
  shared_data_t* shared_data, uint32_t index, uint64_t size_combination);

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

  // scanf used to scan the first 2 datas given by the user
  scanf("%ms %"SCNu64"", &shared_data.alphabet
    , &shared_data.password_lenght);

  shared_data.alphabet_lenght = strlen(shared_data.alphabet);
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

  char * path_folder = "tests/paths/";
  create_temporal_dir(path_folder);
  shared_data.passwords = malloc(sizeof(char*)*paths_quantity);
  shared_data.paths = malloc(sizeof(char*)*paths_quantity);


  clock_t start, end;
  double cpu_time_used;
  start = clock();

  if (paths_quantity >= thread_quantity) {
    if ( thread_quantity < paths_quantity ) {
      thread_quantity = paths_quantity;
    }

    #pragma omp parallel for num_threads(thread_quantity) shared(shared_data, zip_files_paths)  // NOLINT
    for (uint32_t i = 0; i < thread_quantity; ++i) {
      shared_data.paths[i] = zip_files_paths[i];
      shared_data.passwords[i] = NULL;

      find_password(&shared_data, i);
    }

  } else {  // multiple thread will be solving the same zip file
    // there will be usefull_threads for each paths
    for (uint32_t i = 0; i < paths_quantity; ++i) {
      shared_data.paths[i] = zip_files_paths[i];
    }

    dispatch_threads(&shared_data, paths_quantity);
  }


  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("Execution time: %.2f seconds\n", cpu_time_used);

  remove_temporal_dir(path_folder);

  // print the paswords stored in passwords in shared data struct
  for (uint64_t i = 0; i < paths_quantity; ++i) {
    if (shared_data.passwords[i] != NULL) {
      printf("%s %s\n", shared_data.paths[i],
        shared_data.passwords[i]);
    } else {
      printf("%s \n", shared_data.paths[i]);
    }
  }

  for (uint64_t i = 0; i < paths_quantity; ++i) {
    if (shared_data.passwords[i] != NULL) {
      free(shared_data.passwords[i]);
    }
    if (shared_data.paths[i] != NULL) {
      free(shared_data.paths[i]);
    }
  }
  free(shared_data.paths);
  free(shared_data.passwords);
  free(zip_files_paths);

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
 * @brief dispatch_threads function is a function used to dispatch the threads
 * @param shared_data shared_data is the struct containing all the data shared
 * between the threads
 * @param paths_quantity paths_quantity is the number of paths given by the
 * user
 */
void dispatch_threads(shared_data_t* shared_data, uint64_t  paths_quantity) {
  uint64_t usefull_threads =  shared_data->password_lenght;
  // parallel for, is used for each path
  #pragma omp parallel for num_threads(paths_quantity) shared(shared_data, usefull_threads)  // NOLINT
  for (uint32_t i = 0; i < paths_quantity; ++i) {
    char **buffer_array_paths = malloc(usefull_threads*sizeof(char*));
    // loop used to create the named for the files
    srand(time(0));
    for (uint64_t j = 0; j < usefull_threads; ++j) {
      uint32_t size_thread_id = 0;
      uint64_t thread_id = rand()%1000 + rand()%10000 + omp_get_thread_num()+i; // NOLINT
      thread_id += thread_id+rand()%1000;  // NOLINT
      thread_id += i * thread_id;
      uint64_t buffer = thread_id;
      while (buffer != 0) {
        buffer = buffer/10;
        ++size_thread_id;
      }

      buffer_array_paths[j] = malloc(1+size_thread_id);
      memset(buffer_array_paths[j], '\0', 1+size_thread_id);
      snprintf(buffer_array_paths[j], size_thread_id,
        "%"PRIu64, thread_id);
      buffer_array_paths[j][size_thread_id] = '\0';
    }


    char **array_paths = malloc(usefull_threads*sizeof(char*));
    char* general_path = "tests/paths/zip_file_";
    // loop used to create the paths to the files
    for (uint64_t j = 0; j < usefull_threads; ++j) {
      uint64_t size_string = strlen(general_path) +
        strlen(buffer_array_paths[j]) + 1;
      array_paths[j] = malloc(size_string);
      memset(array_paths[j], '\0', size_string);
      strcpy(array_paths[j], general_path);  // NOLINT
      strcat(array_paths[j], buffer_array_paths[j]);  // NOLINT
    }

    for (uint64_t j = 0; j < usefull_threads; ++j) {
      create_temporal_file(shared_data->paths[i], array_paths[j]);
    }

    // case were there is a thread for each length
    uint32_t is_password_found = 0;

    // parallel for each length
    #pragma omp parallel for num_threads(usefull_threads) shared(shared_data, is_password_found, array_paths, i)  // NOLINT
    for (uint32_t h = 1; h <= usefull_threads; ++h) {
      // each thread call the function
      const uint64_t PASSWORD_SIZE = shared_data->password_lenght + 1;
      char* password = malloc(PASSWORD_SIZE);
      // clean password
      memset(password, 0, strlen(password));
      generate_combinations_rec(password, 0, array_paths[h-1],
        &is_password_found, shared_data, i, h);

      free(password);
    }

    // delete each copy

    for (uint64_t h = 0; h < usefull_threads; ++h) {
      remove_temporal_file(array_paths[h]);
    }


    for (uint32_t i = 0; i < usefull_threads; ++i) {
      free(array_paths[i]);
    }
    free(array_paths);
    for (uint32_t i = 0; i < usefull_threads; ++i) {
      free(buffer_array_paths[i]);
    }
    free(buffer_array_paths);
  }
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
void generate_combinations_rec(char *current_combination,
  uint64_t currentIndex, char* zip_file_path, uint32_t* is_password_found,
  shared_data_t* shared_data, uint32_t index, uint64_t size_combination) {
  // base case where the combination is of the correct size
  if (currentIndex == size_combination) {
    struct zip* zip_file = NULL;
    zip_file = zip_open(zip_file_path, 0, NULL);
    if (zip_file == NULL) {
      printf("Error opening zip file: %s\n", zip_file_path);
    }
    if (check_password(current_combination, zip_file)) {
      const uint64_t PASSWORD_SIZE = shared_data->password_lenght +1;
      shared_data->passwords[index] = malloc(PASSWORD_SIZE);
      snprintf(shared_data->passwords[index], PASSWORD_SIZE, "%s",
        current_combination);
      *is_password_found = 1;
    }
    zip_close(zip_file);
    return;
  }

  uint64_t i = 0;

  while (i < shared_data->alphabet_lenght && (*is_password_found) != 1) {
    current_combination[currentIndex] = shared_data->alphabet[i];
    generate_combinations_rec(current_combination, currentIndex + 1,
    zip_file_path, is_password_found, shared_data, index, size_combination);
    ++i;
  }
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
void find_password(shared_data_t* shared_data, uint32_t index) {
  int64_t m = shared_data->password_lenght;
  int64_t n = shared_data->alphabet_lenght;
  const uint32_t PASSWORD_SIZE = m + 1;
  char password[PASSWORD_SIZE];
  int64_t i, j, k;


  struct zip* zip_file = NULL;
  zip_file = zip_open(shared_data->paths[index], 0, NULL);
  if (zip_file == NULL) {
    printf("Error opening zip file: %s\n", shared_data->paths[index]);
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
        shared_data->passwords[index] = malloc(PASSWORD_SIZE);
        snprintf(shared_data->passwords[index],
          PASSWORD_SIZE, "%s", password);
        zip_close(zip_file);
        return;
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
  return;
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
          }
        }
        zip_fclose(file);
      }
    }

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
