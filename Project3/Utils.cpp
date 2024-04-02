#include <chrono>
#include <iostream>
#include <fstream>
#include "/usr/local/Cellar/open-mpi/5.0.2/include/mpi.h"

using namespace std;

string masterCoords = "";
const int MAX_COORD_LENGTH = 1024; // Adjust as necessary

string* obtainLines(int startRow, int endRow, int worldRank, string& fileName,  string* processLines) {
    ifstream file(fileName);
    // Skip lines until reaching the start row for this process
    string line;
    for (int i = 0; i < startRow && getline(file, line); ++i) {}

    // Read and process the assigned rows
    int currentRow = startRow;
    int index = 0;
    while (currentRow <= endRow && getline(file, line)) {
        // Process the line here
        // cout << "Process " << worldRank << " reading row " << currentRow << endl;
        processLines[index] = line;
        index++;
        currentRow++;
    }
    return processLines;
}
void getNumColumnRow(string& fileName, unsigned long& numColumns, unsigned long& numRows) {
    ifstream inputFile(fileName);
    //Make sure file is valid
    if (!inputFile) {
        cerr << "Error opening input file." << endl;
    }

    string line;
    //determine num of columns for input file
    if (getline(inputFile, line)) {
        numColumns = line.length();
        numRows++;
    }
    //determine num of rows for input file
    while (getline(inputFile, line)) {
        ++numRows;
    }
    inputFile.close();
}

void processPattern(string& patternName, unsigned long& numColumns, unsigned long& numRows, string* patternMatch) {
    ifstream inputFile(patternName);
    if (!inputFile) {
        cerr << "Error opening pattern file." << endl;
    }
    string line;
    numRows = 0; // Initialize numRows to 0

    // Read lines from the file and assign each to an index of patternMatch
    while (getline(inputFile, line)) {
        if (numRows == 0) {
            // Determine the number of columns from the first line
            numColumns = line.length();
        }
        if (numRows < 10) { // Ensure we don't exceed the array bounds
            patternMatch[numRows] = line;
        } else {
            cout << "Warning: Pattern file has more than 10 lines; additional lines will be ignored." << endl;
            break; // Exit the loop if there are more than 10 lines
        }
        numRows++;
    }
}

