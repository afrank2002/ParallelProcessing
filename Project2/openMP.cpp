#include <iostream>
#include <fstream>
#include "HashMap.h"
#include "Utils.h"

using namespace std;

int main() {
    cout << "using openMP!" << endl;
    int numThreads = 24;
    string fileName = "combined.txt";
    ifstream inputFile(fileName);

    //Make sure file is valid
    if (!inputFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }
    cout << "File Name: " << fileName << endl;
    cout << "Using " << numThreads << ((numThreads > 1 ) ? " threads" : " thread") << endl;

    unsigned long hashMapSize = estimateHashMapSize(inputFile);

    HashMap wordCount(hashMapSize); // Start with an initial size

    dispatchThreads(numThreads, fileName, wordCount);
    outputHashMap(wordCount, "output.txt");

    return 0;
}
