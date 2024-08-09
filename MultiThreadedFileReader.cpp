#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

class MultiThreadReader {
public:
    MultiThreadReader(const std::string& filename, int numThreads)
        : filename(filename), numThreads(numThreads), currentOffset(0), stop(false) {}

    void readFile() {
        file.open(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open the file!" << std::endl;
            return;
        }

        // Get the total size of the file
        file.seekg(0, std::ios::end);
        totalSize = file.tellg();
        file.seekg(0, std::ios::beg);

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(&MultiThreadReader::worker, this, i);
        }

        // Join threads
        for (auto& thread : threads) {
            thread.join();
        }

        file.close();
    }

private:
    std::string filename;
    int numThreads;
    std::ifstream file;
    std::vector<std::thread> threads;
    std::mutex mtx;
    std::atomic<std::size_t> currentOffset;
    std::size_t totalSize;
    std::atomic<bool> stop;

    void worker(int thread_id) {
        while (true) {
            std::size_t offset = currentOffset.fetch_add(4);

            if (offset >= totalSize) {
                return; // End the thread if there is nothing left to read
            }

            readBytes(thread_id, offset);
        }
    }

    void readBytes(int thread_id, std::size_t offset) {
        char buffer[4] = {0}; // Buffer to store the 4 bytes

        {
            std::lock_guard<std::mutex> lock(mtx); // Ensure only one thread accesses the file stream at a time

            // Seek to the appropriate position
            file.seekg(offset);

            // Calculate the remaining bytes to read
            std::size_t bytesToRead = std::min(static_cast<std::size_t>(4), totalSize - offset);

            // Read bytes
            file.read(buffer, bytesToRead);

            // Check if the read was successful
            if (file) {
                std::cout << "Thread " << thread_id << " read bytes: ";
                for (std::size_t i = 0; i < bytesToRead; ++i) {
                    std::cout << buffer[i] << " ";
                }
                std::cout << std::endl;
            } else {
                std::cerr << "Thread " << thread_id << " failed to read bytes!" << std::endl;
            }
        }
    }
};

int main() {
    const std::string filename = "testbin.bin"; // Your file name
    const int numThreads = 4; // Number of threads

    MultiThreadReader reader(filename, numThreads);
    reader.readFile();

    return 0;
}
