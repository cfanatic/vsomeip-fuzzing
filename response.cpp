#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

class Response
{
public:
    Response() : app_(vsomeip::runtime::get()->create_application("!!RESPONSE!!"))
    {
    }

    void init()
    {
        app_->init();
        app_->register_state_handler(std::bind(&Response::on_state_cbk,
                                               this,
                                               std::placeholders::_1));
        app_->register_message_handler(Response::service_id__,
                                       Response::service_instance_id__,
                                       Response::service_method_id__,
                                       std::bind(&Response::on_message_cbk,
                                                 this,
                                                 std::placeholders::_1));
    }

    void start()
    {
        app_->start();
    }

    void stop()
    {
        app_->stop();
    }

    void on_state_cbk(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            app_->offer_service(Response::service_id__, Response::service_instance_id__);
        }
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message> &_request)
    {
        std::string str_payload;
        str_payload.append(reinterpret_cast<const char *>(_request->get_payload()->get_data()), 0, _request->get_payload()->get_length());

        VSOMEIP_INFO << "--RESPONSE-- Received request with Client/Session ["
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
        app_->send(its_response);
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    static const vsomeip::service_t service_id__ = 0x1234;
    static const vsomeip::method_t service_method_id__ = 0x0421;
    static const vsomeip::instance_t service_instance_id__ = 0x5678;
};

Response *res_ptr(nullptr);

void terminate(int _signal)
{
    if (res_ptr != nullptr && (_signal == SIGINT || _signal == SIGTERM))
    {
        res_ptr->stop();
    }
}

int main()
{
    Response res;
    res_ptr = &res;
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    res.init();
    res.start();
}
