/**
 * @file main.cpp
 * @brief 宿主控制台演示程序 (Host Console Demo) 的入口点。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已重构] (工程化升级):
 * 1. FireGlobal 的调用方式已更新为
 * bus->FireGlobal<TEvent>(...)，不再需要手动 make_shared。
 * 2. 增加了对 AsyncExceptionEvent 的订阅。
 */

 // 1. 包含核心管理器
#include "z3y_plugin_manager/plugin_manager.h"

// 2. 包含需要的 *接口*
#include "interfaces_example/i_simple.h"
#include "interfaces_example/i_logger.h"
#include "framework/i_event_bus.h"
#include "framework/framework_events.h"

// 3. 包含需要的 *ClassID 定义*
#include "plugin_example/simple_impl_a.h"
#include "plugin_example/simple_impl_b.h"
#include "plugin_example/logger_service.h"

// 4. 包含C++标准库
#include <iostream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <thread>
#include <chrono>

// 5. 包含平台 API
#ifdef _WIN32
#include <Windows.h>
#endif

// --- 辅助函数 GetExecutableDirectory 和 HostLogger ---

/**
 * @brief [宿主辅助] 定义一个宿主和订阅者都能看到的事件。
 */
struct HostEvent : public z3y::Event
{
    std::string data;
    explicit HostEvent(std::string d) : data(std::move(d)) {}
};


/**
 * @brief 获取当前可执行文件 (.exe) 所在的目录。
 */
std::filesystem::path GetExecutableDirectory()
{
#ifdef _WIN32
    wchar_t path_buffer[2048];
    DWORD length = ::GetModuleFileNameW(NULL, path_buffer, 2048);
    if (length == 0 || length == 2048)
    {
        throw std::runtime_error("Failed to get executable path.");
    }
    std::filesystem::path exe_path(path_buffer);
    return exe_path.parent_path();
#else
    return "./";
#endif
}

/**
 * @class HostLogger
 * @brief 一个简单的辅助类，用于订阅框架事件。
 */
class HostLogger : public std::enable_shared_from_this<HostLogger>
{
public:
    static std::shared_ptr<HostLogger> Create()
    {
        return std::make_shared<HostLogger>();
    }

    /**
     * @brief 订阅所有框架事件
     */
    void SubscribeToFrameworkEvents(z3y::PluginPtr<z3y::IEventBus> bus)
    {
        if (!bus) return;

        // 订阅插件加载成功事件
        bus->SubscribeGlobal<z3y::event::PluginLoadSuccessEvent>(
            shared_from_this(),
            [this](const z3y::event::PluginLoadSuccessEvent& e)
            {
                std::cout << "[Host Event] + LOADED DLL: "
                    << e.plugin_path << std::endl;
            },
            z3y::ConnectionType::kDirect);

        // 订阅插件加载失败事件
        bus->SubscribeGlobal<z3y::event::PluginLoadFailureEvent>(
            shared_from_this(),
            [this](const z3y::event::PluginLoadFailureEvent& e)
            {
                std::cerr << "[Host Event] ! FAILED DLL: " << e.plugin_path
                    << " (Reason: " << e.error_message << ")" << std::endl;
            },
            z3y::ConnectionType::kQueued); // 异步

        // 订阅组件注册事件
        bus->SubscribeGlobal<z3y::event::ComponentRegisterEvent>(
            shared_from_this(),
            [this](const z3y::event::ComponentRegisterEvent& e)
            {
                if (e.is_singleton)
                {
                    std::cout << "[Host Event]   -> Registered [SERVICE]:" << std::endl;
                }
                else
                {
                    std::cout << "[Host Event]   -> Registered [COMPONENT]:" << std::endl;
                }
                std::cout << "        CLSID: 0x" << std::hex << e.clsid
                    << std::dec << std::endl;
                if (!e.alias.empty())
                {
                    std::cout << "        Alias: \"" << e.alias << "\"" << std::endl;
                }
                if (!e.plugin_path.empty())
                {
                    std::cout << "        From: " << e.plugin_path << std::endl;
                }
            });

        // 订阅我们自己的 HostEvent (异步)
        bus->SubscribeGlobal<HostEvent>(
            shared_from_this(),
            [this](const HostEvent& e)
            {
                std::cout << "[Host Event] *** RECEIVED Global HostEvent! Data: '"
                    << e.data << "' ***" << std::endl;
            },
            z3y::ConnectionType::kQueued // 订阅为异步
        );

        // [新增] 订阅异步异常事件 (必须是 kDirect)
        bus->SubscribeGlobal<z3y::event::AsyncExceptionEvent>(
            shared_from_this(),
            [this](const z3y::event::AsyncExceptionEvent& e)
            {
                std::cerr << "[Host Event] !!! ASYNC EXCEPTION CAUGHT: "
                    << e.exception_what << " !!!" << std::endl;
            },
            z3y::ConnectionType::kDirect // 必须是同步，以立即记录
        );
    }
};


