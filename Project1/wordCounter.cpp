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
    std::atomic<short> value;  // Make 'value' atomic
    HashNode *next;

    //rather than making a copy of a copy, move the copy to the parameter to reduce memory usage
    HashNode(string key, short value) : key(std::move(key)), value(value), next(nullptr) {}
};

class HashMap {
private:
    float threshold = 0.75;
    short maxSize;
    atomic<short> currentSize{0};  // Make 'currentSize' atomic and directly initialize
    mutex mutex;
    std::mutex* bucketMutexes;


    int hashFunction(const string &key) const {
        const short p = 31;  // Prime number used as base
        const int m = 1e9 + 9;  // Large prime number for modulo operation
        long hashValue = 0;
        long p_pow = 1;

        for (char c: key) {
            hashValue = (hashValue + (c - 'a' + 1) * p_pow) % m;
            p_pow = (p_pow * p) % m;
        }

        long index = hashValue % m; // Calculate modulo with m
        if (index < 0) {
            index += m; // Ensure index is positive
        }
        return static_cast<int>(index % tableSize);
    }


public:
    HashNode **table;
    int tableSize;

    explicit HashMap(int size) {
        tableSize = size;
        maxSize = (short) (tableSize * threshold);
        table = new HashNode*[tableSize];
        bucketMutexes = new std::mutex[tableSize];  // Allocate array of mutexes


        //initialize all fields to null
        fill(table, table + tableSize, nullptr);
    }
    // Destructor to prevent memory leaks
    ~HashMap() {
        for (int i = 0; i < tableSize; ++i) {
            HashNode *node = table[i];
            while (node != nullptr) {
                HashNode *temp = node;
                node = node->next;
                //free(temp);
                delete(temp);

            }
        }
        delete[] table;
        delete[] bucketMutexes;

    }

    void rehashInsert(const string &key, short count) {
        int index = hashFunction(key);
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

    void insert(const string &key, const int &loc) {
        int index = loc;
        if(!key.empty()) {
            cout << "Word to insert: " << key << endl;
            cout << "Loc: " << index << endl;
            //see if a resize is needed
            if (currentSize >= maxSize) {
                std::lock_guard<std::mutex> lock(mutex);
                resize();
                index = hashFunction(key);
                cout << "resized index: " << index << endl;
            }
            std::lock_guard<std::mutex> lock(bucketMutexes[index]);
            short count = get(key) + 1;
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
            HashNode *newNode = new HashNode(key, ++count);

            if (previousNode == nullptr) {
                table[index] = newNode;
            } else {

                previousNode->next = newNode;
            }
            ++currentSize;
        }
    }

    void insertWords(const std::string& words) {
        size_t start = 0;
        size_t end = words.find(' ');
        int index;

        while (end != std::string::npos) {
            std::string word = words.substr(start, end - start);
            index = hashFunction(word);
            insert(word, index);

            // Update start and end for the next word
            start = end + 1;
            end = words.find(' ', start);
        }

        // Insert the last word (or the only word if there are no spaces)
        std::string lastWord = words.substr(start);
        index = hashFunction(lastWord);
        insert(lastWord, index);
    }


    short get(const string &key) {

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
        cout << "resizing!" << endl;
        int oldTableSize = tableSize;
        HashNode **oldTable = table;
        tableSize *= 2;
        maxSize = (short) (tableSize * threshold);
        table = new HashNode*[tableSize];
        std::mutex* oldBucket = bucketMutexes;
        bucketMutexes = new std::mutex[tableSize];

        fill(table, table + tableSize, nullptr);

        for (int i = 0; i < oldTableSize; ++i) {
            HashNode *oldNode = oldTable[i];
            while (oldNode != nullptr) {
                rehashInsert(oldNode->key, oldNode->value);
                HashNode *temp = oldNode;
                oldNode = oldNode->next;
                delete(temp);

            }
        }
        delete[] oldTable;
        delete[] oldBucket;
    }
};

class WordCount {
public:
    string word;
    short count;
    WordCount* words[];

    WordCount() : word(""), count(0) {}
    explicit WordCount(std::string  w, int c) : word(std::move(w)), count(c) {}

};

string normalizeWord(const string &word) {
    string normalized;
    for (char ch: word) {
        if (isalpha(ch) || ch == '-') {
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

void swap(WordCount* a, WordCount* b) {
    WordCount t = *a;
    *a = *b;
    *b = t;
}

int partition(WordCount* arr, int low, int high) {
    WordCount pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        // If current element is larger than or equal to the pivot
        if (arr[j].count >= pivot.count) {
            i++;    // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quickSort(WordCount* arr, int low, int high) {
    if (low < high) {
        int pivotIndex = partition(arr, low, high);

        // Recursively apply the same logic to the left and right subarrays
        quickSort(arr, low, pivotIndex - 1);
        quickSort(arr, pivotIndex + 1, high);
    }
}

void outputHashMap(HashMap& hashMap, const string& filename) {
    int totalWords = countWords(hashMap.table, hashMap.tableSize);
    WordCount* wordCounts = new WordCount[totalWords];

    int index = 0;

    for (int i = 0; i < hashMap.tableSize; ++i) {
        HashNode* node = hashMap.table[i];
        while (node != nullptr) {
            wordCounts[index++] = WordCount(node->key, node->value);
            node = node->next;
        }
    }

    // Corrected call to quickSort
    quickSort(wordCounts, 0, totalWords - 1);

    // Output to file
    ofstream outFile(filename);
    for (int i = 0; i < totalWords; ++i) {
        outFile << wordCounts[i].word << ": " << wordCounts[i].count << endl;
    }

    outFile.close();
    delete[] wordCounts; // Cleanup

}

long getFileLength(ifstream& file) {
    if (!file) {
        // Handle error, such as file not opening correctly
        return -1;
    }
    std::streampos currentPos = file.tellg(); // save current position
    file.seekg(0, std::ios::end); // seek end

    long length = file.tellg();

    file.seekg(currentPos); // Restore the file pointer to its original position
    return length; // Return the length of the file
}

void dispatchThreads(int numThreads, ifstream& file, HashMap &wordCount) {
    thread* threads = new thread[numThreads];
    long leng = getFileLength(file);
    string word;
    bool firstWord = true;
    for(int i = numThreads; i > 0; i--) {
        int count = 0;
        int chunkSize = leng / i;
        long end = (long)file.tellg() + chunkSize;
        string segment;
        cout << "Thread #" << i << " ";
        cout << "starting loc: " << file.tellg() << " ";
        while (file >> word && (long)file.tellg() < end) {
            if (!word.empty() && isalpha(word[0])) {
                if (!firstWord) {
                    segment += " ";  // Add a space before all but the first word
                }
                firstWord = false;
                segment += normalizeWord(word);
            }
        }
        cout << "ending loc: " << file.tellg() << endl;
        threads[i - 1] = thread([&wordCount, segment]() {
            wordCount.insertWords(segment);
        });

        leng -= chunkSize;
    }

    // Wait for all threads to finish
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
    delete[] threads; // Cleanup
}


int main() {
    HashMap wordCount(100); // Start with an initial size
    string fileName = "input.txt";
    ifstream inputFile(fileName);

    if (!inputFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }
    dispatchThreads(17, inputFile, wordCount);

    outputHashMap(wordCount, "output.txt");

    return 0;
}