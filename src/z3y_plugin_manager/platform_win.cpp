/**
 * @file platform_win.cpp
 * @brief z3y::PluginManager 类的 Windows 平台特定实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * 16. [重构] [!!]
 * 新增
 * PlatformLoadLibrary,
 * GetFunction,
 * GetError
 * 的实现
 * 17. [FIX] [!!]
 * 新增 PlatformIsPluginFile()
 * 的实现
 * 18. [FIX] [!!]
 * PlatformLoadLibrary()
 * 不再检查扩展名
 * 19. [FIX] [!!]
 * PlatformGetError()
 * 现在统一使用 CP_UTF8
 * (
 * 配合 host
 * 的 SetConsoleOutputCP
 * )
 */

 // 仅在 Windows 平台上编译此文件
#ifdef _WIN32

#include "plugin_manager.h"
#include "framework/framework_events.h"
#include "framework/i_plugin_registry.h"
#include <Windows.h>
#include <algorithm>
#include <stdexcept>
#include <string> // [!! 
                  // 新增 !!] 
                  // 
                  // 
                  // 

namespace z3y {
    // --- 平台特定的辅助函数 (Platform-Specific Helpers) ---
    namespace {
        /**
         * @brief [Win32] 加载动态链接库。
         */
        HMODULE LoadDynamicLibrary(const std::filesystem::path& path) {
            // 使用 W 版 (Unicode) API
            return ::LoadLibraryW(path.c_str());
        }

        /**
         * @brief [Win32] 获取函数地址。
         */
        FARPROC GetFunctionAddress(HMODULE lib_handle, const char* func_name) {
            return ::GetProcAddress(lib_handle, func_name);
        }

        /**
         * @brief [Win32] 卸载动态链接库。
         */
        void UnloadDynamicLibrary(HMODULE lib_handle) {
            ::FreeLibrary(lib_handle);
        }

        /**
         * @brief 插件入口点函数的签名。
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
     * (Windows 平台实现)
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
                // [修改]
                // 将所有加载逻辑委托给内部辅助函数
                LoadPluginInternal(entry.path(), init_func_name);
            }
        }
        else {
            // [修改] 接口 2: 非递归扫描
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
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
     * Windows
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
            UnloadDynamicLibrary(static_cast<HMODULE>(it->second));
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
     * Windows
     * 平台的文件检查
     */
    bool PluginManager::PlatformIsPluginFile(const std::filesystem::path& path)
    {
        return std::filesystem::is_regular_file(path) &&
            path.extension() == ".dll";
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
        return ::LoadLibraryW(path.c_str());
    }

    void* PluginManager::PlatformGetFunction(
        LibHandle handle, const char* func_name)
    {
        return ::GetProcAddress(static_cast<HMODULE>(handle), func_name);
    }

    void PluginManager::PlatformUnloadLibrary(LibHandle handle)
    {
        ::FreeLibrary(static_cast<HMODULE>(handle));
    }

    std::string PluginManager::PlatformGetError()
    {
        // 
        // 
        // 
        DWORD error_id = ::GetLastError();
        if (error_id == 0) {
            return "No error (GetLastError() returned 0)";
        }

        LPWSTR buffer = nullptr;
        // 
        // 
        // 
        size_t size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&buffer, 0, NULL);

        if (size == 0) {
            return "Unknown error (FormatMessage failed)";
        }

        // 
        // 
        // 
        std::wstring w_msg(buffer, size);
        LocalFree(buffer);

        // [!! 
        // 修复 !!] 
        // 
        // 
        // 
        // CP_ACP
        // 
        // 
        // CP_UTF8
        int out_size = WideCharToMultiByte(CP_UTF8, 0, w_msg.c_str(), (int)w_msg.length(),
            NULL, 0, NULL, NULL);
        if (out_size == 0) {
            return "Failed to convert error message to UTF-8";
        }

        std::string msg(out_size, 0);
        WideCharToMultiByte(CP_UTF8, 0, w_msg.c_str(), (int)w_msg.length(),
            &msg[0], out_size, NULL, NULL);

        return msg;
    }


}  // namespace z3y

#endif  // _WIN32