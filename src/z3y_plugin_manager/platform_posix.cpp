/**
 * @file platform_posix.cpp
 * @brief z3y::PluginManager 类的 POSIX (Linux/macOS) 平台特定实现。
 * @author 孙鹏宇 (Adapted for POSIX)
 * @date 2025-11-10
 *
 * @details
 * 1. 此文件是 platform_win.cpp 的 POSIX 对应物。
 * 2. 它使用 #!defined(_WIN32) 守卫，
 * 以确保它只在非 Windows 平台上编译。
 * 3. 它使用 dlopen, dlsym, 和 dlclose
 * 来实现动态库的加载和卸载。
 * 4. 它会查找 .so (Linux) 和 .dylib (macOS)
 * 文件，
 * 而不是 .dll 文件。
 * 5. LoadPluginsFromDirectory 和 UnloadAllPlugins
 * 的实现
 * (包括 Fix 2
 * 中的所有锁和队列清理逻辑)
 * 与 Windows 版本完全相同，
 * 只是它们调用的底层辅助函数不同。
 */

 // 仅在非 Windows 平台上编译此文件
#if !defined(_WIN32)

#include "plugin_manager.h"
#include <stdexcept>
#include <algorithm>
#include <dlfcn.h> // POSIX 动态库头文件
#include "framework/i_plugin_registry.h"
#include "framework/framework_events.h"

namespace z3y
{
    // --- 平台特定的辅助函数 (Platform-Specific Helpers) ---
    namespace
    {
        /**
         * @brief [POSIX] 加载动态链接库 (.so / .dylib)。
         * @param[in] path 库文件的路径。
         * @return void* 库句柄，失败则返回 nullptr。
         */
        void* LoadDynamicLibrary(const std::filesystem::path& path)
        {
            // dlopen 需要一个 C 风格字符串。
            // RTLD_NOW: 立即解析所有符号。
            // RTLD_LOCAL: 
            //    使库中的符号不被
            //    后续加载的库自动使用 
            //    (良好的封装性)。
            return ::dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        }

        /**
         * @brief [POSIX] 获取函数地址。
         * @param[in] lib_handle 库句柄 (来自 dlopen)。
         * @param[in] func_name 要查找的函数名 (C 风格字符串)。
         * @return void* 函数指针，失败则返回 nullptr。
         */
        void* GetFunctionAddress(void* lib_handle, const char* func_name)
        {
            // dlsym 返回一个 void* 指针
            return ::dlsym(lib_handle, func_name);
        }

        /**
         * @brief [POSIX] 卸载动态链接库。
         * @param[in] lib_handle 库句柄。
         */
        void UnloadDynamicLibrary(void* lib_handle)
        {
            ::dlclose(lib_handle);
        }

        /**
         * @brief 插件入口点函数的签名
         * (与 platform_win.cpp
         * 中定义的一致)。
         */
        using PluginInitFunc = void(IPluginRegistry*);

    } // 匿名命名空间


    /**
     * @brief 扫描指定目录(及其子目录)并加载所有插件。
     *
     * (POSIX 平台实现)
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
            const auto extension = entry.path().extension();

            // [POSIX 变更]：
            // 查找 .so (Linux) 或 .dylib (macOS)
            if (entry.is_regular_file() && (extension == ".so" || extension == ".dylib"))
            {
                path_str = entry.path().string();

                // 1. 加载库
                // (调用此文件中的 POSIX 辅助函数)
                LibHandle lib_handle = LoadDynamicLibrary(entry.path());
                if (!lib_handle)
                {
                    if (bus)
                    {
                        // [POSIX 变更]：
                        // 使用 dlerror() 获取详细错误信息
                        const char* err_str = ::dlerror();
                        std::string err_msg = "dlopen failed. ";
                        if (err_str) err_msg += err_str;

                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, err_msg);
                    }
                    continue;
                }

                // 2. 查找入口点函数
                // (调用此文件中的 POSIX 辅助函数)
                PluginInitFunc* init_func =
                    reinterpret_cast<PluginInitFunc*>(
                        GetFunctionAddress(lib_handle, init_func_name.c_str())
                        );

                if (!init_func)
                {
                    if (bus)
                    {
                        // [POSIX 变更]：
                        // 使用 dlerror() 获取详细错误信息
                        const char* err_str = ::dlerror();
                        std::string err_msg = "dlsym failed (z3yPluginInit not found). ";
                        if (err_str) err_msg += err_str;

                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, err_msg);
                    }
                    UnloadDynamicLibrary(lib_handle);
                    continue;
                }

                // 3. 执行入口点函数
                // (这部分逻辑与 Windows 
                //  完全相同)
                try
                {
                    {
                        std::lock_guard<std::mutex> lock(registry_mutex_);
                        current_loading_plugin_path_ = path_str;
                    }

                    init_func(this); // <-- 插件在此处调用 RegisterComponent

                    {
                        std::lock_guard<std::mutex> lock(registry_mutex_);
                        current_loading_plugin_path_ = "";
                        loaded_libs_.push_back(lib_handle);
                    }

                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadSuccessEvent>(path_str);
                    }
                }
                catch (const std::exception& e)
                {
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, e.what());
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
                catch (...)
                {
                    if (bus)
                    {
                        bus->FireGlobal<event::PluginLoadFailureEvent>(
                            path_str, "Unknown exception during init.");
                    }
                    UnloadDynamicLibrary(lib_handle);
                }
            } // end if .so/.dylib
        } // end for loop
    }

    /**
     * @brief 卸载所有已加载的插件并清空所有注册表。
     *
     * (POSIX 平台实现)
     *
     * @design
     * 此函数的代码与 platform_win.cpp
     * 中的实现完全相同。
     * 它调用的 UnloadDynamicLibrary()
     * 将被 C++ 链接器
     * 解析为此文件顶部
     * (匿名命名空间中)
     * 的 POSIX (dlclose) 版本。
     *
     * (包含了 "Fix 2"
     * 的所有安全修复)
     */
    void PluginManager::UnloadAllPlugins()
    {
        // 1. 获取一个指向自身的工厂函数
        auto this_ptr = std::static_pointer_cast<PluginManager>(shared_from_this());
        auto factory = [this_ptr]() -> PluginPtr<IComponent>
            {
                return PluginCast<IComponent>(this_ptr);
            };

        {
            // [Fix 2] (安全修复):
            // 同时锁定所有三个互斥锁
            std::scoped_lock lock(registry_mutex_, event_mutex_, queue_mutex_);

            // 2. 卸载 DLLs (按加载的相反顺序)
            std::reverse(loaded_libs_.begin(), loaded_libs_.end());
            for (LibHandle handle : loaded_libs_)
            {
                // [POSIX 调用]：
                // 调用此文件中的 UnloadDynamicLibrary (dlclose)
                UnloadDynamicLibrary(handle);
            }

            // 3. [Fix 2] (安全修复): 清空异步事件队列
            event_queue_ = {};

            // 4. [Fix 2] (安全修复): 清空 GC 队列
            gc_queue_ = {};

            // 5. 清空所有内部状态
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

#endif // !defined(_WIN32)