// Copyright: Mohammad Reza Bateni
// https://github.com/bateni1380/Multithread-Programming-In-C-And-CPP/

#include <iostream>
#include <sstream>
#include <string>
#include <semaphore.h>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

using namespace std;


// The class that handles matrix operation (a semaphore is indicated to 
//     the whole class to controll the overall treads that will be created by this class)
class Matrix {

public:
    Matrix(const vector<vector<float>>& matrixValues) : values(matrixValues) {} // Construct using a vector of vectors

    Matrix(const string& matrixString) // Construct using an string like "1, 2, 3; 4, 5, 6; 7, 8, 9" which corresponds to a 3*3 matrix 
    {
        stringstream ss(matrixString);
        string rowString;

        while (getline(ss, rowString, ';'))
        {
            stringstream rowSS(rowString);
            string cellString;
            vector<float> rowValues;

            while (getline(rowSS, cellString, ','))
            {
                float value = stof(cellString);
                rowValues.push_back(value);
            }

            values.push_back(rowValues);
        }
    }

    Matrix operator*(const Matrix& other)
    {
        int rows1 = values.size();
        int cols1 = values[0].size();
        int rows2 = other.values.size();
        int cols2 = other.values[0].size();

        // Check if the matrices can be multiplied
        if (cols1 != rows2)
        {
            throw runtime_error("Matrix dimensions are not compatible for multiplication.");
        }

        // Create the result matrix with appropriate dimensions
        vector<vector<float>> result(rows1, vector<float>(cols2, 0.0));

        // Initiallize semaphore
        sem_init(&semaphore, 0, maximumInnerWorkers);

        // Perform matrix multiplication
        for (int i = 0; i < rows1; i++)
        {
            for (int j = 0; j < cols2; j++)
            {
                // Wait until there is available spot in semaphore to proccess
                sem_wait(&semaphore);

                // Make the thread which calculates ijth value of answer matrix
                thread t([this, other, i, j, cols1, &result]()
                    {
                        float answer = calculateMultiplicationCell(other, i, j, cols1);
                        result[i][j] = answer;
                        sem_post(&semaphore); // Release the semaphore once the function f has completed
                        --activeInnerWorkers;      // Decrement active thread count
                    });
                t.detach();
                ++activeInnerWorkers; // Increment active thread count
            }
        }

        // Wait until every cell has calculated
        while (activeInnerWorkers > 0)
        {
            this_thread::yield(); // Yield the CPU to other threads
        }
        return Matrix(result);
    }

    string generateString()
    {
        string result;
        for (const auto& row : values) {
            for (const auto& cell : row) {
                result += std::to_string(cell) + ' ';
            }
            result += '\n';
        }
        return result;
    }


    static void changeMaximumInnerWorkers(int miw)
    {
        m.lock();
        sem_destroy(&semaphore); // Destroy the semaphore
        sem_init(&semaphore, 0, miw); // Create a new semaphore with the updated value
        maximumInnerWorkers = miw;
        m.unlock();
    }

    vector<vector<float>> values;

private:
    float calculateMultiplicationCell(const Matrix& other, int row_indice, int column_indice, int common_dim) const
    {
        float answer = 0;
        // Multiply every cell of ith row of this matrix by every cell of jth column of the other matrix
        for (int k = 0; k < common_dim; k++)
        {
            m.lock();
            answer += values[row_indice][k] * other.values[k][column_indice];
            m.unlock();
        }
        return answer;
    }

    static int activeInnerWorkers;
    static int maximumInnerWorkers;
    static mutex m;
    static sem_t semaphore;
};
int Matrix::activeInnerWorkers = 0;
int Matrix::maximumInnerWorkers = 10;
mutex Matrix::m;
sem_t Matrix::semaphore;


// The class that handles each session (an instance of this object will be created for each request)
class Session : public enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_service& io_service,
        boost::asio::io_service::strand& strand)
        : socket_(io_service), strand_(strand)
    {
    }

    boost::asio::ip::tcp::socket& socket() { return socket_; }

    void start()
    {
        // At the time of starting the session, read data from socket
        doRead();
    }

