/**
 * @file platform_win.cpp
 * @brief z3y::PluginManager 类的 Windows 平台特定实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. FireGlobal 的调用方式已更新为
 * bus->FireGlobal<TEvent>(...args)，不再需要手动 make_shared。
 *
 * [重构 v2 - Fix 2] (安全修复):
 * 1. UnloadAllPlugins()
 * 现在会同时锁定 registry_mutex_,
 * event_mutex_ 和 queue_mutex_。
 * 2. 在卸载 DLL 之前，
 * 它会清空 event_queue_，
 * 以防止 EventLoop 线程
 * 在 DLL 卸载后执行其代码，
 * 导致进程崩溃。
 * 3. 同时清空 gc_queue_，
 * 因为这些 GC 任务已无意义。
 */

 // 仅在 Windows 平台上编译此文件
#ifdef _WIN32

#include "plugin_manager.h"
#include <Windows.h>
#include <stdexcept>
#include <algorithm>
#include "framework/i_plugin_registry.h"
#include "framework/framework_events.h"

namespace z3y
{
    // --- 平台特定的辅助函数 (Platform-Specific Helpers) ---
    namespace
    {
        /**
         * @brief [Win32] 加载动态链接库。
         */
        HMODULE LoadDynamicLibrary(const std::filesystem::path& path)
        {
            // 使用 W 版 (Unicode) API
            return ::LoadLibraryW(path.c_str());
        }

        /**
         * @brief [Win32] 获取函数地址。
         */
        FARPROC GetFunctionAddress(HMODULE lib_handle, const char* func_name)
        {
            return ::GetProcAddress(lib_handle, func_name);
        }

        /**
         * @brief [Win32] 卸载动态链接库。
         */
        void UnloadDynamicLibrary(HMODULE lib_handle)
        {
            ::FreeLibrary(lib_handle);
        }

        /**
         * @brief 插件入口点函数的签名。
         */
        using PluginInitFunc = void(IPluginRegistry*);

    } // 匿名命名空间


    /**
     * @brief 扫描指定目录(及其子目录)并加载所有插件。
     *
     * (Windows 平台实现)
     */
    void PluginManager::LoadPluginsFromDirectory(
        const std::filesystem::path& dir,
        const std::string& init_func_name)
    {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
        {
            return;
        }

        PluginPtr<IEventBus> bus = GetService<IEventBus>(clsid::kEventBus);
        std::string path_str;

        // 递归遍历目录
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            // 仅加载 .dll 文件
            if (entry.is_regular_file() && entry.path().extension() == ".dll")
            {
                path_str = entry.path().string();

                // 1. 加载 DLL
                HMODULE lib_handle = LoadDynamicLibrary(entry.path());
                if (!lib_handle)
                {
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, "LoadLibrary (Win32) failed.");
                    }
                    continue;
                }

                // 2. 查找入口点函数
                PluginInitFunc* init_func =
                    reinterpret_cast<PluginInitFunc*>(
                        GetFunctionAddress(lib_handle, init_func_name.c_str())
                        );

                if (!init_func)
                {
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, "GetProcAddress failed (z3yPluginInit not found).");
                    }
                    UnloadDynamicLibrary(lib_handle);
                    continue;
                }

                // 3. 执行入口点函数
                try
                {
                    // 在调用 init_func 之前，设置当前加载路径 (用于 RegisterComponent)
                    {
                        std::lock_guard<std::mutex> lock(registry_mutex_);
                        current_loading_plugin_path_ = path_str;
                    }

                    init_func(this); // <-- 插件在此处调用 RegisterComponent

                    // 加载成功
                    {
                        std::lock_guard<std::mutex> lock(registry_mutex_);
                        current_loading_plugin_path_ = "";
                        loaded_libs_.push_back(lib_handle); // 保存句柄以便卸载
                    }

                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadSuccessEvent>(path_str);
                    }
                }
                catch (const std::exception& e)
                {
                    // init_func 抛出异常
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, e.what());
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
                catch (...)
                {
                    // init_func 抛出未知异常
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, "Unknown exception during init.");
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
            } // end if .dll
        } // end for loop
    }

    /**
     * @brief 卸载所有已加载的插件并清空所有注册表。
     *
     * (Windows 平台实现)
     *
     * [Fix 2] (安全修复):
     * 此函数现在会安全地锁定所有三个互斥锁，
     * 并在卸载 DLL (FreeLibrary) 之前
     * 清空 event_queue_ 和 gc_queue_。
     */
    void PluginManager::UnloadAllPlugins()
    {
        // 1. 获取一个指向自身的工厂函数，
        // 用于稍后重新注册核心服务
        auto this_ptr = std::static_pointer_cast<PluginManager>(shared_from_this());
        auto factory = [this_ptr]() -> PluginPtr<IComponent>
            {
                return PluginCast<IComponent>(this_ptr);
            };

        {
            // [Fix 2] (安全修复):
            // 必须同时锁定三个互斥锁，
            // 以安全地停止系统：
            // 1. registry_mutex_: 停止所有组件注册/创建。
            // 2. event_mutex_: 停止所有事件订阅/发布
            //    (以及 GC 队列)。
            // 3. queue_mutex_: 停止所有异步事件任务。
            std::scoped_lock lock(registry_mutex_, event_mutex_, queue_mutex_);

            // 2. 卸载 DLLs (按加载的相反顺序)
            std::reverse(loaded_libs_.begin(), loaded_libs_.end());
            for (LibHandle handle : loaded_libs_)
            {
                UnloadDynamicLibrary(static_cast<HMODULE>(handle));
            }

            // 3. [Fix 2] (安全修复): 
            // 清空异步事件队列
            // (在持有 queue_mutex_ 锁时)
            // 确保 EventLoop 
            // 线程不会在 DLL 卸载后
            // 执行任何旧任务。
            event_queue_ = {}; // C++17: 替换为默认构造的空队列

            // 4. [Fix 2] (安全修复): 
            // 清空 GC 队列
            // (在持有 event_mutex_ 锁时)
            gc_queue_ = {};

            // 5. 清空所有内部状态 (在锁仍然持有时)
            loaded_libs_.clear();
            sender_subscribers_.clear();
            global_subscribers_.clear();
            singletons_.clear();
            factories_.clear();
            alias_map_.clear();
            current_loading_plugin_path_.clear();

            // [Fix 4] 清空反向查找表
            global_sub_lookup_.clear();
            sender_sub_lookup_.clear();

        } // [Fix 2] 释放所有三个锁

        // 6. 在锁外重新引导核心服务 (IEventBus)
        RegisterComponent(
            clsid::kEventBus,
            std::move(factory),
            true /* is_singleton */,
            "z3y.core.eventbus" /* alias */
        );

        // 7. (可选) 触发一个事件，通知系统已重置
        auto bus = GetService<IEventBus>(clsid::kEventBus);
        if (bus)
        {
            bus->FireGlobal<event::ComponentRegisterEvent>(
                clsid::kEventBus,
                "z3y.core.eventbus",
                "internal.core",
                true);
        }
    }

} // namespace z3y

#endif // _WIN32