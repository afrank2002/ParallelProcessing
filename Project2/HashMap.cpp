
#include "HashMap.h"

//Constructor
HashMap::HashMap(unsigned long size) {
    tableSize = size;
    table = new HashNode*[tableSize];
    std::fill(table, table + tableSize, nullptr); // Ensure you include <algorithm>
}

// Destructor
HashMap::~HashMap() {
    for (unsigned long i = 0; i < tableSize; ++i) {
        HashNode* node = table[i];
        while (node != nullptr) {
            HashNode* temp = node;
            node = node->next;
            delete temp;
        }
    }
    delete[] table;
}

// Hash Function
unsigned long HashMap::hashFunction(const std::string& key) const {
    const unsigned long fnv_prime = 0x811C9DC5;
    unsigned long hash = 0;
    for (char c : key) {
        hash ^= c;
        hash *= fnv_prime;
    }
    return hash % tableSize;
}


    void HashMap::insert(const string& key) const {
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

    void HashMap::insertWords(const std::string& words) const {
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



