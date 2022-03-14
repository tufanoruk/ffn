// FFNGame.h

#ifndef _FFNGAME_H_
#define _FFNGAME_H_

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

class FFNGame
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

        Question newQuestion();

    private:
        void shuffle (Question& q);
        
        int highestNumber_;
        std::random_device rdev_; // for seed
        std::mt19937 rngen_;
        Distribution dist_;  // distriburion of the generated numbers
    };
    

    class Score {
    public:
        Score(){};

        ~Score();
        
        void startChronometer() {
            start_ = chrono::high_resolution_clock::now();
        } 

        void endChronometer() {
            end_ = chrono::high_resolution_clock::now();
        } 

        void addQuestion (GameNumberGenerator::Question& q) {
            questions_.push_back(q);
        }

        void addGivenAnswer (int a){
            answers_.push_back(a);
        }
        
        void popLastQuestion() {
            questions_.pop_back();
        }

        int getNumberOfQuestions(){
            return questions_.size();
        }

        int getNumberOfCorrectAnswers();

        // return elapsed time in milliseconds
        int getEllapsedGameTime();

        float getScore();
        
    private:
        // work on the copy of the question (pass by value)
        int findAnswer (GameNumberGenerator::Question q) ;

        chrono::time_point<chrono::steady_clock> start_;
        chrono::time_point<chrono::steady_clock> end_;

        vector<GameNumberGenerator::Question> questions_; 
        vector<int> answers_; 
    };


    enum {
        ANSWER_TIMEOUT = 4 + 2, // seconds to type-in
        BUF_LEN = 512
    };

public:
    FFNGame(int highestNum, boost::asio::io_context& ic);

    int numQuestionsAsked() {return score_.getNumberOfQuestions();}
    int numCorrectAnswers() {return score_.getNumberOfCorrectAnswers();}
    int gameDuration() {return score_.getEllapsedGameTime();}
    float getScore() {return score_.getScore();}

private:
    void timeOut(const boost::system::error_code &ec);

    void askQuestion(const boost::system::error_code &ec);

    void getAnswer(const boost::system::error_code &ec, std::size_t len);
   
private:
    GameNumberGenerator numGen_;
    std::array<char, BUF_LEN> buffer_{0};
    boost::asio::io_context& ioContext_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::posix::stream_descriptor output_;
    boost::asio::streambuf stream_buffer_;
    boost::asio::deadline_timer timer_;

    Score score_;
};


#endif