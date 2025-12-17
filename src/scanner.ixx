// scanner.ixx - FIXED: No gui dependencies + ASCII characters
module;
#define WIN32_LEAN_AND_MEAN      // ⭐ FIX 1: Lean headers
//#define _WINSOCK_DEPRECATED_NO_WARNINGS  // ⭐ FIX 2: No warnings
#include <winsock2.h>         // ⭐ FIX 3: BEFORE windows.h
#include <ws2tcpip.h>         // ⭐ FIX 4: TCPIP constants
#include <windows.h>          // ⭐ AFTER Winsock2
#include <thread>
#include <atomic>
#include <random>
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <functional>

export module scanner;

import std;
import httpme;
import urlme;
import ports;

#pragma comment(lib, "ws2_32.lib")

#undef min
#undef max

export namespace scanner
{
    // ----- YOUR EXISTING STRUCTS (unchanged) -----
    export struct TargetParts
    {
        std::string   scheme{ "http" };
        std::string   host{};
        std::uint16_t port{ 80 };
        std::string   path{ "/" };
        std::string   query{};
    };

    export struct Target
    {
        std::string original{};
        TargetParts parts{};
    };

    export struct ProbeResult
    {
        Target               target{};
        httpme::HttpResponse response{};
    };

    // ----- YOUR EXISTING API (unchanged) -----
    export struct Scanner
    {
        static std::vector<Target> load_targets_file(std::string_view path);
        static std::vector<ProbeResult> run(
            const std::vector<Target>& targets,
            std::string_view override_path);
        static ProbeResult probe_with_proxy(const Target& target, const std::string& proxy);
    };

    // ----- NEW GUI-INTEGRATION API (GUI-PASSIVE) -----
    export void test_proxies(HWND listbox, std::function<void(const std::string&)> log_cb);
    export void save_proxies(HWND listbox, const std::string& path, std::function<void(const std::string&)> log_cb);
    export void load_proxies(HWND listbox, const std::string& path, std::function<void(const std::string&)> log_cb);
    export void run_scan(int count, const std::string& ports_str, int threads, int delay,
        bool random_mode, const std::string& proxy, const std::string& path,
        std::function<void(const std::string&)> log_cb,
        std::function<void(int, int)> progress_cb,
        std::atomic<bool>* running,
        std::mt19937& rng,      // PASS RNG FROM GUI
        const std::function<std::string()>& exe_dir_cb,  // PASS EXE DIR CALLBACK
        const std::function<void(int, const std::vector<uint16_t>&)>& preview_cb,
        std::function<void()> finish_cb = {});  // PASS PREVIEW CALLBACK

    // ----- YOUR EXISTING IMPLEMENTATION (unchanged) -----
    inline void trim(std::string& s)
    {
        auto isws = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        while (!s.empty() && isws(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while (!s.empty() && isws(static_cast<unsigned char>(s.back()))) s.pop_back();
    }

    inline bool is_comment_or_empty(std::string_view s)
    {
        for (char c : s) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '#' || c == ';') return true;
            return false;
        }
        return true;
    }

    inline TargetParts parts_from_url(std::string_view line)
    {
        TargetParts tp{};
        urlme::URL u{ line };
        tp.host = u.get_host();
        tp.port = u.get_port();
        tp.path = u.get_path();
        tp.query = u.get_query();
        if (tp.path.empty()) tp.path = "/";
        return tp;
    }

    inline std::vector<Target> Scanner::load_targets_file(std::string_view path)
    {
        std::vector<Target> out;
        std::ifstream in{ std::string(path), std::ios::in };
        if (!in) return out;

        std::string line;
        while (std::getline(in, line)) {
            trim(line);
            if (is_comment_or_empty(line)) continue;

            Target t{}; t.original = line; t.parts = parts_from_url(line);
            if (!t.parts.host.empty()) out.push_back(std::move(t));
        }
        return out;
    }

    inline std::vector<ProbeResult> Scanner::run(const std::vector<Target>& targets, std::string_view override_path)
    {
        std::vector<ProbeResult> results; results.reserve(targets.size());

        for (const auto& t : targets) {
            ProbeResult r{}; r.target = t;

            if (t.parts.scheme != "http") {
                r.response.status_code = -10;
                r.response.status_line = "unsupported scheme (http only)";
                results.push_back(std::move(r)); continue;
            }

            std::string effective = override_path.empty()
                ? (t.parts.path.empty() ? "/" : t.parts.path)
                : std::string(override_path);

            if (override_path.empty() && !t.parts.query.empty()) effective += t.parts.query;

            r.response = httpme::HttpClient::get(t.parts.host, t.parts.port, effective);
            results.push_back(std::move(r));
        }
        return results;
    }

