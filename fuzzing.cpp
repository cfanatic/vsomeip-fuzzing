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

std::shared_ptr<vsomeip::application> app_service;
std::shared_ptr<vsomeip::application> app_client;
std::mutex mutex;
std::condition_variable condition;

std::string afl_msg;

// ---- Crash ------------------------------------------------------------------------------------------------

void crash_thread(std::string payload)
{
    std::vector<std::string> v = {"Hello", "hullo", "hell"};
    if (std::find(v.begin(), v.end(), payload) != v.end())
    {
        *(int *)0 = 0; // crash: null pointers cannot hold a value
    }
}

// ---- Service ----------------------------------------------------------------------------------------------

void on_message_service(const std::shared_ptr<vsomeip::message> &_request)
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

void start_service()
{
    app_service = vsomeip::runtime::get()->create_application("!!SERVICE!!");
    app_service->init();
    app_service->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message_service);
    app_service->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_service->start();
}

// ---- Client -----------------------------------------------------------------------------------------------

void send_message_client()
{
    VSOMEIP_INFO << "--CLIENT-- Sending message to Service";
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
    request->set_service(SAMPLE_SERVICE_ID);
    request->set_instance(SAMPLE_INSTANCE_ID);
    request->set_method(SAMPLE_METHOD_ID);
    VSOMEIP_FATAL << "AFL: " << afl_msg; // VSOMEIP_FATAL is used as a workaround to reduce the log file size
    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(afl_msg), std::end(afl_msg));
    its_payload->set_data(its_payload_data);
    request->set_payload(its_payload);
    app_client->send(request);
}

void on_message_client(const std::shared_ptr<vsomeip::message> &_response)
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

void on_availability_client(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
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

void start_client()
{
    app_client = vsomeip::runtime::get()->create_application("!!CLIENT!!");
    app_client->init();
    app_client->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, on_availability_client);
    app_client->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_client->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message_client);
    app_client->start();
}

// ---- Main -------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    char chr;
    std::ifstream file;
    std::stringstream buffer;

    while (__AFL_LOOP(1000))
    {
        file.open(argv[1]);
        buffer.str("");
        buffer << file.rdbuf();
        afl_msg = buffer.str();
        afl_msg.erase(std::remove(afl_msg.begin(), afl_msg.end(), '\n'), afl_msg.end());

        std::thread service(start_service);
        std::thread client(start_client);

        std::unique_lock<std::mutex> its_lock(mutex);
        condition.wait(its_lock); // wait until the Service is available

        std::thread sender(send_message_client);
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
        file.close();
    }
}
