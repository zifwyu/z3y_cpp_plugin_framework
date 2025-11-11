/**
 * @file plugin_entry.cpp
 * @brief plugin_example 插件的入口点文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件定义了 z3yPluginInit 函数，
 * 它是 PluginManager 加载此 DLL 时调用的唯一入口点。
 * 它使用 z3y::RegisterComponent 和 z3y::RegisterService
 * 辅助函数来自动注册此插件提供的所有组件。
 */

 // 包含我们在此插件中实现的所有组件的头文件
#include "simple_impl_a.h"
#include "simple_impl_b.h"
#include "logger_service.h"

// 包含框架的注册接口和辅助工具
#include "framework/i_plugin_registry.h"
#include "framework/plugin_registration.h" 

// --- 定义 DLL 导出宏 ---
// 在 Windows 上，__declspec(dllexport) 会将函数导出
// 在 Linux/macOS 上，__attribute__((visibility("default"))) 作用相同
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

/**
 * @brief 插件的唯一入口点函数。
 *
 * @design
 * extern "C" 确保了函数名不会被 C++ 编译器“粉碎”(name mangling)，
 * 使得 GetProcAddress / dlsym 可以通过字符串 "z3yPluginInit" 找到它。
 *
 * @param[in] registry 宿主 PluginManager 传入的注册表接口指针。
 */
extern "C" PLUGIN_API void z3yPluginInit(z3y::IPluginRegistry* registry)
{
    if (!registry)
    {
        return;
    }

    // --- 注册 SimpleImplA (普通组件) ---
    // 注册 ClassID: clsid::kCSimpleA
    // 注册 别名: "Simple.A"
    z3y::RegisterComponent<z3y::SimpleImplA>(registry, "Simple.A");

    // --- 注册 SimpleImplB (普通组件) ---
    // 注册 ClassID: clsid::kCSimpleB
    // 注册 别名: "Simple.B"
    z3y::RegisterComponent<z3y::SimpleImplB>(registry, "Simple.B");

    // --- 注册 LoggerService (单例服务) ---
    // 注册 ClassID: clsid::kCLoggerService
    // 注册 别名: "Logger.Default"
    z3y::RegisterService<z3y::LoggerService>(registry, "Logger.Default");
}