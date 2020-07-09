#include <iomanip>
#include <iostream>
#include <sstream>

#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_METHOD_ID 0x0421
#define SAMPLE_INSTANCE_ID 0x5678

std::shared_ptr<vsomeip::application> app;

void on_message(const std::shared_ptr<vsomeip::message> &_request)
{
    std::string str_payload;
    str_payload.append(reinterpret_cast<const char *>(_request->get_payload()->get_data()), 0, _request->get_payload()->get_length());

    VSOMEIP_INFO << "--SERVICE-- Received message with Client/Session ["
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_client() << "/"
                 << std::setw(4) << std::setfill('0') << std::hex << _request->get_session() << "] "
                 << str_payload;

    std::shared_ptr<vsomeip::message> its_response = vsomeip::runtime::get()->create_response(_request);
    std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
    std::string str;
    if (str_payload != "ping")
    {
        str = "Hello Client!";
    }
    else
    {
        str = "pong";
    }
    std::vector<vsomeip::byte_t> its_payload_data(std::begin(str), std::end(str));
    its_payload->set_data(its_payload_data);
    its_response->set_payload(its_payload);
    app->send(its_response);
}

int main()
{
    app = vsomeip::runtime::get()->create_application("!!SERVICE!!");
    app->init();
    app->register_message_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID, on_message);
    app->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID); // offer instance of a service
    app->start();
}
