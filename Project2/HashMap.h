#ifndef PARALLELPROCESSING_HASHMAP_H
#define PARALLELPROCESSING_HASHMAP_H

#include "HashNode.h"
#include <string>

class HashMap {
public:
    HashNode** table;
    unsigned long tableSize;
    unsigned long hashFunction(const std::string& key) const;

    explicit HashMap(unsigned long size);
    ~HashMap();
    void insert(const std::string& key) const;
    void insertWords(const std::string& words) const;
};

#endif //PARALLELPROCESSING_HASHMAP_H
