# zippass_optimized

## Description of the problem

The zippass_optimized program allow the user to recover the forgotten password of a zip folder. 
The program do it using a productor-consumer approach to solve the problem.
The code is written in C programming language, and is based on the procedural programming paradigm.
To process the ZIP files, we use the [libzip](https://libzip.org/) library, a FOSS implementation of the ZIP format. 
The project is based on the [serial version](https://git.ucr.ac.cr/ARCHIBALD.CARRION/concurrente_2023a_archibald_carrion/-/tree/main/tareas/zippass_serial) previously created.
The main difference is that this version use parallel programming to improved the performance of the program, to use the threads we include the pthread library (POSIX threads).

## Description of the solution

### Design of the program

The zippass_optimized program is written in C, to enhance it's performance we use parallel programming and the producer-consumer patern, to do this we use the pthread library (POSIX threads).
The quantity of threads that the program can use is given in command line parameters, if there is no parameter given at the moment of executing the program, therefore the quantity of thread that the program can use is the default cpu quantity.

Unlike the previous version, this version of the programn can use as many thread as the we want, however the program will loose efficiency if the quantity of threads is too high, as there is multiple data that must be shared between all threads, and each read-access to those data must be protected by a mutex, therefore if there is too many threads, the program will spend a lot of time locking and unlocking the mutex, therefore the program will become slower than a version using less threads.

The program is based on five sub-routines:
- create_consumers: this function is used to create the threads that will be used to solve the problem.
- consume: consume the password stored in the consuming queue.
- produce: produce the password that will be stored in the consuming queue.
- wait_consumers: wait for the consumers to finish their work.
- check_password: check if a given password is valid for a given encrypted zip file.

In a same way, the program use two data structures:
- shared_data_t: this data structure is used to store the data that must be shared between all threads.
- queue_t: this data structure is used to store the passwords that must be checked.

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

## Program performance
The main objective of this new versions of the program is to enhance the performance of the zippass program, to do this we use parallel programming. The three versions of the program are the following:
- zippass_optimized: a new version that use producer-consumer approach to solve the problem.
- zippass_serial_optimized: similar to the previous version, but we apply a few optimizations to the code.
- zippass_pthread_optimized: similar to the previous version, but we applied a few enhancements to the code.
As seem in the graph and the given tables, the optimized version  of serial and pthread do present a better performance than the previous versions, however the zippass_optimized version does not present such a better performance as expected.

One of the reasons is the use of critical zones within the program, as the program must use a mutex to protect the access to the shared data, therefore the program must lock and unlock the mutex every time that it needs to access the shared data, and each time a thread is inside the mutex, every single other thread need to wait util getting access to the mutex, as the loops can be executed quite fast, the threads end up spending more time waiting for the mutex than actually working.
Higher the quantity of threads, more threads will be waiting for the mutex.

This way we found that the zippass_optimized still has a better performance than the original serial version, but it's not as good as expected, has both the pthread versions and the enhanced serial version present a better performance than the zippass_optimized version.

But after analyzing the performance of each version, we found that the best version with the adequate thread quantity is the optimized version of the pthread implementation, not the producer-consumer version as expected.

To calculate the performance of the program, we need a ``` T_before ``` and ``` T_after ``` values, the ``` T_before ``` value is the time before being optimized, and the ``` T_after ``` value is the execution time after the program being optimized.

``` T_before = 105 ``` and ``` T_after = 78 ```
Therefore the program has a speedup of ``` ~1.346 ``` and an efficiency of ``` ~0.33 ```. 

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