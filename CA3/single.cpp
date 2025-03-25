#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <utility>
#include <mutex>
#include <chrono>
#include <cmath>
#include <thread>
#include <cctype>
#include <sstream>

using namespace std;

const int BREAD_BAKING_TIME_MS = 2000;
const int OVEN_CAPACITY = 10;
const int MAX_BREAD_ORDER = 15;

queue<pair<string, int>> Baker1Queue;
vector<double> Times;

void trim(string& str) {
    size_t start = str.find_first_not_of(" ");
    size_t end = str.find_last_not_of(" ");
    if (start == string::npos || end == string::npos) {
        str = "";
    }
    else {
        str = str.substr(start, end - start + 1);
    }
}

bool isValidNumber(const string& str) {
    for (char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return !str.empty();
}

void getOrder() {
    string line;
    vector<string> names;
    vector<int> counts;
    
    getline(cin, line);
    trim(line);
    size_t pos = 0;
    while ((pos = line.find(' ')) != string::npos) {
        string name = line.substr(0, pos);
        trim(name);
        if (!name.empty()) {
            names.push_back(name);
        }
        line.erase(0, pos + 1);
    }
    trim(line);
    if (!line.empty()) {
        names.push_back(line);
    }
    getline(cin, line);
    trim(line);
    
    pos = 0;
    while ((pos = line.find(' ')) != string::npos) {
        string countStr = line.substr(0, pos);
        trim(countStr);
        if (isValidNumber(countStr)) {
            counts.push_back(stoi(countStr));
        }
        else {
            cerr << "Invalid number format in counts!\n";
            return;
        }
        line.erase(0, pos + 1);
    }
    trim(line);
    if (!line.empty()) {
        if (isValidNumber(line)) {
            counts.push_back(stoi(line));
        }
        else {
            cerr << "Invalid number format in counts!\n";
            return;
        }
    }

    if (names.size() == counts.size()) {
        for (size_t i = 0; i < names.size(); ++i) {
            if (counts[i] > MAX_BREAD_ORDER) {
                cerr << "Maximum bread order limit is " << MAX_BREAD_ORDER << "\n";
                return;
            }
            Baker1Queue.push({ names[i], counts[i] });
        }
    }
    else {
        cerr << "Names and counts do not match!\n";
    }
}

void RunBakery() {
    while (!Baker1Queue.empty()) {
        auto startTime = chrono::high_resolution_clock::now();
        // Fetch the next order
        auto order = Baker1Queue.front();
        Baker1Queue.pop();
        int remainingBreads = order.second;
        while (remainingBreads > 0) {
            int breadsToBake = min(remainingBreads, OVEN_CAPACITY);
            cout << "Baking " << breadsToBake << " breads...\n";
            this_thread::sleep_for(chrono::milliseconds(BREAD_BAKING_TIME_MS));
            remainingBreads -= breadsToBake;
            if (remainingBreads > 0) {
                cout << "Oven can now process the remaining " << remainingBreads << " breads.\n";
            }
        }
        auto endTime = chrono::high_resolution_clock::now();
        chrono::duration<double> orderTime = endTime - startTime;
        Times.push_back(orderTime.count());

        cout << "Order Complete for " << order.first << ".\n";
    }
}

void calculateStats() {
    double sum = 0.0;
    for (double time : Times) {
        sum += time;
    }
    double mean = sum / Times.size();

    double sqSum = 0.0;
    for (double time : Times) {
        sqSum += (time - mean) * (time - mean);
    }
    double standardDeviation = sqrt(sqSum / Times.size());

    cout << "\n---Statistics---\n";
    cout << "Mean: " << mean << " seconds\n";
    cout << "Standard deviation: " << standardDeviation << " seconds\n";
}

int main() {
    getOrder();
    RunBakery();
    cout << "All orders are complete.\n";
    calculateStats();
    return 0;
}