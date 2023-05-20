#include <cstdio>
#include <io.h>
#include <windows.h>
#include <string>
#include <sys/stat.h> //stat
#include <regex>
#include "tools.h"

using namespace std;

const regex
        re_fpath_abs(R"((([A-Za-z]):(?:.+)?[/\\])([^/\\:\*\?\"<>\|]+))"),
        re_MD5("\n([0-9A-Fa-f]{32})\n");

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
void outputWCol(unsigned short code, const char *format, ...)
{
    va_list args;
    CONSOLE_SCREEN_BUFFER_INFO sbi;              // sbi: screen buffer info
    HANDLE ch = GetStdHandle(STD_OUTPUT_HANDLE); // ch: current handle
    // 存颜色
    GetConsoleScreenBufferInfo(ch, &sbi);
    // 设置颜色
    SetConsoleTextAttribute(ch, code);
    // 输出
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    // 设置颜色
    SetConsoleTextAttribute(ch, sbi.wAttributes);
}

/* 输出行分隔符
 *
 * 参数：
 *     c:     用的字符
 *     times: 输出几次，默认 60 次
 */
void print_sep(char c, bool nl, int times)
{
    for (int i = 0; i < times; i++)
    {
        printf("%c", c);
    }
    if (nl)
    {
        printf("\n");
    }
}

/* 获取当前时间 */
tm *get_tm()
{
    time_t now;
    time(&now);
    return localtime(&now);
}

/* 二进制拷贝文件 */
int copyFile(const string &fileA, const string &fileB, int cf)  // cf: copy flush
{
    FILE *_in, *_out;
    if (!(_in = fopen(fileA.c_str(), "rb"))) // 打开失败
    {
        return 1;
    }
    if (!(_out = fopen(fileB.c_str(), "wb"))) // 新建失败
    {
        fclose(_in);
        return -1;
    }
    // 开始拷贝
    char *flush = new char[cf];
    memset(flush, 0, cf);
    int successNum;    // 成功读取的数量
    while (!feof(_in)) // 如果 eof 了就不读了，代表文件已到尽头
    {
        successNum = (int) fread(flush, 1, cf, _in);
        fwrite(flush, 1, successNum, _out);
    }
    delete[] flush;
    // 别忘了关闭文件
    fclose(_in);
    fclose(_out);
    return 0;
}

/* 在这里P用没有！获取的是工作目录而不是 exe 文件目录
// 当前工作目录
string getWorkDir()
{
    char buffer[MAX_PATH * 2];
    getcwd(buffer, MAX_PATH * 2);
    return buffer;
}
*/

/* 当前exe目录 */
string getExeFileAbsPath()
{
    char buf[MAX_PATH * 2];
    GetModuleFileNameA(nullptr, buf, MAX_PATH * 2);
    return buf;
}

/* 判断文件是否在U盘 */
bool isInUDisk(const string &fp, const string &id)  // id: ignore disk
{
    const char disk = fp[0]; // 当前文件的盘符
    for (char i: id)
    {
        if (disk == i)
        {
            return false;
        }
    }
    return true;
}

/* 获取文件路径的一部分（分割）
 * 参数:
 *     fileAbsPath: 文件路径
 *                  必须是绝对的
 *     type:        获取的信息种类
 *                  0 代表全路径
 *                  1 代表文件夹名
 *                  2 代表盘符
 *                  3 代表文件名
 */
rff<string> getFilePathSplitByAbs(const string &fileAbsPath, int type)
{
    cmatch m;
    if (type >= 0 && type <= 3 && regex_match(fileAbsPath.c_str(), m, re_fpath_abs))
    {
        return {0, m.str(type)};
    }
    else
    {
        return {-1, ""};
    }
}

rff<string> run_cmd(const string &cmd)
{
    char flush[1024] = {0};
    string res;
    FILE *fp;
    if ((fp = popen(cmd.c_str(), "r")) == nullptr)
    {
        return {-1, ""};
    }
    // 读取命令执行过程中的输出
    while (fgets(flush, 1024, fp) != nullptr)
    {
        res += flush;
    }
    // 关闭执行的进程
    _pclose(fp);
    return {0, res};
}

/**
 * @返回值 状态。0 为成功，-1 为执行 cmd 失败，-2 为文件不存在
 */
