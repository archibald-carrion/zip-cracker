// Copyright 2023 Archibald Emmanuel Carrion Claeys
#include <stdio.h>
#include <zip.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "shared_data.h"

char* find_password(shared_data_t* shared_data, char* zip_file_path);
int32_t check_password(const char* password, const char* zip_file_path);

/**
 * @brief main function is the entry point of the program.
 * @return the program return a integer value, a 0 on success.
 */
int main() {
  shared_data_t shared_data = {0};
  char** zip_files_paths = NULL;
  uint64_t paths_quantity = 1;

  // scanf used to scan the first 2 datas given by the user
  scanf("%ms %"SCNu64"", &shared_data.alphabet
    , &shared_data.password_lenght);

  shared_data.size_alphabet = strlen(shared_data.alphabet);
  shared_data.actual_password = malloc(shared_data.password_lenght + 1);
  memset(shared_data.actual_password, 0, shared_data.password_lenght + 1);
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
  for (size_t i = 0; i < paths_quantity; ++i) {
    printf("%s %s \n", zip_files_paths[i],
      find_password(&shared_data, zip_files_paths[i]));
  }

  // freeing allocated memory
  while (paths_quantity != 0) {
    --paths_quantity;
    free(zip_files_paths[paths_quantity]);
  }
  free(zip_files_paths);
  free_shared_data(&shared_data);
  return 0;
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
char* find_password(shared_data_t* shared_data, char* zip_file_path) {
  int64_t m = shared_data->password_lenght;
  int64_t n = shared_data->size_alphabet;
  const uint32_t PASSWORD_SIZE = m + 1;
  char password[PASSWORD_SIZE];
  int64_t i, j, k;

  for (i = 1; i <= m; i++) {
    memset(password, 0, sizeof(password));  // clear the password buffer
    for (j = 0; j < i; j++) {
      password[j] = shared_data->alphabet[0];
      // initialize the password with the first character of the alphabet
    }

    while (password[0] != '\0') {
      int32_t is_password_valid = check_password(password, zip_file_path);
      // printf("%s \n", password);
      if (is_password_valid) {
        // printf("You found the correct password\n");
        snprintf(shared_data->actual_password,
          shared_data->password_lenght + 1, "%s", password);
        return shared_data->actual_password;
        break;
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
int32_t check_password(const char* password, const char* zip_file_path) {
  int32_t is_password_valid = 0;
  // open the zip file
  struct zip* zip_file = NULL;
  zip_file = zip_open(zip_file_path, 0, NULL);
  if (zip_file == NULL) {
    printf("Error opening zip file: %s\n", zip_file_path);
  } else {
    const char* file_name = NULL;
    file_name = zip_get_name(zip_file, 0, 0);
    // printf("%s", file_name);
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
          // printf("%s \n", password);
          // printf("%s \n", buffer);
          char* id = NULL;
          id = strtok(buffer, " \t\n");
          if ((strcmp(id, "CI0117-23a")) == 0) {
            is_password_valid = 1;
          }
        }
        zip_fclose(file);
      }
    }
  }

  // Close the zip file
  zip_close(zip_file);
  return is_password_valid;
}
