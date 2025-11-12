/**
 * @file main.cpp
 * @brief 宿主程序 (Host) 的控制台示例。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * ...
 * 6. [FIX]
 * 使用 argv[0]
 * 获取 exe
 * 目录
 * 7. [FIX]
 * (已完成)
 * 移除 Shutdown()
 * 调用
 * 8. [修改]
 * 更新以使用 z3y::example
 * 命名空间
 * 9. [修改]
 * 更新 IPluginQuery
 * 的用法以打印
 * InterfaceDetails
 */

 // 1. 包含框架核心头文件
#include "framework/z3y_framework.h"
#include "framework/class_id.h"

// 2. 包含此宿主希望直接使用的接口
#include "interfaces_example/i_simple.h"
#include "interfaces_example/i_logger.h"

// 3. 包含用于监听的事件
#include "framework/framework_events.h"

// 4. C++ StdLib
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <filesystem>

// --- 辅助函数 ---

namespace {

    //
    // 
    // 定义一个辅助结构体，
    // 用于演示 enable_shared_from_this
    //
    struct HostLogger : public std::enable_shared_from_this<HostLogger> {

        // 
        // [事件回调] 
        // (保持不变, 
        // 事件仍在 z3y::event 
        // 命名空间)
        //
        void OnPluginLoaded(const z3y::event::PluginLoadSuccessEvent& e) {
            std::cout << "[Host] Plugin Loaded: " << e.plugin_path_ << std::endl;
        }

        void OnPluginFailed(const z3y::event::PluginLoadFailureEvent& e) {
            std::cout << "[Host] PLUGIN FAILED: " << e.plugin_path_
                << " (Error: " << e.error_message_ << ")" << std::endl;
        }

        void OnComponentRegistered(
            const z3y::event::ComponentRegisterEvent& e) {
            std::cout << "[Host] Component Registered:\n"
                << "       - CLSID: 0x" << std::hex << e.clsid_ << std::dec
                << "\n"
                << "       - Alias: " << e.alias_ << "\n"
                << "       - Type: "
                << (e.is_singleton_ ? "Service (Singleton)"
                    : "Component (Transient)")
                << "\n"
                << "       - From: " << e.plugin_path_ << std::endl;
        }

        void OnAsyncException(const z3y::event::AsyncExceptionEvent& e) {
            std::cout << "[Host] ASYNC EXCEPTION: " << e.error_message_
                << std::endl;
        }
    };

}  // 匿名命名空间


// --- 主函数 ---

