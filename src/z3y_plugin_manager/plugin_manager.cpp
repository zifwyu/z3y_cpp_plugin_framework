/**
 * @file plugin_manager.cpp
 * @brief z3y::PluginManager 类的核心实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. 修正了 FireGlobal 的调用语法，
 * 现在使用 FireGlobal<TEvent>(...args)
 * 2. （死锁修复）使用了 registry_mutex_。
 * 3. 移除了 RegisterAlias。
 */

#include "plugin_manager.h"
#include <stdexcept>
#include <sstream>

namespace z3y
{

    /**
     * @brief [工厂函数] 创建 PluginManager 的一个新实例。
     */
    PluginPtr<PluginManager> PluginManager::Create()
    {
        struct MakeSharedEnabler : public PluginManager
        {
            MakeSharedEnabler() : PluginManager()
            {
            }
        };

        PluginPtr<PluginManager> manager = std::make_shared<MakeSharedEnabler>();

        auto factory = [manager]() -> PluginPtr<IComponent>
            {
                return PluginCast<IComponent>(manager);
            };

        // 1. 注册 IEventBus (clsid::kEventBus)
        manager->RegisterComponent(
            clsid::kEventBus,
            std::move(factory),
            true,
            "z3y.core.eventbus"
        );

        // 2. [修正]：
        // 启动事件循环工作线程
        manager->event_loop_thread_ = std::thread(&PluginManager::EventLoop, manager.get());

        // 3. 获取 IEventBus 接口
        auto bus = manager->GetService<IEventBus>(clsid::kEventBus);

        // 4. [修正]：使用正确的模板语法
        if (bus)
        {
            bus->FireGlobal<event::ComponentRegisterEvent>(
                clsid::kEventBus,
                "z3y.core.eventbus",
                "internal.core",
                true);
        }

        return manager;
    }

    /**
     * @brief 默认构造函数（受保护）。
     */
    PluginManager::PluginManager() : running_(true)
    {
    }

    /**
     * @brief 析构函数。
     */
    PluginManager::~PluginManager()
    {
        // 停止工作线程
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            running_ = false;
        }
        queue_cv_.notify_one();
        if (event_loop_thread_.joinable())
        {
            event_loop_thread_.join();
        }

        UnloadAllPlugins();
    }

    /**
     * @brief [IPluginRegistry 接口实现] 注册一个组件类。
     */
    void PluginManager::RegisterComponent(ClassID clsid,
        FactoryFunction factory,
        bool is_singleton,
        const std::string& alias)
    {
        PluginPtr<IEventBus> bus;
        {
            std::lock_guard<std::mutex> lock(registry_mutex_);

            if (factories_.count(clsid))
            {
                std::string error_msg = "ClassID already registered. CLSID=0x";

                // 将 uint64_t 转换为十六进制字符串
                std::stringstream ss;
                ss << std::hex << clsid;
                error_msg += ss.str();

                if (!alias.empty())
                {
                    error_msg += ", Alias='" + alias + "'";
                }

                throw std::runtime_error(error_msg);
            }

            factories_[clsid] = { std::move(factory), is_singleton };

            if (!alias.empty())
            {
                alias_map_[alias] = clsid;
            }

            bus = PluginCast<IEventBus>(shared_from_this());
        }

        // 在锁释放后触发事件
        if (bus)
        {
            // [修正]：使用正确的模板语法
            bus->FireGlobal<event::ComponentRegisterEvent>(
                clsid, alias, current_loading_plugin_path_, is_singleton);
        }
    }

    /**
     * @brief [内部] 通过别名查找 ClassID。
     */
    ClassID PluginManager::GetClsidFromAlias(const std::string& alias)
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = alias_map_.find(alias);
        if (it != alias_map_.end())
        {
            return it->second;
        }
        return 0;
    }

} // namespace z3y