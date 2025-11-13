/**
 * @file plugin_manager.cpp
 * @brief z3y::PluginManager 类的核心实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * 10. [修改]
 * FindComponentsImplementing
 * 以搜索
 * vector<InterfaceDetails>
 * 11. [FIX] [!!]
 * 修正了
 * Create()
 * 和 RegisterComponent()
 * 中对
 * PluginCast
 * 的内部调用，
 * 以匹配 2
 * 参数签名。
 */

#include "plugin_manager.h"
#include "framework/i_plugin_query.h"
#include <algorithm> // [新] 用于 std::find_if
#include <sstream>
#include <stdexcept>

 // [新增] 为析构函数中的 RAII 清理
 // 引入平台特定的库
#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace z3y {

    /**
     * @brief [工厂函数] 创建 PluginManager 的一个新实例。
     */
    PluginPtr<PluginManager> PluginManager::Create() {
        struct MakeSharedEnabler : public PluginManager {
            MakeSharedEnabler() : PluginManager() {}
        };

        PluginPtr<PluginManager> manager =
            std::make_shared<MakeSharedEnabler>();

        std::weak_ptr<PluginManager> weak_manager = manager;

        auto factory = [weak_manager]() -> PluginPtr<IComponent> {
            if (auto strong_manager = weak_manager.lock()) {
                // [!! 
                // 修复 !!] 
                // (
                // 
                // )
                // 
                // 
                // 
                // 
                InstanceError dummy_error;
                return PluginCast<IComponent>(strong_manager, dummy_error);
            }
            return nullptr;
            };

        // [修改] 
        // 调用 GetInterfaceDetails()
        auto iids = PluginManager::GetInterfaceDetails();

        // 1. [修改] 注册 IEventBus 服务
        manager->RegisterComponent(
            clsid::kEventBus,  // 使用 IEventBus 的服务 ID
            factory,
            true, "z3y.core.eventbus", iids);

        // 2. [新增] 注册 IPluginQuery 服务
        manager->RegisterComponent(
            clsid::kPluginQuery,      // 使用 IPluginQuery 的服务 ID
            factory,
            true, "z3y.core.pluginquery", iids);

        // 3. [可选] 注册 PluginManager "实现" 本身
        manager->RegisterComponent(
            PluginManager::kClsid,  // [修改] 
            // 使用 kClsid
            std::move(factory),
            true, "z3y.core.manager", iids);


        // 4. [修正]：
        // 启动事件循环工作线程
        manager->event_loop_thread_ =
            std::thread(&PluginManager::EventLoop, manager.get());

        // 5. 获取 IEventBus 接口 (现在使用自己的ID)
        try {
            auto bus = manager->GetService<IEventBus>(clsid::kEventBus);

            // 6. [修正]：使用正确的模板语法
            if (bus) {
                bus->FireGlobal<event::ComponentRegisterEvent>(
                    clsid::kEventBus, "z3y.core.eventbus", "internal.core",
                    true);
            }
        }
        catch (const PluginException&) {
            // 
            // 
            // 
            // 
            // 
        }

        return manager;
    }

    /**
     * @brief 默认构造函数（受保护）。
     */
    PluginManager::PluginManager() : running_(true) {}

    /**
     * @brief 析构函数。
     * [FIX]
     * (已完成)
     */
    PluginManager::~PluginManager() {
        // 1. 停止工作线程
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            running_ = false;
        }
        queue_cv_.notify_one();
        if (event_loop_thread_.joinable()) {
            event_loop_thread_.join();
        }

        // 2. 卸载所有插件并清空注册表
        {
            std::scoped_lock lock(registry_mutex_, event_mutex_, queue_mutex_);

            // [修正] 1. 
            event_queue_ = {};
            gc_queue_ = {};
            sender_subscribers_.clear();
            global_subscribers_.clear();
            singletons_.clear();
            components_.clear();
            alias_map_.clear();
            current_loading_plugin_path_.clear();
            global_sub_lookup_.clear();
            sender_sub_lookup_.clear();

            // [修正] 2. 
#ifdef _WIN32
            for (auto it = loaded_libs_.rbegin(); it != loaded_libs_.rend();
                ++it) {
                ::FreeLibrary(static_cast<HMODULE>(it->second));
            }
#else
            for (auto it = loaded_libs_.rbegin(); it != loaded_libs_.rend();
                ++it) {
                ::dlclose(it->second);
            }
#endif

            // [修正] 3. 
            loaded_libs_.clear();

        }  // [Fix 2] 释放所有三个锁
    }

    /**
     * @brief [FIX]
     * Shutdown()
     * 已被移除。
     * (已完成)
     */
     // void PluginManager::Shutdown() { ... }

     /**
      * @brief [IPluginRegistry 接口实现] 注册一个组件类。
      * [修改]
      * 更新签名为
      * vector<InterfaceDetails>
      */
    void PluginManager::RegisterComponent(
        ClassId clsid, FactoryFunction factory, bool is_singleton,
        const std::string& alias,
        std::vector<InterfaceDetails> implemented_interfaces) // [修改]
    {
        PluginPtr<IEventBus> bus;
        {
            std::lock_guard<std::mutex> lock(registry_mutex_);

            if (components_.count(clsid)) {
                std::string error_msg = "ClassId already registered. CLSID=0x";
                std::stringstream ss;
                ss << std::hex << clsid;
                error_msg += ss.str();
                if (!alias.empty()) {
                    error_msg += ", Alias='" + alias + "'";
                }
                throw std::runtime_error(error_msg);
            }

            // [修改]
            // 存储所有信息
            components_[clsid] = {
                std::move(factory),
                is_singleton,
                alias,
                current_loading_plugin_path_,
                std::move(implemented_interfaces)  // [修改]
            };

            if (!alias.empty()) {
                alias_map_[alias] = clsid;
            }

            // [FIX] [修改]
            // (已完成)
            if (running_) {
                // [!! 
                // 修复 !!] 
                // (
                // 
                // )
                InstanceError dummy_error;
                bus = PluginCast<IEventBus>(shared_from_this(), dummy_error);
            }
        }

        // 在锁释放后触发事件
        if (bus) {
            bus->FireGlobal<event::ComponentRegisterEvent>(
                clsid, alias, current_loading_plugin_path_, is_singleton);
        }
    }

    /**
     * @brief [内部] 通过别名查找 ClassId。
     */
    ClassId PluginManager::GetClsidFromAlias(const std::string& alias) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = alias_map_.find(alias);
        if (it != alias_map_.end()) {
            return it->second;
        }
        return 0;
    }

    // --- [修改] IPluginQuery 接口实现 ---

    std::vector<ComponentDetails> PluginManager::GetAllComponents() {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<ComponentDetails> details_list;
        details_list.reserve(components_.size());

        for (const auto& pair : components_) {
            details_list.push_back(ComponentDetails{
                pair.first, pair.second.alias, pair.second.is_singleton,
                pair.second.source_plugin_path,
                pair.second.implemented_interfaces // [修改] 
                // 直接复制
                });
        }
        return details_list;
    }

    bool PluginManager::GetComponentDetails(ClassId clsid,
        ComponentDetails& out_details) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = components_.find(clsid);
        if (it == components_.end()) {
            return false;
        }

        out_details = ComponentDetails{ it->first, it->second.alias,
                                       it->second.is_singleton,
                                       it->second.source_plugin_path,
                                       it->second.implemented_interfaces // [修改]
            // 直接复制
        };
        return true;
    }

    /**
     * @brief [IPluginQuery
     * 接口实现]
     * (已完成)
     */
    bool PluginManager::GetComponentDetailsByAlias(
        const std::string& alias,
        ComponentDetails& out_details) {

        ClassId clsid = GetClsidFromAlias(alias);
        if (clsid == 0) {
            return false;
        }
        return GetComponentDetails(clsid, out_details);
    }

    std::vector<ComponentDetails> PluginManager::FindComponentsImplementing(
        InterfaceId iid) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<ComponentDetails> details_list;

        for (const auto& pair : components_) {
            // [修改] 
            // 搜索 vector<InterfaceDetails>
            const auto& iids = pair.second.implemented_interfaces;

            // 使用 C++11 lambda 
            // 或 C++20 
            // 投影 (projection)
            auto it = std::find_if(iids.begin(), iids.end(),
                [iid](const InterfaceDetails& d) {
                    return d.iid == iid;
                });

            if (it != iids.end()) {
                details_list.push_back(ComponentDetails{
                    pair.first, pair.second.alias, pair.second.is_singleton,
                    pair.second.source_plugin_path, iids });
            }
        }
        return details_list;
    }

    std::vector<std::string> PluginManager::GetLoadedPluginFiles() {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<std::string> paths;
        paths.reserve(loaded_libs_.size());
        for (const auto& pair : loaded_libs_) {
            paths.push_back(pair.first);
        }
        return paths;
    }

    std::vector<ComponentDetails> PluginManager::GetComponentsFromPlugin(
        const std::string& plugin_path) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<ComponentDetails> details_list;

        for (const auto& pair : components_) {
            if (pair.second.source_plugin_path == plugin_path) {
                details_list.push_back(ComponentDetails{
                    pair.first, pair.second.alias, pair.second.is_singleton,
                    pair.second.source_plugin_path,
                    pair.second.implemented_interfaces // [修改]
                    // 直接复制
                    });
            }
        }
        return details_list;
    }

}  // namespace z3y