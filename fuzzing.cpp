#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

#define SERVICE_ID 0x1234
#define METHOD_ID 0x0421
#define INSTANCE_ID 0x5678

#define CRASH_SERVICE

std::shared_ptr<vsomeip::application> app_service;
std::string afl_input;

// ---- Service ----------------------------------------------------------------------------------------------

void service_on_message(const std::shared_ptr<vsomeip::message> &_request)
{
    VSOMEIP_INFO << "--SERVICE-- Received message";
}

// ---- Target for Fuzzing -----------------------------------------------------------------------------------

void fuzzing_target(std::string &input)
{
    afl_input = input;

    app_service = vsomeip::runtime::get()->create_application("!!SERVICE!!");

    app_service->init();
    app_service->register_message_handler(SERVICE_ID, INSTANCE_ID, METHOD_ID, service_on_message);
    app_service->offer_service(SERVICE_ID, INSTANCE_ID);
    app_service->start();

    app_service->clear_all_handler();
    app_service->release_service(SERVICE_ID, INSTANCE_ID);
    app_service->stop();
}

// ---- Main -------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    char chr;
    std::ifstream file;
    std::string input;
    std::stringstream buffer;

#ifndef COMPILE_WITH_GCC
    while (__AFL_LOOP(1000)) // macro unknown for gcc compilers
    {
#endif
        file.open(argv[1]);
        buffer.str("");
        buffer << file.rdbuf();
        input = buffer.str();
        input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
        fuzzing_target(input);
        file.close();
#ifndef COMPILE_WITH_GCC
    }
#endif
}
