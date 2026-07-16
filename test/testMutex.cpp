#include "Mutex.hpp"
#include "NoLog.hpp"
#include "StdOutLog.hpp"

#include <iostream>
#include <memory>

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

    auto log = std::make_shared<Axzl::NoLog>();
    Axzl::Mutex testMutex2("testMutex2", log);

    auto log3 = std::make_shared<Axzl::StdOutLog>("testMutex3");
    Axzl::Mutex testMutex3("testMutex3", log3);
    return 0;
}
