#include <cstdio>
#include <fstream>
#include <cstring>
#include <map>
#include <regex>
#include <windows.h>
#include <stdarg.h>

#define LINE_MAX 65536
#define WARNING_COLOR 0xe
#define ERROR_COLOR 0xc

/* 读取配置文件，配置文件解析器 */
class IniParser
{
private:
    bool outputMode;
    std::map<std::string, std::map<std::string, std::string>> sections, sections_temp;

    /* 输出行分隔符
     *
     * 参数：
     *     c:     用的字符
     *     times: 输出几次，默认 60 次
     */
    void print_sep(char c, int times = 60)
    {
        printf("%s\n", std::string(times, c).c_str());
    }
    void print_sep(const char *s, int times = 60)
    {
        for (int i = 0; i < times; i++)
        {
            printf("%s\n", s);
        }
    }

    /* 输出
     *
     * 参数：
     *     code: 颜色码，默认黑底白字
     *           常用的：
     *               黑底黄字：0xe
     *               黑底红字：0xc
     *               黑底白字：0xf
     *     format: 输出格式
     */
    void output_WAE(unsigned short code, const char *format, ...)
    {
        if (outputMode)
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), code); // 设置控制台颜色
            va_list args;                                                   // 定义可变参数列表对象
            va_start(args, format);                                         // format 存储格式字符串，作为可变参数起点的前一个参数标识
            vprintf(format, args);                                          // 整合格式字符串和可变参数
            va_end(args);                                                   // 清除可变参数列表对象args
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0xf);  // 设置控制台颜色
        }
    }

public:
    IniParser(bool output_mode = true)
    {
        outputMode = output_mode;
        sections.clear();
        sections["DEFAULT"] = std::map<std::string, std::string>();
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
    int read(const std::string &f_path)
    {
        FILE *_in;
        if (nullptr == (_in = fopen(f_path.c_str(), "r")))
        {
            output_WAE(ERROR_COLOR,
                       "// 文件 \"%s\" 打开失败！\n", f_path.c_str());
            return -1;
        }
        // 开始读
        int line_num = 0;
        char line[LINE_MAX];
        std::string current_section = "DEFAULT", k, v;
        std::cmatch matched;
        // 编译正则表达式
        std::regex
            re1("\\s*((;|//|#).*\n)?"),                                                // 空行或注释
            re2("\\s*\\[\\s*(\\w+)\\s*\\]\\s*\n"),                                     // section
            re3("\\s*([\\w\\-\\.]+?)\\s*[:=]\\s*([^;]*?)\\s*(?:(?:;|//|#).*)?\\s*\n"); // kv: key value
        sections_temp.clear();
        while (nullptr != fgets(line, LINE_MAX, _in)) // 读文件
        {
            line_num++;
            // printf("%3d|%s", line_num, line);
            // 识别
            if (regex_match(line, matched, re1)) // 识别到空行、注释
            {
                continue;
            }
            else if (regex_match(line, matched, re2)) // 识别到一个 section
            {
                current_section = matched.str(1).c_str();
            }
            else if (regex_match(line, matched, re3)) // 识别到一个键值对(kv)
            {
                k = matched.str(1);
                v = matched.str(2);
                sections_temp[current_section][k] = v;
            }
            else
            {
                output_WAE(ERROR_COLOR,
                           "// 读取配置文件 \"%s\" 第 %d 行时失败（无法解析为ini格式）：\n%3d|%s    %s\n// 为防止进一步出错，整个配置文件 \"%s\" 将作废\n",
                           f_path.c_str(), line_num, line_num, line, std::string(strlen(line) - 1, '~').c_str(), f_path.c_str());
                return -1;
            }
        }
        fclose(_in);
        // 合并
        for (auto each_section : sections_temp)
        {
            for (auto each_kv : each_section.second)
            {
                sections[each_section.first][each_kv.first] = each_kv.second;
            }
        }
        sections_temp.clear();
        return 0;
    }

    /* section 的个数 */
    int n_sections()
    {
        return sections.size();
    }

    /* 输出这个解析器对象中的字典所有键值对 */
    void output()
    {
        print_sep('-');
        for (auto each_section : sections) // c++11 特有循环方式
        {
            printf("[%s]\n", each_section.first.c_str());
            for (auto each_kv : each_section.second)
            {
                printf("\"%s\"=\"%s\"\n", each_kv.first.c_str(), each_kv.second.c_str());
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
    int get_v_string(const std::string &section_name, const std::string &key_name, std::string &buf, const std::string &default_v = "")
    {
        if (sections.count(section_name) == 1) // 有这个 section
        {
            if (sections[section_name].count(key_name) == 1) // 有这个 key
            {
                buf = sections[section_name][key_name];
                return 0;
            }
            else
            {
                output_WAE(WARNING_COLOR,
                           "// 没有找到 \"%s\" 这个 key\n", key_name.c_str());
                buf = default_v;
                return -1;
            }
        }
        else
        {
            output_WAE(WARNING_COLOR,
                       "// 没有找到 \"%s\" 这个 section\n", section_name.c_str());
            buf = default_v;
            return -1;
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
    int get_v_bool(const std::string &section_name, const std::string &key_name, bool &buf, bool default_v = false)
    {
        std::regex
            re_T("(true|on|yes|1|ture|ok)", std::regex_constants::icase),
            re_F("(false|off|no|0|flase|not)", std::regex_constants::icase);
        std::string temp;
        if (get_v_string(section_name, key_name, temp) == 0) // 能找到 section 和 key
        {
            // 匹配 true 或者 false
            if (regex_match(temp, re_T))
            {
                buf = true;
                return 0;
            }
            else if (regex_match(temp, re_F))
            {
                buf = false;
                return 0;
            }
            else
            {
                output_WAE(ERROR_COLOR,
                           "// 无法将 \"%s\" 转换为 bool\n", temp.c_str());
                buf = default_v;
                return -1;
            }
        }
        else
        {
            buf = default_v;
            return -1;
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
    int get_v_int(const std::string &section_name, const std::string &key_name, int &buf, int default_v = 0)
    {
        std::regex re_int("\\-?\\d+");
        std::string temp;
        if (get_v_string(section_name, key_name, temp) == 0) // 能找到 section 和 key
        {
            if (regex_match(temp, re_int)) // 符合 int
            {
                sscanf(temp.c_str(), "%d", &buf);
                if (std::to_string(buf) != temp) // 转换为 int 溢出了
                {
                    output_WAE(WARNING_COLOR,
                               "// 警告：字符串 \"%s\" 转换为 int 会可能导致溢出，你需要知道你这么做会发生什么！\n", temp.c_str());
                }
                return 0;
            }
            else
            {
                output_WAE(ERROR_COLOR,
                           "// 无法将 \"%s\" 转换为 int\n", temp.c_str());
                buf = default_v;
                return -1;
            }
        }
        else
        {
            buf = default_v;
            return -1;
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
    int get_v_double(const std::string &section_name, const std::string &key_name, double &buf, double default_v = 0.0)
    {
        std::regex re_double("\\-?\\d+\\.?\\d*");
        std::string temp;
        if (get_v_string(section_name, key_name, temp) == 0) // 能找到 section 和 key
        {
            if (regex_match(temp.c_str(), re_double))
            {
                sscanf(temp.c_str(), "%lf", &buf);
                return 0;
            }
            else
            {
                output_WAE(ERROR_COLOR,
                           "// 无法将 \"%s\" 转换为 double\n", temp.c_str());
                buf = default_v;
                return -1;
            }
        }
        else
        {
            buf = default_v;
            return -1;
        }
    }
};
