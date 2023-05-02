#ifndef H_NETWORK_SOCKET
#define H_NETWORK_SOCKET

class Socket_TCP
{
private:
    WSADATA wsaData;  // struct
    SOCKET sock;
    bool isSetup = false, isClosed = false;

public:
    int create();

    ~Socket_TCP();

    bool is_setup() const;

    bool is_closed() const;

    int s_connect(const std::string &ip, int port) const;

    int s_send(const char *data, int size) const;

    int s_recv(char *buf, int size) const;

    int s_close();
};

#define PORT 56789

class Socket_copier_part
{
public:
    std::string addr, sk;
    Socket_TCP base;

    Socket_copier_part(const std::string &, const std::string &);

    int send_version() const;

    int send_what(char) const;

    int send_sk() const;

    int send_hash(const std::string &) const;

    int send_name_size(const std::string &) const;

    int send_content_size(const std::string &) const;

    int send_name(const std::string &) const;

    int send_content(const std::string &) const;

    int recv_net_status() const;

    int recv_need_2_send_content() const;
};

#endif
