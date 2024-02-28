#include <chrono>
#include <iostream>
#include <fstream>
#include <omp.h>

using namespace std;

class HashNode {
public:
    string key;
    long value;
    HashNode* next;

    //rather than making a copy of a copy, move the copy to the parameter to reduce memory usage
    HashNode(string key, long value) : key(std::move(key)), value(value), next(nullptr) {}
};
int collisions = 0;
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
            if (i > 1) {
                //cout << "there was a collision!";
                collisions++;
            }
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

class WordCount {
public:
    string word;
    long count;

    WordCount() : word(""), count(0) {}
    explicit WordCount(std::string  w, int c) : word(std::move(w)), count(c) {}

};


string normalizeWord(const string& word) {
    string normalized;
    for (char ch : word) {
        if(ch >= 0)
            if (isalpha(ch) || ch == '-') {
                normalized += tolower(ch);
            }
    }
    return normalized;
}

int countWords(HashNode** table, int tableSize) {
    int count = 0;
    for (int i = 0; i < tableSize; ++i) {
        for (HashNode* node = table[i]; node != nullptr; node = node->next) {
            ++count;
        }
    }
    return count;
}

void merge(WordCount** arr, int low, int mid, int high) {
    int n1 = mid - low + 1;
    int n2 = high - mid;

    WordCount** L = new WordCount * [n1];
    WordCount** R = new WordCount * [n2];

    for (int i = 0; i < n1; ++i) {
        L[i] = arr[low + i];
    }
    for (int j = 0; j < n2; ++j) {
        R[j] = arr[mid + 1 + j];
    }

    int i = 0, j = 0, k = low;
    while (i < n1 && j < n2) {
        if (L[i]->count >= R[j]->count) {
            arr[k++] = L[i++];
        }
        else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) {
        arr[k++] = L[i++];
    }
    while (j < n2) {
        arr[k++] = R[j++];
    }

    delete[] L;
    delete[] R;
}

void mergeSort(WordCount** arr, int low, int high) {
    if (low < high) {
        int mid = low + (high - low) / 2;
        mergeSort(arr, low, mid);
        mergeSort(arr, mid + 1, high);
        merge(arr, low, mid, high);
    }
}
void outputHashMap(HashMap& hashMap, const string& filename) {
    unsigned long totalWords = countWords(hashMap.table, hashMap.tableSize);
    auto** wordCounts = new WordCount * [totalWords];

    int index = 0;

    for (int i = 0; i < hashMap.tableSize; ++i) {
        HashNode* node = hashMap.table[i];
        while (node != nullptr) {
            wordCounts[index++] = new WordCount(node->key, node->value);
            node = node->next;
        }
    }
    //cout << "sorting!" << endl;
    // Corrected call to quickSort
    mergeSort(wordCounts, 0, totalWords - 1);

    // Output to file
    ofstream outFile(filename);
    for (int i = 0; i < totalWords; ++i) {
        outFile << wordCounts[i]->word << ": " << wordCounts[i]->count << endl; // Dereference pointers when accessing WordCount objects
    }

    outFile.close();

    // Cleanup: deallocate memory for WordCount objects
    for (int i = 0; i < totalWords; ++i) {
        delete wordCounts[i];
    }
    delete[] wordCounts;
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

unsigned long estimateHashMapSize(ifstream& file) {
    long fileSize = getFileLength(file);
    if (fileSize == -1) return -1; // File open error

    const int averageWordSize = 5; // Average word size in characters (a rough guess)
    long totalWords = fileSize / averageWordSize; // Estimate total words
    double uniqueFactor = 0.2; // Assume 20% of words are unique, adjust based on expected data
    int hashMapSize = static_cast<int>(totalWords * uniqueFactor);

    return hashMapSize > 100 ? hashMapSize : 100; // Ensure a minimum size for the HashMap
}

void mergeResults(HashMap& mainTable, HashMap* threadTable) {
    for (int i = 0; i < threadTable->tableSize; ++i) {
        HashNode* threadNode = threadTable->table[i];
        while (threadNode != nullptr) {
            // Lock the bucket for thread safety

            // Insert or update the node in the main table
            HashNode** mainNodePtr = &mainTable.table[mainTable.hashFunction(threadNode->key)];
            while (*mainNodePtr != nullptr && (*mainNodePtr)->key != threadNode->key) {
                mainNodePtr = &((*mainNodePtr)->next);
            }

            if (*mainNodePtr == nullptr) {
                // Key not found in the main table, insert a new node
                *mainNodePtr = new HashNode(threadNode->key, threadNode->value);
            }
            else {
                // Key found, update the value
#pragma omp atomic
                (*mainNodePtr)->value += threadNode->value;
            }

            // Move to the next node in the thread-specific table
            threadNode = threadNode->next;
        }
    }
}

unsigned long findEnd(int threadNum, unsigned long start, unsigned long chunkSize, int numThreads, unsigned long length,  ifstream &file) {
    unsigned long endPos = start + chunkSize;
    if(threadNum == numThreads - 1)
    {
        return length;
    }
    else
    {
        file.seekg(endPos);
        char c;
        while (file.get(c) && c != ' ' && endPos < length) {
            ++endPos;
        }
        return endPos;
    }
}

void dispatchThreads(int numThreads, const string& fileName, HashMap& mainTable) {
    ifstream file(fileName);

    //Keep track start and end indices of each thread
    unsigned long* threadIndices = new unsigned long[numThreads * 2];

    unsigned long length = getFileLength(file);
    unsigned long chunkSize = length / numThreads;
    unsigned long threadTableSize = estimateHashMapSize(file)/ numThreads;

    //Variables to be copied per individual thread in parallel section
    unsigned long start = 0;
    unsigned long end = 0;
    HashMap* threadTable; // Array of pointers to HashMaps

    //Compute start and end positions
    for(int i = 0; i < numThreads; i++) {
        threadIndices[i*2] = start;
        end = findEnd(i,start, chunkSize, numThreads, length, file);
        threadIndices[i*2 + 1] = end;
        start = end;
    }

#pragma omp parallel num_threads(numThreads) firstprivate(threadTable, start, end)
    {
        int i = omp_get_thread_num(); // Get the thread index
        threadTable = new HashMap(threadTableSize); //independent thread table

        //obtain thread indices
        start = threadIndices[i * 2];
        end = threadIndices[i * 2 + 1];

        cout << "Thread " << i << ": Start: " << start << " End: " << end << endl;

        ifstream threadFile(fileName);
        threadFile.seekg(start);
        string segment;
        string word;

        // Read until the designated end position for the thread
        while (threadFile.tellg() < end && threadFile >> word) {
            if (!segment.empty()) segment += " ";
            segment += normalizeWord(word);
        }
        threadTable->insertWords(segment);
        mergeResults(mainTable, threadTable);

        delete threadTable;
    }
    delete [] threadIndices;
}


int main() {
    cout << "using openMP!" << endl;
    int numThreads = 12;
    string fileName = "hello_repeated.txt";
    ifstream inputFile(fileName);
    cout << "File Name: " << fileName << endl;
    cout << "Using " << numThreads << ((numThreads > 1 ) ? " threads" : " thread") << endl;

    //Make sure file is valid
    if (!inputFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }

    unsigned long hashMapSize = estimateHashMapSize(inputFile);

    HashMap wordCount(hashMapSize); // Start with an initial size

    dispatchThreads(numThreads, fileName, wordCount);
    cout << "There were " << collisions << " collisions!" << endl;
    outputHashMap(wordCount, "output.txt");

    return 0;
}
