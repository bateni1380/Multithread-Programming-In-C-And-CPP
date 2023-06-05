## Multithreaded Matrix Multiplication CPP Server
### Installation
You have to install boost library and C++ (>11) to be able to run the run_server.cpp file.

### Usage
This c++ code, starts a server on your **http://localhost:8080/** 

#### /matrix_multiplation
You can use 
**http://localhost:8080/matrix_multiplation**
api to feed two matrices to be multiplied together.
You have to feed each matrix in each line of your request body. For example:
```
1, 2, 3; 4, 5, 6; 7, 8, 9
9, 8, 7; 6, 5, 4; 3, 2, 1
```
This is equivalent to multiplication of the following matrices.
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
### Code Review




