#include <iostream>
#include <fstream>
#include "Utils.h"
#include "/usr/local/Cellar/open-mpi/5.0.2/include/mpi.h"

using namespace std;

int main() {
    cout << "using openMPI!!!" << endl;
    string fileName = "/Users/austinfrank/Documents/GitHub/ParallelProcessing/Project3/input.txt";
    string patternName = "/Users/austinfrank/Documents/GitHub/ParallelProcessing/Project3/pattern.txt";
    string line;
    string* patternMatch = new string[10];

    unsigned long fileColumns = 0;
    unsigned long fileRows = 0;
    unsigned long patternColumns = 0;
    unsigned long patternRows = 0;
    int rowsPerProcess = 0;

    //init MPI vars
    MPI_Init(NULL, NULL);
    int worldSize;
    int worldRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    string mpiChunks[worldSize];

    getNumColumnRow(fileName, fileColumns, fileRows);
   // getNumColumnRow(patternFile, patternColumns, patternRows);
    processPattern(patternName, patternColumns, patternRows, patternMatch);

    rowsPerProcess = fileRows / worldSize;
    int startRow = worldRank * rowsPerProcess;
    int endRow = (worldRank + 1) * rowsPerProcess - 1;
   // cout << "PROCESS: " << worldRank << " start row: " << startRow << " end row: " << endRow << endl;
   // cout << "Pattern rows: " << patternRows << " Pattern Columns: " << patternColumns << endl;
    // Adjust the range for the last process to include any remaining rows
    if (worldRank == worldSize - 1) {
        endRow += fileRows % worldSize;
    }


    dispatchMPI(fileName, worldRank, worldSize, startRow, endRow, fileColumns, patternMatch, patternRows);
    cout << "FINALIZING" << endl;
    MPI_Finalize();
    cout <<"Finalized!" << endl;
    return 0;
}
