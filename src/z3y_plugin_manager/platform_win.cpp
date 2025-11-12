/**
 * @file platform_win.cpp
 * @brief z3y::PluginManager 类的 Windows 平台特定实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ... (原有的 v2 修复日志) ...
 *
 * [v2.3 修复] (API 重构):
 * 1.
 * LoadPluginsFromDirectory
 * 被重构，
 * 现在只负责遍历目录，
 * 并调用 LoadPluginInternal。
 * 2.
 * 新增 LoadPlugin
 * 接口。
 * 3.
 * 新增 LoadPluginInternal
 * 辅助函数，
 * 包含核心的加载和异常处理逻辑。
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
     * @brief [修改]
     * 扫描指定目录(及其子目录)并加载所有插件。
     * (Windows 平台实现)
     */
    void PluginManager::LoadPluginsFromDirectory(
        const std::filesystem::path& dir,
        bool recursive,
        const std::string& init_func_name)
    {
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
        {
            return;
        }

        if (recursive)
        {
            // [修改] 接口 1: 递归扫描
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
            {
                // [修改]
                // 将所有加载逻辑委托给内部辅助函数
                LoadPluginInternal(entry.path(), init_func_name);
            }
        }
        else
        {
            // [修改] 接口 2: 非递归扫描
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                // [修改]
                // 将所有加载逻辑委托给内部辅助函数
                LoadPluginInternal(entry.path(), init_func_name);
            }
        }
    }

    /**
     * @brief [新]
     * 加载一个指定的插件 DLL/SO 文件。
     * (Windows 平台实现)
     */
    bool PluginManager::LoadPlugin(
        const std::filesystem::path& file_path,
        const std::string& init_func_name)
    {
        // [修改] 接口 3:
        // 委托给内部辅助函数
        return LoadPluginInternal(file_path, init_func_name);
    }

    /**
     * @brief [新]
     * 加载单个插件文件的内部核心逻辑。
     * (Windows 平台实现)
     */
    bool PluginManager::LoadPluginInternal(
        const std::filesystem::path& file_path,
        const std::string& init_func_name)
    {
        // 1. [新] 检查是否为常规文件以及扩展名
        if (!std::filesystem::is_regular_file(file_path) ||
            file_path.extension() != ".dll")
        {
            return false;
        }

        PluginPtr<IEventBus> bus = GetService<IEventBus>(clsid::kEventBus);
        std::string path_str = file_path.string();

        // 2. [原逻辑] 加载 DLL
        HMODULE lib_handle = LoadDynamicLibrary(file_path);
        if (!lib_handle)
        {
            if (bus)
            {
                bus->FireGlobal<event::PluginLoadFailureEvent>(
                    path_str, "LoadLibrary (Win32) failed.");
            }
            return false; // [修改]
        }

        // 3. [原逻辑] 查找入口点函数
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
            return false; // [修改]
        }

        // 4. [原逻辑] 执行入口点函数
        try
        {
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

            return true; // [修改]
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
            return false; // [修改]
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
            return false; // [修改]
        }
    }


    /**
     * @brief 卸载所有已加载的插件并清空所有注册表。
     * (Windows 平台实现)
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
            std::scoped_lock lock(registry_mutex_, event_mutex_, queue_mutex_);

            // 2. 卸载 DLLs (按加载的相反顺序)
            std::reverse(loaded_libs_.begin(), loaded_libs_.end());
            for (LibHandle handle : loaded_libs_)
            {
                UnloadDynamicLibrary(static_cast<HMODULE>(handle));
            }

            // 3. [Fix 2] (安全修复): 
            event_queue_ = {};

            // 4. [Fix 2] (安全修复): 
            gc_queue_ = {};

            // 5. 清空所有内部状态 (在锁仍然持有时)
            loaded_libs_.clear();
            sender_subscribers_.clear();
            global_subscribers_.clear();
            singletons_.clear();
            factories_.clear();
            alias_map_.clear();
            current_loading_plugin_path_.clear();
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