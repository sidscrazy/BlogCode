/*
This code benchmarks and measures the performance of memory allocation and deallocation operations for various data sizes.

- The program can run a maximum of MAX_THREADS=500 threads concurrently.
- It collects totalDataPoints=30,000 data points per specified size.
- A global atomic counter provides a unique label for each operation.

The program defines:
- A `Result` structure to store details of each operation, capturing information like its type, data size, and duration.
- The `MyObject` class, which represents an object of approximately 500 bytes.

There are four global vectors representing data sizes of 1KB, 16KB, 24MB, and 60MB. These vectors serve as source data for copying operations.

The `allocateAndLog()` function:
- Determines the number of `MyObject` instances needed for the desired size.
- Repeatedly (5 times) performs an allocation by copying data from a source vector and then a deallocation.
- Logs the duration of each operation, saving the results in a global results vector.
- Between allocation and deallocation, a sleep is introduced for a random duration between 100ms and 200ms.
- Atomic counters track and display the number of completed operations for each data size.

The main program:
- Initializes the global source vectors.
- Launches threads to perform the `allocateAndLog()` function.
- Waits for all threads for a data size to finish before moving on to the next.
- Once all operations are completed for all sizes, the results are written to a CSV file (`memory_alloc_perf2.csv`).

Overall, this program provides insights into the performance characteristics of memory operations for different sizes in a multithreaded environment. 
The generated CSV file can be used for further analysis or visualization.
*/


#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <array>
#include <future>  // For std::async
# include <atomic>  // For std::atomic
#include <Windows.h>

const size_t MAX_THREADS = 500; // Maximum number of threads to run at once
size_t totalDataPoints = 30000;  // Total number of data points to collect per size
std::atomic<int> atomicSerialNumber(0); // Atomic serial number for each data point
bool Enable_SegmentedHeap = false; // Enable segmented heap

struct Result {
    int serialNum;
    std::string operation;
    std::string size;
    int64_t startTimestamp;
    int64_t endTimestamp;
    int64_t duration;
};

std::vector<Result> results;
std::mutex resultsMutex;

class MyObject {
public:
    MyObject() {
        // Use std::fill to simulate data
        std::fill(data.begin(), data.end(), 0x42);
    }

private:
    std::array<char, 500> data;  // Placeholder size; change as needed
};

// Define global sourceVecs for each size
std::vector<MyObject> sourceVec1KB;
std::vector<MyObject> sourceVec16KB;
std::vector<MyObject> sourceVec24MB;
std::vector<MyObject> sourceVec60MB;

std::vector<MyObject>& getSourceVec(size_t sizeInKB) {
    if (sizeInKB == 1) return sourceVec1KB;
    if (sizeInKB == 16) return sourceVec16KB;
    if (sizeInKB == 24 * 1024) return sourceVec24MB;
    if (sizeInKB == 60 * 1024) return sourceVec60MB;
    throw std::runtime_error("Invalid sizeInKB provided!");
}

// Define global counters for each size to record completion in cmd prompt while executing.
std::atomic<int> completed1KB{ 0 };
std::atomic<int> completed16KB{ 0 };
std::atomic<int> completed24MB{ 0 };
std::atomic<int> completed60MB{ 0 };