void requestAdditionalLines(int neededLines, int worldRank, int worldSize, int numColumns, string* receivedLines) {

    cout << "[" << worldRank << "] Requesting additional lines..." << endl;
    if (worldRank + 1 >= worldSize) {
        cout << "[" << worldRank << "] No next process to request lines from. Exiting." << endl;
        // No next process to request lines from
        return;
    }

    int nextProcess = worldRank + 1;

    cout << "[" << worldRank << "] Sending request for " << neededLines << " lines to process " << nextProcess << endl;

    // Non-blocking send to request lines
    MPI_Ssend(&neededLines, 1, MPI_INT, nextProcess, 7, MPI_COMM_WORLD);

    // Allocate and initialize buffer for receiving lines
    const int maxLineSize = numColumns + 1;
    char* buffer = new char[neededLines * maxLineSize];
    memset(buffer, 0, neededLines * maxLineSize);

   MPI_Recv(buffer, neededLines * maxLineSize, MPI_CHAR, worldRank - 1, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Convert buffer to strings and assign to receivedLines
    int bufferIndex = 0;
    for (int i = 0; i < neededLines; i++) {
        receivedLines[i].assign(&buffer[bufferIndex]);
        bufferIndex += receivedLines[i].length() + 1; // Move past the current string and '\0'
    }

    // Cleanup
    delete[] buffer;
}

void addCoords(string& outputCoords) {
    // Output to file
    ofstream outFile("/Users/austinfrank/Documents/GitHub/ParallelProcessing/Project3/output.txt");
    outFile << outputCoords << endl;
    outFile.close();
}

void convertAndGatherMasterCoords(const string& processCoords, int worldRank, int worldSize) {
    // Convert masterCoords to a char array
    char localCoords[MAX_COORD_LENGTH];
    memset(localCoords, 0, MAX_COORD_LENGTH); // Initialize with zeros
    strncpy(localCoords, processCoords.c_str(), MAX_COORD_LENGTH - 1); // Copy the string

    // Allocate memory for gathering all coords at process 0
    char* allCoords = nullptr;
    if (worldRank == 0) {
        allCoords = new char[MAX_COORD_LENGTH * worldSize];
    }

    // Gather all localCoords to process 0
    MPI_Gather(localCoords, MAX_COORD_LENGTH, MPI_CHAR, allCoords, MAX_COORD_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Process 0 combines all gathered coords
    if (worldRank == 0) {
        string combinedCoords;
        for (int i = 0; i < worldSize; ++i) {
            combinedCoords += string(allCoords + i * MAX_COORD_LENGTH) + " ";
        }
        addCoords(combinedCoords); // Assuming addCoords can take a std::string
        delete[] allCoords;
    }
}
/**
 *
 * @param numThreads
 * @param fileName
 * @param mainTable
 *
 * Steps:
 * 1. assign each mpi a certain amount of lines
 * 2. parse each row to find a pattern for that row, if match, index same spot in next row
 * 3. if next row isnt within bounds of mpi rank, pass index to next in
 *    line (index being column num and row++)
 * 4. if match, add to output list
 *
 */

void dispatchMPI(string& fileName, int worldRank, int worldSize, int startRow, int endRow, int numColumns, string* patternMatch,unsigned long rowLength) {

    int counter = 0;
    string* processLines = new string[endRow - startRow + 1];
    processLines = obtainLines(startRow, endRow, worldRank, fileName, processLines);

    size_t columnLength = patternMatch[0].length();
    string coordOutput;
    int topRow = 0;
    int topCol = 0;
    int receivedLines;
    int needLines = 0;
cout << "RANK: " << worldRank << " START: " << startRow+1 << " END: " << endRow+1 << endl;
    for (int i = 0; i < endRow - startRow; i++) {
        for (int j = 0; j < numColumns - columnLength; j++) {
            bool patternFound = true;
            bool patternComplete = false;

            for (size_t pi = 0; pi < rowLength && patternFound; pi++) {
                for (size_t pj = 0; pj < columnLength && patternFound; pj++) {
                    int currentRow = i + pi;

                    if (currentRow < endRow - startRow + 1) {
                        if (processLines[currentRow][j + pj] == patternMatch[pi][pj]) {
                            if (pi == 0 && pj == 0) { // Note top-left corner of pattern match
                                topRow = i + 1; // Adjust for 1-based indexing
                                topCol = j + 1;
                                patternFound = true;
                            }
                            if (pi == rowLength - 1 && pj == columnLength - 1) {
                                patternComplete = true; //full pattern found
                            }
                        } else {
                            patternFound = false;
                            //break; // Exit the inner loop on first mismatch
                        }
                    } else {
                        patternFound = false;
                       // break; // Not enough lines for the pattern
                    }

                    //any process other than the first
                    if(worldRank != 0  && i == int((endRow - startRow) - 1)) {
                        counter++;

                        cout << "RANK " << worldRank << " RECV PENDING " << counter << " ====" << endl;
                        MPI_Recv(&needLines, 1, MPI_INT, worldRank - 1, 18, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                      //  cout << "PROCESS " << worldRank << " RECV STATUS (18) : " << statuss <<  endl;
                        cout << "RANK " << worldRank << " RECV COMPLETE " << counter << " ========== " << endl;

                        if(needLines == 1)
                        {
                            cout << "[" << worldRank - 1 << "] NEEDS LINES. Preparing to send request." << endl;

                            MPI_Recv(&receivedLines, 1, MPI_INT, worldRank - 1, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            linesObtained = true;
                            cout << "[" << worldRank << "] received request." << endl;

                            int totalSize = 0;
                            for (int index = 0; index < receivedLines; index++) {
                                totalSize += processLines[index].length() + 1; // +1 for '\n' or '\0'
                            }
                            //  cout << "Total Size: " << totalSize << endl;

                            char *sendLines = new char[totalSize + 1]; // +1 for final null terminator
                            sendLines[0] = '\0'; // Initialize the buffer

                            int currentPosition = 0;
                            for (int index = 0; index < receivedLines; index++) {
                                strcpy(sendLines + currentPosition, processLines[index].c_str());
                                currentPosition += processLines[index].length();
                                sendLines[currentPosition] = '\n'; // Add newline as separator
                                currentPosition++; // Move past the newline
                            }
                            sendLines[currentPosition - 1] = '\0'; // Replace last '\n' with null terminator if needed
                            cout << "[" << worldRank << "] SENDING Lines." << endl;
                            MPI_Ssend(sendLines, currentPosition, MPI_CHAR, worldRank - 1, 10, MPI_COMM_WORLD);
                            cout << "[" << worldRank << "] SENDING Lines COMPLETE." << endl;



                            delete[] sendLines;

                            needLines = 0;
                        }
                    }
                    //No more rows and pattern not complete
                    //Request more rows if not last Process
                    if (worldRank != worldSize - 1) {
                        if(patternFound && !patternComplete &&
                        i == endRow - startRow && pi < rowLength - 1) { // Handle pattern extending beyond current chunk
                            cout << "WORLD RANK: " << worldRank << " PI: " << pi << " PJ: " << pj << endl;
                            needLines = 1; //true
                         //   cout << "SEND PENDING!" << endl;
                            MPI_Send(&needLines, 1, MPI_INT, worldRank + 1, 18, MPI_COMM_WORLD);
                          //  cout << "SEND COMPLETE!" << endl;

                            int neededLines = rowLength - pi;
                            string* additionalLines = new string[neededLines]; // Corrected variable name
                            requestAdditionalLines(neededLines, worldRank, worldSize, numColumns, additionalLines);
                            for (int extraLine = 0; extraLine < neededLines && patternFound; extraLine++, currentRow++) {
                                for (size_t extraChar = 0; extraChar < columnLength && patternFound; extraChar++) {
                                    if (additionalLines[extraLine][j + extraChar] != patternMatch[pi + extraLine][extraChar]) {
                                        patternFound = false;
                                    }
                                }
                            }
                            if(patternFound)
                            {
                                patternComplete = true;
                            }
                            delete[] additionalLines; // Ensure to delete after use within each pattern checking iteration
                            linesObtained = true;
                        }
                        else if((patternFound && patternComplete) ||
                                !patternFound){
                            needLines = 0;
                            MPI_Send(&needLines, 1, MPI_INT, worldRank + 1, 18, MPI_COMM_WORLD);
                        }
                    }
                }
            }
            if (patternFound && patternComplete) {
                coordOutput = to_string(startRow + topRow) + "," + to_string(topCol) + "\n"; // Corrected global row index calculation
                //cout << "adding coords: " << coordOutput << endl;
                masterCoords += coordOutput;
                cout << "[" << worldRank << "] Pattern found. Adding coordinates: " << coordOutput << endl;
            }
        }
    }
    cout << "[" << worldRank << "] Finished processing. Adding coords if applicable." << endl;
    convertAndGatherMasterCoords(masterCoords, worldRank, worldSize);

    cout << "[" << worldRank << "] Process complete. Cleaning up." << endl;
    cout << "AlL THE COORDS: " << masterCoords << endl;
    delete[] processLines; // Clean up after processing all lines
}
