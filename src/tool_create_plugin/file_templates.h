/**
 * @file file_templates.h
 * @brief 包含用于生成新插件的 C++ 原始字符串模板。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#ifndef Z3Y_TOOL_CREATE_PLUGIN_FILE_TEMPLATES_H_
#define Z3Y_TOOL_CREATE_PLUGIN_FILE_TEMPLATES_H_

#include <string>

namespace z3y
{
    namespace tool
    {
        namespace templates
        {
            // 模板使用 $$TOKEN$$ 作为替换占位符

            // 1. 接口头文件 (e.g., i_my_component.h)
            const std::string kInterfaceHeader = R"raw(/**
 * @file $$INTERFACE_FILENAME$$
 * @brief 定义 $$INTERFACE_NAME$$ 接口。
 * @author 孙鹏宇
 * @date $$DATE$$
 */

#pragma once

#ifndef $$INTERFACE_INCLUDE_GUARD$$
#define $$INTERFACE_INCLUDE_GUARD$$

#include "framework/i_component.h"

namespace z3y
{
    /**
     * @class $$INTERFACE_NAME$$
     * @brief 
     */
    class $$INTERFACE_NAME$$ : public IComponent
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~$$INTERFACE_NAME$$() = default;

        /**
         * @brief 示例业务函数，请修改。
         */
        virtual void MyFunction() = 0;
    };

} // namespace z3y

#endif // $$INTERFACE_INCLUDE_GUARD$$
)raw";


            // 2. 实现类头文件 (e.g., my_component_impl.h)
            const std::string kImplHeader = R"raw(/**
 * @file $$IMPL_FILENAME_H$$
 * @brief 定义 $$IMPL_CLASS_NAME$$ 实现类。
 * @author 孙鹏宇
 * @date $$DATE$$
 */

#pragma once

#ifndef $$IMPL_INCLUDE_GUARD_H$$
#define $$IMPL_INCLUDE_GUARD_H$$

#include "framework/plugin_impl.h"
#include "$$INTERFACE_PATH$$/$$INTERFACE_FILENAME$$"

namespace z3y
{
    /**
     * @brief $$IMPL_CLASS_NAME$$ 实现类的 ClassID。
     */
    namespace clsid
    {
        constexpr ClassID $$IMPL_CONST_NAME$$ =
            ConstexprHash("$$UUID$$");
    } // namespace clsid

    /**
     * @class $$IMPL_CLASS_NAME$$
     * @brief $$INTERFACE_NAME$$ 接口的具体实现。
     */
    class $$IMPL_CLASS_NAME$$ : public z3y::PluginImpl<$$IMPL_CLASS_NAME$$,
                                                 clsid::$$IMPL_CONST_NAME$$,
                                                 $$INTERFACE_NAME$$>
    {
    public:
        /**
         * @brief 构造函数。
         */
        $$IMPL_CLASS_NAME$$();

        /**
         * @brief 析构函数。
         */
        virtual ~$$IMPL_CLASS_NAME$$();

        // --- $$INTERFACE_NAME$$ 接口的实现 ---

        /**
         * @brief 示例业务函数实现。
         */
        void MyFunction() override;
    };

} // namespace z3y

#endif // $$IMPL_INCLUDE_GUARD_H$$
)raw";


            // 3. 实现类源文件 (e.g., my_component_impl.cpp)
            const std::string kImplSource = R"raw(/**
 * @file $$IMPL_FILENAME_CPP$$
 * @brief $$IMPL_CLASS_NAME$$ 实现类的源文件。
 * @author 孙鹏宇
 * @date $$DATE$$
 */

#include "$$IMPL_FILENAME_H$$"
#include <iostream>

namespace z3y
{

    /**
     * @brief 构造函数。
     */
    $$IMPL_CLASS_NAME$$::$$IMPL_CLASS_NAME$$()
    {
        std::cout << "[$$PLUGIN_NAME$$]: $$IMPL_CLASS_NAME$$ created." << std::endl;
    }

    /**
     * @brief 析构函数。
     */
    $$IMPL_CLASS_NAME$$::~$$IMPL_CLASS_NAME$$()
    {
        std::cout << "[$$PLUGIN_NAME$$]: $$IMPL_CLASS_NAME$$ destroyed." << std::endl;
    }

    /**
     * @brief 示例业务函数实现。
     */
    void $$IMPL_CLASS_NAME$$::MyFunction()
    {
        std::cout << "[$$PLUGIN_NAME$$]: $$IMPL_CLASS_NAME$$::MyFunction() called." << std::endl;
    }

} // namespace z3y
)raw";


            // 4. 插件入口文件 (plugin_entry.cpp)
            const std::string kPluginEntry = R"raw(/**
 * @file plugin_entry.cpp
 * @brief $$PLUGIN_NAME$$ 插件的入口点文件。
 * @author 孙鹏宇
 * @date $$DATE$$
 *
 * @details
 * 此文件定义了 z3yPluginInit 函数，
 * 它是 PluginManager 加载此 DLL 时调用的唯一入口点。
 */

#include "$$IMPL_FILENAME_H$$"
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

    // 注册 $$IMPL_CLASS_NAME$$
    // (您可以选择 RegisterComponent 或 RegisterService)
    z3y::RegisterComponent<z3y::$$IMPL_CLASS_NAME$$>(registry, "$$ALIAS$$");
}
)raw";


        } // namespace templates
    } // namespace tool
} // namespace z3y

#endif // Z3Y_TOOL_CREATE_PLUGIN_FILE_TEMPLATES_H_