/**
 * @file main.cpp
 * @brief 宿主程序 (Host) 的控制台示例。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * ...
 * 13. [修改] [!!]
 * 演示使用
 * GetDefaultService<T>()
 * API
 * 14. [修改] [!!]
 * IPluginQuery
 * 打印逻辑
 * 现在显示
 * "is_registered_as_default"
 * 15. [优化] [!!]
 * 演示使用 z3y::GetDefaultService
 * 和 z3y::FireGlobalEvent
 * 等全局辅助函数，
 * 简化代码。
 */

 // 1. 包含框架核心头文件
 // z3y_framework.h 
 // 现在自动包含 z3y_service_locator.h
#include "framework/z3y_framework.h"
#include "framework/class_id.h"
#include "framework/plugin_exceptions.h" // [!! 
                                         // 新增 !!]

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
#include <map> // 用于 EventTracePoint 映射

#ifdef _WIN32
#include <Windows.h> // 
                     // 
#endif

// --- 辅助函数 ---

namespace {

    // 映射枚举值到可读字符串
    const std::map<z3y::EventTracePoint, const char*> kTracePointNames = {
        {z3y::EventTracePoint::kEventFired, "EVENT_FIRED (Published)"},
        {z3y::EventTracePoint::kDirectCallStart, "DIRECT_CALL (Start)"},
        {z3y::EventTracePoint::kQueuedEntry, "QUEUED_ENTRY (Enqueued)"},
        {z3y::EventTracePoint::kQueuedExecuteStart, "QUEUE_EXECUTE (Start)"},
        {z3y::EventTracePoint::kQueuedExecuteEnd, "QUEUE_EXECUTE (End)"},
    };

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

    /**
     * @brief [修正] 临时事件结构体被移出 main() 函数，以解决 C2246 错误。
     */
    struct FakeEvent : public z3y::Event {
        // 使用 ConstexprHash 确保 EventId 可被 Hook 识别
        Z3Y_DEFINE_EVENT(FakeEvent, "z3y-event-fake-event-UUID-FFFFFFFF")
    };


}  // 匿名命名空间


