/**
 * @file main.cpp
 * @brief 插件脚手架 (Scaffolding) 工具的主程序。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. ...
 * 2. [修改]
 * 更新令牌 (Token)
 * 生成逻辑以适应新的宏和命名空间设计。
 * 3. [修改]
 * 现在生成两个 UUID (
 * 接口一个，
 * 实现一个
 * )。
 * 4. [修改]
 * 移除了已废弃的 IMPL_CONST_NAME
 * (kCMyClassName)
 * 令牌。
 * 5. [重构] [!!]
 * 所有辅助函数已移入匿名命名空间。
 */

#include "uuid_gen.h"
#include "file_templates.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm> // 用于 std::transform
#include <cctype>    // 用于 ::tolower, ::toupper
#include <ctime>     // 用于 std::time_t, std::tm, std::strftime


namespace { // [!! 
    // 重构 !!] 
    // 
    // 

// 辅助函数：将字符串转为全小写
// (函数: PascalCase)
    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    // [已废弃]
    // 辅助函数：将 "MyClassName" 转换为 "kCMyClassName"
    // (函数: PascalCase)
    // std::string ToKName(std::string s)
    // {
    //     ...
    // }

    // 辅助函数：将 "IMyInterface" 转换为 "i_my_interface.h"
    // (函数: PascalCase)
    std::string ToInterfaceFilename(std::string s)
    {
        if (s.empty() || (s[0] != 'I' && s[0] != 'i'))
        {
            return "i_unknown.h";
        }
        // 将 "IMyInterface" 变为 "myInterface"
        s.erase(0, 1);
        s[0] = std::tolower(s[0]);

        std::string result = "i_";
        for (char c : s)
        {
            if (std::isupper(c))
            {
                result += '_';
                result += std::tolower(c);
            }
            else
            {
                result += c;
            }
        }
        return result + ".h";
    }

    // 辅助函数：将 "MyComponentImpl" 转换为 "my_component_impl"
    // (函数: PascalCase)
    std::string ToImplFilenameBase(std::string s)
    {
        if (s.empty())
        {
            return "component_impl";
        }
        s[0] = std::tolower(s[0]);
        std::string result;
        for (char c : s)
        {
            if (std::isupper(c))
            {
                result += '_';
                result += std::tolower(c);
            }
            else
            {
                result += c;
            }
        }
        return result;
    }

    // 辅助函数：生成头文件卫哨
    // (函数: PascalCase)
    std::string ToIncludeGuard(const std::string& prefix, const std::string& filename)
    {
        std::string guard = prefix + "_" + filename;
        std::transform(guard.begin(), guard.end(), guard.begin(),
            [](unsigned char c)
            {
                return (std::isalnum(c) ? std::toupper(c) : '_');
            });
        return guard + "_";
    }

    // 辅助函数：获取当前日期 (跨平台线程安全)
    // (函数: PascalCase)
    std::string GetCurrentDate()
    {
        std::time_t t = std::time(nullptr);
        std::tm tm_struct = {}; // 零初始化

        // [修正]：
        // 使用平台特定的线程安全函数
#ifdef _WIN32
    // 使用 Microsoft 的 "safe" 版本
        localtime_s(&tm_struct, &t);
#else
    // 使用 POSIX (Linux, macOS) 的 "thread-safe" 版本
        localtime_r(&t, &tm_struct);
#endif

        char mbstr[100];
        if (std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d", &tm_struct))
        {
            return mbstr;
        }
        return "2025-01-01";
    }

    // 辅助函数：执行字符串替换
    // (函数: PascalCase)
    std::string ReplaceTokens(std::string text,
        const std::map<std::string, std::string>& tokens)
    {
        for (const auto& [token, value] : tokens)
        {
            std::string token_tag = "$$" + token + "$$";
            size_t pos = text.find(token_tag);
            while (pos != std::string::npos)
            {
                text.replace(pos, token_tag.length(), value);
                pos = text.find(token_tag, pos + value.length());
            }
        }
        return text;
    }

    /**
     * @brief 写入文件（如果不存在）
     * (函数: PascalCase)
     */
    void WriteFile(const std::filesystem::path& path, const std::string& content)
    {
        if (std::filesystem::exists(path))
        {
            std::cout << "[Skipped] File already exists: " << path.string() << std::endl;
            return;
        }

        std::ofstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[Error] Failed to open file for writing: " << path.string() << std::endl;
            return;
        }

        file << content;
        file.close();
        std::cout << "[Created] File: " << path.string() << std::endl;
    }

} // 匿名命名空间 [!! 
  // 重构 !!]

/**
 * @brief 主函数
 */
