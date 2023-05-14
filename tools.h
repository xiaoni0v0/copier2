#ifndef H_TOOLS
#define H_TOOLS

template<class T>
class rff // return for function
{
private:
    int _status{};
    T _value;

public:
    rff(int status, T val)
    {
        _status = status;
        _value = val;
    }

    int status()
    {
        return _status;
    }

    T value()
    {
        return _value;
    }
};

#define outputWarning(...) outputWCol(0xe, __VA_ARGS__)
#define outputError(...) outputWCol(FOREGROUND_RED, __VA_ARGS__)
#define outputSuccess(...) outputWCol(FOREGROUND_GREEN, __VA_ARGS__)

void outputWCol(unsigned short, const char *, ...);

void print_sep(char, bool = true, int = 60);

tm *get_tm();

int copyFile(const std::string &, const std::string &, int);

std::string getExeFileAbsPath();

bool isInUDisk(const std::string &, const std::string &);

rff<std::string> getFilePathSplitByAbs(const std::string &, int);

rff<std::string> run_cmd(const std::string &);

int getMD5FromFile(const std::string &, char[16]);

int getFileSize(const std::string &);

char *GBKCharToUTF8Char(const char *);

std::string GBKStringToUTF8String(const std::string &);

#endif
