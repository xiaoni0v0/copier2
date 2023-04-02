#include <cstdio>
#include <cstring>
#include "ini_parser.h"

using namespace std;

int ret;
IniParser iniparser;
string res1;
bool res2;
int res3;
double res4;

int main()
{
    printf("// 读取文件状态：%d\n", ret = iniparser.read("./copier.ini"));

    printf("// 字典键数：%d\n", iniparser.n_sections());
    iniparser.output();

    // 测试
    printf("// string 获取测试\n");
    printf("%d\n", iniparser.get_v_string("copier", "PowerPointPath", res1));
    printf("%s\n\n", res1.c_str());
    printf("// bool 获取测试\n");
    printf("%d\n", iniparser.get_v_bool("copier", "LogMode", res2));
    printf("%d\n\n", res2);
    printf("// int 获取测试\n");
    printf("%d\n", iniparser.get_v_int("copier", "fuck", res3));
    printf("%d\n\n", res3);
    printf("// double 获取测试\n");
    printf("%d\n", iniparser.get_v_double("copier", "shit", res4));
    printf("%f\n\n", res4);
    return 0;
}