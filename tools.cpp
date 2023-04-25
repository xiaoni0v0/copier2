#include <cstdio>
#include <windows.h>
#include <string>
#include <sys/stat.h> //stat
#include <regex>
#include "tools.h"

using namespace std;

regex
        re_comma("((\\s*,\\s*)|(\\s*，\\s*))"),
        re_fpath_abs(R"((([A-Za-z]):(?:.+)?[/\\])([^/\\:\*\?\"<>\|]+))");

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

void print_sep(const char *s, bool nl, int times)
{
    for (int i = 0; i < times; i++)
    {
        printf("%s", s);
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

/* 判断文件是否存在 */
bool fileExists(const string &fp)
{
    struct stat buf{};
    return (stat(fp.c_str(), &buf) == 0); // == 0 代表读取文件状态成功 -> 文件存在
}

/* 二进制拷贝文件 */
int copyFile(const string &fileA, const string &fileB, int cf)  // cf: copy flush
{
    FILE *_in, *_out;
    _in = fopen(fileA.c_str(), "rb");
    if (!_in) // 打开失败
    {
        return 1;
    }
    _out = fopen(fileB.c_str(), "wb");
    if (!_out) // 新建失败
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

// 在这里P用没有！获取的是工作目录而不是exe文件目录
/* 当前工作目录 */
// string getWorkDir() {
//     char buffer[MAX_PATH * 2];
//     getcwd(buffer, MAX_PATH * 2);
//     return buffer;
// }

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
    string disk = getFilePathSplitByAbs(fp, 2).value(); // 当前文件的盘符
    const sregex_token_iterator end;
    for (sregex_token_iterator iter(id.begin(), id.end(), re_comma, -1); iter != end; iter++)
    {
        if (iter->str() == disk)
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

int run_cmd(const string &cmd, string &buf)
{
    char flush[1024];
    FILE *fp;
    if ((fp = popen(cmd.c_str(), "r")) == nullptr)
    {
        return -1;
    }
    memset(flush, 0, 1024);
    // 读取命令执行过程中的输出
    while (fgets(flush, 1024, fp) != nullptr)
    {
        buf += flush;
    }
    // 关闭执行的进程
    _pclose(fp);
    return 0;
}
