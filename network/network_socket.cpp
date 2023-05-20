#include <string>
#include <winsock.h>
#include "./../tools.h"
#include "network_socket.h"

using namespace std;

/** 版本
 *  @分配
 *      1、2位代表大版本，3、4、5位代表小版本，6、7、8代表通信版本
 */
const char VER = (char) 0211;  // 2.1.1

/** 创建套接字
 *  @返回值
 *      创建状态，0 表示成功。-1 表示注册失败，-2 表示创建失败
 */
int Socket_TCP::create()
{
    // 先注册
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        outputError("套接字注册失败！\n");
        return -1;
    }
    // 创建套接字
    if (INVALID_SOCKET == (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)))
    {
        outputError("套接字创建失败！\n");
        return -2;
    }
    isSetup = true;
    return 0;
}

Socket_TCP::~Socket_TCP()
{
    if (isSetup)
    {
        s_close();
    }
    WSACleanup();
}

bool Socket_TCP::is_setup() const
{
    return isSetup;
}

bool Socket_TCP::is_closed() const
{
    return isClosed;
}

/** 连接
 *  @返回值
 *      连接状态。0 代表成功，-1 代表失败，-2 表示还未安装
 */
int Socket_TCP::s_connect(const string &ip, int port) const
{
    if (isSetup && !isClosed)
    {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        server_addr.sin_port = htons(port);
        return connect(sock, (sockaddr *) &server_addr, sizeof(server_addr));
    }
    else
    {
        return -2;
    }
}

/** 发送
 *  @返回值
 *      发送长度。若为 -1 则为发送失败，-2 表示还未安装
 */
int Socket_TCP::s_send(const char *data, int size) const
{
    if (isSetup && !isClosed)
    {
        return send(sock, data, size, 0);
    }
    else
    {
        return -2;
    }
}

/** 接收
 *  @返回值
 *      接收长度。若为 -1 则为接收失败，-2 表示还未安装
 */
int Socket_TCP::s_recv(char *buf, int size) const
{
    if (isSetup && !isClosed)
    {
        return recv(sock, buf, size, 0);
    }
    else
    {
        return -2;
    }
}

/** 关闭
 *  @返回值
 *      关闭状态。0 为关闭成功，-1 为失败
 */
int Socket_TCP::s_close()
{
    if (!isClosed)
    {
        isClosed = true;
        return closesocket(sock);
    }
    else
    {
        return 0;
    }
}

/* -------------------------------------------------------------------------------- */

#define status(v) ((v) == -1 ? -1 : 0)

Socket_copier_part::Socket_copier_part(const string &_addr, const string &_sk)  // _sk: secret key
{
    // addr
    addr = _addr;
    // 密钥
    // sk = (_sk + string(8, '0')).substr(0, 8);
    sk = _sk, sk.resize(8, '0');  // 防止不够 8 位导致访问非法内存
    // 创建
    base.create();
    if ((sta = base.s_connect(_addr, PORT)))
    {
        outputError("连接失败！\n");
    }
    else
    {
        outputSuccess("连接成功！\n");
    }
}

/** 发送当前通信的版本
 *  @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_version() const
{
    return status(base.s_send(&VER, 1));
}

/** @param what 干什么？0 为检测网络环境，1 为传输文件
 *  @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_what(const char what) const
{
    return status(base.s_send(&what, 1));
}

/** @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_sk() const
{
    return status(base.s_send(sk.c_str(), 8));
}

/** @返回值
 *      0 为成功，-1 为发送失败，-2 为计算 MD5 失败
 */
int Socket_copier_part::send_hash(const string &fp) const
{
    char buffer[16];
    if (getMD5FromFile(fp, buffer) != 0)
    {
        memset(buffer, -1, 16);
        base.s_send(buffer, 16);
        return -2;
    }
    else
    {
        return status(base.s_send(buffer, 16));
    }
}

/** @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_name_size(const string &fp) const
{
    auto name_size = (short) getFilePathSplitByAbs(fp, 3).value().length();
    char name_size_array[2] = {
            (char) (name_size >> 8),
            (char) (name_size)
    };
    return status(base.s_send(name_size_array, 2));
}

/** @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_content_size(const string &fp) const
{
    int content_size = getFileSize(fp);
    char content_size_array[4] = {
            (char) (content_size >> 24),
            (char) (content_size >> 16),
            (char) (content_size >> 8),
            (char) (content_size)
    };
    return status(base.s_send(content_size_array, 4));
}

/** @返回值
 *      0 为成功，-1 为失败
 */
int Socket_copier_part::send_name(const string &fp) const
{
    string s = getFilePathSplitByAbs(fp, 3).value();
    return status(base.s_send(s.c_str(), s.length()));
}

/** @返回值
 *      0 为成功，-1 为发送失败，-2 为读取文件失败
 */
int Socket_copier_part::send_content(const string &fp) const
{
    FILE *_in;
    int successNum;
    if (!(_in = fopen(fp.c_str(), "rb")))
    {
        return -2;
    }
    else
    {
        char flush[1024];
        while (!feof(_in))
        {
            successNum = (int) fread(flush, 1, 1024, _in);
            if (status(base.s_send(flush, successNum)))
            {
                fclose(_in);
                return -1;
            }
        }
        fclose(_in);
    }
    return 0;
}

/** 网络连接状态
 *  @返回值
 *      0 为成功，-1 为网络未连接， 1 为密码错误
 */
int Socket_copier_part::recv_net_status() const
{
    char buf = -1;
    base.s_recv(&buf, 1);
    return buf == 0 || buf == 1 ? buf : -1;
}

/** 网络连接状态
 *  @返回值
 *      0 为继续发送，1 为不需要发送
 */
int Socket_copier_part::recv_need_2_send_content() const
{
    char buf = 1;
    base.s_recv(&buf, 1);
    return buf ? 1 : 0;
}

#undef status
