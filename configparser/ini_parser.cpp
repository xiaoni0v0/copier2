#include <cstdio>
#include <fstream>
#include <cstring>
#include <map>
#include <regex>
#include <windows.h>
#include "./../tools.h"
#include "ini_parser.h"

using namespace std;
#define LINE_MAX 65536

regex
        re_line1("\\s*((;|//|#).*\n)?"),
        re_line2("\\s*\\[\\s*(\\w+)\\s*\\]\\s*\n"),
        re_line3("\\s*([\\w\\-\\.]+?)\\s*[:=]\\s*([^;]*?)\\s*(?:(?:;|//|#).*)?\\s*\n"),
        re_T("(true|on|yes|1|ture|ok|t)", regex_constants::icase),
        re_F("(false|off|no|0|flase|not|f)", regex_constants::icase),
        re_int(R"(\-?\d+)"),
        re_double(R"(\-?\d+\.?\d*)");

/* 读取配置文件，配置文件解析器 */
IniParser::IniParser()
{
    sections.clear();
    sections["DEFAULT"] = map<string, string>();
}

/* 读取文件中的键值对
 *
 * 参数：
 *     f_path: 文件路径
 * 返回值：
 *     读取状态
 *     读取成功返回 0, 读取失败返回 -1
 *     (失败可能的原因：文件不存在、文件占用、文件不符合ini配置文件语法)
 */
int IniParser::read(const string &f_path)
{
    FILE *_in;
    if (!(_in = fopen(f_path.c_str(), "r")))
    {
        outputError("[ini_parser][ERROR] 文件 \"%s\" 打开失败！\n", f_path.c_str());
        return -1;
    }
    // 开始读
    int line_num = 0;
    char line[LINE_MAX];
    string current_section = "DEFAULT", k, v;
    cmatch matched;
    sections_temp.clear();
    while (nullptr != fgets(line, LINE_MAX, _in)) // 读文件
    {
        line_num++;
        // printf("%3d|%s", line_num, line);
        // 识别
        if (regex_match(line, matched, re_line1)) // 识别到空行、注释
        {
            continue;
        }
        else if (regex_match(line, matched, re_line2)) // 识别到一个 section
        {
            current_section = matched.str(1);
        }
        else if (regex_match(line, matched, re_line3)) // 识别到一个键值对(kv)
        {
            k = matched.str(1);
            v = matched.str(2);
            sections_temp[current_section][k] = v;
        }
        else
        {
            outputError(R"([ini_parser][ERROR] 读取配置文件 "%s" 第 %d 行时失败 (无法解析为ini格式):

    %3d| %s         %s

[ini_parser][ERROR] 为防止进一步出错，整个配置文件 "%s" 将作废
)",
                        f_path.c_str(), line_num, line_num, line, string(strlen(line) - 1, '~').c_str(),
                        f_path.c_str());
            return -1;
        }
    }
    fclose(_in);
    // 合并
    for (const auto &each_section: sections_temp)
    {
        for (const auto &each_kv: each_section.second)
        {
            sections[each_section.first][each_kv.first] = each_kv.second;
        }
    }
    sections_temp.clear();
    return 0;
}

/* section 的个数 */
int IniParser::n_sections()
{
    return sections.size();
}

/* 输出这个解析器对象中的字典所有键值对 */
void IniParser::output()
{
    print_sep('-');
    for (const auto &each_section: sections) // c++11 特有循环方式
    {
        printf("[%s]\n", each_section.first.c_str());
        for (const auto &each_kv: each_section.second)
        {
            printf("    %s = \"%s\"\n", each_kv.first.c_str(), each_kv.second.c_str());
        }
    }
    print_sep('-');
}

/* 使用 section 和 key 来获取 value，string 型
 *
 * 参数：
 *     section_name: section 名字
 *     key_name:     key 名字
 *     buf:          缓冲器，将内容写进这里面
 *     default_v:    默认值
 * 返回值：
 *     获取状态，0 代表成功，-1 代表失败
 */
