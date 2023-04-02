#include <cstdio>
#include <string>
#include <regex>
#include <windows.h>
#include <sys/stat.h> //stat
#include <direct.h>   //_mkdir
#include <stdarg.h>   //可变参数
#include "configparser/ini_parser.h"

using namespace std;

// 定义函数
int conf();
tm *get_tm();
void log(const char *, ...);
bool fileExists(const string &);
bool isInUDisk(const string &);
int copyFile(const string &, const string &);
int getFilePathSplitByAbs(const string &, string &, int);
string getFilePathSplitByAbs(const string &, int);
HANDLE executePPTFile(const string &);
string getExeFileAbsPath();
void checkEnvironment_with1arg(int);
void executeAndCopyFile_with2args(const string &);

/* 配置变量 */
bool LogMode;
string PowerPointPath, CopyPath, IgnoreDisk;
int retAll;

/* 加载配置 */
int conf()
{
    int read_ret;
    IniParser iniparser;
    read_ret = iniparser.read(getFilePathSplitByAbs(getExeFileAbsPath(), 1) + "copier.ini"); // 读
    if (read_ret == 0)
    {
        read_ret |= iniparser.get_v_bool("copier", "LogMode", LogMode);                 // 1
        read_ret |= iniparser.get_v_string("copier", "PowerPointPath", PowerPointPath); // 2
        read_ret |= iniparser.get_v_string("copier", "CopyPath", CopyPath);             // 3
        read_ret |= iniparser.get_v_string("copier", "IgnoreDisk", IgnoreDisk);         // 4
    }
    return read_ret;
}

tm *get_tm()
{
    time_t now;
    time(&now);
    return localtime(&now);
}

void log(const char *format, ...)
{
    if (LogMode)
    {
        tm *p = get_tm();
        // 输出时间
        printf("[%04d/%02d/%02d %02d:%02d:%02d] ",
               p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        // 输出调试信息
        va_list args;           // 定义可变参数列表对象
        va_start(args, format); // format 存储格式字符串，作为可变参数起点的前一个参数标识
        vprintf(format, args);  // 整合格式字符串和可变参数
        va_end(args);           // 清除可变参数列表对象args
    }
}

/* 判断文件是否存在 */
bool fileExists(const string &fp)
{
    struct stat buf;
    return (stat(fp.c_str(), &buf) == 0); // == 0 代表读取文件状态成功 -> 文件存在
}

/* 判断文件是否在U盘 */
bool isInUDisk(const string &fp)
{
    regex re_comma("((\\s*,\\s*)|(\\s*，\\s*))");
    string disk = getFilePathSplitByAbs(fp, 2); // 当前文件的盘符
    const sregex_token_iterator end;
    for (sregex_token_iterator iter(IgnoreDisk.begin(), IgnoreDisk.end(), re_comma, -1); iter != end; iter++)
    {
        if (iter->str() == disk)
        {
            return false;
        }
    }
    return true;
}

/* 二进制拷贝文件 */
int copyFile(const string &fileA, const string &fileB)
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
    const int FLUSH_NUM = 1024 * 1024; // 一次读 1 MB 数据
    char flush[FLUSH_NUM];
    memset(flush, 0, sizeof(flush));
    int successNum;    // 成功读取的数量
    while (!feof(_in)) // 如果 eof 了就不读了，代表文件已到尽头
    {
        successNum = (int)fread(flush, 1, FLUSH_NUM, _in);
        fwrite(flush, 1, successNum, _out);
    }
    // 别忘了关闭文件
    fclose(_in);
    fclose(_out);
    return 0;
}

/* 获取文件路径的一部分（分割）
 * 参数:
 *     filePath: 文件路径
 *                   必须是绝对的
 *     type:     获取的信息种类
 *                   0 代表全路径
 *                   1 代表文件夹名
 *                   2 代表盘符
 *                   3 代表文件名
 * 返回值:
 *     获取状态
 *     0 为成功，-1 为失败
 */
int getFilePathSplitByAbs(const string &fileAbsPath, string &buf, int type)
{
    regex re_fpath_abs("(([A-Za-z])+:(?:.+)?[/\\\\])([^/\\\\:\\*\\?\"<>\\|]+)");
    cmatch m;
    if (type >= 0 && type <= 3 && regex_match(fileAbsPath.c_str(), m, re_fpath_abs))
    {
        buf = m.str(type);
        return 0;
    }
    else
    {
        return -1;
    }
}

/* 获取文件路径的一部分（分割）
 * 参数:
 *     filePath: 文件路径
 *                   必须是绝对的
 *     type:     获取的信息种类
 *                   0 代表全路径
 *                   1 代表文件夹名
 *                   2 代表盘符
 *                   3 代表文件名
 * 返回值:
 *     获取状态
 *     0 为成功，-1 为失败
 */
