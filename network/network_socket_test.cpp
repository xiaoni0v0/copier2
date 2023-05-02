#include <iostream>
#include "network_socket.cpp"
#include "./../tools.cpp"

int main1()
{
    Socket_TCP s;
    printf("s.create:\t%d\n", s.create());
    printf("s.s_connect:\t%d\n", s.s_connect("127.0.0.1", 56789));
    printf("s.s_send:\t%d\n", s.s_send((char *) "abcde", 5));
    printf("s.s_close:\t%d\n", s.s_close());
    return 0;
}

/** 检查网连 */
int main2()
{
    // char buffer[1024];
    Socket_copier_part s("127.0.0.1", "super_pw");
    printf("version: %d\n", s.send_version());
    printf("what: %d\n", s.send_what(0));
    printf("sk: %d\n", s.send_sk());
    printf("-> net_status: %d\n", s.recv_net_status());
    return 0;
}

/** 传输文件 */
int main3()
{
//    string fp = "E:/Programs/copier/copier_cpp/network/network_socket_test.cpp";
//    string fp = "E:/test.xmind";
//    string fp = "E:/icon_.jpg";
    string fp = "E:/魔术方法大总结";
    int need;
//    printf("等待你输入文件名：");
//    getline(cin, fp);
    // char buffer[1024];
    Socket_copier_part s("127.0.0.1", "super_pw");
    printf("version: %d\n", s.send_version());
    printf("what: %d\n", s.send_what(1));
    printf("sk: %d\n", s.send_sk());
    printf("md5: %d\n", s.send_hash(fp));
    printf("name_size: %d\n", s.send_name_size(fp));
    printf("content_size: %d\n", s.send_content_size(fp));
    printf("name: %d\n", s.send_name(fp));
    printf("-> need_content? %d\n", need = s.recv_need_2_send_content());
    if (!need)
    {
        printf("content: %d\n", s.send_content(fp));
    }
    return 0;
}

int main()
{
//    system("chcp 65001");
    main3();
    printf("S!\n");
    return 0;
}