// --- 主函数 ---

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // 
    // 
    // 
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "--- z3y C++ Plugin Framework Host Demo ---" << std::endl;

    try { // [!! 
        // 修改 !!] 
        // 
        // 
        // 
        // 

      // 1. 创建 PluginManager
      // (
      // 
      // 
      // 
      // )
        z3y::PluginPtr<z3y::PluginManager> manager = z3y::PluginManager::Create();

        // [!! 核心新增 !!] 
        // 2. 演示设置事件追踪钩子
        std::cout << "\n[Host] Setting up Event Trace Hook (Multi-Stage Diagnosis)..." << std::endl;
        // 修正了函数名和 Lambda 签名，使其匹配新的 EventTraceHook
        manager->SetEventTraceHook(
            [](z3y::EventTracePoint point, z3y::EventId event_cls_id, void* event_instance_ptr, const char* info) {
                // 查找追踪点名称
                const char* point_name = "UNKNOWN";
                auto it = kTracePointNames.find(point);
                if (it != kTracePointNames.end()) {
                    point_name = it->second;
                }

                std::cout << "[TRACE] [" << point_name << "]"
                    << " ID: 0x" << std::hex << std::setw(16) << std::setfill('0') << event_cls_id
                    << " Ptr: 0x" << event_instance_ptr
                    << std::dec << " Info: " << info
                    << std::endl;
            });


        // 3. [!! 优化 !!] 
        // (
        // 
        // 
        // 
        // )
        // z3y::PluginPtr<z3y::IEventBus> bus =
        //     manager->GetService<z3y::IEventBus>(z3y::clsid::kEventBus);
        // (
        // 
        // )

        // 4. [演示] [!! 优化 !!] 
        // 使用全局辅助函数订阅
        auto logger = std::make_shared<HostLogger>();
        // (
        // 
        // 
        // )
        std::cout << "\n[Host] Subscribing to framework events..."
            << std::endl;
        z3y::SubscribeGlobalEvent<z3y::event::PluginLoadSuccessEvent>(
            logger, &HostLogger::OnPluginLoaded);
        z3y::SubscribeGlobalEvent<z3y::event::PluginLoadFailureEvent>(
            logger, &HostLogger::OnPluginFailed);
        z3y::SubscribeGlobalEvent<z3y::event::ComponentRegisterEvent>(
            logger, &HostLogger::OnComponentRegistered);

        // [!! 修复 A.2 !!]
        // 异步异常事件*必须*使用 kDirect 
        // (同步) 
        // 连接，
        // 否则如果异常处理程序*自己*也抛出异常，
        // 将导致无限的异步异常循环。
        z3y::SubscribeGlobalEvent<z3y::event::AsyncExceptionEvent>(
            logger, &HostLogger::OnAsyncException,
            z3y::ConnectionType::kDirect // [!! 
            // 修复 A.2 !!]
        );

        // 5. [演示] 加载插件
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

        // 6. [演示] [新增] 查询已加载的插件和组件
        std::cout << "\n[Host] Querying loaded plugins and components..."
            << std::endl;

        // [!! 优化 !!] 
        // 
        // 
        // 
        // (
        // 
        // )
        z3y::PluginPtr<z3y::IPluginQuery> query_service =
            z3y::GetService<z3y::IPluginQuery>(z3y::clsid::kPluginQuery);

        // (
        // 
        // 
        // )
        // 6a. 获取 DLL 列表
        std::vector<std::string> loaded_plugins =
            query_service->GetLoadedPluginFiles();
        std::cout << "--- Loaded Plugin Files (" << loaded_plugins.size()
            << ") ---" << std::endl;
        for (const std::string& path : loaded_plugins) {
            std::cout << "  - " << path << std::endl;
        }

        // 6b. 获取组件列表
        std::vector<z3y::ComponentDetails> components =
            query_service->GetAllComponents();
        std::cout << "--- Registered Components (" << components.size()
            << ") ---" << std::endl;
        for (const auto& detail : components) {
            std::cout << "  - Alias: " << detail.alias
                << " (Singleton: " << std::boolalpha
                << detail.is_singleton << ")"
                << " (IsDefault: " << detail.is_registered_as_default << ")" // [!! 
                // 新增 !!]
                << "\n"
                << "    CLSID: 0x" << std::hex << detail.clsid
                << std::dec << "\n"
                << "    From: " << detail.source_plugin_path << "\n"
                << "    Interfaces:" << std::endl; // [新]

            // (
            // 上一轮已修改
            // )
            for (const auto& iface : detail.implemented_interfaces) {
                std::cout << "      - " << iface.name
                    << " (IID: 0x" << std::hex << iface.iid
                    << std::dec << ")"
                    << " [v" << iface.version.major << "." << iface.version.minor << "]" // [新]
                    << std::endl;
            }
        }


        // 7. [演示] [!! 
        //    优化 !!] 
        //    
        // 
        // 
        std::cout << "\n[Host] Getting *Default* Logger service..." << std::endl;

        z3y::PluginPtr<z3y::example::ILogger> logger_service =
            z3y::GetDefaultService<z3y::example::ILogger>();

        logger_service->Log("[Host] Default Logger service acquired successfully.");


        // 8. [演示] [!! 
        //    优化 !!] 
        //    
        // 
        // 
        // 
        std::cout << "\n[Host] Creating *Default* 'ISimple' component instance..."
            << std::endl;

        z3y::PluginPtr<z3y::example::ISimple> simple_default =
            z3y::CreateDefaultInstance<z3y::example::ISimple>();

        std::cout << "[Host] Default ISimple says: " << simple_default->GetSimpleString()
            << std::endl;

        // 
        // 
        // 
        std::cout << "[Host] Creating 'Simple.B' (by alias) component instance..." << std::endl;
        z3y::PluginPtr<z3y::example::ISimple> simple_b =
            z3y::CreateInstance<z3y::example::ISimple>("Simple.B");
        std::cout << "[Host] Simple.B says: " << simple_b->GetSimpleString()
            << std::endl;


        // 9. [演示] 演示事件监控钩子
        std::cout << "\n[Host] Demonstrating Event Monitor Hook (Firing a known event and a fake event)..." << std::endl;

        // 9a. [!! 优化 !!] 
        // 
        // 
        // 
        z3y::FireGlobalEvent<z3y::event::ComponentRegisterEvent>(
            z3y::ConstexprHash("DEMO-CLSID-001"), "Demo.Component", "Host.Main", false);

        // 9b. [!! 优化 !!] 
        // 
        // 
        // FakeEvent 
        // 
        z3y::FireGlobalEvent<FakeEvent>();

        // 9c. [!! 优化 !!] 
        // 
        // 
        z3y::FireGlobalEvent<z3y::event::AsyncExceptionEvent>("Demo Async Test");


        // 10. [演示] 卸载所有插件 (并重置管理器)
        std::cout << "\n[Host] Unloading all plugins..." << std::endl;

        // [!! 崩溃修复 !!]
        // 
        // (已完成) 
        // 在 manager 
        // 析构前，
        // 必须释放所有插件对象
        logger_service.reset();
        simple_default.reset(); // [!! 
        // 修改 !!]
        simple_b.reset();

        // 
        // 
        // 
        // 
        // (
        // 
        // 
        // )
        // bus.reset(); // 
        query_service.reset();

        manager->UnloadAllPlugins();

        // 11. [演示] 尝试再次获取服务 (此时应失败)
        std::cout << "\n[Host] Re-testing 'Logger.Default' after unload..."
            << std::endl;

        try { // [!! 
            // 修改 !!] 
            // 
            // 
            // 
            // 
            z3y::PluginPtr<z3y::example::ILogger> logger_service_2 =
                z3y::GetDefaultService<z3y::example::ILogger>(); // [!! 
            // 优化 !!]

// 
// 
// 
            std::cout << "[Host] ERROR: Logger service is still valid!"
                << std::endl;
        }
        catch (const z3y::PluginException& e) {
            // 
            // 
            // 
            // 
            std::cout
                << "[Host] Logger service is null (Unload successful). Reason: "
                << z3y::ResultToString(e.GetError()) << std::endl; // [!! 
            // 修改 !!]
        }

        std::cout << "\n--- Demo Finished. Press Enter to Exit ---" << std::endl;

        // [!! 崩溃修复 !!]
        // 
        // 
        // (
        // 
        // 
        // )

        // [修改]
        // 不再需要调用 manager->Shutdown()
        // manager 
        // 将在 main 
        // 退出时自动析构
        std::cout << "[Host] Exiting... PluginManager will now auto-destruct (RAII)."
            << std::endl;

        // (
        // 
        // 
        // )
        manager.reset();
    }
    catch (const z3y::PluginException& e) {
        // [!! 
        // 修改 !!] 
        // 
        // 
        // 
        // 
        // 
        // 
        std::cerr << "\n[Host] [!! FATAL !!] A plugin exception was caught at the top level: "
            << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        // 
        // 
        // 
        std::cerr << "\n[Host] [!! FATAL !!] A standard exception was caught: "
            << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        // 
        // 
        // 
        std::cerr << "\n[Host] [!! FATAL !!] An unknown exception was caught." << std::endl;
        return 1;
    }

    //std::cin.get();
    return 0;
}