#ifndef H_INI_PARSER
#define H_INI_PARSER

class IniParser
{
private:
    std::map<std::string, std::map<std::string, std::string>> sections, sections_temp;

public:
    IniParser();

    int read(const std::string &);

    int n_sections();

    void output();

    rff<std::string> get_v_string(const std::string &, const std::string &, const std::string & = "");

    rff<bool> get_v_bool(const std::string &, const std::string &, bool = false);

    rff<int> get_v_int(const std::string &, const std::string &, int = 0.0);

    rff<double> get_v_double(const std::string &, const std::string &, double = 0.0);
};

#endif
