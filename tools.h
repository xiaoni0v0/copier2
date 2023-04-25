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
#define outputInfo(...) outputWCol(FOREGROUND_BLUE, __VA_ARGS__)

void outputWCol(unsigned short, const char *, ...);

void print_sep(char, bool = true, int = 60);

void print_sep(const char *, bool = true, int = 60);

tm *get_tm();

bool fileExists(const std::string &);

int copyFile(const std::string &, const std::string &, int);

std::string getExeFileAbsPath();

bool isInUDisk(const std::string &, const std::string &);

rff<std::string> getFilePathSplitByAbs(const std::string &, int);

int run_cmd(const std::string &, std::string &);
