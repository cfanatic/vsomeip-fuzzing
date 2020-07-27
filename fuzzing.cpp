#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <condition_variable>
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

// ---- Service ----------------------------------------------------------------------------------------------

void on_message_service(const std::shared_ptr<vsomeip::message> &_request)
{
    std::string str_payload;
    str_payload.append(reinterpret_cast<const char *>(_request->get_payload()->get_data()), 0, _request->get_payload()->get_length());
    VSOMEIP_INFO << "--SERVICE-- Received message with Client/Session ["
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_client() << "/"
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_session() << "] "
                 << str_payload;

    VSOMEIP_INFO << "--SERVICE-- Sending message to Client";
    std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::string str("HELLO CLIENT!");
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(str), std::end(str));
    its_payload->set_data(its_payload_data);
    its_response->set_payload(its_payload);
    app_service->send(its_response);
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

int main()
{
    char chr;

    std::thread service(start_service);
    std::thread client(start_client);

    std::unique_lock<std::mutex> its_lock(mutex);
    condition.wait(its_lock); // wait until the Service is available

    afl_msg = "afl_fuzzer_input";
    std::thread sender(send_message_client);
    sender.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // wait until the Client has received the response from the Service

    app_client->clear_all_handler();
    app_client->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_client->stop();
    client.join();
    app_service->clear_all_handler();
    app_service->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_service->stop();
    service.join();
}
