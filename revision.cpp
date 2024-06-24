#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <string>

using namespace std;

const int TABLE_SIZE = 10007; // Prime number for better distribution
const int CHUNK_SIZE = 8;

// Simple hash function for a string
unsigned long hashString(const string &str) {
    unsigned long hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Custom hash table class
class HashTable {
private:
    vector<list<pair<unsigned long, int>>> table;

public:
    HashTable() {
        table.resize(TABLE_SIZE);
    }

    // stores a chunk and it's position in the table
    void insert(const string &chunk, int position) {
        unsigned long hash = hashString(chunk) % TABLE_SIZE;
        table[hash].emplace_back(hashString(chunk), position);
    }

    // retrieves all positions of a given chunk by computing it's hash and comparing the full hash
    vector<int> getPositions(const string &chunk) {
        unsigned long hash = hashString(chunk) % TABLE_SIZE;
        unsigned long fullHash = hashString(chunk);
        vector<int> positions;

        for (const auto &entry : table[hash]) {
            if (entry.first == fullHash) {
                positions.push_back(entry.second);
            }
        }

        return positions;
    }
};

void createRevision(istream &fold, istream &fnew, ostream &frevision) {
    // read in the content
    string oldContent((istreambuf_iterator<char>(fold)), istreambuf_iterator<char>());
    string newContent((istreambuf_iterator<char>(fnew)), istreambuf_iterator<char>());

    // Custom hash table to store positions of chunks in the old file
    HashTable oldChunks;
    size_t oldSize = oldContent.size();

    // iterate through old file to store chunks and positions
    for (int i = 0; i <= oldSize - CHUNK_SIZE; ++i) {
        string chunk = oldContent.substr(i, CHUNK_SIZE);
        oldChunks.insert(chunk, i);
    }

    size_t newSize = newContent.size();
    int i = 0;

    while (i < newSize) {
        string chunk = newContent.substr(i, CHUNK_SIZE);

        // Check if the chunk is in the old file
        vector<int> positions = oldChunks.getPositions(chunk);
        if (!positions.empty()) {
            // Find the longest match
            int maxMatchLen = 0;
            int bestMatchPos = -1;
            for (int pos : positions) {
                int matchLen = 0;
                while (i + matchLen < newSize && pos + matchLen < oldSize && newContent[i + matchLen] == oldContent[pos + matchLen]) {
                    ++matchLen;
                }
                if (matchLen > maxMatchLen) {
                    maxMatchLen = matchLen;
                    bestMatchPos = pos;
                }
            }

            // Emit a copy instruction
            if (maxMatchLen > 0) {
                frevision << "#" << bestMatchPos << "," << maxMatchLen;
                i += maxMatchLen;
                continue;
            }
        }

        // Emit an add instruction
        int addStart = i;
        while (i < newSize && oldChunks.getPositions(newContent.substr(i, CHUNK_SIZE)).empty()) {
            ++i;
        }
        string addData = newContent.substr(addStart, i - addStart);
        char delimiter = '/';
        for (char c : addData) {
            if (c == delimiter) {
                delimiter = ';';
                break;
            }
        }
        frevision << "+" << delimiter << addData << delimiter;
    }
}

bool revise(istream &fold, istream &frevision, ostream &fnew) {
    // Read in the content
    string oldContent((istreambuf_iterator<char>(fold)), istreambuf_iterator<char>());
    string revision((istreambuf_iterator<char>(frevision)), istreambuf_iterator<char>());

    ostringstream newContent;

    // Iterate through instructions and apply them
    size_t i = 0;
    while (i < revision.size()) {
        if (revision[i] == '#') {
            ++i;
            int pos = 0, len = 0;
            // Parse position
            while (i < revision.size() && isdigit(revision[i])) {
                pos = pos * 10 + (revision[i++] - '0');
            }
            if (i >= revision.size() || revision[i] != ',') return false; // Check for comma
            ++i; // Skip the comma
            // Parse length
            while (i < revision.size() && isdigit(revision[i])) {
                len = len * 10 + (revision[i++] - '0');
            }
            if (pos < 0 || len < 0 || pos + len > oldContent.size()) return false; // Check validity
            newContent << oldContent.substr(pos, len);
        } else if (revision[i] == '+') {
            ++i;
            if (i >= revision.size()) return false; // Check for valid delimiter
            char delimiter = revision[i++];
            size_t addStart = i;
            while (i < revision.size() && revision[i] != delimiter) {
                ++i;
            }
            if (i >= revision.size()) return false; // Check for closing delimiter
            newContent << revision.substr(addStart, i - addStart);
            ++i; // Skip the delimiter
        } else {
            return false; // Invalid instruction character
        }
    }

    // Write the new content to the output stream
    fnew << newContent.str();
    return true;
}

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <cassert>
using namespace std;

bool runtest(string oldName, string newName, string revisionName, string newName2)
{
    if (revisionName == oldName  ||  revisionName == newName  ||
        newName2 == oldName  ||  newName2 == revisionName  ||
            newName2 == newName)
    {
        cerr << "Files used for output must have names distinct from other files" << endl;
        return false;
    }
    ifstream oldFile(oldName, ios::binary);
    if (!oldFile)
    {
        cerr << "Cannot open " << oldName << endl;
        return false;
    }
    ifstream newFile(newName, ios::binary);
    if (!newFile)
    {
        cerr << "Cannot open " << newName << endl;
        return false;
    }
    ofstream revisionFile(revisionName, ios::binary);
    if (!revisionFile)
    {
        cerr << "Cannot create " << revisionName << endl;
        return false;
    }
    createRevision(oldFile, newFile, revisionFile);
    revisionFile.close();

    oldFile.clear();   // clear the end of file condition
    oldFile.seekg(0);  // reset back to beginning of the file
    ifstream revisionFile2(revisionName, ios::binary);
    if (!revisionFile2)
    {
        cerr << "Cannot read the " << revisionName << " that was just created!" << endl;
        return false;
    }
    ofstream newFile2(newName2, ios::binary);
    if (!newFile2)
    {
        cerr << "Cannot create " << newName2 << endl;
        return false;
    }
    assert(revise(oldFile, revisionFile2, newFile2));
    newFile2.close();

    newFile.clear();
    newFile.seekg(0);
    ifstream newFile3(newName2, ios::binary);
    if (!newFile)
    {
        cerr << "Cannot open " << newName2 << endl;
        return false;
    }
    if ( ! equal(istreambuf_iterator<char>(newFile), istreambuf_iterator<char>(),
                 istreambuf_iterator<char>(newFile3), istreambuf_iterator<char>()))
    {
        cerr << newName2 << " is not identical to " << newName
                 << "; test FAILED" << endl;
        return false;
    }
    return true;
}

int main()
{
    assert(runtest("warandpeace1.txt", "warandpeace2.txt", "warandpeace_revisionFile_hannieHUFFMAN2.txt", "warandpeace_generatedNewFile_hannieHUFFMAN2.txt"));
    cerr << "Test PASSED" << endl;
}