int main(int argc, char* argv[])
{
    // 1. 解析命令行参数 (简易)
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0 && (i + 1) < argc)
        {
            args[arg.substr(2)] = argv[++i];
        }
    }

    if (args.find("name") == args.end() ||
        args.find("interface") == args.end() ||
        args.find("plugin") == args.end() ||
        args.find("interface_path") == args.end() // [修改] 
        // 强制要求
        )
    {
        std::cerr << "Usage: tool_create_plugin.exe "
            << "--name <ImplClassName> "
            << "--interface <IInterfaceName> "
            << "--plugin <plugin_name> "
            << "--interface_path <interface_dir_name>" // [修改]
            << std::endl;
        std::cerr << "Example: tool_create_plugin.exe "
            << "--name SimpleImplA "
            << "--interface ISimple "
            << "--plugin plugin_example "
            << "--interface_path interfaces_example"
            << std::endl;
        return 1;
    }

    std::string impl_class_name = args["name"];
    std::string interface_name = args["interface"];
    std::string plugin_name = args["plugin"];
    std::string interface_path = args["interface_path"]; // [修改]
    std::string alias = impl_class_name; // 
    // [保留] 
    // 别名仍由 plugin_entry.cpp 
    // 定义

// 2. 推导所有需要的名称
// [修改] 
// 为命名空间添加令牌
    std::string interface_namespace = interface_path;
    std::string plugin_namespace = plugin_name;

    // [修改] 
    // 移除已废弃的 impl_const_name
    // std::string impl_const_name = ToKName(impl_class_name);

    std::string interface_filename = ToInterfaceFilename(interface_name);
    std::string impl_filename_base = ToImplFilenameBase(impl_class_name);
    std::string impl_filename_h = impl_filename_base + ".h";
    std::string impl_filename_cpp = impl_filename_base + ".cpp";
    std::string plugin_guard_prefix = "Z3Y_SRC_" + ToLower(plugin_name);

    // [修改] 
    // 
    // 
    std::string iface_guard_prefix = "Z3Y_SRC_" + ToLower(interface_path);

    std::map<std::string, std::string> tokens = {
        {"INTERFACE_NAME", interface_name},
        {"INTERFACE_NAMESPACE", interface_namespace}, // [新]
        {"INTERFACE_FILENAME", interface_filename},
        {"INTERFACE_PATH", interface_path},
        {"INTERFACE_INCLUDE_GUARD", ToIncludeGuard(iface_guard_prefix, interface_filename)}, // [修改]
        {"IMPL_CLASS_NAME", impl_class_name},
        // {"IMPL_CONST_NAME", impl_const_name}, // [移除]
        {"IMPL_FILENAME_H", impl_filename_h},
        {"IMPL_FILENAME_CPP", impl_filename_cpp},
        {"IMPL_INCLUDE_GUARD_H", ToIncludeGuard(plugin_guard_prefix, impl_filename_h)},
        {"PLUGIN_NAME", plugin_name},
        {"PLUGIN_NAMESPACE", plugin_namespace}, // [新]
        {"ALIAS", alias},
        {"UUID_IFACE", z3y::tool::generate_uuid_v4()}, // [新]
        {"UUID_IMPL", z3y::tool::generate_uuid_v4()}, // [新] 
        // (
        // 以前叫 UUID)
{"DATE", GetCurrentDate()}
    };

    try
    {
        // 3. 确定路径
        std::filesystem::path root_path = std::filesystem::current_path().parent_path();
        std::filesystem::path src_path = root_path / "src";

        std::filesystem::path iface_dir = src_path / interface_path;
        std::filesystem::path plugin_dir = src_path / plugin_name;

        // 4. 创建目录
        std::filesystem::create_directories(iface_dir);
        std::filesystem::create_directories(plugin_dir);

        // 5. 写入文件
        WriteFile(iface_dir / interface_filename,
            ReplaceTokens(z3y::tool::templates::kInterfaceHeader, tokens));

        WriteFile(plugin_dir / impl_filename_h,
            ReplaceTokens(z3y::tool::templates::kImplHeader, tokens));

        WriteFile(plugin_dir / impl_filename_cpp,
            ReplaceTokens(z3y::tool::templates::kImplSource, tokens));

        WriteFile(plugin_dir / "plugin_entry.cpp",
            ReplaceTokens(z3y::tool::templates::kPluginEntry, tokens));

        std::cout << "\n[Success] Created plugin '" << plugin_name
            << "' with class '" << impl_class_name << "'." << std::endl;
        std::cout << "Please add the new files and projects to your 'projects/msvc' solution." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}