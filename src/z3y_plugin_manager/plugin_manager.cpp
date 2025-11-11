/**
 * @file plugin_manager.cpp
 * @brief z3y::PluginManager 类的核心实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. 实现了新的 RegisterComponent(..., alias) 签名。
 * 2. 移除了 RegisterAlias()。
 * 3. Create() 函数现在调用新的 RegisterComponent。
 * 4. RegisterComponent 现在触发*一个*包含所有信息的事件。
 */

#include "plugin_manager.h"
#include <stdexcept>

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

        // --- 引导 (Bootstrap) 核心服务 ---
        auto factory = [manager]() -> PluginPtr<IComponent>
            {
                return PluginCast<IComponent>(manager);
            };

        // [修正]：调用新的 RegisterComponent 签名，一次性注册所有信息
        manager->RegisterComponent(
            clsid::kEventBus,
            std::move(factory),
            true,                       // is_singleton = true
            "z3y.core.eventbus"         // alias
        );

        return manager;
    }

    /**
     * @brief 默认构造函数（受保护）。
     */
    PluginManager::PluginManager()
    {
    }

    /**
     * @brief 析构函数。
     */
    PluginManager::~PluginManager()
    {
        UnloadAllPlugins();
    }

    /**
     * @brief [IPluginRegistry 接口实现] 注册一个组件类。
     * [已修正]：这是新的核心注册函数。
     */
    void PluginManager::RegisterComponent(ClassID clsid,
        FactoryFunction factory,
        bool is_singleton,
        const std::string& alias)
    {
        PluginPtr<IEventBus> bus;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (factories_.count(clsid))
            {
                throw std::runtime_error("ClassID already registered.");
            }

            // 1. 注册工厂
            factories_[clsid] = { std::move(factory), is_singleton };

            // 2. 注册别名 (如果提供了)
            if (!alias.empty())
            {
                alias_map_[alias] = clsid;
            }

            // 3. 获取事件总线 (自己)
            bus = PluginCast<IEventBus>(shared_from_this());
        }

        // 4. [修正]：在锁释放后，触发 *一个* 包含所有信息的事件
        if (bus)
        {
            bus->FireGlobal(event::ComponentRegisterEvent(
                clsid, alias, current_loading_plugin_path_, is_singleton));
        }
    }

    // [已移除]：RegisterAlias() 函数已被合并

    /**
     * @brief [内部] 通过别名查找 ClassID。
     */
    ClassID PluginManager::GetClsidFromAlias(const std::string& alias)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = alias_map_.find(alias);
        if (it != alias_map_.end())
        {
            return it->second;
        }
        return 0; // 0 是一个无效的 ClassID
    }

} // namespace z3y