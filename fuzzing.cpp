#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_METHOD_ID 0x0421
#define SAMPLE_INSTANCE_ID 0x5678

#define CRASH_SERVICE

std::shared_ptr<vsomeip::application> app_service;
std::shared_ptr<vsomeip::application> app_client;
std::mutex mutex;
std::condition_variable condition;

std::string afl_input;

// ---- Crash ------------------------------------------------------------------------------------------------

void crash_thread(std::string &payload)
{
    std::vector<std::string> v = {"Hello", "hullo", "hell"};
    if (std::find(v.begin(), v.end(), payload) != v.end())
    {
        *(int *)0 = 0; // crash: null pointers cannot be dereferenced to a value
    }
}

// ---- Service ----------------------------------------------------------------------------------------------

void service_on_message(const std::shared_ptr<vsomeip::message> &_request)
{
    std::string str_payload;
    str_payload.append(reinterpret_cast<const char *>(_request->get_payload()->get_data()), 0, _request->get_payload()->get_length());
    VSOMEIP_INFO << "--SERVICE-- Received message with Client/Session ["
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_client() << "/"
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_session() << "] "
                 << str_payload;
#ifdef CRASH_SERVICE
    crash_thread(str_payload);
#endif

    VSOMEIP_INFO << "--SERVICE-- Sending message back to Client";
    std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(str_payload), std::end(str_payload));
    its_payload->set_data(its_payload_data);
    its_response->set_payload(its_payload);
    app_service->send(its_response);
#ifdef CRASH_LIBRARY
    VSOMEIP_FATAL << str_payload; // trigger crash in instrumented vsomeip/implementation/logger/src/message.cpp
#endif
}

void service_start()
{
    app_service = vsomeip::runtime::get()->create_application("!!SERVICE!!");
    app_service->init();
    app_service->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, service_on_message);
    app_service->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_service->start();
}

// ---- Client -----------------------------------------------------------------------------------------------

void client_send_message()
{
    VSOMEIP_INFO << "--CLIENT-- Sending message to Service";
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
    request->set_service(SAMPLE_SERVICE_ID);
    request->set_instance(SAMPLE_INSTANCE_ID);
    request->set_method(SAMPLE_METHOD_ID);
    VSOMEIP_FATAL << "AFL: " << afl_input; // VSOMEIP_FATAL is used as a workaround to reduce the log file size
    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(afl_input), std::end(afl_input));
    its_payload->set_data(its_payload_data);
    request->set_payload(its_payload);
    app_client->send(request);
}

void client_on_message(const std::shared_ptr<vsomeip::message> &_response)
{
    std::string str_payload;
    str_payload.append(reinterpret_cast<const char *>(_response->get_payload()->get_data()), 0, _response->get_payload()->get_length());
    VSOMEIP_INFO << "--CLIENT-- Received message with Client/Session ["
                 << std::setw(4) << std::setfill('0') << std::hex << _response->get_client() << "/"
                 << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "] "
                 << str_payload;
#ifdef CRASH_CLIENT
    crash_thread(str_payload);
#endif
}

void client_on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
{
    VSOMEIP_INFO << "--CLIENT-- Service ["
                 << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                 << "] is "
                 << (_is_available ? "available." : "NOT available.");
    if (_is_available == true)
    {
        condition.notify_one();
    }
}

void client_start()
{
    app_client = vsomeip::runtime::get()->create_application("!!CLIENT!!");
    app_client->init();
    app_client->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, client_on_availability);
    app_client->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_client->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, client_on_message);
    app_client->start();
}

// ---- Target for Fuzzing -----------------------------------------------------------------------------------

void fuzzing_target(std::string &input)
{
    afl_input = input;

    std::thread service(service_start);
    std::thread client(client_start);

    std::unique_lock<std::mutex> its_lock(mutex);
    condition.wait(its_lock); // wait until the Service is available

    std::thread sender(client_send_message);
    sender.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // wait until the Client has received the response from the Service

    app_client->clear_all_handler();
    app_client->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_client->stop();
    client.join();
    app_service->clear_all_handler();
    app_service->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_service->stop();
    service.join();
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
