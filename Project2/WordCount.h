#ifndef PARALLELPROCESSING_WORDCOUNT_H
#define PARALLELPROCESSING_WORDCOUNT_H

#include <iostream>

using namespace std;

class WordCount {
public:
    string word;
    long count;

    WordCount() : word(""), count(0) {}
    explicit WordCount(std::string  w, int c) : word(std::move(w)), count(c) {}
};

#endif //PARALLELPROCESSING_WORDCOUNT_H