rff<string> IniParser::get_v_string(const string &section_name, const string &key_name, const string &default_v)
{
    if (sections.count(section_name) == 1) // 有这个 section
    {
        if (sections[section_name].count(key_name) == 1) // 有这个 key
        {
            return {0, sections[section_name][key_name]};
        }
        else
        {
            outputWarning("[ini_parser][WARNING] 没有找到 \"%s\" 这个 key\n", key_name.c_str());
            return {-1, default_v};
        }
    }
    else
    {
        outputWarning("[ini_parser][WARNING] 没有找到 \"%s\" 这个 section\n", section_name.c_str());
        return {-1, default_v};
    }
}

/* 使用 section 和 key 来获取 value，bool 值
 *
 * 参数：
 *     section_name: section 名字
 *     key_name:     key 名字
 *     buf:          缓冲器，将内容写进这里面
 *                   先使用 get_v_string() 获取到字符串，
 *                   如果是 true, on, yes, 1, ok   中的任意一种就返回 true，
 *                   如果是 false, off, no, 0, not 中的任意一种就返回 false （不区分大小写），
 *                   其他的无法识别，导致出错
 *     default_v:    默认值
 * 返回值：
 *     获取状态，0 代表成功，-1 代表失败
 */
rff<bool> IniParser::get_v_bool(const string &section_name, const string &key_name, bool default_v)
{
    rff<string> temp = get_v_string(section_name, key_name);
    if (temp.status() == 0) // 能找到 section 和 key
    {
        // 匹配 true 或者 false
        if (regex_match(temp.value(), re_T))
        {
            return {0, true};
        }
        else if (regex_match(temp.value(), re_F))
        {
            return {0, false};
        }
        else
        {
            outputError("[ini_parser][ERROR] 无法将 \"%s\" 转换为 bool\n", temp.value().c_str());
            return {-1, default_v};
        }
    }
    else
    {
        return {-1, default_v};
    }
}

/* 使用 section 和 key 来获取 value，int 型
 *
 * 参数：
 *     section_name: section 名字
 *     key_name:     key 名字
 *     buf:          缓冲器，将内容写进这里面
 *                   先使用 get_v_string() 获取到字符串，再转为数字。如果字符串不能转成 int，则失败
 *     default_v:    默认值
 * 返回值：
 *     获取状态，0 代表成功，-1 代表失败
 */
rff<int> IniParser::get_v_int(const string &section_name, const string &key_name, int default_v)
{
    int temp_ret;
    rff<string> temp = get_v_string(section_name, key_name);
    if (temp.status() == 0) // 能找到 section 和 key
    {
        if (regex_match(temp.value(), re_int)) // 符合 int
        {
            temp_ret = strtol(temp.value().c_str(), (char **) nullptr, 10);
            if (to_string(temp_ret) != temp.value()) // 转换为 int 溢出了
            {
                outputWarning(
                        "[ini_parser][WARNING] 字符串 \"%s\" 转换为 int 会可能导致溢出，你需要知道你这么做会发生什么！\n",
                        temp.value().c_str());
            }
            return {0, temp_ret};
        }
        else
        {
            outputError("[ini_parser][ERROR] 无法将 \"%s\" 转换为 int\n", temp.value().c_str());
            return {-1, default_v};
        }
    }
    else
    {
        return {-1, default_v};
    }
}

/* 使用 section 和 key 来获取 value，double 型
 *
 * 参数：
 *     section_name: section 名字
 *     key_name:     key 名字
 *     buf:          缓冲器，将内容写进这里面
 *                   先使用 get_v_string() 获取到字符串，再转为数字。如果字符串不能转成 double，则失败
 *     default_v:    默认值
 * 返回值：
 *     获取状态，0 代表成功，-1 代表失败
 */
rff<double> IniParser::get_v_double(const string &section_name, const string &key_name, double default_v)
{
    rff<string> temp = get_v_string(section_name, key_name);
    if (temp.status() == 0) // 能找到 section 和 key
    {
        if (regex_match(temp.value().c_str(), re_double))
        {
            return {0, strtod(temp.value().c_str(), (char **) nullptr)};
        }
        else
        {
            outputError("[ini_parser][ERROR] 无法将 \"%s\" 转换为 double\n", temp.value().c_str());
            return {-1, default_v};
        }
    }
    else
    {
        return {-1, default_v};
    }
}
