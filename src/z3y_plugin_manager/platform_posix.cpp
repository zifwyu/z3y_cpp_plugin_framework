/**
 * @file platform_posix.cpp
 * @brief z3y::PluginManager 类的 POSIX (Linux/macOS) 平台特定实现。
 * @author 孙鹏宇 (Adapted for POSIX)
 * @date 2025-11-10
 *
 * ...
 * 15. [重构] [!!]
 * 新增 PlatformSpecificLibraryUnload()
 * 的实现
 * 16. [重构] [!!]
 * LoadPluginInternal()
 * 已被移除 (
 * 移至 plugin_manager.cpp
 * )
 * 17. [重构] [!!]
 * 新增
 * PlatformLoadLibrary,
 * GetFunction,
 * GetError
 * 的实现
 * 18. [FIX] [!!]
 * 新增 PlatformIsPluginFile()
 * 的实现
 * 19. [FIX] [!!]
 * PlatformLoadLibrary()
 * 不再检查扩展名
 */

 // 仅在非 Windows 平台上编译此文件
#if !defined(_WIN32)

#include "plugin_manager.h"
#include "framework/framework_events.h"
#include "framework/i_plugin_registry.h"
#include <dlfcn.h>  // POSIX 动态库头文件
#include <algorithm>
#include <stdexcept>
#include <string> // [!! 
                  // 新增 !!]

namespace z3y {
    // --- 平台特定的辅助函数 (Platform-Specific Helpers) ---
    namespace {
        /**
         * @brief [POSIX] 加载动态链接库 (.so / .dylib)。
         */
        void* LoadDynamicLibrary(const std::filesystem::path& path) {
            return ::dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        }

        /**
         * @brief [POSIX] 获取函数地址。
         */
        void* GetFunctionAddress(void* lib_handle, const char* func_name) {
            return ::dlsym(lib_handle, func_name);
        }

        /**
         * @brief [POSIX] 卸载动态链接库。
         */
        void UnloadDynamicLibrary(void* lib_handle) {
            ::dlclose(lib_handle);
        }

        /**
         * @brief 插件入口点函数的签名
         */
         // [!! 
         // 重构 !!] 
         // 
         // 
         // 
         // 
         // 
         // 
         // 
         // using PluginInitFunc = void(IPluginRegistry*);

    }  // 匿名命名空间


    /**
     * @brief [修改]
     * 扫描指定目录(及其子目录)并加载所有插件。
     * (POSIX 平台实现)
     */
    void PluginManager::LoadPluginsFromDirectory(
        const std::filesystem::path& dir, bool recursive,
        const std::string& init_func_name) {
        if (!std::filesystem::exists(dir) ||
            !std::filesystem::is_directory(dir)) {
            return;
        }

        if (recursive) {
            // [修改] 接口 1: 递归扫描
            for (const auto& entry :
                std::filesystem::recursive_directory_iterator(dir)) {
                LoadPluginInternal(entry.path(), init_func_name);
            }
        }
        else {
            // [修改] 接口 2: 非递归扫描
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                LoadPluginInternal(entry.path(), init_func_name);
            }
        }
    }

    /**
     * @brief [新]
     * 加载一个指定的插件 DLL/SO 文件。
     * (POSIX 平台实现)
     */
    bool PluginManager::LoadPlugin(const std::filesystem::path& file_path,
        const std::string& init_func_name) {
        // [修改] 接口 3:
        // 委托给内部辅助函数
        return LoadPluginInternal(file_path, init_func_name);
    }

    /**
     * @brief [!!
     * 重构 !!]
     * LoadPluginInternal()
     * 已被移除，
     * 平台无关逻辑
     * * 已移至
     * plugin_manager.cpp
     * *
     */


     // [!! 
     // 修复 B.1 !!] 
     // 
     // UnloadAllPlugins() 
     // 的平台无关实现
     // * 已移至
     // plugin_manager.cpp
     // *
     // 


    /**
     * @brief [!!
     * 重构 !!]
     * 平台相关的库卸载 (
     * POSIX
     * )
     * (
     * 由 ClearAllRegistries
     * 调用
     * )
     */
    void PluginManager::PlatformSpecificLibraryUnload()
    {
        // 
        // 
        // 
        // 

        // 1. 
        // 
        // 
        for (auto it = loaded_libs_.rbegin(); it != loaded_libs_.rend();
            ++it) {
            UnloadDynamicLibrary(it->second);
        }

        // 2. 
        // 
        // 
        loaded_libs_.clear();
    }

    // --- [!! 
    // 
    // 
    // 
    // 
    // 
    // 
    // !!] ---

    /**
     * @brief [!!
     * 新增 !!]
     * POSIX
     * 平台的文件检查
     */
    bool PluginManager::PlatformIsPluginFile(const std::filesystem::path& path)
    {
        if (!std::filesystem::is_regular_file(path)) {
            return false;
        }
        const auto extension = path.extension();
        return (extension == ".so" || extension == ".dylib");
    }

    PluginManager::LibHandle PluginManager::PlatformLoadLibrary(
        const std::filesystem::path& path)
    {
        // [!! 
        // 修复 !!] 
        // 
        // 
        // 
        // (
        // 
        // 
        // 
        // )
        return ::dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
    }

    void* PluginManager::PlatformGetFunction(
        LibHandle handle, const char* func_name)
    {
        return ::dlsym(handle, func_name);
    }

    void PluginManager::PlatformUnloadLibrary(LibHandle handle)
    {
        ::dlclose(handle);
    }

    std::string PluginManager::PlatformGetError()
    {
        const char* err_str = ::dlerror();
        if (err_str) {
            return std::string(err_str);
        }
        return "Unknown POSIX dynamic library error";
    }


}  // namespace z3y

#endif  // !defined(_WIN32)