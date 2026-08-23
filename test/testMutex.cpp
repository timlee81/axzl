#include "Log.hpp"
#include "Mutex.hpp"
#include "NoLog.hpp"
#include "StdOutLog.hpp"
#include "Thread.hpp"

#include <iostream>
#include <memory>

void func()
{
}

int main()
{
    std::cout << "Testing libaxzl_core Mutex..." << std::endl;

    // Axzl::Thread t1 { "t1", Axzl::GetLog(), Axzl::SchedFair, func };

    // Create a Mutex instance
    Axzl::Mutex testMutex("testMutex", Axzl::GetLog());
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

    auto log4 { Axzl::MakeStdOutLog("testMutex4") };
    Axzl::Mutex testMutex4("testMutex4", log4);

    auto log5 { Axzl::MakeStdOutLog("testMutex5") };
    Axzl::Mutex testMutex5("testMutex5", log5, Axzl::Mutex::Config { }.SetRecursive());

    auto log6 { Axzl::MakeStdOutLog("testMutex6") };
    auto testMutex6 = Axzl::Mutex::MakeRecursive("testMutex6", log6);

    return 0;
}
