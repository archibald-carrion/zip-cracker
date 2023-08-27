# zippass_pthread

## Description of the problem

The zippass_pthread program allow the user to recover the forgotten password of a zip folder. 
The program do it using a brute force approach to solve the problem, the code is written in C programming language, and is based on the procedural programming paradigm.
To process the ZIP files, we use the [libzip](https://libzip.org/) library, a FOSS implementation of the ZIP format. 
The project is based on the [serial version](https://git.ucr.ac.cr/ARCHIBALD.CARRION/concurrente_2023a_archibald_carrion/-/tree/main/tareas/zippass_serial) previously created.
The main difference is that this version use parallel programming to improved the performance of the program, to use the threads we include the pthread library (POSIX threads).

## Description of the solution

### Design of the program

The zippass_pthread program is written in C, to enhance it's performance we use parallel programming in different way.
The quantity of threads that the program can use is given in command line parameters, if there is no parameter given at the momento of executing the program, therefore the quantity of thread that the program can use is the default cpu quantity.
Depending on the quantity of disponible cpus the program can do multiple things:

- If there is enough threads so that each path is treated by a different thread, therefore each thread will work with it's own paths
- If there is more paths than threads, then a mapping algorithm based on blocks will be used.
- If there is more threads than path then each path will be studied by different threads, to do this successfully the program must create as many copy of the zip files as there is usefull threads. The maximum threads than only one zip file can get is equal to the maximum lenght of it's password.

The program is based on multiple subroutines and 3 data structure.

\*\*main\*\*: the main function is the entry point of the program, it launch the password cracker algorithm.

\*\*find_passwords\*\*: the find_passwords function is used to choose where to redirect the threads, to the function that calculate the combination with only one thread, to the function that use at least two thread, or if this thread must solve multiple path, therefore loop and solve every given path.

\*\*create_combinations_multiple_sizes\*\*: is used when a thread must search all the combinations that can be made with more than one lenght.

\*\*create_combinations\*\*: is used when a thread must search all the combinations that can be made with only one lenght, this function is only used to call a recursive function called **generate_combinations_rec** that generates and check the passwords.

\*\*find_password_threads\*\*: this function is used when there is more threads than paths, it map the task given to it's threads, and create the temporal copies of the zip files. This function depending on the quantity of threads called the create_combinations function or/and the create_combinations_multiple_sizes.

\*\*find_password\*\*: this is the original functio that works in a serial way, it's only used when a thread has more than one path to solve.

\*\*check_password\*\*: check_password function is used to check if a given password is valid for a given encrypted zip file, first the program open the zip file given by the path, then try to open the first file (index 0) stored inside the zip file, and if this procedure works, the program check if the very first data stored in the file is the "CI0117-23a" string, if it is, then the program return 1, else the program return 0.

The first data structure used to solve the problem is **shared_data_t** a simple data structure that contains multiple data used by the different functions of the project:

- char\* alphabet: it's the set of chars that can be used in the password.
- uint64_t password_lenght: it's the maximum length of the password.
- uint64_t size_alphabet: it's the size of the alphabet.

The second data structure used to solve the problem is **private_data_t**, a data structure that contains data that is own by only one thread and it's subthreads (is there is):

- char** passwords: it's a variable used to store the passwords found by the thread, if the thread must solve multiple zip files, then is variable hold multiple passwords.
- char** paths: it's a variable used to store the paths to every zip file that must be solved 
- uint64_t paths_quantity: this variable is used so that the thread know how many problem (zip files)  must be solved.
- shared_data_t* shared_data: this pointer to a shared_data_t is used so that the thread can directly access the shared_data struct.
- uint64_t thread_quantity: this variable is used so that the thread knows how many subthread it can use, if thread_quantity is equal to 1, this mean the thread does not have additional threads.
- uint64_t thread_id: this variable is just a integer value used to know wich thread it is, this is usefull to create unique names for the zip files copies.
- pthread_mutex_t mutex: this mutex is used to protect critical area in the code.


The third data structure used to solve the problem is **internal_data_t**, a data structure that contains data that is own by only one sub-thread of one of the main threads:

-  shared_data_t\* shared_data: this pointer is used to points to the shared_data that all threads must be able to access
-  private_data_t\* private_data: this pointer points to the private_data of the thread that created it
-  char\* password: this char\* is used to store the password if found, if it's not found then the char\* remains empty (first index in '\0')
-  char\* path: this char\* contains the path to the temporary zip files that we want to unlock
-  uint64_t\* is_password_found: this integer value is used as a boolean, it's very important because it's used as a condition that tell the other threads to stop searching to the password once once thread found it. We the thread find the correct password it change it's value to 1.
-  uint64_t size_combination: this integer variable is used to store the size that the combination must conform to (except case where the function create_combinations_multiple_sizes is called, in wich case the size is used as a maximum, not a fixed value)


To learn more about the functions and data structures' relations, you can check the [UML](./design/README.md) graphs.


### Program input

To compile the code use the given Makefile and execute the following command:

```
make clean
make
```

To execute the program with a given quantity of thread use this command, with the number of threads that you want to be used instead of "n":

```
./bin/zippass_pthread n
```

Then you should give the necessary data to the program, using the following input format:

```
0123456789
5

tests/zip_05/f01.zip
tests/zip_05/f23.zip
tests/zip_05/f09.zip
```


### Program output

The output format used by the program is the following:

```
tests/zip_05/f01.zip 00112
tests/zip_05/f23.zip
tests/zip_05/f09.zip 9209
```

The first part of each line is the path to the zip file, and the second one is the password that was found.

### Program dependencies

To process the ZIP file we need to have installed the libzip:

```
$ sudo apt install libzip-dev
```

### Credits

Makefile created by [Profesor Jeisson Hidalgo-Céspedes](https://jeisson.ecci.ucr.ac.cr/misc/Makefile).

Code written by Archibald Emmanuel Carrion Claeys (archibald.carrion@ucr.ac.cr).

The function create_temporal_dir, create_temporal_file, remove_temporal_dir, and remove_temporal_file were created by the Profesor 