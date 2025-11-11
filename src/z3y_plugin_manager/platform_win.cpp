/**
 * @file platform_win.cpp
 * @brief z3y::PluginManager 类的 Windows 平台特定实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. UnloadAllPlugins() 中重新引导 IEventBus 的调用
 * 已更新为新的 RegisterComponent 签名。
 */

#ifdef _WIN32

#include "plugin_manager.h"
#include <Windows.h>
#include <stdexcept>
#include <algorithm>
#include "framework/i_plugin_registry.h" 
#include "framework/framework_events.h"

namespace z3y
{
    // ... (LoadDynamicLibrary, GetFunctionAddress, UnloadDynamicLibrary, PluginInitFunc 保持不变) ...
    namespace
    {
        HMODULE LoadDynamicLibrary(const std::filesystem::path& path)
        {
            return ::LoadLibraryW(path.c_str());
        }
        FARPROC GetFunctionAddress(HMODULE lib_handle, const char* func_name)
        {
            return ::GetProcAddress(lib_handle, func_name);
        }
        void UnloadDynamicLibrary(HMODULE lib_handle)
        {
            ::FreeLibrary(lib_handle);
        }
        using PluginInitFunc = void(IPluginRegistry*);
    } // 匿名命名空间


    /**
     * @brief 扫描指定目录(及其子目录)并加载所有插件。
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

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".dll")
            {
                path_str = entry.path().string();
                HMODULE lib_handle = LoadDynamicLibrary(entry.path());
                if (!lib_handle)
                {
                    if (bus)
                    {
                        bus->FireGlobal(event::PluginLoadFailureEvent(
                            path_str, "LoadLibrary (Win32) failed."));
                    }
                    continue;
                }

                PluginInitFunc* init_func =
                    reinterpret_cast<PluginInitFunc*>(
                        GetFunctionAddress(lib_handle, init_func_name.c_str())
                        );

                if (!init_func)
                {
                    if (bus)
                    {
                        bus->FireGlobal(event::PluginLoadFailureEvent(
                            path_str, "GetProcAddress failed (z3yPluginInit not found)."));
                    }
                    UnloadDynamicLibrary(lib_handle);
                    continue;
                }

                try
                {
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        current_loading_plugin_path_ = path_str;
                    }

                    init_func(this); // <-- 插件在此处调用 RegisterComponent

                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        current_loading_plugin_path_ = "";
                        loaded_libs_.push_back(lib_handle);
                    }

                    if (bus)
                    {
                        bus->FireGlobal(event::PluginLoadSuccessEvent(path_str));
                    }
                }
                catch (const std::exception& e)
                {
                    if (bus)
                    {
                        bus->FireGlobal(event::PluginLoadFailureEvent(path_str, e.what()));
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
                catch (...)
                {
                    if (bus)
                    {
                        bus->FireGlobal(event::PluginLoadFailureEvent(path_str, "Unknown exception during init."));
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
            }
        }
    }

    /**
     * @brief 卸载所有已加载的插件并清空所有注册表。
     */
    void PluginManager::UnloadAllPlugins()
    {
        // 1. [修正]：
        // 我们必须在锁 *之外* 获取 IEventBus (shared_from_this)，
        // 否则在 RegisterComponent 中会发生死锁。
        auto factory = [this_ptr = shared_from_this()]() -> PluginPtr<IComponent>
            {
                return PluginCast<IComponent>(this_ptr);
            };

        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 2. 卸载 DLLs
            std::reverse(loaded_libs_.begin(), loaded_libs_.end());
            for (LibHandle handle : loaded_libs_)
            {
                UnloadDynamicLibrary(static_cast<HMODULE>(handle));
            }

            // 3. 清空所有状态
            loaded_libs_.clear();
            sender_subscribers_.clear();
            global_subscribers_.clear();
            singletons_.clear();
            factories_.clear();
            alias_map_.clear();
            current_loading_plugin_path_.clear();
        }

        // 4. [修正]：在锁之外重新引导核心服务
        // (注意：此时锁已释放，RegisterComponent 可以安全地获取锁)
        RegisterComponent(
            clsid::kEventBus,
            std::move(factory),
            true /* is_singleton */,
            "z3y.core.eventbus" /* alias */
        );
    }

} // namespace z3y

#endif // _WIN32