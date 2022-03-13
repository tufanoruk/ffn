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
// This mimics a part of GIA test for number speed and accuracy
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
            // std::random_shuffle does not exist in MacOS C++17!
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
    

    class Score {
    public:
        Score(){};

        ~Score() {
            int i = 0;
            vector<string> q_and_a; 
            for  (auto q : questions_) {
                string s;
                for (auto r : q) {
                    s += std::to_string(r) + " ";  
                }
                s += " -> ";
                s += std::to_string(answers_[i++]);
                s += "\n";

                q_and_a.push_back (s);
            }
            for (auto s : q_and_a) {
                cout << s;
            }

        }
        
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
            cout << "answer " << a << " added" << endl;
            answers_.push_back(a);
        }
        
        int getNumberOfQuestions(){
            return questions_.size();
        }

        int getNumberOfCorrectAnswers(){            
            // if an answer is "0" then it is either timed out or unanswered
            int i = 0;
            int numCorrectAnswers = 0;
            for  (auto q : questions_) {
                if (findAnswer(q) == answers_[i++]) {
                    numCorrectAnswers++;
                }
            }
            return numCorrectAnswers; 
        }

        // return elapsed time in milliseconds
        int getEllapsedGameTime() {
            auto duration = chrono::duration_cast<chrono::milliseconds> (end_ - start_);
            return duration.count();
        }

        float getScore() {
            return (getNumberOfQuestions() - (getNumberOfQuestions() - getNumberOfCorrectAnswers())) / (getEllapsedGameTime() / 1000);
        }
        
    private:
        // work on the copy of the question (pass by value)
        int findAnswer (GameNumberGenerator::Question q) {
            std::sort(q.begin(), q.end());
            return ( (q[1] - q[0]) > (q[2] - q[1]) ? q[0] : q[2] );
         }

        chrono::time_point<chrono::steady_clock> start_;
        chrono::time_point<chrono::steady_clock> end_;

        vector<GameNumberGenerator::Question> questions_; 
        vector<int> answers_; 
    };


    enum {
        ANSWER_TIMEOUT = 4,
        BUF_LEN = 512
    };

public:
    FFNGame(int highestNum, boost::asio::io_context& ic) : ioContext_ (ic),
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

    int numQuestionsAsked() {return score_.getNumberOfQuestions();}
    int numCorrectAnswers() {return score_.getNumberOfCorrectAnswers();}
    int gameDuration() {return score_.getEllapsedGameTime();}
    float getScore() {return score_.getScore();}

private:
    void timeOut(const boost::system::error_code &ec)
    {
        if (!ec)
        {
            std::string msg{"\nYou didn't give your anwer in time. Asking a new one!\n\n"};

            score_.addGivenAnswer (0); // add no answer! 

            boost::asio::async_write(output_,
                                     boost::asio::buffer(msg.c_str(), msg.length()),
                                     boost::bind(&FFNGame::askQuestion, this,
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

            score_.addQuestion (q);

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

    void getAnswer(const boost::system::error_code &ec, std::size_t len)
    {
        if (!ec)
        {
            // get answer and ask question
            buffer_.fill('\0');
            // read data from stream_buffer upto \n into buffer  
            stream_buffer_.sgetn(buffer_.data(), len - 1);
            stream_buffer_.consume(1); // Remove newline from input, otherwise it will be read again!

            if (buffer_[0] == 'q' || buffer_[0] == 'Q') {
                ioContext_.stop();
                score_.endChronometer();
                return;
            }

            score_.addGivenAnswer (std::atoi(buffer_.data()));

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
    FFNGame ffn(max, std::ref(ioc));

    ioc.run(); 

    cout << "You quit the game____ " << endl;
    cout << "Questions asked.....: " << ffn.numQuestionsAsked() << endl;
    cout << "Correct answers.....: " << ffn.numCorrectAnswers() << endl;
    cout << "Total game duration.: " << ffn.gameDuration() << " milliseconds" << endl;

    cout << "Your score is " << ffn.getScore() << endl;

    return 0;
}
