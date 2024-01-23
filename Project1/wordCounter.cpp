#include <thread> 
#include <atomic> 
#include <semaphore> 
#include <chrono> 
#include <iostream> 
#include <mutex>
#include <fstream>

class HashNode {
public:
    std::string key;
    int value;
    HashNode* next;

    HashNode(std::string key, int value) : key(key), value(value), next(nullptr) {}
};

class HashMap {
private:
    HashNode** table;
    int tableSize;
    int threshold = 0.75f;
    int maxSize;
    int currentSize = 0;
    int hashFunction(const std::string& key) {
        const int p = 31;  // Prime number used as base
        const int m = 1e9 + 9;  // Large prime number for modulo operation
        long long hashValue = 0;
        long long p_pow = 1;

        for (char c : key) {
            hashValue = (hashValue + (c - 'a' + 1) * p_pow) % m;
            p_pow = (p_pow * p) % m;
        }

        return hashValue % tableSize;  // Ensure it fits into the table
    }


public:
    HashMap(int size) {
        tableSize = size;
        maxSize = (int) (tableSize * threshold);
        table = new HashNode*[tableSize];
        //initialize all fields to null
        for(int i = 0; i < tableSize; ++i) {
            table[i] = nullptr;
        }
     }

    void insert(std::string key, int count) {
        int index = hashFunction(key);
        HashNode* currentNode = table[index];
        HashNode* previousNode = nullptr;
        if(currentSize <= maxSize)
        {
            while (currentNode != nullptr) {
                if (currentNode->key == key) {
                    currentNode->value = count;
                    return; 
                }
                previousNode = currentNode;
                currentNode = currentNode->next;
            }
            //node not found in map already, must make new node
            HashNode* newNode = new HashNode(key, count);;
            if(previousNode == nullptr) {
                previousNode = newNode;
            }
            else 
            {
                previousNode->next = newNode;
            }
            ++currentSize;
        }
        else {
            resize(key, count);
        }
    }

    int get(std::string key) {
        // Compute the hash code and find the corresponding bucket index
        int index = hashFunction(key) % tableSize;
        
        // Search for the key in the linked list at the computed index
        HashNode* node = table[index];
        while (node != nullptr) {
            if (node->key == key) {
                return node->value;  // Key found, return value
            }
            node = node->next;
        }

        // Key not found, return a default value
        return -1;
    }

    int resize(std::string key, int count) {
        int oldTableSize = tableSize;
        HashNode** oldTable = table;
        tableSize *= 2;
        maxSize = (int) (tableSize * threshold);
        table = new HashNode* [tableSize];
        for(int i = 0; i < oldTableSize; ++i) {
            HashNode* currentNode = table[i];
            HashNode* previousNode = nullptr;
            if(currentNode == nullptr)
            {
                table[i] = nullptr;
            }
            else {
                table[i] = currentNode;
                while (currentNode != nullptr) {
                    previousNode = currentNode;
                    currentNode = currentNode->next;
                    previousNode->next = currentNode;
                }
            }      
        }
        for(int j = oldTableSize; j < tableSize; ++j) {
            table[j] = nullptr;
        }
        insert(key, count);
    }
    // Additional functions like resize, remove, etc.
};

std::string normalizeWord(const std::string& word) {
    std::string normalized;
    for (char ch : word) {
        if (std::isalpha(ch)) {
            normalized += std::tolower(ch);
        }
    }
    return normalized;
}

int main() {
    HashMap wordCount(1000); // Start with an initial size
    std::ifstream inputFile("input.txt");
    std::string word;

    if (!inputFile) {
        std::cerr << "Error opening input file." << std::endl;
        return 1;
    }

    while (inputFile >> word) {
        word = normalizeWord(word);
        if (!word.empty()) {
            int currentCount = wordCount.get(word);
            wordCount.insert(word, currentCount + 1);
        }
    }

    inputFile.close();

    // Extract and sort the words by occurrences
    // ...

    // Output results
    // ...

    return 0;
}