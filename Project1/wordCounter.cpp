#include <thread>
#include <atomic>
#include <semaphore>
#include <chrono>
#include <iostream>
#include <mutex>
#include <fstream>

using namespace std;

class HashNode {
public:
    string key;
    short value;
    HashNode *next;

    HashNode(string key, short value) : key(key), value(value), next(nullptr) {}
};

class HashMap {
private:
    float threshold = 0.75;
    short maxSize;
    short currentSize = 0;

    short hashFunction(const string &key) const {
        const short p = 31;  // Prime number used as base
        const int m = 1e9 + 9;  // Large prime number for modulo operation
        long long hashValue = 0;
        long long p_pow = 1;

        for (char c: key) {
            hashValue = (hashValue + (c - 'a' + 1) * p_pow) % m;
            p_pow = (p_pow * p) % m;
        }

        return (short) (hashValue % tableSize);  // Ensure it fits into the table
    }


public:
    HashNode **table;
    int tableSize;

    explicit HashMap(int size) {
        tableSize = size;
        maxSize = (short) (tableSize * threshold);
        table = new HashNode *[tableSize];
        //initialize all fields to null
        fill(table, table + tableSize, nullptr);
    }
    void rehashInsert(const string &key, short count) {
        short index = hashFunction(key);
        HashNode *currentNode = table[index];
        HashNode *previousNode = nullptr;

        while (currentNode != nullptr) {
            if (currentNode->key == key) {
                currentNode->value = count;
                return;
            }
            previousNode = currentNode;
            currentNode = currentNode->next;
        }

        HashNode *newNode = new HashNode(key, count);
        if (previousNode == nullptr) {
            table[index] = newNode;
        } else {
            previousNode->next = newNode;
        }
    }

    void insert(const string &key, short count) {
        //see if a resize is needed
        if (currentSize >= maxSize) {
            resize();
        }

        short index = hashFunction(key);
        HashNode *currentNode = table[index];
        HashNode *previousNode = nullptr;

        while (currentNode != nullptr) {
            if (currentNode->key == key) {
                currentNode->value = count;
                return;
            }
            previousNode = currentNode;
            currentNode = currentNode->next;
        }

        //node not found in map already, must make new node
        HashNode *newNode = new HashNode(key, count);;
        if (previousNode == nullptr) {
            table[index] = newNode;
        } else {
            previousNode->next = newNode;
        }
        ++currentSize;
    }

    int get(const string &key) {
        // Compute the hash code and find the corresponding bucket index
        int index = hashFunction(key) % tableSize;

        // Search for the key in the linked list at the computed index
        HashNode *node = table[index];
        while (node != nullptr) {
            if (node->key == key) {
                return node->value;  // Key found, return value
            }
            node = node->next;
        }

        // Key not found, return a default value
        return -1;
    }

    void resize() {
        int oldTableSize = tableSize;
        HashNode **oldTable = table;
        tableSize *= 2;
        maxSize = (short) (tableSize * threshold);
        table = new HashNode *[tableSize];
        fill(table, table + tableSize, nullptr);

        for (int i = 0; i < oldTableSize; ++i) {
            HashNode *oldNode = oldTable[i];
            while (oldNode != nullptr) {
                rehashInsert(oldNode->key, oldNode->value);
                HashNode *temp = oldNode;
                oldNode = oldNode->next;
                delete temp;
            }
        }

        delete[] oldTable;
    }

    // Additional functions like resize, remove, etc.
};

class WordCount {
public:
    string word;
    short count;
    WordCount* words[];

    WordCount() : word(""), count(0) {}
    explicit WordCount(const std::string& w, int c) : word(w), count(c) {}

};

string normalizeWord(const string &word) {
    string normalized;
    for (char ch: word) {
        if (isalpha(ch)) {
            normalized += tolower(ch);
        }
    }
    return normalized;
}

int countWords(HashNode **table, int tableSize) {
    int count = 0;
    for(int i = 0; i < tableSize; ++i) {
        for(HashNode* node = table[i]; node != nullptr; node = node->next) {
        ++count;
        }
    }
    return count;
}
//used for quick sort
int compareWordCount(const void* a, const void* b) {
    const WordCount *wordA = static_cast<const WordCount*>(a);
    const WordCount *wordB = static_cast<const WordCount*>(b);
    return wordB->count - wordA->count; // Descending order
}

/*int wordArray(HashNode **table, int tableSize) {
    for(int i = 0; i < tableSize; ++i) {
        HashNode* word = table[i];

        while (word != nullptr) {
            WordCount* newWord = new WordCount(word->key, word->value);
            word = word->next;
        }

    }
}*/

void outputHashMap(HashMap& hashMap, const string& filename) {
    int totalWords = countWords(hashMap.table, hashMap.tableSize); // Assuming countWords function is implemented
    WordCount* wordCounts = new WordCount[totalWords];

    int index = 0;
    for (int i = 0; i < hashMap.tableSize; ++i) {
        HashNode* node = hashMap.table[i];
        while (node != nullptr) {
            wordCounts[index++] = WordCount(node->key, node->value);
            node = node->next;
        }
    }

    // Sort the word counts
    qsort(wordCounts, totalWords, sizeof(WordCount), compareWordCount);

    // Output to file
    ofstream outFile(filename);
    for (int i = 0; i < totalWords; ++i) {
        outFile << wordCounts[i].word << ": " << wordCounts[i].count << endl;
    }

    outFile.close();
    delete[] wordCounts; // Cleanup
}


int main() {
    HashMap wordCount(1000); // Start with an initial size
    ifstream inputFile("input.txt");
    string word;

    if (!inputFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }

    while (inputFile >> word) {
        word = normalizeWord(word);
        if (!word.empty()) {
            short currentCount = wordCount.get(word);
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