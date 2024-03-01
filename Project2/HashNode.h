#ifndef PARALLELPROCESSING_HASHNODE_H
#define PARALLELPROCESSING_HASHNODE_H

#include <iostream>

using namespace std;

class HashNode {
public:
    string key;
    long value;
    HashNode* next;

    //rather than making a copy of a copy, move the copy to the parameter to reduce memory usage
    HashNode(string key, long value) : key(std::move(key)), value(value), next(nullptr) {}
};

#endif //PARALLELPROCESSING_HASHNODE_H
