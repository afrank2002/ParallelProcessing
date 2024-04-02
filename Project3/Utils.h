#include <chrono>
#include <iostream>
#include <fstream>

using namespace std;

#ifndef PARALLELPROCESSING_UTILS_H
#define PARALLELPROCESSING_UTILS_H

void dispatchMPI(string& fileName, int worldRank, int worldSize,  int startRow, int endRow, int numColumns, string* patternMatch, unsigned long patternRows);
void getNumColumnRow(string& fileName, unsigned long& numColumns, unsigned long& numRows);
void processPattern(string& patternName, unsigned long& numColumns, unsigned long& numRows, string* patternMatch);
char** requestAdditionalLines(int neededLines, int worldRank, int worldSize, int numColumns, int& receivedLines);
void handleEdgeCases(int worldRank, int worldSize, int rowIndex, int rowLength, int numColumns);
void addCoords(string& outputCoords);
string* obtainLines(int startRow, int endRow, int worldRank, string& fileName,  string* processLines);


#endif //PARALLELPROCESSING_UTILS_H
