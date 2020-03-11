#include "detection.h"

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

// optional locking up readers during data reloads
// - this prevents mem leak currently present, but it impacts performance
#define READ_LOCKING 0
#if READ_LOCKING
#include <condition_variable>

std::mutex dataReadyMutex;
std::condition_variable dataReadyNotify;
bool ready = false;
#endif

// Number of threads to run simple detection
const unsigned THREADS_NUM = 40;

// num of running threads and it's mutex
unsigned threadsRunning = 0;
std::mutex threadsRunningMutex;


// User-Agent string of an iPhone mobile device.
const char* mobileUserAgent = ("Mozilla/5.0 (iPhone; CPU iPhone OS 7_1 like Mac OS X) "
        "AppleWebKit/537.51.2 (KHTML, like Gecko) 'Version/7.0 Mobile/11D167 "
        "Safari/9537.53");

// User-Agent string of Firefox Web browser version 41 on desktop.
const char* desktopUserAgent = ("Mozilla/5.0 (Windows NT 6.3; WOW64; rv:41.0) "
        "Gecko/20100101 Firefox/41.0");

// User-Agent string of a iPad device.
const char* tabletUserAgent = ("Mozilla/5.0 (iPad; CPU OS 12_2 like Mac OS X) "
        "AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148");


using namespace devicedetection;


void runDetection(DeviceDetection &provider, unsigned loops) {
    {
        // inc number of threads running
        std::lock_guard<std::mutex> guard(threadsRunningMutex);
        ++threadsRunning;
    }

    while (loops-- > 0) {
        #if READ_LOCKING
        // wait for data if ready == false (race condition in "if" is neglected)
        if (!ready) {
            std::unique_lock<std::mutex> lk(dataReadyMutex);
            dataReadyNotify.wait(lk, []{return ready;});
            lk.unlock();
            dataReadyNotify.notify_one();
        }
        #endif

        if (DeviceDetection::Result::MOBILE != provider.detect(mobileUserAgent)) {
            throw std::runtime_error{"mobile supposed to be detected!"};
        }
        if (DeviceDetection::Result::DESKTOP != provider.detect(desktopUserAgent)) {
            throw std::runtime_error{"desktop supposed to be detected!"};
        }
        if (DeviceDetection::Result::TABLET != provider.detect(tabletUserAgent)) {
            throw std::runtime_error{"table supposed to be detected!"};
        }
    }

    {
        // dec number of threads running
        std::lock_guard<std::mutex> guard(threadsRunningMutex);
        --threadsRunning;
    }
}


int main() {
    const char *fileName = "../data/HashTrieV34.latest";
    DeviceDetection provider(fileName);

    std::vector<std::thread> threads;
    for (unsigned i = 0; i < THREADS_NUM; i++) {
        threads.emplace_back(runDetection, std::ref(provider), 1000000);
    }

    while (threadsRunning > 0) {
        #if READ_LOCKING
        {
            // stop readers access to reload provider
            std::unique_lock<std::mutex> lk(dataReadyMutex);
            ready = false;
        }
        #endif

        provider.reload();

        #if READ_LOCKING
        {
            // allow readers access again
            std::unique_lock<std::mutex> lk(dataReadyMutex);
            ready = true;
        }
        dataReadyNotify.notify_one();
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (std::thread &t: threads) {
        t.join();
    }
    return 0;
}
