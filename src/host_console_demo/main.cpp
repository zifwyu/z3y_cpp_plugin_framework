/**
 * @file main.cpp
 * @brief 宿主控制台演示程序 (Host Console Demo) 的入口点。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. HostLogger 现在可以区分“组件”和“服务”。
 */

 // ... (includes 保持不变) ...
#include "z3y_plugin_manager/plugin_manager.h"
#include "interfaces_example/i_simple.h"
#include "interfaces_example/i_logger.h"
#include "framework/i_event_bus.h"
#include "framework/framework_events.h" // 包含框架事件
#include "plugin_example/simple_impl_a.h"
#include "plugin_example/logger_service.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <filesystem>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace z3y
{
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
        /**
         * @brief 静态工厂函数
         */
        static std::shared_ptr<HostLogger> Create()
        {
            return std::make_shared<HostLogger>();
        }

        /**
         * @brief 订阅所有框架事件
         */
        void SubscribeToFrameworkEvents(PluginPtr<IEventBus> bus)
        {
            if (!bus) return;

            // 订阅插件加载成功事件
            bus->SubscribeGlobal<event::PluginLoadSuccessEvent>(
                shared_from_this(),
                [this](const event::PluginLoadSuccessEvent& e)
                {
                    std::cout << "[Host Event] + LOADED DLL: "
                        << e.plugin_path << std::endl;
                });

            // 订阅插件加载失败事件
            bus->SubscribeGlobal<event::PluginLoadFailureEvent>(
                shared_from_this(),
                [this](const event::PluginLoadFailureEvent& e)
                {
                    std::cerr << "[Host Event] ! FAILED DLL: " << e.plugin_path
                        << " (Reason: " << e.error_message << ")" << std::endl;
                });

            // [修正] 订阅组件注册事件
            bus->SubscribeGlobal<event::ComponentRegisterEvent>(
                shared_from_this(),
                [this](const event::ComponentRegisterEvent& e)
                {
                    // [修正]：现在我们可以区分组件和服务了
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

                    // 内部事件 (kEventBus) 可能没有 plugin_path
                    if (!e.plugin_path.empty())
                    {
                        std::cout << "        From: " << e.plugin_path << std::endl;
                    }
                });
        }
    };

} // namespace z3y


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
        // 1. --- 创建 PluginManager ---
        z3y::PluginPtr<z3y::PluginManager> plugin_manager = z3y::PluginManager::Create();

        // 2. --- 订阅框架调试事件 ---
        // (我们从 GetService 获取 IEventBus，而不是直接 cast manager)
        auto event_bus = plugin_manager->GetService<z3y::IEventBus>(z3y::clsid::kEventBus);
        auto host_logger = z3y::HostLogger::Create();
        host_logger->SubscribeToFrameworkEvents(event_bus);

        std::cout << "[Host]: Subscribed to framework events. Starting plugin load..." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        // 3. --- 加载插件 ---
        std::filesystem::path plugin_dir = z3y::GetExecutableDirectory();
        plugin_manager->LoadPluginsFromDirectory(plugin_dir);
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "[Host]: Plugin loading finished." << std::endl;


        // 4. --- 获取单例服务 (ILogger) ---
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


        // 5. --- 创建普通组件 (SimpleImplA) ---
        logger->Log("Host Requesting SimpleImplA (via clsid::kCSimpleA)...");
        z3y::PluginPtr<z3y::ISimple> simple_a =
            plugin_manager->CreateInstance<z3y::ISimple>(z3y::clsid::kCSimpleA);

        if (!simple_a)
        {
            throw std::runtime_error("Failed to create kCSimpleA instance!");
        }

        // 6. --- 创建普通组件 (SimpleImplB) ---
        logger->Log("Host Requesting SimpleImplB (via alias 'Simple.B')...");
        z3y::PluginPtr<z3y::ISimple> simple_b =
            plugin_manager->CreateInstance<z3y::ISimple>("Simple.B");

        if (!simple_b)
        {
            throw std::runtime_error("Failed to create 'Simple.B' instance!");
        }

        // 7. --- 调用组件方法 ---
        int result_a = simple_a->Add(10, 20); // 应为 130
        logger->Log("Host called simple_a->Add(10, 20), result = " + std::to_string(result_a));

        int result_b = simple_b->Add(10, 20); // 应为 2030
        logger->Log("Host called simple_b->Add(10, 20), result = " + std::to_string(result_b));

        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "[Host]: Reaching end of scope. Objects will be destroyed." << std::endl;
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