private:
    // Filling requestTypeStr and requestAddressStr and requestBodyStr
    void doRead()
    {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, request_, "\r\n\r\n",
            [this, self](boost::system::error_code ec, size_t length)
            {
                if (!ec)
                {
                    string data(istreambuf_iterator<char>(&request_), {}); // Getting request string from request_

                    size_t receivedRequestPos = data.find("Received request:");

                    // Extracting request type (POST, GET, ...) from the string
                    size_t requestTypeStartPos = receivedRequestPos + 1;
                    size_t requestTypeEndPos = data.find(" ", requestTypeStartPos);
                    requestTypeStr = data.substr(requestTypeStartPos, requestTypeEndPos - requestTypeStartPos);

                    // Extracting request address (/api1, /api2, ...) from the string
                    size_t requestAddressStartPos = requestTypeEndPos + 1;
                    size_t requestAddressEndPos = data.find(" ", requestAddressStartPos);
                    requestAddressStr = data.substr(requestAddressStartPos, requestAddressEndPos - requestAddressStartPos);

                    // Extracting request body data from the string
                    size_t bodyLengthPos = data.find("Content-Length: ");
                    size_t bodyLenghtStartPos = bodyLengthPos + 16;
                    size_t bodyLenghtEndPos = data.find("\r\n", bodyLenghtStartPos);
                    string bodyLengthStr = data.substr(bodyLenghtStartPos, bodyLenghtEndPos - bodyLenghtStartPos);
                    size_t bodyLength = stoull(bodyLengthStr);
                    requestBodyStr = data.substr(bodyLenghtEndPos + 4, bodyLength);

                    cout << "Request received: " << requestTypeStr << " " << requestAddressStr << endl;
                    cout << "Request body: " << endl << requestBodyStr << endl;

                    doJob();
                }
            });
    }

    // Using requestTypeStr and requestAddressStr and requestBodyStr to calculate reponseBodyStr
    void doJob()
    {
        if (requestTypeStr == "POST" && requestAddressStr == "/matrix_multiplication")
        {
            try
            {
                stringstream ss(requestBodyStr);
                string rowString;
                vector<Matrix> matrices;

                // Iterating on the request lines
                while (getline(ss, rowString, '\n'))
                {
                    matrices.push_back(Matrix(rowString));
                }

                // Calculating the multiplication of the matrices corresponding to first line and second line of the request
                Matrix c = matrices[0] * matrices[1];

                // Converting the answer to a propper string and putting it in reponseBodyStr to be sent back
                reponseBodyStr = c.generateString();
                doWrite();
            }
            catch (const std::exception& ex)
            {
                cout << "Exception: " << ex.what() << endl;

                // Sending the exception to client
                reponseBodyStr = ex.what();
                doWrite();
            }

        }
        else if (requestTypeStr == "POST" && requestAddressStr == "/change_maximum_matrix_workers")
        {
            try
            {
                int maximum_matrix_workers = std::stoi(requestBodyStr); // Using a built in function to convert the body string to int
                Matrix::changeMaximumInnerWorkers(maximum_matrix_workers); // Using the static method of Matrix class to change number of workers
                reponseBodyStr = "Maximum matrix workers count has been changed to " + to_string(maximum_matrix_workers);
                doWrite();
            }
            catch (const std::exception& ex)
            {
                cout << "Exception: " << ex.what() << endl;

                // Sending the exception to client
                reponseBodyStr = ex.what();
                doWrite();
            }
        }
        if (requestTypeStr == "POST" && requestAddressStr == "/check")
        {
            // Checking if the server can handle two requests at the same time
            doWrite();
            this_thread::sleep_for(chrono::milliseconds(5000));
            cout << "yes" << endl;
        }
    }

    // Responding reponseBodyStr
    void doWrite()
    {
        boost::asio::write(socket_, boost::asio::buffer("HTTP/1.1 200 OK\r\n\r\n" + reponseBodyStr + "\r\n"));
        cout << "Response Sent.\n" << endl;
        socket_.close(); // Close the connection
    }

    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand& strand_;
    boost::asio::streambuf request_;

    string requestTypeStr;
    string requestAddressStr;
    string requestBodyStr;

    string reponseBodyStr;
};


// The class that handles the whole server
class Server
{
public:
    Server(boost::asio::io_service& io_service,
        boost::asio::io_service::strand& strand,
        const boost::asio::ip::tcp::endpoint& endpoint)
        : io_service_(io_service), strand_(strand), acceptor_(io_service, endpoint)
    {
        run();
    }

private:
    void run()
    {
        shared_ptr<Session> new_session(new Session(io_service_, strand_)); // Make a new session for each request
        acceptor_.async_accept(new_session->socket(), strand_.wrap(boost::bind(&Server::onAccept, this, new_session, _1)));
    }

    void onAccept(shared_ptr<Session> new_session, const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
        }
        run();
    }

    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand& strand_;
    boost::asio::ip::tcp::acceptor acceptor_;
};


// The class that handles workers that wait for requests
class workerThread
{
public:
    static void run(shared_ptr<boost::asio::io_service> io_service)
    {
        m.lock();
        cout << "[" << this_thread::get_id() << "] Worker thread starts" << endl;
        m.unlock();

        io_service->run();

        m.lock();
        cout << "[" << this_thread::get_id() << "] Worker thread ends" << endl;
        m.unlock();
    }
private:
    static mutex m;
};
mutex workerThread::m;


int main()
{
    const int workers_count = 2;
    const int end_port = 8080;
    try
    {
        shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
        boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io_service));
        boost::shared_ptr<boost::asio::io_service::strand> strand(new boost::asio::io_service::strand(*io_service));
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), end_port);

        shared_ptr<Server> server(new Server(*io_service, *strand, endpoint));

        boost::thread_group workers;
        for (int i = 0; i < workers_count; i++)
        {
            boost::thread* t = new boost::thread{ boost::bind(&workerThread::run, io_service) };
            workers.add_thread(t);
        }
        workers.join_all();
    }
    catch (const exception& ex)
    {
        cout << "Exception: " << ex.what() << endl;
    }

    return 0;
}
