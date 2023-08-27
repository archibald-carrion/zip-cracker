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
#include <mpi.h>

#include "shared_data.h"

int32_t check_password(const char* password, struct zip* zip_file);
int create_temporal_dir(char* path);
int remove_temporal_dir(char* path);
int remove_temporal_file(char* tmp_file_path);
int create_temporal_file(char* path_original_file, char* tmp_file_path);
char* dispatch_threads(shared_data_t* shared_data, char* path,
  int32_t rank, int32_t thread_quantity);
void generate_combinations_rec(char *current_combination,
  uint64_t currentIndex, char* zip_file_path, uint32_t* is_password_found,
  shared_data_t* shared_data, char* real_password, uint64_t size_combination,
  int32_t rank);

/**
 * @brief main function is the entry point of the program.
 * @return the program return a integer value, a 0 on success.
 */
int main(int argc, char *argv[]) {
  if ( MPI_Init(&argc, &argv) == MPI_SUCCESS ) {
    int32_t quantity_process = 0;
    int32_t rank = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &quantity_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // SIZE_LIMIT is used to create char* for the paths
    const uint64_t SIZE_LIMIT = 256;

    // if the process is process 0, read data and send info to the others
    if ( rank == 0 ) {
      shared_data_t shared_data = {0};
      char** zip_files_paths = NULL;
      int32_t paths_quantity = 1;
      // paths_quantity is the quantity of zip files that must be solved
      int32_t thread_quantity = 0;
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
      // path is used as a temporary buffer for storing the given paths
      char path[SIZE_LIMIT];

      // while loop used to scan all the zip files'path given by the user
      while (fgets(path, sizeof(path), stdin) != NULL) {
        if (path[0] == '\n' || path[1] == '\n') {
          // separation line in the input data
          continue;
        }
        zip_files_paths = realloc(zip_files_paths,
          sizeof(char*)*paths_quantity);
        zip_files_paths[paths_quantity-1] = malloc(SIZE_LIMIT);
        sscanf(path, "%255s", zip_files_paths[paths_quantity-1]);

        ++paths_quantity;
      }
      --paths_quantity;

      char * path_folder = "tests/paths/";
      create_temporal_dir(path_folder);

      // while there is more or equal quantity of path than process
      if ((quantity_process-1) >= paths_quantity) {
        // printf("inside correct if \n");
        // initialize all the parameters that the other process will need
        for (int32_t i = 1; i < quantity_process; ++i) {
          MPI_Send(&paths_quantity, 1, MPI_INT,
            /* target */ i, 0, MPI_COMM_WORLD);
        }
        for (int32_t i = 1; i < paths_quantity+1; ++i) {
          // send size of alphabet
          MPI_Send(&shared_data.alphabet_lenght, 1, MPI_UNSIGNED_LONG_LONG,
            /* target */ i, 0, MPI_COMM_WORLD);
          // send size of password
          MPI_Send(&shared_data.password_lenght, 1, MPI_UNSIGNED_LONG_LONG,
            /* target */ i, 0, MPI_COMM_WORLD);
          // send alphabet
          MPI_Send(shared_data.alphabet, shared_data.alphabet_lenght + 1,
            MPI_CHAR, /* target */ i, 0, MPI_COMM_WORLD);
          // send the i path to i process, path is of size SIZE_LIMIT
          MPI_Send(zip_files_paths[i-1], strlen(zip_files_paths[i-1]) + 1,
            MPI_CHAR, /* target */ i, 0, MPI_COMM_WORLD);
          // thread_quantity
          MPI_Send(&thread_quantity, 1, MPI_INT, /* target */ i, 0,
            MPI_COMM_WORLD);
        }

        shared_data.passwords = malloc(sizeof(char*)*paths_quantity);

        // for each path, receive the password
        for (int32_t i = 1; i < paths_quantity+1; ++i) {
          char* password = malloc(shared_data.password_lenght +1);
          MPI_Status status;
          // receive the ith password
          MPI_Recv(password, shared_data.password_lenght + 1, MPI_CHAR,
            MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
          // send them a null path (stop condition)
          uint64_t index = status.MPI_SOURCE -1;
          shared_data.passwords[index] = password;
          char* stop_condition[SIZE_LIMIT];
          stop_condition[0] = '\0';
          MPI_Send(stop_condition, 1, MPI_CHAR, /* target */ index+1, 0,
            MPI_COMM_WORLD);
        }
      } else {  // dynamic mapping
        for (int32_t i = 1; i < quantity_process; ++i) {
          MPI_Send(&paths_quantity, 1, MPI_INT, /* target */ i, 0,
            MPI_COMM_WORLD);
        }
        for (int32_t i = 1; i < quantity_process; ++i) {
          // send size of alphabet
          MPI_Send(&shared_data.alphabet_lenght, 1, MPI_UNSIGNED_LONG_LONG,
            /* target */ i, 0, MPI_COMM_WORLD);
          // send size of password
          MPI_Send(&shared_data.password_lenght, 1, MPI_UNSIGNED_LONG_LONG,
            /* target */ i, 0, MPI_COMM_WORLD);
          // send alphabet
          MPI_Send(shared_data.alphabet, shared_data.alphabet_lenght + 1,
            MPI_CHAR, /* target */ i, 0, MPI_COMM_WORLD);
          // send the i path to i process, path is of size SIZE_LIMIT
          MPI_Send(zip_files_paths[i-1], strlen(zip_files_paths[i-1]) + 1,
            MPI_CHAR, /* target */ i, 0, MPI_COMM_WORLD);
        }

        shared_data.passwords = malloc(sizeof(char*)*paths_quantity);

        // index map represent the number of solved zips for each process
        int32_t map_solved[quantity_process+1];  // NOLINT
        for (int32_t i = 1; i < quantity_process; ++i) {
          map_solved[i] = i-1;
        }

        int32_t counter_solved = quantity_process-1;

        for (int32_t i = 0; i < paths_quantity; ++i) {
          char* password = malloc(shared_data.password_lenght +1);
          MPI_Status status;
          // receive the ith password
          MPI_Recv(password, shared_data.password_lenght + 1, MPI_CHAR,
          MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
          // send them a null path (stop condition)
          uint64_t index = status.MPI_SOURCE;
          shared_data.passwords[map_solved[index]] = password;
          // if counter_solved == quantity_process, send stop_condition
          if (counter_solved == paths_quantity) {
            char* stop_condition[SIZE_LIMIT];
            stop_condition[0] = '\0';
            MPI_Send(stop_condition, 1, MPI_CHAR,
              /* target */ index, 0, MPI_COMM_WORLD);
          } else {
            MPI_Send(zip_files_paths[counter_solved], SIZE_LIMIT, MPI_CHAR,
              /* target */ index, 0, MPI_COMM_WORLD);
            map_solved[index] = counter_solved;
            ++counter_solved;
          }
        }
      }

      remove_temporal_dir(path_folder);

      // print passwords
      for (int32_t i = 0; i < paths_quantity; ++i) {
        if (shared_data.passwords[i] != NULL) {
          printf("%s %s\n", zip_files_paths[i],
            shared_data.passwords[i]);
        } else {
          printf("%s \n", zip_files_paths[i]);
        }
      }

      for (int32_t i = 0; i < paths_quantity; ++i) {
        if (shared_data.passwords[i] != NULL) {
          free(shared_data.passwords[i]);
        }
        if (zip_files_paths[i] != NULL) {
          free(zip_files_paths[i]);
        }
      }
      free(shared_data.passwords);
      free(zip_files_paths);


    } else {  // workers process
      shared_data_t shared_data = {0};

      int32_t quantity_paths;
      MPI_Recv(&quantity_paths, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
      if (quantity_paths >= rank) {
        // recevie the alphabet size
        uint64_t alphabet_lenght;
        MPI_Recv(&alphabet_lenght, 1, MPI_UNSIGNED_LONG_LONG, 0, 0,
          MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        shared_data.alphabet_lenght = alphabet_lenght;
        // receive password size
        uint64_t password_lenght;
        MPI_Recv(&password_lenght, 1, MPI_UNSIGNED_LONG_LONG, 0, 0,
          MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        shared_data.password_lenght = password_lenght;

        // receive the alphabet
        char alphabet[alphabet_lenght];  // NOLINT
        MPI_Recv(alphabet, alphabet_lenght +1 , MPI_CHAR, 0, 0,
          MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        shared_data.alphabet = alphabet;
        // receive the path
        char path[SIZE_LIMIT];
        MPI_Recv(path, SIZE_LIMIT, MPI_CHAR, 0, 0, MPI_COMM_WORLD,
          MPI_STATUS_IGNORE);

        int32_t thread_quantity;
        MPI_Recv(&thread_quantity, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
          MPI_STATUS_IGNORE);

        while (path[0] != '\0') {
          // search password and send to 0
          char* password;
          password = dispatch_threads(&shared_data, path, rank,
            thread_quantity);

          // send to main the found password (or null)
          MPI_Send(password, strlen(password) + 1, MPI_CHAR,
            /* target */ 0, 0, MPI_COMM_WORLD);
          free(password);
          MPI_Recv(path, SIZE_LIMIT, MPI_CHAR, 0, 0, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
        }
      }
    }

    MPI_Finalize();

  } else {
    fprintf(stderr, " Error: could not init MPI \n");
  }

  return 0;
}

/**
 * @brief dispatch_threads function is a function used to dispatch the threads
 * @param shared_data shared_data is the struct containing all the data shared
 * between the threads
 * @param paths_quantity paths_quantity is the number of paths given by the
 * user
 */
char* dispatch_threads(shared_data_t* shared_data, char* path,
  int32_t rank, int32_t thread_quantity) {
  uint64_t usefull_threads = 0;
  if ((uint64_t)thread_quantity > shared_data->password_lenght) {
    usefull_threads =  shared_data->password_lenght;
  } else {
    usefull_threads =  shared_data->password_lenght;
  }
  // parallel for, is used for each path
  char **buffer_array_paths = malloc(usefull_threads*sizeof(char*));
  // loop used to create the named for the files
  srand(time(0));
  for (uint64_t j = 0; j < usefull_threads; ++j) {
    uint32_t size_thread_id = 0;
    uint64_t thread_id = rand()%1000 + rand()%10000 + omp_get_thread_num();  // NOLINT
    thread_id += thread_id+rand()%1000 + rank;  // NOLINT
    thread_id += thread_id*rank;
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
    create_temporal_file(path, array_paths[j]);
  }

  // case were there is a thread for each length
  uint32_t is_password_found = 0;

  char* true_password = malloc(shared_data->password_lenght + 1);
  memset(true_password, 0, strlen(true_password));  // clean password

  // parallel for each length
  #pragma omp parallel for num_threads(usefull_threads) shared(shared_data, is_password_found, array_paths, true_password, rank)  // NOLINT
  for (uint32_t h = 1; h <= usefull_threads; ++h) {
    // each thread call the function
    const uint64_t PASSWORD_SIZE = shared_data->password_lenght + 1;
    char* password = malloc(PASSWORD_SIZE);
    memset(password, 0, strlen(password));  // clean password
    generate_combinations_rec(password, 0, array_paths[h-1], &is_password_found,
      shared_data, true_password, h, rank);

    free(password);
  }

  for (uint64_t j = 0; j < usefull_threads; ++j) {
    remove_temporal_file(array_paths[j]);
  }

  for (uint64_t h = 0; h < usefull_threads; ++h) {
    remove_temporal_file(array_paths[h]);
  }

  for (uint32_t h = 0; h < usefull_threads; ++h) {
    free(array_paths[h]);
  }
  free(array_paths);
  for (uint32_t h = 0; h < usefull_threads; ++h) {
    free(buffer_array_paths[h]);
  }
  free(buffer_array_paths);
  return true_password;
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
  shared_data_t* shared_data, char* real_password, uint64_t size_combination,
  int32_t rank) {
  // base case where the combination is of the correct size
  if (currentIndex == size_combination) {
    struct zip* zip_file = NULL;
    zip_file = zip_open(zip_file_path, 0, NULL);
    if (zip_file == NULL) {
      printf("Error opening zip file: %s\n", zip_file_path);
    }
    if (check_password(current_combination, zip_file)) {
      const uint64_t PASSWORD_SIZE = shared_data->password_lenght +1;
      snprintf(real_password, PASSWORD_SIZE, "%s", current_combination);
      *is_password_found = 1;
    }
    zip_close(zip_file);
    return;
  }

  uint64_t i = 0;

  while (i < shared_data->alphabet_lenght && (*is_password_found) != 1) {
    current_combination[currentIndex] = shared_data->alphabet[i];
    generate_combinations_rec(current_combination, currentIndex + 1,
    zip_file_path, is_password_found, shared_data, real_password,
      size_combination, rank);
    ++i;
  }
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
