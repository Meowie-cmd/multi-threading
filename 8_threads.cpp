// Include necessary standard libraries for various functionalities
#include <iostream>     // Enables input and output stream operations
#include <vector>       // Enables use of the vector container
#include <cmath>        // Provides mathematical functions like sqrt and pow
#include <thread>       // Provides tools for multithreading
#include <mutex>        // Allows thread-safe access using mutexes
#include <algorithm>    // Enables algorithms like sort and min
#include <fstream>      // Facilitates file input and output
#include <chrono>       // Used for measuring execution time

using namespace std; // Use the standard namespace to simplify code

// Declare a mutex for synchronizing access to shared resources
mutex resultMutex;

// Function to compute all prime numbers up to a given limit using the Sieve of Eratosthenes
vector<int> simpleSieve(int limit) {
    vector<bool> isPrime(limit + 1, true); // Create a boolean array of size limit+1, initialized to true
    vector<int> primes;                    // Vector to store the prime numbers

    isPrime[0] = isPrime[1] = false;       // Mark 0 and 1 as not prime

    // Iterate over numbers from 2 up to sqrt(limit)
    for (int i = 2; i * i <= limit; i++) {
        if (isPrime[i]) { // If the number is still marked prime
            for (int j = i * i; j <= limit; j += i) { // Mark all multiples of i as not prime
                isPrime[j] = false;
            }
        }
    }

    // Collect all numbers marked as prime into the result vector
    for (int i = 2; i <= limit; i++) {
        if (isPrime[i]) {
            primes.push_back(i);
        }
    }

    return primes; // Return the list of primes
}

// Function that each thread will execute to mark non-primes in a sub-range
void sieveWorker(long long start, long long end, const vector<int>& smallPrimes, vector<long long>& result) {
    vector<bool> isPrime(end - start + 1, true); // Boolean array representing primality in the current range

    // Loop over all small primes and mark their multiples as non-prime in the range [start, end]
    for (int prime : smallPrimes) {
        long long firstMultiple = max((long long)prime * prime, ((start + prime - 1) / prime) * prime); // First multiple of prime in range
        for (long long j = firstMultiple; j <= end; j += prime) {
            isPrime[j - start] = false; // Mark composite
        }
    }

    vector<long long> localPrimes; // Store found primes in this thread's range

    // Collect all numbers in range that remain marked as prime
    for (long long i = start; i <= end; i++) {
        if (i > 1 && isPrime[i - start]) {
            localPrimes.push_back(i);
        }
    }

    // Lock access to the shared result vector to prevent data races
    lock_guard<mutex> lock(resultMutex);
    result.insert(result.end(), localPrimes.begin(), localPrimes.end()); // Safely insert results
}

// Entry point of the program
int main() {
    long long start = 1;                        // Define starting point of range
    long long end = pow(10, 8);                 // Define end of range (10^4 = 10000)
    int numThreads = 8;                         // Number of threads to use

    auto startTime = chrono::high_resolution_clock::now(); // Capture start time

    long long limit = sqrt(end);               // Only need to sieve up to sqrt(end) for base primes
    vector<int> smallPrimes = simpleSieve(limit); // Get small primes using basic sieve

    vector<thread> threads;                    // Vector to hold thread objects
    vector<long long> finalPrimes;             // Vector to hold all primes found

    long long rangeSize = end - start + 1;     // Total size of the range
    long long chunkSize = (rangeSize + numThreads - 1) / numThreads; // Chunk size per thread (rounded up)

    // Launch threads to perform the segmented sieve
    for (int i = 0; i < numThreads; i++) {
        long long chunkStart = start + i * chunkSize; // Start of current chunk
        long long chunkEnd = min(end, chunkStart + chunkSize - 1); // End of current chunk

        if (chunkStart > end) break;           // Stop if chunk is outside the target range

        // Start a thread to handle this chunk
        threads.emplace_back(sieveWorker, chunkStart, chunkEnd, cref(smallPrimes), ref(finalPrimes));
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    sort(finalPrimes.begin(), finalPrimes.end()); // Sort the final list of primes

    auto endTime = chrono::high_resolution_clock::now(); // Capture end time
    auto duration = chrono::duration_cast<chrono::duration<double>>(endTime - startTime); // Calculate elapsed time

    long long sum = 0;                          // Variable to store the sum of primes
    for (long long p : finalPrimes) {
        sum += p;
    }

    vector<long long> topTenPrimes;            // Store the last ten primes
    int count = finalPrimes.size();            // Total number of primes found
    for (int i = max(0, count - 10); i < count; ++i) {
        topTenPrimes.push_back(finalPrimes[i]);
    }

    // Write the results to a file
    ofstream outFile("primes.txt");
    if (outFile.is_open()) {
        outFile << duration.count() << " " << finalPrimes.size() << " " << sum << "\n"; // Time, count, sum
        for (long long prime : topTenPrimes) {
            outFile << prime << " ";           // Write top ten primes
        }
        outFile << "\n";
        outFile.close();                        // Close the file
    } else {
        cerr << "Unable to open file for writing.\n"; // Handle file open failure
    }

    // Print the results to the console
    cout << "Primes in range [" << start << ", " << end << "] found using " << numThreads << " threads:\n";
    for (long long prime : finalPrimes) {
        cout << prime << " ";                   // Print each prime
    }
    cout << "\n";

    return 0;                                   // Exit the program
}
