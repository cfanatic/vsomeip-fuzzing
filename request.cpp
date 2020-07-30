#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <condition_variable>
#include <thread>
#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

class Request
{
public:
    Request() : app_(vsomeip::runtime::get()->create_application("!!REQUEST!!"))
    {
    }

    ~Request()
    {
        if (message_loop_.joinable() == true)
        {
            message_loop_.join();
            VSOMEIP_INFO << "--REQUEST-- Message loop joined";
        }
    }

    void init()
    {
        app_->init();
        app_->register_availability_handler(Request::service_id__,
                                            Request::service_instance_id__,
                                            std::bind(&Request::on_availability_cbk,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3));
        app_->register_state_handler(std::bind(&Request::on_state_cbk,
                                               this,
                                               std::placeholders::_1));
        app_->register_message_handler(Request::service_id__,
                                       Request::service_instance_id__,
                                       Request::service_method_id__,
                                       std::bind(&Request::on_message_cbk,
                                                 this,
                                                 std::placeholders::_1));
    }

    void start()
    {
        app_->start();
    }

    void stop()
    {
        stop_ = true;
        app_->stop();
    }

    void on_state_cbk(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            app_->request_service(Request::service_id__, Request::service_instance_id__);
        }
    }

    void on_availability_cbk(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
    {
        if (_service == Request::service_id__ && _instance == Request::service_instance_id__)
        {
            VSOMEIP_INFO << "--REQUEST-- Service ["
                         << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                         << "] is "
                         << (_is_available ? "available." : "NOT available.");
            if (_is_available == true)
            {
                message_loop_ = std::thread(&Request::message_loop, this);
            }
        }
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message> &_response)
    {
        std::string str_payload;
        str_payload.append(reinterpret_cast<const char *>(_response->get_payload()->get_data()), 0, _response->get_payload()->get_length());

        VSOMEIP_INFO << "--REQUEST-- Received response with Service/Session ["
                     << std::setw(4) << std::setfill('0') << std::hex << _response->get_service() << "/"
                     << std::setw(4) << std::setfill('0') << std::hex << _response->get_session() << "] "
                     << str_payload;
    }

    void send_message()
    {
        std::shared_ptr<vsomeip::message> request = vsomeip::runtime::get()->create_request();
        request->set_service(Request::service_id__);
        request->set_instance(Request::service_instance_id__);
        request->set_method(Request::service_method_id__);

        std::shared_ptr<vsomeip::payload> its_payload = vsomeip::runtime::get()->create_payload();
        std::string str("Hello Service!");
        std::vector<vsomeip::byte_t> its_payload_data(std::begin(str), std::end(str));
        its_payload->set_data(its_payload_data);
        request->set_payload(its_payload);
        app_->send(request);
    }

    void message_loop()
    {
        char chr;
        while (stop_ == false && std::cin.get(chr))
        {
            if (chr == 's')
            {
                std::thread sender(&Request::send_message, this);
                sender.detach();
            }
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    std::thread message_loop_;
    bool stop_ = false;
    static const vsomeip::service_t service_id__ = 0x1234;
    static const vsomeip::method_t service_method_id__ = 0x0421;
    static const vsomeip::instance_t service_instance_id__ = 0x5678;
};

Request *req_ptr(nullptr);

void terminate(int _signal)
{
    if (req_ptr != nullptr && (_signal == SIGINT || _signal == SIGTERM))
    {
        req_ptr->stop();
    }
}

int main()
{
    Request req;
    req_ptr = &req;
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    req.init();
    req.start();
}
