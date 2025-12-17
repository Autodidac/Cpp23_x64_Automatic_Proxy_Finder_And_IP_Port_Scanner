// version.ixx
module;
#include <string>
#include <fstream>

export module version;

export namespace app_version
{
    inline constexpr const char* NAME = "Automatic Proxy Finder and IP/Port Scanner";

    inline std::string read_version_file()
    {
        std::ifstream file("VERSION");
        std::string line;
        if (file && std::getline(file, line))
        {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            {
                line.pop_back();
            }
            if (!line.empty())
            {
                return line;
            }
        }
        return "1.0.1";
    }

    inline const std::string& version()
    {
        static std::string v = read_version_file();
        return v;
    }

    inline const std::string& title()
    {
        static std::string t = std::string(NAME) + " v" + version();
        return t;
    }
}