int main(int argc, char* argv[]) {
    std::cout << "--- z3y C++ Plugin Framework Host Demo ---" << std::endl;

    // 1. 创建 PluginManager
    z3y::PluginPtr<z3y::PluginManager> manager = z3y::PluginManager::Create();

    // 2. 获取核心服务 (IEventBus)
    z3y::PluginPtr<z3y::IEventBus> bus =
        manager->GetService<z3y::IEventBus>(z3y::clsid::kEventBus);

    // 3. [演示] 订阅事件
    auto logger = std::make_shared<HostLogger>();
    if (bus) {
        std::cout << "\n[Host] Subscribing to framework events..."
            << std::endl;
        bus->SubscribeGlobal<z3y::event::PluginLoadSuccessEvent>(
            logger, &HostLogger::OnPluginLoaded);
        bus->SubscribeGlobal<z3y::event::PluginLoadFailureEvent>(
            logger, &HostLogger::OnPluginFailed);
        bus->SubscribeGlobal<z3y::event::ComponentRegisterEvent>(
            logger, &HostLogger::OnComponentRegistered);
        bus->SubscribeGlobal<z3y::event::AsyncExceptionEvent>(
            logger, &HostLogger::OnAsyncException,
            z3y::ConnectionType::kQueued);
    }

    // 4. [演示] 加载插件
    std::cout << "\n[Host] Loading 'plugin_example' (recursive)..."
        << std::endl;

    std::filesystem::path exe_dir = ".";
    if (argc > 0 && argv[0]) {
        exe_dir = std::filesystem::path(argv[0]).parent_path();
    }
    std::cout << "[Host] Loading plugins from: " << exe_dir.string()
        << std::endl;

    manager->LoadPluginsFromDirectory(
        exe_dir,
        true
    );

    // 5. [演示] [新增] 查询已加载的插件和组件
    std::cout << "\n[Host] Querying loaded plugins and components..."
        << std::endl;

    z3y::PluginPtr<z3y::IPluginQuery> query_service =
        manager->GetService<z3y::IPluginQuery>(z3y::clsid::kPluginQuery);

    if (query_service) {
        // 5a. 获取 DLL 列表
        std::vector<std::string> loaded_plugins =
            query_service->GetLoadedPluginFiles();
        std::cout << "--- Loaded Plugin Files (" << loaded_plugins.size()
            << ") ---" << std::endl;
        for (const std::string& path : loaded_plugins) {
            std::cout << "  - " << path << std::endl;
        }

        // 5b. 获取组件列表
        std::vector<z3y::ComponentDetails> components =
            query_service->GetAllComponents();
        std::cout << "--- Registered Components (" << components.size()
            << ") ---" << std::endl;
        for (const auto& detail : components) {
            std::cout << "  - Alias: " << detail.alias
                << " (Singleton: " << std::boolalpha
                << detail.is_singleton << ")\n"
                << "    CLSID: 0x" << std::hex << detail.clsid
                << std::dec << "\n"
                << "    From: " << detail.source_plugin_path << "\n"
                << "    Interfaces:" << std::endl; // [新]

            // [修改] 
            // 遍历 InterfaceDetails 
            // 并打印可读的
            // name
            for (const auto& iface : detail.implemented_interfaces) {
                std::cout << "      - " << iface.name
                    << " (IID: 0x" << std::hex << iface.iid
                    << std::dec << ")" << std::endl;
            }
        }
    }
    else {
        std::cout << "[Host] ERROR: Failed to get IPluginQuery service."
            << std::endl;
    }


    // 6. [演示] 获取 "Logger.Default" (单例服务)
    std::cout << "\n[Host] Getting 'Logger.Default' service..." << std::endl;
    // [修改] 
    // 使用 z3y::example::ILogger
    z3y::PluginPtr<z3y::example::ILogger> logger_service =
        manager->GetService<z3y::example::ILogger>("Logger.Default");

    if (logger_service) {
        logger_service->Log("[Host] Logger service acquired successfully.");
    }
    else {
        std::cout << "[Host] ERROR: Failed to get 'Logger.Default' service."
            << std::endl;
    }

    // 7. [演示] 创建 "Simple.A" (普通组件)
    std::cout << "\n[Host] Creating 'Simple.A' component instance..."
        << std::endl;
    // [修改] 
    // 使用 z3y::example::ISimple
    z3y::PluginPtr<z3y::example::ISimple> simple_a =
        manager->CreateInstance<z3y::example::ISimple>("Simple.A");

    if (simple_a) {
        std::cout << "[Host] Simple.A says: " << simple_a->GetSimpleString()
            << std::endl;
    }
    else {
        std::cout << "[Host] ERROR: Failed to create 'Simple.A' instance."
            << std::endl;
    }

    // 8. [演示] 创建 "Simple.B" (普通组件)
    std::cout << "\n[Host] Creating 'Simple.B' component instance..."
        << std::endl;
    // [修改] 
    // 使用 z3y::example::ISimple
    z3y::PluginPtr<z3y::example::ISimple> simple_b =
        manager->CreateInstance<z3y::example::ISimple>("Simple.B");

    if (simple_b) {
        std::cout << "[Host] Simple.B says: " << simple_b->GetSimpleString()
            << std::endl;
    }
    else {
        std::cout << "[Host] ERROR: Failed to create 'Simple.B' instance."
            << std::endl;
    }

    // 9. [演示] 卸载所有插件 (并重置管理器)
    std::cout << "\n[Host] Unloading all plugins..." << std::endl;

    // [!! 崩溃修复 !!]
    // 
    // (已完成) 
    // 在 manager 
    // 析构前，
    // 必须释放所有插件对象
    logger_service.reset();
    simple_a.reset();
    simple_b.reset();

    manager->UnloadAllPlugins();

    // 10. [演示] 尝试再次获取服务 (此时应失败)
    std::cout << "\n[Host] Re-testing 'Logger.Default' after unload..."
        << std::endl;
    // [修改] 
    // 使用 z3y::example::ILogger
    z3y::PluginPtr<z3y::example::ILogger> logger_service_2 =
        manager->GetService<z3y::example::ILogger>("Logger.Default");

    if (!logger_service_2) {
        std::cout
            << "[Host] Logger service is null (Unload successful)."
            << std::endl;
    }
    else {
        std::cout << "[Host] ERROR: Logger service is still valid!"
            << std::endl;
    }

    std::cout << "\n--- Demo Finished. Press Enter to Exit ---" << std::endl;

    // [!! 崩溃修复 !!]
    // 
    // (已完成) 
    // 在 manager 
    // 析构前释放所有指针
    bus.reset();
    query_service.reset();
    logger_service_2.reset();

    // [修改]
    // 不再需要调用 manager->Shutdown()
    // manager 
    // 将在 main 
    // 退出时自动析构
    std::cout << "[Host] Exiting... PluginManager will now auto-destruct (RAII)."
        << std::endl;

    // (显式 reset 
    // 是最佳实践，
    // 以确保析构顺序)
    manager.reset();

    //std::cin.get();
    return 0;
}