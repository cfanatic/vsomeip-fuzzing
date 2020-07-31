#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <condition_variable>
#include <thread>
#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

#define SERVICE_ID 0x1234
#define SERVICE_METHOD_ID 0x0421
#define SERVICE_INSTANCE_ID 0x5678
#define SERVICE_EVENTGROUP_ID 0x4465
#define SERVICE_EVENT_ID 0x8778

class Publish
{
public:
    Publish() : app_(vsomeip::runtime::get()->create_application("!!SERVICE!!")),
                blocked_(false),
                running_(true),
                is_offered_(false)
    {
    }

    ~Publish()
    {
        if (offer_thread_.joinable() == true)
        {
            offer_thread_.join();
            VSOMEIP_INFO << "--PUBLISH-- Offer thread joined";
        }
        if (notify_thread_.joinable() == true)
        {
            notify_thread_.join();
            VSOMEIP_INFO << "--PUBLISH-- Notify thread joined";
        }
    }

    void init()
    {
        app_->init();
        app_->register_state_handler(std::bind(&Publish::on_state_cbk, this, std::placeholders::_1));
    }

    void start()
    {
        app_->start();
    }

    void stop()
    {
        running_ = false;
        blocked_ = true;
        stop_offer();
        app_->stop();
    }

    void offer()
    {
        std::lock_guard<std::mutex> its_lock(notify_mutex_);
        app_->offer_service(Publish::service_id__, Publish::service_instance_id__);
        is_offered_ = true;
        notify_condition_.notify_one();
    }

    void stop_offer()
    {
        app_->stop_offer_service(Publish::service_id__, Publish::service_instance_id__);
        is_offered_ = false;
    }

    void on_state_cbk(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            VSOMEIP_INFO << "--PUBLISH-- Application " << app_->get_name() << " is "
                         << (_state == vsomeip::state_type_e::ST_REGISTERED ? "registered." : "deregistered.");
            offer_thread_ = std::thread(&Publish::run, this);
            notify_thread_ = std::thread(&Publish::notify, this);
        }
    }

    void run()
    {
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!blocked_)
            condition_.wait(its_lock); // calling condition_.notify_one() in init() unblocks the waiting thread "run"

        bool is_offer(true);
        while (running_)
        {
            if (is_offer)
                offer();
            else
                stop_offer();
            for (int i = 0; i < 10 && running_; i++)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait for 10 seconds until we start/stop service offer
            is_offer = !is_offer;
        }
    }

    void notify()
    {
        std::shared_ptr<vsomeip::message> its_message = vsomeip::runtime::get()->create_request();
        its_message->set_service(Publish::service_id__);
        its_message->set_method(Publish::service_method_id__);
        its_message->set_instance(Publish::service_instance_id__);

        vsomeip::byte_t its_data[10];
        uint32_t its_size = 1;

        std::cout << sizeof(its_data) << std::endl;

        while (running_)
        {
            std::unique_lock<std::mutex> its_lock(notify_mutex_);
            while (!is_offered_ && running_)
                notify_condition_.wait(its_lock); // calling notify_condition_.notify_one() in offer() unblocks the waiting thread "notify"
            while (is_offered_ && running_)
            {
                if (its_size == sizeof(its_data))
                    its_size = 1;
                for (uint32_t i = 0; i < its_size; ++i)
                    its_data[i] = static_cast<uint8_t>(i);
                {
                    std::lock_guard<std::mutex> its_lock(payload_mutex_);
                    std::cout << "Setting event (Length=" << std::dec << its_size << ")." << std::endl;
                    payload_->set_data(its_data, its_size);
                    app_->notify(Publish::service_id__, Publish::service_instance_id__, Publish::service_event_id, payload_);
                }
                its_size++;
                std::this_thread::sleep_for(std::chrono::milliseconds(Publish::cycle__)); // messages with payload [0], [0 1], [0 1 2], [1 2 3], etc. are sent each second
            }
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool blocked_;
    bool running_;
    bool is_offered_;
    std::mutex mutex_;
    std::mutex notify_mutex_;
    std::mutex payload_mutex_;
    std::condition_variable condition_;
    std::condition_variable notify_condition_;
    std::shared_ptr<vsomeip::payload> payload_;
    std::thread offer_thread_;
    std::thread notify_thread_;
    static const uint32_t cycle__ = 1000;
    static const vsomeip::service_t service_id__ = SERVICE_ID;
    static const vsomeip::method_t service_method_id__ = SERVICE_METHOD_ID;
    static const vsomeip::instance_t service_instance_id__ = SERVICE_INSTANCE_ID;
    static const vsomeip::instance_t service_event_id = SERVICE_EVENT_ID;
};

const uint32_t Publish::cycle__;
Publish *pub_ptr(nullptr);

void terminate(int _signal)
{
    if (pub_ptr != nullptr && (_signal == SIGINT || _signal == SIGTERM))
    {
        pub_ptr->stop();
    }
}

int main()
{
    Publish pub;
    pub_ptr = &pub;
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    pub.init();
    pub.start();
}
