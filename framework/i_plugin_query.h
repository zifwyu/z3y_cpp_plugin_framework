/**
 * @file i_plugin_query.h
 * @brief 定义 z3y::IPluginQuery 接口，用于框架的内省和
 * 服务发现。
 * @author (您的名字)
 * @date 2025-11-12
 *
 * [修改]
 * 1. [新]
 * 定义 InterfaceDetails
 * 结构体。
 * 2. [修改]
 * ComponentDetails
 * 现在返回
 * vector<InterfaceDetails>
 * 而不是
 * vector<InterfaceId>。
 * 3. [修改]
 * 使用 Z3Y_DEFINE_INTERFACE
 * 宏。
 * 4. [修改]
 * (已完成)
 * 增加 GetComponentDetailsByAlias
 * 接口
 * 5. [修改] [!!]
 * 新增 InterfaceVersion
 * 结构体，
 * 提升清晰度
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_PLUGIN_QUERY_H_
#define Z3Y_FRAMEWORK_I_PLUGIN_QUERY_H_

#include "framework/i_component.h"
#include "framework/class_id.h"
#include "framework/interface_helpers.h" // [新]
#include <string>
#include <vector>

namespace z3y {

    namespace clsid {
        /**
         * @brief [新增] 框架核心插件查询服务的 "服务ID"。
         * 宿主程序使用此 ID 来 GetService。
         */
        constexpr ClassId kPluginQuery =
            ConstexprHash("z3y-core-plugin-query-SERVICE-UUID");
    }  // namespace clsid

    /**
     * @struct InterfaceVersion
     * @brief [新]
     * 用于清晰地封装接口的版本号。
     */
    struct InterfaceVersion {
        uint32_t major;     //!< 接口主版本号
        uint32_t minor;     //!< 接口次版本号
        // 
        // (
        // 
        // 
        // )
    };


    /**
     * @struct InterfaceDetails
     * @brief [新]
     * 描述一个已注册接口的详细信息。
     */
    struct InterfaceDetails {
        InterfaceId iid;      //!< 接口ID (uint64_t)，用于 PluginCast
        std::string name;     //!< 
        //!< 接口的字符串名称 (
        //!< 例如 "ISimple")

        /**
         * @brief [修改]
         * 插件实现的接口版本
         * (vMajor.vMinor)
         */
        InterfaceVersion version;
    };

    /**
     * @struct ComponentDetails
     * @brief 描述一个已注册组件（实现类）的详细信息。
     * (遵从 Google 风格：struct 成员变量使用 lower_case)
     */
    struct ComponentDetails {
        ClassId clsid;                    //!< 实现类的 ClassId
        std::string alias;                //!< 注册的别名 (例如 "Simple.A")
        bool is_singleton;                //!< 是否为单例服务
        std::string source_plugin_path;   //!< 注册此组件的插件 DLL/SO
        //!< 的路径

        /**
         * @brief [修改]
         * 此组件实现的所有接口 (Id,
         * Name,
         * * 和版本
         * )
         * 列表
         */
        std::vector<InterfaceDetails> implemented_interfaces;
    };

    /**
     * @class IPluginQuery
     * @brief [框架核心] 插件注册表查询接口。
     *
     * [修改] 必须使用 public virtual
     * 继承以解决钻石问题
     */
    class IPluginQuery : public virtual IComponent {  // [修改]
    public:
        /**
         * @brief [修改]
         * 使用 Z3Y_DEFINE_INTERFACE
         * 宏
         */
         // (已在上一轮修复)
        Z3Y_DEFINE_INTERFACE(IPluginQuery, "z3y-core-IPluginQuery-IID-A0000003", 1, 0)

            /**
             * @brief 获取所有已注册组件的详细信息。
             */
            virtual std::vector<ComponentDetails> GetAllComponents() = 0;

        /**
         * @brief 获取特定 ClassId 的组件的详细信息。
         * @param[in] clsid 要查询的组件 ClassId。
         * @param[out] out_details 如果找到，
         * 用于填充详细信息的结构体。
         * @return true 如果找到该 clsid，否则
         * false。
         */
        virtual bool GetComponentDetails(ClassId clsid,
            ComponentDetails& out_details) = 0;

        /**
         * @brief [新增]
         * 获取特定别名 (Alias)
         * 的组件的详细信息。
         * (已完成)
         */
        virtual bool GetComponentDetailsByAlias(const std::string& alias,
            ComponentDetails& out_details) = 0;

        /**
         * @brief 查找所有实现了特定接口 (Iid) 的组件。
         *
         * @param[in] iid
         * 要查询的接口
         * ID (例如 ISimple::kIid)。
         * @return
         * 实现了该接口的所有组件的
         * ComponentDetails 列表。
         */
        virtual std::vector<ComponentDetails> FindComponentsImplementing(
            InterfaceId iid) = 0;

        /**
         * @brief 获取所有已成功加载的插件文件（DLL/SO）的路径列表。
         */
        virtual std::vector<std::string> GetLoadedPluginFiles() = 0;

        /**
         * @brief 获取由特定插件文件注册的所有组件。
         *
         * @param[in] plugin_path
         * 插件文件的完整路径
         * (与 GetLoadedPluginFiles()
         * 返回的字符串匹配)。
         * @return
         * 该插件注册的所有组件的
         * ComponentDetails 列表。
         */
        virtual std::vector<ComponentDetails> GetComponentsFromPlugin(
            const std::string& plugin_path) = 0;
    };

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_I_PLUGIN_QUERY_H_