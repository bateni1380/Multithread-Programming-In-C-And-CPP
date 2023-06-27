# Matrix Multiplication Server (C++) and Linux Grep (C)
This was two project's for Operating System course tought by Dr. Bejani in June 2023 at Amirkabir University of Tecknology.

The run_server.cpp is a cpp server which provides multithreaded matrix multiplication api's using only boost library.

The grep.c is another project for my os course which is a simple implementation of grep in C (not c++) which search for regx patterns in folders using built-in libraries of C in linux.


## Multithreaded Matrix Multiplication C++ Server
The run_server.cpp is a cpp server which provides multithreaded matrix multiplication using only boost library.

![Capture](https://github.com/bateni1380/Multithreaded-Matrix-Multiplication-CPP-Server/assets/65423010/3c4c4680-fada-41d4-986e-133dae4e0232)

An example of using the run_server.cpp api's (using postman)
### Installation (run_server.cpp)
You have to install boost library and C++ (>11) to be able to run the run_server.cpp file.

For package installing in C++, you can use [this link](https://vcpkg.io/en/getting-started.html).
### Usage (run_server.cpp)
This c++ code, starts a server on your **http://localhost:8080/** 

#### /matrix_multiplation
You can use 
**http://localhost:8080/matrix_multiplication**
api to feed two matrices to be multiplied together.
You have to feed each matrix in each line of your request body. For example:
```
1, 2, 3; 4, 5, 6; 7, 8, 9
9, 8, 7; 6, 5, 4; 3, 2, 1
```
This is equivalent to multiplication of the following matrices.
####
$$
\begin{array}{cc}
\begin{bmatrix}
1 & 2 & 3 \\
4 & 5 & 6 \\
7 & 8 & 9
\end{bmatrix}
&
\begin{bmatrix}
9 & 8 & 7 \\
6 & 5 & 4 \\
3 & 2 & 1
\end{bmatrix}
\end{array}
$$

#### /change_maximum_matrix_workers
You can use 
**http://localhost:8080/change_maximum_matrix_workers**
api to change the quantity of threads that are working to calculate the multiplication. You can use this api during the multiplication proccess too (and the calculation will resume with that amount of threads)

You just need to feed amount of desired workers as the request body. For example:
```
10
```
### Code Review: explaining main classes and includes (run_server.cpp)
The following libraries have been included in this project. 
```c++
#include <iostream>
#include <sstream>
#include <string>
#include <semaphore.h>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
```
We need three first namespaces for working with strings and streams in the code.

We need semaphore for handling the amount of threads  which can calculate cells at the same time.

We need thread to handle requests at the same time (when we call matrix_multiplication, the request is not finished yet and suddonly we call change_maximum_matrix_workers)

We need boost libraries to make a c++ http server.

##
```c++
class Matrix 
{
public:
    Matrix(const vector<vector<float>>& matrixValues) : values(matrixValues);
    Matrix(const string& matrixString);
    Matrix operator*(const Matrix& other);
    string generateString();
    static void changeMaximumInnerWorkers(int miw);
    vector<vector<float>> values;
private:
    float calculateMultiplicationCell(const Matrix& other, int row_indice, int column_indice, int common_dim) const;
    static int activeInnerWorkers;
    static int maximumInnerWorkers;
    static mutex m;
    static sem_t semaphore;
};
int Matrix::activeInnerWorkers = 0;
int Matrix::maximumInnerWorkers = 10;
mutex Matrix::m;
sem_t Matrix::semaphore;
```

This is the class that contains multiplication methods and multithreading methods. The **operator*** method is using **calculateMultiplicationCell** method for each cell to calculate the output. It is guaranteed that the **calculateMultiplicationCell** method is being called less than **maximumInnerWorkers** times at the same time.

##

```c++
class Session
{
public:
    Session(io_service& io_service, strand& strand);
    socket& socket();
    void start();
private:
    void doRead();
    void doJob();
    void doWrite();

    socket socket_;
    strand& strand_;
    streambuf request_;

    string requestTypeStr;
    string requestAddressStr;
    string requestBodyStr;

    string reponseBodyStr;
};
```
This class contains functionality for each coming request.
In **doRead()** function, it reads the information from the request and fills **requestTypeStr** and **requestAddressStr** and **requestBodyStr** attributes of the class. After reading the data, it calls **doJob()**, which contains the functionality of each api of our program and uses the mentioned attributes to process the request and constructs the answer of the request which will be saved in **reponseBodyStr** attribute which will be posted to the client using **doWrite()** function.
##

```c++
class Server
{
public:
    Server(io_service& io_service, strand& strand, const endpoint& endpoint);
private:
    void run();
    void onAccept(shared_ptr<Session> new_session, const error_code& error);
    io_service& io_service_;
    strand& strand_;
    acceptor acceptor_;
};
```
This class is the server class which accepts requests and makes a new session for each request.


#

## Grep
The grep.c is a simple implementation of grep in C (not c++) using built-in libraries of C in linux.

This program is like grep in linux. It searchs for an specefic regx pattern in a root directory.

![Capture](https://github.com/bateni1380/Multithread-Programming-In-C-And-CPP/assets/65423010/36c3d9cf-cec9-424e-ac0f-36e87bff0d8f)

An example of using the grep.c (using Ubunto subsystem in windows 10)

### Installation (grep.c)
You just need to install C compiler in a linux os. 

(for example Ubuntu)

```bash
sudo apt update
sudo apt install build-essential
```

Note that this C code can be executed only in linux. (not windows)

### Usage (grep.c)
First, you need to compile your project using gcc compiler.

(for example if your C file was named grep.c)

```bash
 gcc grep.c -o grep
```
Second, you need to have all the files and folders that you want to be searched in a floder named root beside the executable file.

Then you need to run the executable file and specify options afterwards.

```bash
sudo ./grep [pattern] [isSearchingFile] [isShowingLine] [maxSearchDepth] [maxWorkingThreads] [isReverseGrep]
```
In the code above, pattern corresponds to the regx format that you are looking for. 

isSearchingFile indicates if you wanna search files contents or folder names.

isShowingLine says if you want to write line of found pattern in the file or not.

maxSearchDepth is the maximum depth of search in folders

maxWorkingThreads is the maximum number of threads that can be executed simultaneously

isReverseGrep indicates if you wanna print the lines that doesn't have the pattern in them or does.

An example of using the grep is shown below.

```bash
sudo ./grep he[a-z]llo 1 1 10 3 0
```

### Code Review: explaining main functions and includes (grep.c)
The following libraries have been included in this project. 
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <sys/stat.h>
#include <regex.h>
```
We need three first namespaces for working with strings and streams in the code.

We need semaphore for handling the amount of threads  which can search at the same time.

We need thread to define a thread for each file searching. (then we limit it using semaphore)

We need fourth and fifth library for accessing folders and files in system (linux)

We need last library to match the regx pattern with folder names or file contents.

##
```c
void check_folder_name(char* path, char* f_name);
```
This is a function that gets the folder name and path and prints its name if the pattern matches the folder name.
##
```c
void* search_file(void* arg);
```
This function searchs for the pattern for every lines of a file...The arg input is just the path and its basicly a char* but beacuase we wanna run this function in a thread, the input must be void* and we convert it to char* afterwards.

Also, this function uses semaphore so no more than [maxWorkingThreads] threads of search_file can be executed simultaneously.
##
```c
void search_directory(const char* directory, int depth, int max_depth);
```
This function searchs all the directories recursively.
