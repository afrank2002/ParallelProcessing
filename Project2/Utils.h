#include <chrono>
#include <iostream>
#include <fstream>
#include "HashNode.h"
#include "WordCount.h"
#include "HashMap.h"

using namespace std;

#ifndef PARALLELPROCESSING_UTILS_H
#define PARALLELPROCESSING_UTILS_H

string normalizeWord(const string& word);
int countWords(HashNode** table, int tableSize);
void merge(WordCount** arr, int low, int mid, int high);
void mergeSort(WordCount** arr, int low, int high);
void outputHashMap(HashMap& hashMap, const string& filename);
long getFileLength(ifstream& file);
unsigned long estimateHashMapSize(ifstream& file);
void mergeResults(HashMap& mainTable, HashMap* threadTable);
unsigned long findEnd(int threadNum, unsigned long start, unsigned long chunkSize, int numThreads, unsigned long length,  ifstream &file);
void dispatchThreads(int numThreads, const string& fileName, HashMap& mainTable);


#endif //PARALLELPROCESSING_UTILS_H