int getMD5FromFile(const string &fp, char buf[16])
{
    string cmd = "certutil -hashfile \"" + fp + "\" MD5", s;
    print_sep('-', true, 5);
    printf("%s\n", GBKStringToUTF8String(cmd).c_str());
    print_sep('-', true, 5);
    rff<string> r = run_cmd(cmd);
    if (r.status() != 0)
    {
        return -1;
    }

    char temp1, temp2;
    int indexOfSpace;
    s = GBKStringToUTF8String(r.value());
    cmatch m;
    while ((indexOfSpace = (int) s.find(' ')) != -1)
    {
        s.erase(indexOfSpace, 1);
    }
    printf("%s", s.c_str());
    print_sep('-', true, 5);    // 去空格

    // 找 hash
    if (!regex_search(s.c_str(), m, re_MD5))
    {
        return -2;
    }
    s = m.str(1);
    printf("%s\n", s.c_str());
    print_sep('-', true, 5);

    // 填入数组
    for (int i = 0; i < 16; i++)
    {
        temp1 = s[i * 2], temp2 = s[i * 2 + 1];
        temp1 = (char) (temp1 >= 'a' ? temp1 - ('a' - 10) : (temp1 >= 'A' ? temp1 - ('A' - 10) : temp1 - '0'));
        temp2 = (char) (temp2 >= 'a' ? temp2 - ('a' - 10) : (temp2 >= 'A' ? temp2 - ('A' - 10) : temp2 - '0'));
        buf[i] = (char) ((temp1 << 4 & 0xf0) | (temp2 & 0xf));
    }
    return 0;
}

/** 获取文件大小
 * @return 文件大小，单位：byte。若为 -1 则文件不存在
 */
int getFileSize(const std::string &fp)
{
    // 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
    struct stat statBuf{};
    // 提供文件名字符串，获得文件属性结构体
    if (stat(fp.c_str(), &statBuf) == 0)
    {
        // 获取文件大小
        return statBuf.st_size;
    }
    else
    {
        return -1;
    }
}

char *GBKCharToUTF8Char(const char *gbk_str)
{
    // 转换编码
    int gbk_len = (int) strlen(gbk_str) + 1;
    // 将 GBK 编码的字符串转换成 UTF-16 编码的宽字符集
    int utf16_len = MultiByteToWideChar(CP_ACP, 0, gbk_str, gbk_len, nullptr, 0);
    auto *utf16_str = new wchar_t[utf16_len];
    MultiByteToWideChar(CP_ACP, 0, gbk_str, gbk_len, utf16_str, utf16_len);

    // 将 UTF-16 编码的宽字符串转换成 UTF-8 编码的多字节字符集
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, utf16_len, nullptr, 0, nullptr, nullptr);
    char *utf8_str = new char[utf8_len];
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, utf16_len, utf8_str, utf8_len, nullptr, nullptr);
    delete[]utf16_str;
    return utf8_str;
}

string GBKStringToUTF8String(const string &gbk_str)
{
    char *res = GBKCharToUTF8Char(gbk_str.c_str());
    string ret = res;
    delete[]res;
    return ret;
}

char *UTF8CharToGBKChar(const char *utf8_str)
{
    // 转换编码
    int utf8_len = (int) strlen(utf8_str) + 1;
    // 将 GBK 编码的字符串转换成 UTF-16 编码的宽字符集
    int utf16_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, nullptr, 0);
    auto *utf16_str = new wchar_t[utf16_len];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, utf16_str, utf16_len);

    // 将 UTF-16 编码的宽字符串转换成 UTF-8 编码的多字节字符集
    int gbk_len = WideCharToMultiByte(CP_ACP, 0, utf16_str, utf16_len, nullptr, 0, nullptr, nullptr);
    char *gbk_str = new char[utf8_len];
    WideCharToMultiByte(CP_ACP, 0, utf16_str, utf16_len, gbk_str, gbk_len, nullptr, nullptr);
    delete[]utf16_str;
    return gbk_str;
}

string UTF8StringToGBKString(const string &gbk_str)
{
    char *res = UTF8CharToGBKChar(gbk_str.c_str());
    string ret = res;
    delete[]res;
    return ret;
}

wchar_t *UTF8CharToUTF16WChar(const char *utf8_str)
{
    // 转换编码
    int gbk_len = (int) strlen(utf8_str) + 1;
    // 将 GBK 编码的字符串转换成 UTF-16 编码的宽字符集
    int utf16_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, gbk_len, nullptr, 0);
    auto *utf16_str = new wchar_t[utf16_len];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, gbk_len, utf16_str, utf16_len);

    return utf16_str;
}

wstring UTF8StringToUTF16WString(const string &utf8_str)
{
    wchar_t *res = UTF8CharToUTF16WChar(utf8_str.c_str());
    wstring ret = res;
    delete[]res;
    return ret;
}
