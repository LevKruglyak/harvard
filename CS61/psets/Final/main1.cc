#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>

class shared_mutex2 {
    std::mutex ex;
    std::condition_variable_any cv;
    int mode = 0; // -1 means exclusive mode, 0 means unlocked, >0 means shared mode

    void lock() { 
        ex.lock();
        mode = -1;
    }

    void unlock() { 
        if (mode == -1) {
            mode = 0;
        }
        ex.unlock();
    }

    void lock_shared() { 
        ex.lock();
        if (mode >= 0) {
            ++mode;
        }
        ex.unlock();
    }

    void unlock_shared() { 
        ex.lock();
        if (mode >= 0) {
            --mode;
        }
        ex.unlock();
    } 
};
