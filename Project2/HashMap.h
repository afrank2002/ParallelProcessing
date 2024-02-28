#ifndef PARALLELPROCESSING_HASHMAP_H
#define PARALLELPROCESSING_HASHMAP_H

#include "HashNode.h"

class HashMap {

public:
    HashNode** table;
    unsigned long tableSize;

    explicit HashMap(unsigned long size) {
        tableSize = size;
        table = new HashNode * [tableSize];


        //initialize all fields to null
        fill(table, table + tableSize, nullptr);
    }
    // Destructor to prevent memory leaks
    ~HashMap() {
        for (int i = 0; i < tableSize; ++i) {
            HashNode* node = table[i];
            while (node != nullptr) {
                HashNode* temp = node;
                node = node->next;
                delete(temp);
            }
        }
        delete[] table;
    }

    //FNV-1a Hash Function
    unsigned long hashFunction(const string& key) const {
        const unsigned long fnv_prime = 0x811C9DC5;
        unsigned long hash = 0;
        for (char c : key) {
            hash ^= c;
            hash *= fnv_prime;
        }
        return hash % tableSize;
    }


    void insert(const string& key) const {
        if (key.empty()) return; // Early exit if the key is empty

        unsigned long index = hashFunction(key);

        // Directly access this HashMap's table
        HashNode** slot = &table[index];
        int i = 0;
        for (HashNode* currentNode = *slot; currentNode; currentNode = currentNode->next) {
            i++;
            if (currentNode->key == key) {
                currentNode->value++;
                return;
            }
        }

        // Node not found, create a new node and link it
        auto* newNode = new HashNode(key, 1);
        newNode->next = *slot;
        *slot = newNode;

        // Increase the size. This is safe since the HashMap is thread-specific.
    }

    void insertWords(const std::string& words) const {
        size_t start = 0;
        size_t end = words.find(' ');
        int index;

        while (end != std::string::npos) {
            std::string word = words.substr(start, end - start);
            this->insert(word);

            // Update start and end for the next word
            start = end + 1;
            end = words.find(' ', start);
        }

        // Insert the last word (or the only word if there are no spaces)
        std::string lastWord = words.substr(start);
        this->insert(lastWord);
    }


    long get(const string& key) {

        // Compute the hash code and find the corresponding bucket index
        unsigned long index = hashFunction(key) % tableSize;

        // Search for the key in the linked list at the computed index
        HashNode* node = table[index];
        while (node) {
            if (node->key == key) {
                return node->value;  // Key found, return value
            }
            node = node->next;
        }
        // Key not found, return a default value
        return -1;
    }

};

#endif //PARALLELPROCESSING_HASHMAP_H