/**
 * @brief 宿主程序主入口点。
 */
int main(int argc, char* argv[])
{
    int return_code = 0;
    std::cout << "[Host]: z3y C++ Plugin Framework Host Demo starting..." << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    try
    {
        z3y::PluginPtr<z3y::PluginManager> plugin_manager = z3y::PluginManager::Create();

        auto event_bus = plugin_manager->GetService<z3y::IEventBus>(z3y::clsid::kEventBus);
        auto host_logger = HostLogger::Create();
        host_logger->SubscribeToFrameworkEvents(event_bus);

        std::cout << "[Host]: Subscribed to framework events. Starting plugin load..." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        std::filesystem::path plugin_dir = GetExecutableDirectory();
        plugin_manager->LoadPluginsFromDirectory(plugin_dir);
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "[Host]: Plugin loading finished." << std::endl;


        std::string logger_alias = "Logger.Default";
        std::cout << "[Host]: Requesting LoggerService via alias '"
            << logger_alias << "'..." << std::endl;

        z3y::PluginPtr<z3y::ILogger> logger =
            plugin_manager->GetService<z3y::ILogger>(logger_alias);

        if (!logger)
        {
            throw std::runtime_error("Failed to get LoggerService!");
        }
        logger->Log("Host application acquired ILogger service successfully.");


        logger->Log("Host Requesting SimpleImplA (via clsid::kCSimpleA)...");
        z3y::PluginPtr<z3y::ISimple> simple_a =
            plugin_manager->CreateInstance<z3y::ISimple>(z3y::clsid::kCSimpleA);

        if (!simple_a)
        {
            throw std::runtime_error("Failed to create kCSimpleA instance!");
        }

        logger->Log("Host Requesting SimpleImplB (via alias 'Simple.B')...");
        z3y::PluginPtr<z3y::ISimple> simple_b =
            plugin_manager->CreateInstance<z3y::ISimple>("Simple.B");

        if (!simple_b)
        {
            throw std::runtime_error("Failed to create 'Simple.B' instance!");
        }

        int result_a = simple_a->Add(10, 20); // 应为 130
        logger->Log("Host called simple_a->Add(10, 20), result = " + std::to_string(result_a));

        int result_b = simple_b->Add(10, 20); // 应为 2030
        logger->Log("Host called simple_b->Add(10, 20), result = " + std::to_string(result_b));

        std::cout << "--------------------------------------------------" << std::endl;

        if (event_bus)
        {
            logger->Log("Host firing 'HostEvent' (asynchronously)...");

            // [修正]：使用新的可变参数模板，无需手动 make_shared
            event_bus->FireGlobal<HostEvent>("Hello from Host!");

            logger->Log("Host::FireGlobal has returned (event is now in queue).");
        }

        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "[Host]: Reaching end of scope. Objects will be destroyed." << std::endl;

        // 稍作暂停，让异步事件有时间被后台工作线程处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Host] FATAL ERROR: " << e.what() << std::endl;
        return_code = 1;
    }
    catch (...)
    {
        std::cerr << "[Host] FATAL ERROR: Unknown exception." << std::endl;
        return_code = 2;
    }

    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "[Host]: Host Demo finished. Exiting." << std::endl;

#ifdef _DEBUG
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
#endif

    return return_code;
}