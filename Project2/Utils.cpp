#include <chrono>
#include <iostream>
#include <fstream>
#include <omp.h>
#include "HashNode.h"
#include "HashMap.h"
#include "WordCount.h"

using namespace std;

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

//returns total count of words within a table
int countWords(HashNode** table, int tableSize) {
    int count = 0;
    for (int i = 0; i < tableSize; ++i) {
        for (HashNode* node = table[i]; node != nullptr; node = node->next) {
            ++count;
        }
    }
    return count;
}

//helper function for merge sort
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

//sorting algo
void mergeSort(WordCount** arr, int low, int high) {
    if (low < high) {
        int mid = low + (high - low) / 2;
        mergeSort(arr, low, mid);
        mergeSort(arr, mid + 1, high);
        merge(arr, low, mid, high);
    }
}

//outputs final results to output file
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
    streampos currentPos = file.tellg(); // save current position
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

        // cout << "Thread " << i << ": Start: " << start << " End: " << end << endl;

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