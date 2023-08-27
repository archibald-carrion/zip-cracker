#  Report
## Program performance 
### Comparison between the different versions of the program (serial, pthread and optimized versions) 
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

### Comparison between the different **new** versions of the  program (OpenMP VS MPI)
The main objective of this new versions of the program is to enhance the performance of the zippass program, to do this, instead of using pthread, we chose to use OpenMP and MPI.
OpenMP, unlike Ptreads, openmp use declarative parallelism, and MPI use process-based parallelism, both technologies are way easier to use than pthread.

The OpenMP version does show some really similar results to the pthread version, however the MPI should present a way better performance as it can uses multiples machines to execute each given path in a different process.
For multiple reason, we couldn't uses the Poas cluster to perform the calculations.
Therefore we used the same machine to execute the program instead of using a distributed system.
The MPI version ended up presenting one of the worse performance of all the versions, but it's not a fair comparison as we couldn't use the Poas cluster.

**The data presented in the excel file were obtained using my personal machine, therefore the data probably doesn't show the true time execution of the program in a more adapted environment.**