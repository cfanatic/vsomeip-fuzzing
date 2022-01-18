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

#include <configuration/include/configuration.hpp>
#include <configuration/include/configuration_impl.hpp>
#include <configuration/include/configuration_plugin.hpp>
#include <plugin/include/plugin_manager_impl.hpp>
#include <security/include/security_impl.hpp>

// ---- Targets for Fuzzing -----------------------------------------------------------------------------------

void fuzzing_target_1(std::string &input)
{
    std::shared_ptr<vsomeip::application> app_service;
    setenv("VSOMEIP_CONFIGURATION", input.c_str(), 1);
    app_service = vsomeip::runtime::get()->create_application("!!SERVICE!!");
    app_service->init();
}

void fuzzing_target_2(const std::string &_config_file)
{
    // Set environment variable to config file and load it
    setenv("VSOMEIP_CONFIGURATION", _config_file.c_str(), 1);

    // Create configuration object
    std::shared_ptr<vsomeip::configuration> its_configuration;
    auto its_plugin = vsomeip::plugin_manager_impl::get()->get_plugin(
        vsomeip::plugin_type_e::CONFIGURATION_PLUGIN, VSOMEIP_CFG_LIBRARY);
    if (its_plugin)
    {
        auto its_configuration_plugin = std::dynamic_pointer_cast<vsomeip::configuration_plugin>(its_plugin);
        if (its_configuration_plugin)
            its_configuration = its_configuration_plugin->get_configuration("!!SERVICE!!");
    }

    // Did we get a configuration object?
    if (0 == its_configuration)
    {
        VSOMEIP_INFO << "No configuration object. Either memory overflow or loading error detected!";
        return;
    }

    vsomeip::cfg::configuration_impl its_copied_config(
        static_cast<vsomeip::cfg::configuration_impl &>(*its_configuration));
    vsomeip::cfg::configuration_impl *its_new_config =
        new vsomeip::cfg::configuration_impl(its_copied_config);
    delete its_new_config;

    // Do some fancy stuff
    its_configuration->set_configuration_path("/my/test/path");

    // Print some fancy stuff
    VSOMEIP_INFO << its_configuration->get_sd_ttl();
}

// ---- Main -------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    char chr;
    std::ifstream file;
    std::string file_path, file_input;
    std::stringstream buffer;

#ifndef COMPILE_WITH_GCC
    while (__AFL_LOOP(1000)) // macro unknown for gcc compilers
    {
#endif
        file_path = argv[1];
        file.open(file_path);
        buffer.str("");
        buffer << file.rdbuf();
        file_input = buffer.str();
        file_input.erase(std::remove(file_input.begin(), file_input.end(), '\n'), file_input.end());
        fuzzing_target_1(file_path);
        fuzzing_target_2(file_path);
        file.close();
#ifndef COMPILE_WITH_GCC
    }
#endif
}
