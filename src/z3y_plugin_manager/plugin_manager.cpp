/**
 * @file plugin_manager.cpp
 * @brief z3y::PluginManager 类的核心实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 */

#include "plugin_manager.h"
#include "framework/i_plugin_query.h"
#include "framework/framework_events.h" // [!! 
 // 新增 !!]
#include <algorithm> // [新] 用于 std::find_if
#include <sstream>
#include <stdexcept>
#include <unordered_map> // [!! 新增 !!] 用于 PluginManager::* 的实现

 // [!! 
 // 重构 !!] 
 // 
 // 
 // 
 // (
 // 
 // 
 // )
 //#ifdef _WIN32
 //#include <Windows.h>
 //#else
 //#include <dlfcn.h>
 //#endif

namespace z3y {

    // [!! 
    // 重构 !!] 
    // 
    // 
    // 
    // 
    // 
    using PluginInitFunc = void(IPluginRegistry*);


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
                // (
                // 
                // 
                // )
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
            true, "z3y.core.eventbus", iids,
            true // [!! 
                 // 修复 !!] 
                 // 
                 // 
                 // 
                 // 
                 // 
                 // 
        );

        // 2. [新增] 注册 IPluginQuery 服务
        manager->RegisterComponent(
            clsid::kPluginQuery,      // 使用 IPluginQuery 的服务 ID
            factory,
            true, "z3y.core.pluginquery", iids,
            false // [!! 
                  // 修复 !!] 
                  // 
                  // 
                  // 
                  // 
                  // 
        );

        // 3. [可选] 注册 PluginManager "实现" 本身
        manager->RegisterComponent(
            PluginManager::kClsid,  // [修改] 
            // 使用 kClsid
            std::move(factory),
            true, "z3y.core.manager", iids,
            false // 
                  // 
                  // 
        );


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
    PluginManager::PluginManager()
        : running_(true), current_added_components_(nullptr), event_trace_hook_(nullptr) { // [修改] 初始化 event_trace_hook_
    }

    /**
     * @brief 析构函数。
     * [!!
     * 重构 !!]
     * (
     * * * * )
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

        // 2. [!! 
        //    重构 !!] 
        //    调用共享的清理函数
        ClearAllRegistries();
    }

    /**
     * @brief [!! 新增 !!] 设置事件追踪钩子。
     */
    void PluginManager::SetEventTraceHook(std::function<EventTraceHook> hook)
    {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);
        event_trace_hook_ = std::move(hook);
    }

    /**
     * @brief [!!
     * 重构 !!]
     * 共享的核心清理函数
     * (
     * 平台无关
     * )
     */
    void PluginManager::ClearAllRegistries()
    {
        // 
        // 
        // 
        // 
        // 
        std::scoped_lock lock(registry_mutex_, event_mutex_, queue_mutex_);

        // [修正] 1. 
        event_queue_ = {};
        gc_queue_ = {};
        sender_subscribers_.clear();
        global_subscribers_.clear();
        global_sub_lookup_.clear();
        sender_sub_lookup_.clear();

        // [修正] 2. 
        // Note: singletons_, components_, alias_map_, default_map_ are now unordered_map.
        singletons_.clear();
        components_.clear();
        alias_map_.clear();
        default_map_.clear();
        loaded_libs_.clear(); // loaded_libs_ is now unordered_map
        current_loading_plugin_path_.clear();
        current_added_components_ = nullptr;

        // [新增] 3. 清理 Hook
        event_trace_hook_ = nullptr;

        // [修正] 4. [!! 
        //    重构 !!] 
        //    调用平台相关的卸载
        PlatformSpecificLibraryUnload();
        // (
        // 
        // 
        // 
        // 
        // )
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
      * 实现了 is_default
      * 冲突检测和
      * * * 事务性追踪
      * *
      */
    void PluginManager::RegisterComponent(
        ClassId clsid, FactoryFunction factory, bool is_singleton,
        const std::string& alias,
        std::vector<InterfaceDetails> implemented_interfaces, // [修改]
        bool is_default) // [!! 
        // 新增 !!]
    {
        PluginPtr<IEventBus> bus;
        {
            // Note: components_ is now unordered_map.
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

            // [!! 
            // 新增 !!] 
            // 
            // 
            // 
            if (is_default) {
                for (const auto& iface : implemented_interfaces) {
                    // 
                    // 
                    // 
                    // 
                    if (iface.iid == IComponent::kIid) {
                        continue;
                    }
                    // Note: default_map_ is now unordered_map.
                    auto it = default_map_.find(iface.iid);
                    if (it != default_map_.end()) {
                        // 
                        // 
                        // 
                        std::stringstream ss_old, ss_new;
                        ss_old << std::hex << it->second;
                        ss_new << std::hex << clsid;

                        throw std::runtime_error(
                            "Default implementation conflict: Interface '" + iface.name +
                            "' (IID 0x" + std::to_string(iface.iid) +
                            ") already has a default (CLSID: 0x" + ss_old.str() +
                            "). Cannot register new default (CLSID: 0x" + ss_new.str() + ")."
                        );
                    }
                    // 
                    // 
                    default_map_[iface.iid] = clsid;
                }
            }


            // [修改]
            // 存储所有信息
            components_[clsid] = {
                std::move(factory),
                is_singleton,
                alias,
                current_loading_plugin_path_,
                std::move(implemented_interfaces),  // [修改]
                is_default // [!! 
                           // 新增 !!] 
                           // 
                           // 
                           // 
            };

            // [!! 
            // 新增 !!] 
            // 
            // 
            // 
            if (current_added_components_) {
                current_added_components_->push_back(clsid);
            }
            // Note: alias_map_ is now unordered_map.
            if (!alias.empty()) {
                alias_map_[alias] = clsid;
            }

            // [FIX] [修改]
            // (已完成)
            if (running_) {
                // (
                // 
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
     * @brief [!!
     * 新增 !!]
     * 事务性回滚
     * (
     * 在加载失败时调用
     * )
     */
    void PluginManager::RollbackRegistrations(const std::vector<ClassId>& clsid_list)
    {
        // Note: components_, alias_map_, default_map_, singletons_ are now unordered_map.
        std::lock_guard<std::mutex> lock(registry_mutex_);

        for (const ClassId clsid : clsid_list)
        {
            auto it = components_.find(clsid);
            if (it == components_.end()) {
                continue;
            }

            const ComponentInfo& info = it->second;

            // 1. 
            // 
            // 
            if (!info.alias.empty()) {
                alias_map_.erase(info.alias);
            }

            // 2. 
            // 
            // 
            // [!! 
            // 修改 !!] (
            // 
            // 
            // )
            if (info.is_default_registration) {
                for (const auto& iface : info.implemented_interfaces) {
                    auto default_it = default_map_.find(iface.iid);
                    if (default_it != default_map_.end() && default_it->second == clsid) {
                        default_map_.erase(default_it);
                    }
                }
            }

            // 3. 
            // 
            // 
            // (
            // 
            // 
            // )
            singletons_.erase(clsid);

            // 4. 
            // 
            // 
            components_.erase(it);
        }
    }


    /**
     * @brief [内部] 通过别名查找 ClassId。
     */
    ClassId PluginManager::GetClsidFromAlias(const std::string& alias) {
        // Note: alias_map_ is now unordered_map.
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = alias_map_.find(alias);
        if (it != alias_map_.end()) {
            return it->second;
        }
        return 0;
    }


    /**
     * @brief [!!
     * 重构 !!]
     * 平台无关的核心加载逻辑
     * (
     * 原 platform_*.cpp
     * 中的
     * LoadPluginInternal
     * )
     */
    bool PluginManager::LoadPluginInternal(
        const std::filesystem::path& file_path,
        const std::string& init_func_name) {

        // 1. [!! 
        //    FIX !!] 
        //    
        // 
        // 
        // 
        if (!PlatformIsPluginFile(file_path)) {
            return false; // 
            // 
            // 
        }

        PluginPtr<IEventBus> bus;
        try {
            bus = GetService<IEventBus>(clsid::kEventBus);
        }
        catch (const PluginException&) {
            /* 在加载早期阶段 bus
            可能不存在，
            忽略
            */
        }

        std::string path_str = file_path.string();

        // 2. [!! 
        //    重构 !!] 
        //    调用平台抽象
        LibHandle lib_handle = PlatformLoadLibrary(file_path);
        if (!lib_handle) {
            if (bus) {
                bus->FireGlobal<event::PluginLoadFailureEvent>(
                    path_str, "LoadLibrary failed: " + PlatformGetError());
            }
            return false;
        }

        // 3. [!! 
        //    重构 !!] 
        //    调用平台抽象
        PluginInitFunc* init_func = reinterpret_cast<PluginInitFunc*>(
            PlatformGetFunction(lib_handle, init_func_name.c_str()));

        if (!init_func) {
            if (bus) {
                bus->FireGlobal<event::PluginLoadFailureEvent>(
                    path_str,
                    "GetProcAddress failed (z3yPluginInit not found): " + PlatformGetError());
            }
            PlatformUnloadLibrary(lib_handle);
            return false;
        }

        // 
        // 
        // 
        // 
        // 
        std::vector<ClassId> added_components_this_session;

        // 4. 
        // 
        // 
        // (
        // 
        // 
        // )
        try {
            {
                // Note: loaded_libs_ is now unordered_map.
                std::lock_guard<std::mutex> lock(registry_mutex_);
                current_loading_plugin_path_ = path_str;
                current_added_components_ = &added_components_this_session;
            }

            init_func(this);  // <-- 插件在此处调用 RegisterComponent

            // 加载成功
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                current_loading_plugin_path_ = "";
                current_added_components_ = nullptr;
                loaded_libs_[path_str] = lib_handle;
            }

            if (bus) {
                bus->FireGlobal<event::PluginLoadSuccessEvent>(path_str);
            }

            return true;
        }
        catch (const std::exception& e) {
            // init_func 抛出异常
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                current_loading_plugin_path_ = "";
                current_added_components_ = nullptr;
            }

            RollbackRegistrations(added_components_this_session);

            if (bus) {
                bus->FireGlobal<event::PluginLoadFailureEvent>(path_str,
                    e.what());
            }
            PlatformUnloadLibrary(lib_handle);
            return false;
        }
        catch (...) {
            // init_func 抛出未知异常
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                current_loading_plugin_path_ = "";
                current_added_components_ = nullptr;
            }

            RollbackRegistrations(added_components_this_session);

            if (bus) {
                bus->FireGlobal<event::PluginLoadFailureEvent>(
                    path_str, "Unknown exception during init.");
            }
            PlatformUnloadLibrary(lib_handle);
            return false;
        }
    }


    // --- [修改] IPluginQuery 接口实现 ---

    std::vector<ComponentDetails> PluginManager::GetAllComponents() {
        // Note: components_ is now unordered_map.
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<ComponentDetails> details_list;
        details_list.reserve(components_.size());

        for (const auto& pair : components_) {
            details_list.push_back(ComponentDetails{
                pair.first, pair.second.alias, pair.second.is_singleton,
                pair.second.source_plugin_path,
                pair.second.is_default_registration, // [!! 
                // 新增 !!]
pair.second.implemented_interfaces
                });
        }
        return details_list;
    }

    bool PluginManager::GetComponentDetails(ClassId clsid,
        ComponentDetails& out_details) {
        // Note: components_ is now unordered_map.
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = components_.find(clsid);
        if (it == components_.end()) {
            return false;
        }

        out_details = ComponentDetails{ it->first, it->second.alias,
                                       it->second.is_singleton,
                                       it->second.source_plugin_path,
                                       it->second.is_default_registration, // [!! 
            // 新增 !!]
it->second.implemented_interfaces
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
        // Note: components_ is now unordered_map.
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
                    pair.second.source_plugin_path,
                    pair.second.is_default_registration, // [!! 
                    // 新增 !!]
iids });
            }
        }
        return details_list;
    }

    std::vector<std::string> PluginManager::GetLoadedPluginFiles() {
        // Note: loaded_libs_ is now unordered_map.
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
        // Note: components_ is now unordered_map.
        std::lock_guard<std::mutex> lock(registry_mutex_);
        std::vector<ComponentDetails> details_list;

        for (const auto& pair : components_) {
            if (pair.second.source_plugin_path == plugin_path) {
                details_list.push_back(ComponentDetails{
                    pair.first, pair.second.alias, pair.second.is_singleton,
                    pair.second.source_plugin_path,
                    pair.second.is_default_registration, // [!! 
                    // 新增 !!]
pair.second.implemented_interfaces
                    });
            }
        }
        return details_list;
    }

}  // namespace z3y