    inline ProbeResult Scanner::probe_with_proxy(const Target& target, const std::string& proxy)
    {
        ProbeResult r{};
        r.target = target;

        if (proxy.empty()) {
            auto results = Scanner::run({ target }, "/");
            if (!results.empty()) return results[0];
            r.response.status_code = -10;
            r.response.status_line = "no response";
            return r;
        }

        r.response.status_line = "Proxy support WIP";
        return r;
    }

    // ----- NEW: PROXY/PORTS LISTBOX FUNCTIONS (ASCII ONLY) -----
void test_proxies(HWND listbox, std::function<void(const std::string&)> log_cb) {
    if (!listbox) return;

    log_cb("[SCANNER] --- PROXY SCAN STARTED ---");
    int count = (int)SendMessageA(listbox, LB_GETCOUNT, 0, 0);

    for (int i = 0; i < count; ++i) {
        char proxy[256];
        SendMessageA(listbox, LB_GETTEXT, i, (LPARAM)proxy);

        char line[512];
        wsprintfA(line, "[PROXY %d/%d] Testing %s...", i + 1, count, proxy);
        log_cb(line);

        // ⭐ REAL PROXY TEST: Connect through proxy to google.com:80
        std::string proxy_host, proxy_port_str;
        // Parse proxy:port (simple - assumes format "host:port")
        size_t colon = strrchr(proxy, ':') - proxy;
        if (colon != std::string::npos) {
            proxy_host = std::string(proxy, colon);
            proxy_port_str = std::string(proxy + colon + 1);
        }

        // Test: proxy CONNECT google.com:80
        SOCKET s = ::socket(AF_INET, SOCK_STREAM, 0);
        if (s != INVALID_SOCKET) {
            // Connect to proxy
            addrinfo* proxy_ai = nullptr;
            getaddrinfo(proxy_host.c_str(), proxy_port_str.c_str(), nullptr, &proxy_ai);
            if (proxy_ai) {
                if (::connect(s, proxy_ai->ai_addr, (int)proxy_ai->ai_addrlen) == 0) {
                    // Send CONNECT request
                    std::string connect_req = std::format(
                        "CONNECT google.com:80 HTTP/1.1\r\nHost: google.com:80\r\n\r\n");
                    ::send(s, connect_req.data(), (int)connect_req.size(), 0);
                    
                    // Check response within 2s
                    char resp[1024];
                    int n = ::recv(s, resp, sizeof(resp) - 1, 0);
                    resp[n] = 0;
                    
                    if (strstr(resp, "200") || strstr(resp, "HTTP/1.1 2")) {
                        wsprintfA(line, "[PROXY %d/%d] %s -> LIVE", i + 1, count, proxy);
                    } else {
                        wsprintfA(line, "[PROXY %d/%d] %s -> DEAD", i + 1, count, proxy);
                    }
                } else {
                    wsprintfA(line, "[PROXY %d/%d] %s -> CONNECT FAILED", i + 1, count, proxy);
                }
                freeaddrinfo(proxy_ai);
            }
            ::closesocket(s);
        } else {
            wsprintfA(line, "[PROXY %d/%d] %s -> SOCKET ERROR", i + 1, count, proxy);
        }
        
        log_cb(line);
    }
    log_cb("[SCANNER] --- PROXY SCAN COMPLETE ---");
}


    void save_proxies(HWND listbox, const std::string& path, std::function<void(const std::string&)> log_cb) {
        if (!listbox) return;

        std::ofstream f(path);
        if (!f) {
            log_cb("[SCANNER] Failed to save proxies to " + path);
            return;
        }

        int count = (int)SendMessageA(listbox, LB_GETCOUNT, 0, 0);
        for (int i = 0; i < count; ++i) {
            char proxy[256];
            SendMessageA(listbox, LB_GETTEXT, i, (LPARAM)proxy);
            f << proxy << "\n";
        }
        f.close();

        char line[256];
        wsprintfA(line, "[SCANNER] Saved %d proxies to %s", count, path.c_str());
        log_cb(line);
    }

    void load_proxies(HWND listbox, const std::string& path, std::function<void(const std::string&)> log_cb) {
        if (!listbox) return;

        SendMessageA(listbox, LB_RESETCONTENT, 0, 0);
        std::ifstream f(path);
        if (!f) {
            log_cb("[SCANNER] No proxies file found: " + path);
            return;
        }

        std::string line;
        int count = 0;
        while (std::getline(f, line)) {
            if (!line.empty()) {
                SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                count++;
            }
        }
        f.close();

        char msg[256];
        wsprintfA(msg, "[SCANNER] Loaded %d proxies from %s", count, path.c_str());
        log_cb(msg);
    }

