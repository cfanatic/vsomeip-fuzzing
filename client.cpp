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
#define CLIENT_SERVICE_ID 0x1111
#define CLIENT_INSTANCE_ID 0x3333

std::shared_ptr<vsomeip::application> app;
std::mutex mutex;
std::condition_variable condition;

void send_message()
{
    std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
    request->set_service(SAMPLE_SERVICE_ID);
    request->set_instance(SAMPLE_INSTANCE_ID);
    request->set_method(SAMPLE_METHOD_ID);

    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::string str("Hello Service!");
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(str), std::end(str));
    its_payload->set_data(its_payload_data);
    request->set_payload(its_payload);
    app->send(request);
}

void send_message_loop()
{
    char chr;
    std::unique_lock<std::mutex> its_lock(mutex);
    condition.wait(its_lock);
    do
    {
        std::cin.get(chr);
        if (chr == 's')
        {
            std::thread sender(send_message);
            sender.detach();
        }
    } while (chr != 'q');
    VSOMEIP_INFO << "--CLIENT-- Leave send_message_loop()";
}

void on_message(const std::shared_ptr<vsomeip::message> &_response)
{
    std::string str_payload;
    str_payload.append(reinterpret_cast<const char*>(_response->get_payload()->get_data()), 0, _response->get_payload()->get_length());

    VSOMEIP_INFO << "--CLIENT-- Received message with Client/Session ["
                 << std::setw(4) << std::setfill('0') << std::hex << _response->get_client() << "/"
                 << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "] "
                 << str_payload;
}

void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
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

int main()
{
    app = vsomeip::runtime::get()->create_application("!!CLIENT!!");
    app->init();
    app->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, on_availability);
    app->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID); // tell that we want to use a service
    app->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message);
    std::thread sender(send_message_loop); // create thread because event-loop in start() does not return
    app->offer_service(CLIENT_SERVICE_ID, CLIENT_INSTANCE_ID); // workaround: it avoids warnings of missing SD messages on service side
    app->start();
}
