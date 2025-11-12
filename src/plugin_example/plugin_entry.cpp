/**
 * @file plugin_entry.cpp
 * @brief plugin_example 插件的入口点文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1. [修改]
 * 更新注册的类以使用
 * z3y::example
 * 命名空间
 */

 // 包含我们在此插件中实现的所有组件的头文件
#include "simple_impl_a.h"
#include "simple_impl_b.h"
#include "logger_service.h"

// 包含框架的注册接口和辅助工具
#include "framework/i_plugin_registry.h"
#include "framework/plugin_registration.h" 

// --- 定义 DLL 导出宏 ---
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

/**
 * @brief 插件的唯一入口点函数。
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
    // [修改] 
    // 使用 z3y::example 
    // 命名空间
    z3y::RegisterComponent<z3y::example::SimpleImplA>(registry, "Simple.A");

    // --- 注册 SimpleImplB (普通组件) ---
    // [修改] 
    // 使用 z3y::example 
    // 命名空间
    z3y::RegisterComponent<z3y::example::SimpleImplB>(registry, "Simple.B");

    // --- 注册 LoggerService (单例服务) ---
    // [修改] 
    // 使用 z3y::example 
    // 命名空间
    z3y::RegisterService<z3y::example::LoggerService>(registry, "Logger.Default");
}