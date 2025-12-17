module;   // global module fragment: ONLY preprocessor / includes here

export module httpme;  // first named-module line

import std;
import <chrono>;
import <winsock2.h>;
import <ws2tcpip.h>;

export namespace httpme
{
    export struct HttpResponse
    {
        int status_code{ -1 };
        std::string status_line{};
        std::map<std::string, std::string> headers{};
        std::string body{};
    };

    export struct HttpClient
    {
        static HttpResponse get(
            std::string_view host,
            std::uint16_t port,
            std::string_view path_and_query);
    };

    // -------------------------
    // IMPLEMENTATION
    // -------------------------

    export namespace data
    {
#ifdef _WIN32
        struct WsaInit
        {
            bool ok{ false };
            WsaInit() { WSADATA w{}; ok = (WSAStartup(MAKEWORD(2, 2), &w) == 0); }
            ~WsaInit() { if (ok) WSACleanup(); }
        };
#endif

        // ⭐ FIXED: TIMEOUT recv_all() - THIS WAS BLOCKING FOREVER
        std::string recv_all(SOCKET s, int timeout_ms = 3000)
        {
            std::string out;
            char buf[4096];
            auto start = std::chrono::steady_clock::now();

            fd_set readfds;
            timeval tv{ 0, 100000 };  // 100ms select timeout

            for (;;) {
                // Global timeout check
                if (timeout_ms > 0) {
					auto starttime = std::chrono::steady_clock::now() - start;
                    auto elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(starttime).count());
                    if (elapsed > timeout_ms) {
                        break;  // Timeout
                    }
                }

                FD_ZERO(&readfds);
                FD_SET(s, &readfds);

                int n = select(0, &readfds, NULL, NULL, &tv);
                if (n == 0) continue;  // Timeout, try again
                if (n < 0) break;      // Error

                n = ::recv(s, buf, sizeof(buf), 0);
                if (n <= 0) break;
                out.append(buf, buf + n);
            }
            return out;
        }

        void trim(std::string& v)
        {
            auto ws = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
            while (!v.empty() && ws(static_cast<unsigned char>(v.front()))) v.erase(v.begin());
            while (!v.empty() && ws(static_cast<unsigned char>(v.back()))) v.pop_back();
        }

        void parse_headers(std::string_view blob, HttpResponse& r)
        {
            std::istringstream iss(static_cast<std::string>(blob));
            std::string line;

            if (std::getline(iss, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                r.status_line = std::move(line);
                std::istringstream sl(r.status_line);
                std::string httpver;
                sl >> httpver >> r.status_code;
            }

            while (std::getline(iss, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) break;
                auto c = line.find(':');
                if (c == std::string::npos) continue;
                std::string k = line.substr(0, c);
                std::string v = line.substr(c + 1);
                trim(k); trim(v);
                r.headers.emplace(std::move(k), std::move(v));
            }
        }
    }

    HttpResponse
        HttpClient::get(std::string_view host,
            std::uint16_t port,
            std::string_view path_and_query)
    {
        HttpResponse r{};

#ifdef _WIN32
        data::WsaInit wsa;
        if (!wsa.ok) { r.status_line = "WSAStartup failed"; return r; }
#endif

        if (path_and_query.empty()) path_and_query = "/";

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        //hints.ai_protocol = IPPROTO_TCP;

        addrinfo* res = nullptr;
        auto port_s = std::to_string(port);

        if (getaddrinfo(std::string(host).c_str(), port_s.c_str(), &hints, &res) != 0 || !res) {
            r.status_line = "DNS failure";
            return r;
        }

        SOCKET s = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == INVALID_SOCKET) {
            freeaddrinfo(res);
            r.status_line = "socket failed";
            return r;
        }

        // ⭐ FIXED: Non-blocking connect with timeout
        u_long mode = 1;
        ::ioctlsocket(s, FIONBIO, &mode);

        int res_connect = ::connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen));
        if (res_connect == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                closesocket(s);
                freeaddrinfo(res);
                r.status_line = "connect failed";
                return r;
            }
        }

        // Poll for connect completion (5s timeout)
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(s, &writefds);
        timeval tv_connect = { 5, 0 };
        if (::select(0, NULL, &writefds, NULL, &tv_connect) <= 0) {
            closesocket(s);
            freeaddrinfo(res);
            r.status_line = "connect timeout";
            return r;
        }

        freeaddrinfo(res);

        const auto req = std::format("GET {} HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n", path_and_query, host);
        ::send(s, req.data(), static_cast<int>(req.size()), 0);

        // ⭐ FIXED: recv_all with 3s timeout
        const auto raw = data::recv_all(s, 3000);
        closesocket(s);

        const auto sep = raw.find("\r\n\r\n");
        if (sep != std::string::npos) {
            data::parse_headers(raw.substr(0, sep), r);
            r.body = raw.substr(sep + 4);
        }
        else {
            r.body = std::move(raw);
        }

        return r;
    }
}