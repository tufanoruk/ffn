
// FFNGame.cpp

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

#include "FFNGame.h"

using namespace std;


//
// FFNGame::GameNumberGenerator
//

FFNGame::GameNumberGenerator::Question FFNGame::GameNumberGenerator::newQuestion()
{
    Question q = {0, 0, 0};

    bool numbers_valid{false};
    do
    {
        q[0] = dist_(rngen_);
        q[1] = dist_(rngen_);
        q[2] = dist_(rngen_);
        std::sort(q.begin(), q.end());
        numbers_valid = (q[0] != q[1]) && (q[0] != q[2]) && (q[1] != q[2]) && ((q[1] - q[0]) != (q[2] - q[1]));
    } while (!numbers_valid);
    // std::random_shuffle does not exist in MacOS C++17!
    // std::random_shuffle(std::begin(q), std::end(q), dist_);
    shuffle(q);
    return q; // return the copy of the int[3] array
}

void FFNGame::GameNumberGenerator::shuffle(Question &q)
{
    std::srand(std::time(nullptr));
    Question::iterator first{q.begin()}, end{q.end()};
    for (Question::iterator i = first; i != end; i++)
    {
        int r = std::rand() % q.size();
        std::swap(*i, *(first + r));
    }
}


//
// FFNGame::Score
//

FFNGame::Score::~Score()
{
    int i = 0;
    vector<string> q_and_a;
    for (auto q : questions_)
    {
        string s;
        for (auto r : q)
        {
            s += std::to_string(r) + " ";
        }
        s += " -> ";
        s += std::to_string(answers_[i++]);
        s += "\n";

        q_and_a.push_back(s);
    }
    for (auto s : q_and_a)
    {
        cout << s;
    }
}

int FFNGame::Score::getNumberOfCorrectAnswers()
{
    // if an answer is "0" then it is either timed out or unanswered
    int i = 0;
    int numCorrectAnswers = 0;
    for (auto q : questions_)
    {
        if (findAnswer(q) == answers_[i++])
        {
            numCorrectAnswers++;
        }
    }
    return numCorrectAnswers;
}

// return elapsed time in milliseconds
int FFNGame::Score::getEllapsedGameTime()
{
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_ - start_);
    return duration.count();
}

float FFNGame::Score::getScore()
{
    if (getNumberOfCorrectAnswers() == 0)
        return 0.0;

    float correctness = getNumberOfQuestions() / getNumberOfCorrectAnswers();                                               // num/num -> dimentionless ratio
    float speed = (getNumberOfQuestions() * ANSWER_TIMEOUT * 1000) / (getEllapsedGameTime() / getNumberOfCorrectAnswers()); // ms/q / ms/q -> dimentionless ratio
    float score = 0.8 * correctness + 0.2 * speed;                                                                          // 80% correctness, 20% speed
    return 100 * score;
}

// work on the copy of the question (pass by value)
int FFNGame::Score::findAnswer(GameNumberGenerator::Question q)
{
    std::sort(q.begin(), q.end());
    return ((q[1] - q[0]) > (q[2] - q[1]) ? q[0] : q[2]);
}

//
// FFNGame
//

FFNGame::FFNGame(int highestNum, boost::asio::io_context &ic) : ioContext_(ic),
                                                                numGen_(highestNum),
                                                                input_(ic, ::dup(STDIN_FILENO)),
                                                                output_(ic, ::dup(STDOUT_FILENO)),
                                                                stream_buffer_(BUF_LEN),
                                                                timer_(ic),
                                                                score_()

{
    std::string msg{"Starting the game. Good luck!\n\n"};
    boost::asio::async_write(output_,
                             boost::asio::buffer(msg.c_str(), msg.length()),
                             boost::bind(&FFNGame::askQuestion, this,
                                         boost::asio::placeholders::error));
    score_.startChronometer();
}

void FFNGame::timeOut(const boost::system::error_code &ec)
{
    if (!ec)
    {
        std::string msg{"\nYou didn't give your anwer in time. Asking a new one!\n\n"};

        score_.addGivenAnswer(0); // add no answer!

        boost::asio::async_write(output_,
                                 boost::asio::buffer(msg.c_str(), msg.length()),
                                 boost::bind(&FFNGame::askQuestion, this,
                                             boost::asio::placeholders::error));
        timer_.cancel();
    }
}

void FFNGame::askQuestion(const boost::system::error_code &ec)
{
    if (!ec)
    {
        // ask question
        GameNumberGenerator::Question q = numGen_.newQuestion();
        cout << "\nNumbers : " << q[0] << " " << q[1] << " " << q[2];
        cout << "\nAnswer (q to quit): ";
        cout.flush();

        score_.addQuestion(q);

        // get answer
        boost::asio::async_read_until(input_, stream_buffer_, '\n',
                                      boost::bind(&FFNGame::getAnswer, this,
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
        int timeout = ANSWER_TIMEOUT;
        timer_.expires_from_now(boost::posix_time::seconds(timeout));
        timer_.async_wait(boost::bind(&FFNGame::timeOut, this,
                                      boost::asio::placeholders::error));
    }
}

void FFNGame::getAnswer(const boost::system::error_code &ec, std::size_t len)
{
    if (!ec)
    {
        // get answer and ask question
        buffer_.fill('\0');
        // read data from stream_buffer upto \n into buffer
        stream_buffer_.sgetn(buffer_.data(), len - 1);
        stream_buffer_.consume(1); // Remove newline from input, otherwise it will be read again!

        if (buffer_[0] == 'q' || buffer_[0] == 'Q')
        {
            ioContext_.stop();
            score_.endChronometer();
            // game over, last question cannot be answered, pop it back
            score_.popLastQuestion();
            return;
        }

        score_.addGivenAnswer(std::atoi(buffer_.data()));

        static char prefix[] = {"Your answer is '"};
        static char suffix[] = {"'\n"};
        boost::array<boost::asio::const_buffer, 3> buffers = {{boost::asio::buffer(prefix),
                                                               boost::asio::buffer(buffer_.data(), std::strlen(buffer_.data())),
                                                               boost::asio::buffer(suffix)}};

        boost::asio::async_write(output_, buffers,
                                 boost::bind(&FFNGame::askQuestion, this,
                                             boost::asio::placeholders::error));
    }
}
