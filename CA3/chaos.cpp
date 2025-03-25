#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <unistd.h>
#include <cmath>
#include <iomanip>

using namespace std;

const int BREAD_BAKING_TIME = 2000000;
const int MAX_BREADS_PER_CUSTOMER = 15;

map<string, int> customerOrders;
queue<string> signalQueue;
map<string, long long> orderStartTimes;
vector<double> orderDurations;
map<string, vector<string>> sharedSpace;
mutex storageMutex;
mutex signalMutex;
mutex ovenMutex;
mutex timeMutex;
condition_variable signalCV;
condition_variable ovenCV;
condition_variable pickupCV;
int currentOvenUsage = 0;
int ovenCapacity = 0;
bool bakeryOpen = true;

long long getCurrentTimeMicros() {
    using namespace std::chrono;
    return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

long long computeDurationMicros(long long start, long long end) {
    return end - start;
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
    cout << std::fixed << std::setprecision(2);
    cout << "Mean: " << mean << "\n";
    cout << "Standard deviation: " << stdDev << "\n";
}

void getInput(int &numCustomers, int &numBakers, vector<string> &customerNames, map<string, int> &customerOrders) {
    cout << "number of customers: ";
    cin >> numCustomers;
    cout << "number of bakers: ";
    cin >> numBakers;
    customerNames.resize(numCustomers);
    for (int i = 0; i < numCustomers; ++i) {
        cout << "customer name and order: ";
        cin >> customerNames[i];
        int breads;
        cin >> breads;
        while (breads > MAX_BREADS_PER_CUSTOMER) {
            cout << "Maximum bread order is " << MAX_BREADS_PER_CUSTOMER << ". try again " << customerNames[i] << ": ";
            cin >> breads;
        }
        customerOrders[customerNames[i]] = breads;
    }
}

void* bakerThread(void* arg) {
    int bakerId = *(int*)arg;
    while (true) {
        string customerName;
        {
            unique_lock<mutex> lock(signalMutex);
            signalCV.wait(lock, [] {
                return !signalQueue.empty() || !bakeryOpen;
            });
            if (signalQueue.empty() && !bakeryOpen) {
                break;
            }
            if (!signalQueue.empty()) {
                customerName = signalQueue.front();
                signalQueue.pop();
            }
        }
        if (!customerName.empty()) {
            int breadsToBake = customerOrders[customerName];
            cout << "Baker " << bakerId + 1 << " is preparing order for " << customerName
                 << " with " << breadsToBake << " breads.\n";
            int remainingBreads = breadsToBake;
            while (remainingBreads > 0) {
                unique_lock<mutex> lock(ovenMutex);
                int batchToBake = min(remainingBreads, ovenCapacity - currentOvenUsage);
                ovenCV.wait(lock, [batchToBake] {
                    return currentOvenUsage + batchToBake <= ovenCapacity;
                });
                currentOvenUsage += batchToBake;
                lock.unlock();
                cout << "Baker " << bakerId + 1 << " is baking " << batchToBake << " breads for " 
                     << customerName << ".\n";
                usleep(BREAD_BAKING_TIME);
                {
                    lock_guard<mutex> sLock(storageMutex);
                    for (int i = 0; i < batchToBake; ++i) {
                        int breadIndex = (breadsToBake - remainingBreads + i + 1);
                        string breadName = "Bread " + to_string(breadIndex);
                        sharedSpace[customerName].push_back(breadName);
                    }
                    pickupCV.notify_all();
                }
                {
                    lock_guard<mutex> ovenLock(ovenMutex);
                    currentOvenUsage -= batchToBake;
                    ovenCV.notify_all();
                }
                remainingBreads -= batchToBake;
            }
            {
                long long endTime = getCurrentTimeMicros();
                long long startTime = 0;
                {
                    lock_guard<mutex> tLock(timeMutex);
                    startTime = orderStartTimes[customerName];
                }
                double duration = static_cast<double>(computeDurationMicros(startTime, endTime)) / 1000000.0;  // In seconds
                {
                    lock_guard<mutex> tLock(timeMutex);
                    orderDurations.push_back(duration);
                }
            }
            cout << "Baker " << bakerId + 1<< " has completed order for " << customerName << ".\n";
        }
    }
    return nullptr;
}

void* customerThread(void* arg) {
    string customerName = *(string*)arg;
    long long startTime = getCurrentTimeMicros();
    {
        lock_guard<mutex> lock(timeMutex);
        orderStartTimes[customerName] = startTime;
    }
    {
        lock_guard<mutex> lock(signalMutex);
        signalQueue.push(customerName);
    }
    signalCV.notify_one();
    cout << "Customer " << customerName << " has signaled their order.\n";
    {
        unique_lock<mutex> lock(storageMutex);
        int neededBreads = customerOrders[customerName];
        pickupCV.wait(lock, [&] {
            return (int)sharedSpace[customerName].size() >= neededBreads;
        });
        sharedSpace[customerName].clear();
        cout << "Customer " << customerName << " has picked up their order.\n";
    }
    return nullptr;
}

int main() {
    int numCustomers, numBakers;
    vector<string> customerNames;
    
    getInput(numCustomers, numBakers, customerNames, customerOrders);
    ovenCapacity = 10 * numBakers;

    vector<pthread_t> customers(numCustomers);
    vector<pthread_t> bakers(numBakers);
    vector<int> bakerIds(numBakers);

    for (int i = 0; i < numCustomers; ++i) {
        pthread_create(&customers[i], nullptr, customerThread, &customerNames[i]);
    }
    for (int i = 0; i < numBakers; ++i) {
        bakerIds[i] = i;
        pthread_create(&bakers[i], nullptr, bakerThread, &bakerIds[i]);
    }
    for (int i = 0; i < numCustomers; ++i) {
        pthread_join(customers[i], nullptr);
    }
    {
        lock_guard<mutex> lock(signalMutex);
        bakeryOpen = false;
    }
    signalCV.notify_all();
    for (int i = 0; i < numBakers; ++i) {
        pthread_join(bakers[i], nullptr);
    }
    cout << "All orders are complete.\n";
    printStatistics(orderDurations);
    return 0;
}