void allocateAndLog(size_t sizeInKB) {
    int serialNum = atomicSerialNumber.fetch_add(1);
    // Determine how many items are needed to fill the desired size
    const size_t itemSize = sizeof(MyObject);  // This should be roughly 500 bytes
    size_t numItems = (sizeInKB * 1024) / itemSize;

    // Random sleep generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100, 200);

    auto& sourceVec = getSourceVec(sizeInKB);  // Get the corresponding global sourceVec

    for (int j = 0; j < 5; ++j) {  // Repeatedly perform the operations 5 times
        Result allocResult;
        allocResult.serialNum = serialNum;
        allocResult.operation = "Allocation";
        allocResult.size = std::to_string(sizeInKB) + "KB";

        auto startAlloc = std::chrono::high_resolution_clock::now();
        std::vector<MyObject> destVec = sourceVec;  // Copy operation
        auto endAlloc = std::chrono::high_resolution_clock::now();

        allocResult.startTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(startAlloc.time_since_epoch()).count();
        allocResult.endTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(endAlloc.time_since_epoch()).count();
        allocResult.duration = std::chrono::duration_cast<std::chrono::microseconds>(endAlloc - startAlloc).count();

        // Sleep for a random duration between 0ms and 50ms
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

        Result deallocResult;
        deallocResult.serialNum = serialNum;
        deallocResult.operation = "DeAllocation";
        deallocResult.size = std::to_string(sizeInKB) + "KB";

        auto startDealloc = std::chrono::high_resolution_clock::now();
        destVec.clear();
        destVec.shrink_to_fit();  // Ensure memory is released
        auto endDealloc = std::chrono::high_resolution_clock::now();

        deallocResult.startTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(startDealloc.time_since_epoch()).count();
        deallocResult.endTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(endDealloc.time_since_epoch()).count();
        deallocResult.duration = std::chrono::duration_cast<std::chrono::microseconds>(endDealloc - startDealloc).count();

        // Store the results in the shared structure using a mutex for thread safety
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(allocResult);
            results.push_back(deallocResult);
        }

        // Update the completed count based on the size and print it
        if (sizeInKB == 1) {
            int count = completed1KB.fetch_add(1);
            std::cout << "Completed 1KB operations: " << count + 1 << std::endl;
        }
        else if (sizeInKB == 16) {
            int count = completed16KB.fetch_add(1);
            std::cout << "Completed 16KB operations: " << count + 1 << std::endl;
        }
        else if (sizeInKB == 24 * 1024) {
            int count = completed24MB.fetch_add(1);
            std::cout << "Completed 24MB operations: " << count + 1 << std::endl;
        }
        else if (sizeInKB == 60 * 1024) {
            int count = completed60MB.fetch_add(1);
            std::cout << "Completed 60MB operations: " << count + 1 << std::endl;
        }
    }
}

int main() {

    // Check if segmented heap is enabled
    HANDLE hHeap = GetProcessHeap();
    ULONG heapType = 3; // HEAP_SEGMENTED_HEAP
    if (Enable_SegmentedHeap) {
        if (HeapSetInformation(hHeap, HeapCompatibilityInformation, &heapType, sizeof(heapType))) {
			std::cout << "Segmented Heap is enabled" << std::endl;
		}
        else {
			std::cout << "Segmented Heap is not enabled" << std::endl;
		}
	}    

    const size_t sizesInKB[] = { 1, 16, 24 * 1024, 60 * 1024 };  // 1KB, 16KB, 24MB, and 60MB

     //Initialize global sourceVecs
    sourceVec1KB.resize(1 * 1024 / sizeof(MyObject));
    std::fill(sourceVec1KB.begin(), sourceVec1KB.end(), MyObject());

    sourceVec16KB.resize(16 * 1024 / sizeof(MyObject));
    std::fill(sourceVec16KB.begin(), sourceVec16KB.end(), MyObject());

    sourceVec24MB.resize(24 * 1024 * 1024 / sizeof(MyObject));
    std::fill(sourceVec24MB.begin(), sourceVec24MB.end(), MyObject());

    sourceVec60MB.resize(60 * 1024 * 1024 / sizeof(MyObject));
    std::fill(sourceVec60MB.begin(), sourceVec60MB.end(), MyObject());

    size_t dataPointsPerSize = totalDataPoints / (sizeof(sizesInKB) / sizeof(sizesInKB[0]));

    std::vector<std::future<void>> futures;

    for (const auto& size : sizesInKB) {
        std::vector<std::future<void>> futures;

        for (size_t i = 0; i < dataPointsPerSize; ++i) {
            // Wait until we have room for a new thread
            while (futures.size() >= MAX_THREADS) {
                futures.erase(std::remove_if(futures.begin(), futures.end(),
                    [](const std::future<void>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }),
                    futures.end());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Start a new task
            futures.push_back(std::async(std::launch::async, allocateAndLog, size));
        }

        // Wait for all threads for this size to finish
        for (auto& f : futures) {
            f.wait();
        }
    }

    // Write the results to the CSV file
    std::ofstream outFile("memory_alloc_perf2_segmentedheap.csv");
    outFile << "Serial Number,MemoryOperation,MemorySize_in_Operation,Timestamp_Start,Timestamp_End,TotalDuration" << std::endl;

    for (const auto& result : results) {
        outFile << result.serialNum << ","
            << result.operation << ","
            << result.size << ","
            << result.startTimestamp << ","
            << result.endTimestamp << ","
            << result.duration << std::endl;
    }

    outFile.close();
    return 0;
}
