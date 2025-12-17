module;

export module urlme;

import std;

export namespace urlme
{
    export struct UrlParts
    {
        std::string host;
        std::uint16_t port{ 80 };
        std::string path{ "/" };
        std::string query{};
    };

    export class URL
    {
    public:
        explicit URL(std::string_view u) { parse(u); }

        const std::string& get_host()  const { return parts.host; }
        const std::string& get_path()  const { return parts.path; }
        const std::string& get_query() const { return parts.query; }
        std::uint16_t      get_port()  const { return parts.port; }

    private:
        UrlParts parts;

        static std::uint16_t parse_port(std::string_view s)
        {
            std::uint16_t p{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), p);
            return ec == std::errc{} ? p : 80;
        }

        void parse(std::string_view u)
        {
            // strip scheme
            if (auto p = u.find("://"); p != std::string_view::npos)
                u.remove_prefix(p + 3);

            // split host/path
            auto slash = u.find('/');
            std::string_view hostport = (slash == std::string_view::npos)
                ? u
                : u.substr(0, slash);

            parts.path = (slash == std::string_view::npos)
                ? "/"
                : std::string(u.substr(slash));

            // split query
            if (auto q = parts.path.find('?'); q != std::string::npos)
            {
                parts.query = parts.path.substr(q);
                parts.path.resize(q);
            }

            // split port
            if (auto c = hostport.find(':'); c != std::string_view::npos)
            {
                parts.host = hostport.substr(0, c);
                parts.port = parse_port(hostport.substr(c + 1));
            }
            else
            {
                parts.host = hostport;
            }

            if (parts.path.empty())
                parts.path = "/";
        }
    };
}