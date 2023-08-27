# zippass_optimized

## Description of the problem

The zippass_omp_mpi program allow the user to recover the forgotten password of a zip folder. 
The program do it using a productor-consumer approach to solve the problem.
The code is written in C programming language, and is based on the procedural programming paradigm.
To process the ZIP files, we use the [libzip](https://libzip.org/) library, a FOSS implementation of the ZIP format. 
The project is based on the [serial version](https://git.ucr.ac.cr/ARCHIBALD.CARRION/concurrente_2023a_archibald_carrion/-/tree/main/tareas/zippass_serial) previously created.
The main difference is that this version use parallel programming to improved the performance of the program, to use the threads we include the pthread library (POSIX threads).

## Description of the solution

### Design of the program

The zippass_omp_mpi program is written in C, to enhance it's performance we use parallel programming using MPI and OpenMP technologies.
The quantity of threads that the program can use is given in command line parameters, if there is no parameter given at the moment of executing the program, therefore the quantity of thread that the program can use is the default cpu quantity.

Unlike the previous version, this version of the programn can use as many thread as the we want and as many process as we want, the program will use the given quantity of threads and process to solve the problem. MPI create multiple process and those process solves the paths using a dynamic mapping, the first one to finish solve another path until there is no more paths to solves.

The program is based on four sub-routines:
- dispatch_threads: this function is used to dispatch the threads that will be used to solve the problem, openmp will use the given quantity of threads to solve the problem
- generate_combinations_rec: generate all the possible combinations of a given alphabet (used by the omp version and the mpi version)
- find_password: find the password of a given zip file (only used by the only omp version)
- check_password: check if a given password is valid for a given encrypted zip file.

In a same way, the program use the data structure:
- shared_data_t: this data structure is used to store the data that must be shared between all threads.


The shared_data_t data structure is composed of the following variables:

-  char* path_folder: this char* is used to store the path to the folder that contains the zip files that we want to unlock.
-  char* path_zip_files: this char* is used to store the path to the zip files that we want to unlock.
-  uint64_t is_password_found: this integer value is used as a boolean, it's very important because it's used as a condition that tell the other threads to stop searching to the password once once thread found it. We the thread find the correct password it change it's value to 1.
-  queue_t* queue: this pointer is used to points to the queue that contains the passwords that must be checked.
-  pthread_mutex_t mutex: this mutex is used to protect the access to the shared_data_t data structure and the consuming queue.
-  char** passwords: this char** is used to store the passwords that must be checked.
-  uint64_t password_length: this integer value is used to store the length of the passwords that must be checked.

The queue_t dta structure has the following functions:
- queue_t* queue_create(): this function is used to create a new queue.
- void queue_destroy(queue_t* queue): this function is used to destroy a queue.
- int is_empty(queue_t* queue): this function is used to check if a queue is empty.
- void enqueue(queue_t* queue, char* data): this function is used to add a new element to the queue.
- char* dequeue(queue_t* queue): this function is used to remove the first element of the queue.
- void queue_empty(queue_t* queue): this function is used to empty a queue.

To learn more about the functions and data structures' relations, you can check the [UML](./design/README.md) graphs.


### Program input

To compile the code use the given Makefile and execute the following command:

```
make clean
make
```

To execute the program with a given quantity of thread use this command, with the number of threads that you want to be used instead of "n":

```
./bin/zippass_omp_mpi n
```

Then you should give the necessary data to the program, using the following input format:

```
0123456789
5

tests/zip_05/f01.zip
tests/zip_05/f23.zip
tests/zip_05/f09.zip
```

### Input of the mpi version

```
mpiexec -n N ./bin/zippass_omp_mpi <./tests/input.txt
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