// Copyright 2023 Archibald Emmanuel Carrion Claeys
#ifndef SHARED_DATA
#define SHARED_DATA

typedef struct {
  char* alphabet;
  /** @brief alphabet is the set of chars that can be used in the password. */
  uint64_t password_lenght;
  /** @brief password_lenght is the maximum length of the password. */
  uint64_t alphabet_lenght;
  /** @brief size_alphabet is the size of the alphabet. */
  char** passwords;
  char** paths;
} shared_data_t;
#endif  // SHARED_DATA
