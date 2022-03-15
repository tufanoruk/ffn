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

#include "FFNGame.h"

using namespace std;

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