string getFilePathSplitByAbs(const string &fileAbsPath, int type)
{
    regex re_fpath_abs("(([A-Za-z]):(?:.+)?[/\\\\])([^/\\\\:\\*\\?\"<>\\|]+)");
    cmatch m;
    if (type >= 0 && type <= 3 && regex_match(fileAbsPath.c_str(), m, re_fpath_abs))
    {
        return m.str(type);
    }
    else
    {
        return "";
    }
}

/* 用PNT打开指定文件 */
HANDLE executePPTFile(const string &f_path)
{
    string p1 = "\"", p2 = "\"";
    p1.append(PowerPointPath);
    p1.append("\"");
    p2.append(f_path);
    p2.append("\"");
    return ShellExecuteA(nullptr, "open",
                         p1.c_str(),
                         p2.c_str(),
                         nullptr, SW_SHOWNORMAL);
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
    return string(buf);
}

void checkEnvironment_with1arg(int argc)
{
    log("参数为 %d 个 (不是2个), 执行检查环境\n", argc);
    // 康康 PNT 是否存在
    if (fileExists(PowerPointPath))
    {
        log("环境正常！可以使用！\n");
        MessageBoxA(nullptr, "环境正常！可以使用！", "copier v2.0--by XN & XY", MB_ICONINFORMATION);
    }
    else
    {
        log("未找到PowerPoint程序！请检查配置！\n");
        MessageBoxA(nullptr, "未找到PowerPoint程序！\n请检查配置！", "copier v2.0--by XN & XY", MB_ICONERROR);
    }
}

void executeAndCopyFile_with2args(const string &f_path)
{
    // 有 2 个参数，打开（，再拷贝）
    string f_name = getFilePathSplitByAbs(f_path, 3);
    log("参数为 2 个，执行文件操作\n");
    log("打开文件路径: %s\n", f_path.c_str());
    log("文件所在盘: %c\n", toupper(f_path[0]));
    log("文件名: %s\n", f_name.c_str());
    executePPTFile(f_path); // 打开文件
    // 判断是否在 U 盘
    if (isInUDisk(f_path)) // 在U盘中，复制
    {
        log("这个文件 *在* U盘中，先打开再复制\n");
        log("创建文件夹状态: %d\n",
            _mkdir(CopyPath.c_str())); // 创建文件夹，0 为原来不存在但创建成功，-1 为原来存在文件夹而不用创建
        log("文件拷贝目标: %s%s\n", CopyPath.c_str(), getFilePathSplitByAbs(f_path, 3).c_str());
        log("文件拷贝状态: %d\n",                                           // 复制文件
            copyFile(f_path, CopyPath + getFilePathSplitByAbs(f_path, 3))); // 0 为正常，1 为找不到源文件，-1 为创建文件失败
    }
    else // 不在U盘中，不复制
    {
        log("这个文件 *不在* U盘中，直接打开\n");
    }
}

/* 主函数 */
int main(int argc, char *argv[])
{
    // 配置文件读取
    retAll = conf();
    if (LogMode)
    {
        // 重定向文件
        freopen((getFilePathSplitByAbs(getExeFileAbsPath(), 1) + "copier.log").c_str(), "a", stdout);
        tm *p = get_tm();
        printf("\n-------------------------copier v2.0 于 %04d/%02d/%02d %02d:%02d:%02d 启动-------------------------\n",
               p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    }
    log("EXE 路径: %s\n", getExeFileAbsPath().c_str());
    log("工作目录: %s\n", getFilePathSplitByAbs(getExeFileAbsPath(), 1).c_str());
    log("日志文件路径: %s\n", (getFilePathSplitByAbs(getExeFileAbsPath(), 1) + "copier.log").c_str());
    log("配置文件路径: %s\n", (getFilePathSplitByAbs(getExeFileAbsPath(), 1) + "copier.ini").c_str());
    log("配置状态: %d\n", retAll); // 0 为正常，-1 为读取配置文件异常
    log("配置信息如下: \n    LogMode = %d\n    PowerPointPath = %s\n    CopyPath = %s\n    IgnoreDisk = %s\n",
        LogMode, PowerPointPath.c_str(), CopyPath.c_str(), IgnoreDisk.c_str());

    // 干正事
    if (argc == 2)
    {
        executeAndCopyFile_with2args(argv[1]);
    }
    else
    {
        checkEnvironment_with1arg(argc);
    }
    log("程序正常退出\n");
    if (LogMode)
    {
        fclose(stdout);
    }
    // 好习惯
    return 0;
}
