## Multithreaded Matrix Multiplication CPP Server
This is a cpp server which provides multithreaded matrix multiplication using only boost library.

![An example of using the apps api's (using postman)](https://github.com/bateni1380/Multithreaded-Matrix-Multiplication-CPP-Server/assets/65423010/e8d6ebe0-bfcd-4423-a5b5-b6d0ce613104)
An example of using the apps api's (using postman)

### Installation
You have to install boost library and C++ (>11) to be able to run the run_server.cpp file.

### Usage
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
### Code Review (explaining main classes and includes)
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

we need semaphore for handling the amount of threads  which can do calculate cells at the same time.

we need thread to handle requests at the same time (when we call matrix_multiplication, the request is not finished yet and suddonly we call change_maximum_matrix_workers)
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
    void start()
private:
    void doRead()
    void doJob()
    void doWrite()

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
    Server(io_service& io_service, strand& strand, const endpoint& endpoint)
private:
    void run()
    void onAccept(shared_ptr<Session> new_session, const error_code& error)
    io_service& io_service_;
    strand& strand_;
    acceptor acceptor_;
};
```
This class is the server class which accepts requests and makes a new session for each request.



