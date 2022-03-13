// Find Furthest Number game
//
// diplay 3 number to the player within given range 
// player need to enter the number which is the furthest from middle number within 4sec
//
// "This test measures your speed and accuracy in carrying out number tasks in your head. For each problem 
// presented, start by finding the highest and the lowest of the three numbers displayed.  
// Having identified those, decide whether the highest number or the lowest number is numerically further away 
// from the remaining number.""
//
// Example
//    9 3 7
// 7 is the middle number
// 3 is furthest from 7
//
// This mimics a part of GIA test fır number speed and accuracy
//


#include <iostream>
#include <array>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <functional>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind/bind.hpp>

using namespace std;

class FFTGame
{
    class GameNumberGenerator
    {
    public:
        static const int _lowestNumber = 1;

        typedef array<int, 3> Question;
        typedef uniform_int_distribution<int> Distribution;

        GameNumberGenerator(int hn) : highestNumber_(hn),
                                      rdev_(),
                                      rngen_(rdev_()),
                                      dist_(1, hn) {}

        Question newQuestion()
        {
            Question q = {0, 0, 0};

            bool numbers_valid{false};
            do {
                q[0] = dist_(rngen_);
                q[1] = dist_(rngen_);
                q[2] = dist_(rngen_);
                std::sort(q.begin(), q.end());
                numbers_valid = (q[0] != q[1]) && (q[0] != q[2]) && (q[1] != q[2]) && ((q[1] - q[0]) != (q[2] - q[1])); 
            } while (!numbers_valid);
            // std::random_shuffle does not exist in MacOs C++17!
            //std::random_shuffle(std::begin(q), std::end(q), dist_);
            shuffle(q);
            return q; // return the copy of the int[3] array
        }

    private:

        void shuffle (Question& q) 
        {
            std::srand(std::time(nullptr));
            Question::iterator first{q.begin()}, end{q.end()};
            for (Question::iterator i = first; i != end; i++) {
                int r = std::rand() % q.size();        
                std::swap(*i, *(first+r));
            }
        }
        
        int highestNumber_;
        std::random_device rdev_; // for seed
        std::mt19937 rngen_;
        Distribution dist_;  // distriburion of the generated numbers
    };
    
    enum {
        ANSWER_TIMEOUT = 4,
        BUF_LEN = 512
    };

public:
    FFTGame(int highestNum, boost::asio::io_context& ic) : ioContext_ (ic),
                                                           numGen_(highestNum), 
                                                           input_(ic, ::dup(STDIN_FILENO)),
                                                           output_(ic, ::dup(STDOUT_FILENO)),
                                                           stream_buffer_(BUF_LEN),
                                                           timer_(ic)
    {
        std::string msg{"Starting the game. Good luck!\n\n"};
        boost::asio::async_write(output_,
                                 boost::asio::buffer(msg.c_str(), msg.length()),
                                 boost::bind(&FFTGame::askQuestion, this,
                                             boost::asio::placeholders::error));
    }

private:
    void timeOut(const boost::system::error_code &ec)
    {
        if (!ec)
        {
            std::string msg{"\nYou didn't give your anwer in time. Asking a new one!\n\n"};

            boost::asio::async_write(output_,
                                     boost::asio::buffer(msg.c_str(), msg.length()),
                                     boost::bind(&FFTGame::askQuestion, this,
                                                 boost::asio::placeholders::error));
            timer_.cancel();
        }
    }

    void askQuestion(const boost::system::error_code &ec)
    {
        if (!ec)
        {
            // ask question
            GameNumberGenerator::Question q = numGen_.newQuestion();
            cout << "\nNumbers : " << q[0] << " " << q[1] << " " << q[2];
            cout << "\nAnswer (q to quit): "; 
            cout.flush();

            // get answer
            boost::asio::async_read_until(input_, stream_buffer_, '\n',
                                          boost::bind(&FFTGame::getAnswer, this,
                                                      boost::asio::placeholders::error,
                                                      boost::asio::placeholders::bytes_transferred));
            int timeout = ANSWER_TIMEOUT;
            timer_.expires_from_now(boost::posix_time::seconds(timeout));
            timer_.async_wait(boost::bind(&FFTGame::timeOut, this,
                                          boost::asio::placeholders::error));
        }
    }

    void getAnswer(const boost::system::error_code &ec, std::size_t len)
    {
        if (!ec)
        {
            // get answer and ask question
            buffer_.fill('\0');
            // read data from stream_buffer into buffer
            stream_buffer_.sgetn(buffer_.data(), len - 1);
            stream_buffer_.consume(1); // Remove newline from input, otherwise it will be read!

            if (buffer_[0] == 'q' || buffer_[0] == 'Q') {
                ioContext_.stop();
                return;
            }

            //cout << "Your answer is " << "'" << buffer_.data() << "'" << endl;

            static char prefix[] = {"Your answer is '"};
            static char suffix[] = {"'\n"};
            boost::array<boost::asio::const_buffer, 3> buffers = {{boost::asio::buffer(prefix),
                                                                   boost::asio::buffer(buffer_.data(), std::strlen(buffer_.data())),
                                                                   boost::asio::buffer(suffix)}};

            boost::asio::async_write(output_, buffers,
                                     boost::bind(&FFTGame::askQuestion, this,
                                                 boost::asio::placeholders::error));
        }
    }

private:
    GameNumberGenerator numGen_;
    std::array<char, BUF_LEN> buffer_{0};
    boost::asio::io_context& ioContext_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::posix::stream_descriptor output_;
    boost::asio::streambuf stream_buffer_;
    boost::asio::deadline_timer timer_;
};

// make sure the user has entered integer input and return it
int read_number()
{

    int value{0};

    while (!(cin >> value))
    {
        cin.clear();
        string line;
        getline(cin, line);
        cout << line << "\nI am sorry, but '" << line << "' is not a number\n";
    }
    return value;
}

int read_answer()
{
    int answer{0};

    cout << "Enter the answer : " << endl;

    chrono::time_point<chrono::high_resolution_clock> before = chrono::high_resolution_clock::now();
    answer = read_number();
    chrono::time_point<chrono::high_resolution_clock> after = chrono::high_resolution_clock::now();

    chrono::duration<double> duration = after - before;

    cout << "It took " << duration.count() << " sec to get the input" << endl;

    return answer;
}

int main(int, char**) 
{   
    int max {0};   
    cout << "Enter the max value for the numbers in the game." << endl;
    max = read_number();

    if (max < 4 || max > 100) {
        cout << "Max numnbr must be [4 - 100]" << endl;
        return EXIT_FAILURE;
    }

    boost::asio::io_context ioc;
    FFTGame fft(max, std::ref(ioc));

    ioc.run();    

    return 0;
}