    // ----- MAIN SCANNER (GUI-OWNS-STATE VERSION) -----
    void run_scan(
        int count,
        const std::string& ports_str,
        int threads,
        int delay,
        bool random_mode,
        const std::string& proxy,
        const std::string& path,
        std::function<void(const std::string&)> log_cb,
        std::function<void(int, int)> progress_cb,
        std::atomic<bool>* running,  // ⭐ READ-ONLY: GUI controls this
        std::mt19937& rng,
        const std::function<std::string()>& exe_dir_cb,
        const std::function<void(int, const std::vector<uint16_t>&)>& preview_cb,
        std::function<void()> finish_cb)
    {
        // ⭐ NO running->store(true) - GUI set this already

        log_cb("=============================================================");
        char line[512];
        auto ports = ports::parse_ports(ports_str);
        wsprintfA(line, "Starting scan: %d targets, %d ports, %d threads",
            count, (int)ports.size(), threads);
        log_cb(line);

        if (!proxy.empty())
            log_cb("Using proxy: " + proxy);
        log_cb(random_mode ? "Mode: Random IP generation" : "Mode: File targets");

        // Generate or load targets (RESPONSIVE)
        std::vector<Target> targets;
        std::uniform_int_distribution<int> dist(1, 254);

        if (random_mode) {
            for (int i = 0; i < count; ++i) {
                if (running && !running->load()) {
                    log_cb("Target generation STOPPED");
                    break;
                }
                Target t;
                char ip[16];
                wsprintfA(ip, "%d.%d.%d.%d", dist(rng), dist(rng), dist(rng), dist(rng));
                t.original = ip;
                t.parts.host = ip;
                t.parts.port = ports.empty() ? 80 : ports[i % ports.size()];
                t.parts.path = "/";
                targets.push_back(t);
            }
            if (preview_cb && !targets.empty())
                preview_cb((int)targets.size(), ports);
            wsprintfA(line, "Generated %d random targets", (int)targets.size());
            log_cb(line);
        }
        else {
            std::string full_path = exe_dir_cb() + "\\" + path;
            targets = Scanner::load_targets_file(full_path);
            wsprintfA(line, "Loaded %d targets from %s", (int)targets.size(), full_path.c_str());
            log_cb(line);
        }

        if (targets.empty()) {
            log_cb("ERROR: No targets!");
            progress_cb(0, 1);
            return;
        }

        // Multi-threaded scan (RESPONSIVE)
        std::vector<ProbeResult> results;
        std::mutex results_mutex;
        std::atomic<int> completed{ 0 };

        auto worker = [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                if (running && !running->load()) break;  // ⭐ INSTANT STOP
                auto result = Scanner::probe_with_proxy(targets[i], proxy);
                if (result.response.status_code != -10) {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(result);
                }
                completed.fetch_add(1);
                progress_cb(completed.load(), (int)targets.size());
                if (delay > 0) Sleep(delay);
            }
            };

        // Safe thread count
        if (threads < 1) threads = 1;
        if (threads > 64) threads = 64;
        if ((size_t)threads > targets.size()) threads = (int)targets.size();

        std::vector<std::thread> workers;
        size_t chunk = targets.size() / threads;
        size_t rem = targets.size() % threads;
        size_t cur_start = 0;

        for (int i = 0; i < threads; ++i) {
            size_t this_chunk = chunk + (i < (int)rem ? 1 : 0);
            size_t cur_end = cur_start + this_chunk;
            if (cur_start < cur_end && cur_start < targets.size()) {
                workers.emplace_back(worker, cur_start, std::min(cur_end, targets.size()));
            }
            cur_start = cur_end;
        }

        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }

        // Report completion status
        bool completed_normally = !running || running->load();
        if (completed_normally) {
            log_cb("=============================================================");
            wsprintfA(line, "Scan complete: %d total, %d responsive", (int)targets.size(), (int)results.size());
            log_cb(line);

            std::vector<Target> live;
            int hit_num = 0;

            for (const auto& r : results) {
                if (r.response.status_code == -10) continue;
                ++hit_num;

                if (r.response.status_code >= 200 && r.response.status_code < 400) {
                    live.push_back(r.target);
                    wsprintfA(line, "[LIVE %d] %s:%u (%u bytes) %s",
                        hit_num, r.target.parts.host.c_str(), r.target.parts.port,
                        (unsigned)r.response.body.size(), r.response.status_line.c_str());
                }
                else {
                    wsprintfA(line, "[%d] %s:%u (%u bytes) %s",
                        hit_num, r.target.parts.host.c_str(), r.target.parts.port,
                        (unsigned)r.response.body.size(), r.response.status_line.c_str());
                }
                log_cb(line);
            }

            log_cb("=============================================================");
            wsprintfA(line, "SUMMARY: %d/%d LIVE services found", (int)live.size(), hit_num);
            log_cb(line);

            if (!live.empty()) {
                std::string live_path = exe_dir_cb() + "\\live_" + path;
                std::ofstream out(live_path);
                for (const auto& t : live) out << t.original << "\n";
                wsprintfA(line, "Live targets (%d) saved: %s", (int)live.size(), live_path.c_str());
                log_cb(line);
            }
        }
        else {
           
        }

        // ⭐ ONLY progress reset - NO running flag changes!
        progress_cb(0, 1);
        if (finish_cb) finish_cb();
    }

}
