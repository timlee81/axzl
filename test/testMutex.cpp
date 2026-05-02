#include "Mutex.h"
#include <iostream>

int main()
{
    std::cout << "Testing libaxzl_core Mutex..." << std::endl;

    // Create a Mutex instance
    Axzl::Mutex testMutex("testMutex");
    std::cout << "Mutex created successfully" << std::endl;

    // Test lock and unlock
    testMutex.lock();
    std::cout << "Lock acquired" << std::endl;

    testMutex.unlock();
    std::cout << "Lock released" << std::endl;

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
