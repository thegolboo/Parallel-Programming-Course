#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <utility>
#include <pthread.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <map>

using namespace std;

const int BREAD_BAKING_TIME = 2000000;

vector<queue<pair<string, int>>> bakerQueues;
vector<pair<string, int>> sharedBreadStorage;
mutex storageMutex;
mutex queueMutex;
condition_variable bakerCV;
condition_variable customerCV;
bool allOrdersCompleted = false;
int ovenCapacity;
int currentOvenUsage = 0;
pthread_mutex_t ovenMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ovenCond = PTHREAD_COND_INITIALIZER;
map<string, queue<chrono::steady_clock::time_point>> orderStartTimes;
vector<double> deliveryTimes;
map<string, int> customerOrderCounts;

void trim(string& str) {
    size_t start = str.find_first_not_of(" ");
    size_t end = str.find_last_not_of(" ");
    if (start == string::npos || end == string::npos) {
        str = "";
    } else {
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

double calculateMean(const vector<double>& times) {
    if (times.empty()) return 0.0;
    double sum = 0.0;
    for (double t : times) {
        sum += t;
    }
    return sum / times.size();
}

double calculateStdDev(const vector<double>& times, double mean) {
    if (times.size() < 2) return 0.0;
    double variance = 0.0;
    for (double t : times) {
        double diff = t - mean;
        variance += diff * diff;
    }
    variance /= (times.size() - 1);
    return sqrt(variance);
}

void printStatistics(const vector<double>& deliveryTimes) {
    double mean = calculateMean(deliveryTimes);
    double stdDev = calculateStdDev(deliveryTimes, mean);
    cout << "\n---Statistics---\n";
    cout << "Mean:" << mean << "\n";
    cout << "Standard deviation: " << stdDev << "\n";
}

void recordOrderStart(const string& customerName) {
    orderStartTimes[customerName].push(chrono::steady_clock::now());
}

void getInput() {
    int numBakers;
    cin >> numBakers;
    cin.ignore();
    bakerQueues.resize(numBakers);
    ovenCapacity = 10 * numBakers;
    for (int i = 0; i < numBakers; ++i) {
        string line;
        getline(cin, line);
        trim(line);
        vector<string> names;
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
        vector<int> counts;
        pos = 0;
        while ((pos = line.find(' ')) != string::npos) {
            string countStr = line.substr(0, pos);
            trim(countStr);
            if (isValidNumber(countStr)) {
                counts.push_back(stoi(countStr));
            } else {
                return;
            }
            line.erase(0, pos + 1);
        }
        trim(line);
        if (!line.empty()) {
            if (isValidNumber(line)) {
                counts.push_back(stoi(line));
            } else {
                return;
            }
        }
        if (names.size() == counts.size()) {
            for (size_t j = 0; j < names.size(); ++j) {
                bakerQueues[i].push({names[j], counts[j]});
                recordOrderStart(names[j]);
                if (customerOrderCounts.find(names[j]) == customerOrderCounts.end()) {
                    customerOrderCounts[names[j]] = counts[j];
                } else {
                    customerOrderCounts[names[j]] += counts[j];
                }
            }
        }
    }
}

void* bakerThread(void* arg) {
    int bakerId = *(int*)arg;
    while (true) {
        pair<string, int> order;
        {
            unique_lock<mutex> lock(queueMutex);
            bakerCV.wait(lock, [&]() {
                return !bakerQueues[bakerId].empty() || allOrdersCompleted;
            });
            if (allOrdersCompleted) {
                break;
            }
            if (!bakerQueues[bakerId].empty()) {
                order = bakerQueues[bakerId].front();
                bakerQueues[bakerId].pop();
            }
        }
        if (order.first.empty()) {
            continue;
        }
        cout << "Baker " << bakerId + 1 << " is preparing order for: " << order.first << "\n";
        int remainingBreads = order.second;
        int batch = 1;
        while (remainingBreads > 0) {
            int breadsToBake = min(remainingBreads, ovenCapacity - currentOvenUsage);
            {
                pthread_mutex_lock(&ovenMutex);
                while (currentOvenUsage + breadsToBake > ovenCapacity) {
                    pthread_cond_wait(&ovenCond, &ovenMutex);
                }
                currentOvenUsage += breadsToBake;
                pthread_mutex_unlock(&ovenMutex);
            }
            cout << "Baker " << bakerId + 1 << " is baking batch " << batch
                      << " (" << breadsToBake << " breads) for " << order.first << "\n";
            usleep(BREAD_BAKING_TIME);
            {
                pthread_mutex_lock(&ovenMutex);
                currentOvenUsage -= breadsToBake;
                pthread_cond_broadcast(&ovenCond);
                pthread_mutex_unlock(&ovenMutex);
            }
            {
                lock_guard<mutex> lock(storageMutex);
                for (int i = 1; i <= breadsToBake; ++i) {
                    sharedBreadStorage.push_back(
                        make_pair(order.first, order.second - remainingBreads + i)
                    );
                }
            }
            remainingBreads -= breadsToBake;
            ++batch;
            if (remainingBreads > 0) {
                cout << "Oven can now process the remaining " 
                          << remainingBreads << " breads for " << order.first << ".\n";
            }
        }
        cout << "Baker " << bakerId + 1 << " completed order for: " << order.first << "\n";
        customerCV.notify_all();
    }
    return nullptr;
}

void* queueThread(void* arg) {
    int numBakers = *(int*)arg;
    while (true) {
        {
            lock_guard<mutex> lock(queueMutex);
            bool allEmpty = true;
            for (const auto& q : bakerQueues) {
                if (!q.empty()) {
                    allEmpty = false;
                    break;
                }
            }
            {
                lock_guard<mutex> lock(storageMutex);
                if (allEmpty && sharedBreadStorage.empty()) {
                    allOrdersCompleted = true;
                    bakerCV.notify_all();
                    break;
                }
            }
        }
        {
            unique_lock<mutex> lock(storageMutex);
            customerCV.wait(lock, [&]() { return !sharedBreadStorage.empty(); });
            for (auto it = sharedBreadStorage.begin(); it != sharedBreadStorage.end();) {
                const string& customerName = it->first;
                if (!orderStartTimes[customerName].empty()) {
                    auto start = orderStartTimes[customerName].front();
                    orderStartTimes[customerName].pop();
                    auto end = chrono::steady_clock::now();
                    double diffMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
                    deliveryTimes.push_back(diffMs);
                }
                if (customerOrderCounts.find(customerName) != customerOrderCounts.end()) {
                    customerOrderCounts[customerName]--;
                    if (customerOrderCounts[customerName] == 0) {
                        //cout << customerName << " has collected their full order.\n";
                        customerOrderCounts.erase(customerName);
                    }
                }
                it = sharedBreadStorage.erase(it);
            }
        }
        bakerCV.notify_all();
    }
    bakerCV.notify_all();
    return nullptr;
}

int main() {
    getInput();
    int numBakers = bakerQueues.size();
    vector<pthread_t> bakers(numBakers);
    vector<int> bakerIds(numBakers);
    pthread_t queueManager;
    pthread_create(&queueManager, nullptr, queueThread, &numBakers);
    for (int i = 0; i < numBakers; ++i) {
        bakerIds[i] = i;
        pthread_create(&bakers[i], nullptr, bakerThread, &bakerIds[i]);
    }
    for (int i = 0; i < numBakers; ++i) {
        pthread_join(bakers[i], nullptr);
    }
    pthread_join(queueManager, nullptr);
    printStatistics(deliveryTimes);
    return 0;
}
