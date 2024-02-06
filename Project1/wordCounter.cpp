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
    std::atomic<long> value;  // Make 'value' atomic
    HashNode *next;

    //rather than making a copy of a copy, move the copy to the parameter to reduce memory usage
    HashNode(string key, long value) : key(std::move(key)), value(value), next(nullptr) {}
};

class HashMap {
private:
    float threshold = 0.8;
    long maxSize;
    atomic<long> currentSize{0};  // Make 'currentSize' atomic and directly initialize
    mutex globalMutex;
    std::mutex* bucketMutexes;
    int segmentSize = 1000;
    int mutexCount;



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
    size_t getSegmentIndex(size_t bucketIndex) const {
        return bucketIndex / segmentSize;
    }


public:
    HashNode **table;
    int tableSize;

    explicit HashMap(int size) {
        tableSize = size;
        maxSize = (long) (tableSize * threshold);
        table = new HashNode*[tableSize];
        mutexCount = (tableSize + segmentSize - 1) / segmentSize; // Ceiling division
        bucketMutexes = new std::mutex[mutexCount];  // Allocate array of mutexes


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

    void rehashInsert(HashNode* node) {
        int index = hashFunction(node->key);
        node->next = table[index]; // Directly link the existing node to the start of the list
        table[index] = node; // Make the node the new head of the list at index
    }
    void resize() {
        int newTableSize = tableSize * 2;
        auto newTable = new HashNode*[newTableSize]();
        int newMutexCount = (newTableSize + segmentSize - 1) / segmentSize;
        auto newBucketMutexes = new std::mutex[newMutexCount];
        std::fill(newTable, newTable + newTableSize, nullptr);

        // Temporarily save the old table information
        HashNode** oldTable = table;
        int oldTableSize = tableSize;

        std::mutex* oldBucket = bucketMutexes;
        bucketMutexes = new std::mutex[tableSize];

        // Update the hash map to use the new table
        table = newTable;
        tableSize = newTableSize;
        maxSize = static_cast<long>(threshold * tableSize);

        // Iterate through the old table and rehash each node
        for (int i = 0; i < oldTableSize; ++i) {
            HashNode* currentNode = oldTable[i];
            while (currentNode != nullptr) {
                HashNode* nextNode = currentNode->next; // Temporarily save the next node
                currentNode->next = nullptr; // Disconnect the current node from the list
                rehashInsert(currentNode); // Rehash the current node into the new table
                currentNode = nextNode; // Move to the next node in the old list
            }
        }

        delete[] oldTable;
        delete[] bucketMutexes; // Delete the old mutex array
        bucketMutexes = newBucketMutexes; // Use the new mutex array
    }


    void insert(const string &key, const int &loc) {
        if (key.empty()) return; // Early exit if the key is empty

        int index = loc;

        // First check to potentially avoid locking
        if (currentSize.load(std::memory_order_relaxed) >= maxSize) {
            std::lock_guard<std::mutex> lock(globalMutex);
            if (currentSize.load(std::memory_order_relaxed) >= maxSize) { // Double-check pattern
                resize(); // This may be optimized to avoid resizing too often
                index = hashFunction(key); // Only rehash if resize occurred
            }
        }

        size_t segmentIndex = getSegmentIndex(index);
       // cout << "Word: " << key << " Index: " << index << endl;

        // cout << "Segment index: " << segmentIndex << endl;
        {
            std::lock_guard<std::mutex> lock(bucketMutexes[segmentIndex]);
            HashNode *&slot = table[index];
            for (HashNode *currentNode = slot; currentNode; currentNode = currentNode->next) {
                if (currentNode->key == key) {
                    currentNode->value.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            }

            // Node not found, create a new node and link it
            HashNode *newNode = new HashNode(key, 1);
            newNode->next = slot;
            slot = newNode;
        }

        // Increase the size after successfully adding a new node
        currentSize.fetch_add(1, std::memory_order_relaxed);
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


    long get(const string &key) {

        // Compute the hash code and find the corresponding bucket index
        int index = hashFunction(key) % tableSize;

        // Search for the key in the linked list at the computed index
        HashNode *node = table[index];
        while (node) {
            if (node->key.compare(key) == 0) {
                return node->value.load();  // Key found, return value
            }
            node = node->next;
        }
        // Key not found, return a default value
        return -1;
    }

};

class WordCount {
public:
    string word;
    long count;
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

void dispatchThreads(int numThreads, const string& fileName, HashMap &wordCount) {
    if (numThreads == 1) {
        // Perform the operation in the main thread without creating new threads
        ifstream file(fileName);
        string word;
        string segment;
        while (file >> word) {
            segment += normalizeWord(word) + " ";
        }
        wordCount.insertWords(segment);
    } else {
        numThreads--;
        thread *threads = new thread[numThreads];
        ifstream file(fileName); // Open the file to get its length
        long length = getFileLength(file);
        file.close(); // Close the initial file stream

        long chunkSize = length / numThreads; // Basic chunk size for each thread

        for (int i = 0; i < numThreads; i++) {
            // Calculate start and end positions for each thread
            long startPos = i * chunkSize;
            long endPos = (i < numThreads - 1) ? (startPos + chunkSize) : length;

            threads[i] = thread([=, &wordCount]() {
                ifstream threadFile(fileName); // Open a new file stream for each thread
                threadFile.seekg(startPos); // Seek to the start position

                string word;
                string segment;
                bool firstWord = true;
                long currentPosition = startPos;

                // Read until the end position or the end of the file
                while (currentPosition < endPos && threadFile >> word) {
                    if (!word.empty() && isalpha(word[0])) {
                        if (!firstWord) {
                            segment += " "; // Add a space before all but the first word
                        }
                        firstWord = false;
                        segment += normalizeWord(word);
                    }
                    currentPosition = threadFile.tellg();
                }

                // If not at the end of the file, read the rest of the current word
                if (currentPosition < length) {
                    char c;
                    while (threadFile.get(c) && isalpha(c)) {
                        segment += tolower(c);
                    }
                }

                wordCount.insertWords(segment); // Process the segment
            });
        }

        // Wait for all threads to finish
        for (int i = 0; i < numThreads; i++) {
            threads[i].join();
        }

        delete[] threads; // Clean up the dynamically allocated thread array
    }
}

int main() {
    HashMap wordCount(10000); // Start with an initial size
    string fileName = "Bible.txt";
    ifstream inputFile(fileName);
    cout << "Using 1 thread"  << endl;

    if (!inputFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }
    dispatchThreads(1, "Bible.txt", wordCount);

    outputHashMap(wordCount, "output.txt");

    return 0;
}