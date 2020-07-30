#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vsomeip/vsomeip.hpp>
#include <vsomeip/internal/logger.hpp>

#define SERVICE_ID 0x1234
#define SERVICE_METHOD_ID 0x0421
#define SERVICE_INSTANCE_ID 0x5678
#define SERVICE_EVENTGROUP_ID 0x1000
#define SERVICE_EVENT_ID 0x1001

class Publish
{
public:
    Publish() : app_(vsomeip::runtime::get()->create_application("!!SERVICE!!"))
    {
    }

    void init()
    {
        app_->init();
        app_->register_state_handler(std::bind(&Publish::on_state_cbk,
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
            const vsomeip::byte_t its_data[] = {0x99};
            std::shared_ptr<vsomeip::payload> payload = vsomeip::runtime::get()->create_payload();
            payload->set_data(its_data, sizeof(its_data));
            std::set<vsomeip::eventgroup_t> its_groups;
            its_groups.insert(SERVICE_EVENTGROUP_ID);

            app_->offer_event(Publish::service_id__, Publish::service_instance_id__, SERVICE_EVENT_ID, its_groups);
            app_->notify(Publish::service_id__, Publish::service_instance_id__, SERVICE_EVENT_ID, payload);
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    static const vsomeip::service_t service_id__ = SERVICE_ID;
    static const vsomeip::method_t service_method_id__ = SERVICE_METHOD_ID;
    static const vsomeip::instance_t service_instance_id__ = SERVICE_INSTANCE_ID;
};

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
