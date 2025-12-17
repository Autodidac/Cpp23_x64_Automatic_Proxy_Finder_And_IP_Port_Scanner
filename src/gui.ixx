// gui.ixx - FIXED: Proper braces + ASCII + string_view fix
module;
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <thread>
#include <atomic>
#include <random>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <functional>
#include <future>

export module gui;

import scanner;
import ports;
import version;

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "ws2_32.lib")

#undef min
#undef max

export namespace gui
{
    inline HWND g_log = nullptr, g_btn_scan = nullptr, g_btn_stop = nullptr, g_btn_gen = nullptr;
    inline HWND g_btn_save = nullptr, g_btn_load = nullptr, g_btn_clear = nullptr, g_btn_common_ports = nullptr;
        inline HWND g_cbo_proxy = nullptr;
    inline HWND g_lst_proxy = nullptr, g_lst_ports = nullptr, g_edt_ips = nullptr, g_edt_ports = nullptr, g_prog = nullptr;
    inline HWND g_chk_random = nullptr, g_lbl_status = nullptr, g_edt_threads = nullptr;
    inline HWND g_edt_delay = nullptr, g_edt_path = nullptr, g_edt_proxy_test = nullptr;
    inline HWND g_btn_proxy_add = nullptr, g_btn_proxy_remove = nullptr, g_btn_proxy_scan = nullptr;
    inline HWND g_btn_ports_add = nullptr, g_btn_ports_remove = nullptr, g_btn_proxy_save = nullptr, g_btn_proxy_load = nullptr;
    inline HWND g_btn_proxy_find = nullptr;
    inline HWND g_edt_proxy_range = nullptr;

    inline int g_prog_y = 0;
    inline int g_status_y = 0;
    inline int g_log_y = 0;

    inline std::atomic<bool> g_running{ false };
    inline std::mt19937 g_rng(std::random_device{}());

    std::string exe_dir();
    void log_line(const std::string& s);
    void clear_log();
    void update_progress(int current, int total);
    void preview_random_targets(int count, const std::vector<uint16_t>& ports);
    void find_proxies();
    void start_scan();
    LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l);

    void disable_controls() {
        EnableWindow(g_btn_scan, FALSE);
        EnableWindow(g_btn_gen, FALSE);
        EnableWindow(g_btn_load, FALSE);
        EnableWindow(g_btn_save, FALSE);
        EnableWindow(g_btn_proxy_add, FALSE);
        EnableWindow(g_btn_proxy_remove, FALSE);
        EnableWindow(g_btn_proxy_scan, FALSE);
        EnableWindow(g_btn_proxy_find, FALSE);
        EnableWindow(g_btn_ports_add, FALSE);
        EnableWindow(g_btn_ports_remove, FALSE);
        EnableWindow(g_btn_clear, FALSE);
        EnableWindow(g_btn_common_ports, FALSE);
        EnableWindow(g_edt_ips, FALSE);
        EnableWindow(g_edt_threads, FALSE);
        EnableWindow(g_edt_delay, FALSE);
        EnableWindow(g_btn_stop, TRUE);
    }

    void enable_controls() {
        EnableWindow(g_btn_scan, TRUE);
        EnableWindow(g_btn_gen, TRUE);
        EnableWindow(g_btn_load, TRUE);
        EnableWindow(g_btn_save, TRUE);
        EnableWindow(g_btn_proxy_add, TRUE);
        EnableWindow(g_btn_proxy_remove, TRUE);
        EnableWindow(g_btn_proxy_scan, TRUE);
        EnableWindow(g_btn_proxy_find, TRUE);
        EnableWindow(g_btn_ports_add, TRUE);
        EnableWindow(g_btn_ports_remove, TRUE);
        EnableWindow(g_btn_clear, TRUE);
        EnableWindow(g_btn_common_ports, TRUE);
        EnableWindow(g_edt_ips, TRUE);
        EnableWindow(g_edt_threads, TRUE);
        EnableWindow(g_edt_delay, TRUE);
        EnableWindow(g_btn_stop, FALSE);
        update_progress(0, 1);
    }
}

std::string gui::exe_dir() {
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string p(path);
    return p.substr(0, p.find_last_of("\\/"));
}

void gui::log_line(const std::string& s) {
    if (!g_log) return;
    int len = GetWindowTextLengthA(g_log);
    SendMessageA(g_log, EM_SETSEL, len, len);
    SendMessageA(g_log, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
    SendMessageA(g_log, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
    SendMessageA(g_log, EM_SCROLLCARET, 0, 0);
}

void gui::clear_log() {
    if (g_log) SetWindowTextA(g_log, "Log cleared\r\n");
}

void gui::update_progress(int current, int total) {
    if (g_prog && total > 0) {
        int percent = (current * 100) / total;
        SendMessageA(g_prog, PBM_SETPOS, percent, 0);
        char status[64];
        wsprintfA(status, "%d/%d (%d%%)", current, total, percent);
        SetWindowTextA(g_lbl_status, status);
    }
}

void gui::preview_random_targets(int count, const std::vector<uint16_t>& ports) {
    int preview_count = std::min(count, 20);
    log_line("--- TARGETS PREVIEW (first 20) ---");
    std::uniform_int_distribution<int> dist(1, 254);
    for (int i = 0; i < preview_count; ++i) {
        char ip[16];
        wsprintfA(ip, "%d.%d.%d.%d", dist(g_rng), dist(g_rng), dist(g_rng), dist(g_rng));
        uint16_t port = ports.empty() ? 80 : ports[i % ports.size()];
        char line[128];
        wsprintfA(line, "[%d] %s:%u /", i + 1, ip, port);
        log_line(line);
    }
    if (count > 20) log_line("... and " + std::to_string(count - 20) + " more");
    log_line("---------------------------------------");
}

void gui::find_proxies() {
    if (!g_lst_proxy || !g_edt_proxy_range) {
        log_line("[PROXY FIND] Controls not ready");
        return;
    }

    char buf[32];
    GetWindowTextA(g_edt_proxy_range, buf, sizeof(buf));
    int scan_count = std::max(100, std::min(5000, atoi(buf[0] ? buf : "1000")));

    log_line("--- PROXY DISCOVERY STARTED ---");
    log_line(std::string("Scanning ") + std::to_string(scan_count) + " random IPs on common proxy ports...");

    std::vector<uint16_t> proxy_ports = { 8080, 3128, 1080, 4145, 8000, 8118 };

    std::atomic<int> live_proxies{ 0 };
    std::mutex proxy_mutex;

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        log_line("[PROXY FIND] WSAStartup failed");
        return;
    }

    auto worker = [&](int start, int end) {
        thread_local std::mt19937 rng_local{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(1, 254);
        for (int i = start; i < end; ++i) {
            char ip[16];
            wsprintfA(ip, "%d.%d.%d.%d", dist(rng_local), dist(rng_local), dist(rng_local), dist(rng_local));

            for (uint16_t port : proxy_ports) {
                SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (s != INVALID_SOCKET) {
                    sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(port);
                    inet_pton(AF_INET, ip, &addr.sin_addr);

                    if (::connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
                        std::lock_guard<std::mutex> lock(proxy_mutex);
                        char proxy[64];
                        wsprintfA(proxy, "%s:%u", ip, port);
                        SendMessageA(g_lst_proxy, LB_ADDSTRING, 0, (LPARAM)proxy);
                        live_proxies.fetch_add(1);
                        log_line(std::string("LIVE PROXY: ") + proxy);
                    }
                    ::closesocket(s);
                }
            }
        }
    };

    std::vector<std::thread> hunters;
    int chunk = scan_count / 8;
    for (int i = 0; i < 8; ++i) {
        int end = (i == 7) ? scan_count : (i + 1) * chunk;
        hunters.emplace_back(worker, i * chunk, end);
    }

    for (auto& t : hunters) t.join();

    WSACleanup();

    char summary[128];
    wsprintfA(summary, "Found %d live proxies!", live_proxies.load());
    log_line(summary);
    log_line("--- PROXY DISCOVERY COMPLETE ---");
}

void gui::start_scan() {
    char buf[256];
    GetWindowTextA(g_edt_ips, buf, sizeof(buf));
    int count = std::max(1, atoi(buf[0] ? buf : "1"));  // ⭐ VALIDATED

    GetWindowTextA(g_edt_ports, buf, sizeof(buf));
    std::string ports_str = buf[0] ? buf : std::string(ports::COMMON_PORTS);

    // Get ports from LISTBOX
    if (g_lst_ports) {
        int port_count = SendMessageA(g_lst_ports, LB_GETCOUNT, 0, 0);
        ports_str.clear();
        for (int i = 0; i < port_count; i++) {
            char port[32];
            SendMessageA(g_lst_ports, LB_GETTEXT, i, (LPARAM)port);
            ports_str += port;
            if (i < port_count - 1) ports_str += ",";
        }
    }

    GetWindowTextA(g_edt_threads, buf, sizeof(buf));

    unsigned requested = buf[0] ? static_cast<unsigned>(std::atoi(buf)) : std::thread::hardware_concurrency(); 
    if (requested == 0) requested = 2;          // hardware_concurrency() may return 0

    int threads = std::max(1, std::min(static_cast<int>(requested), 64));

    //GetWindowTextA(g_edt_threads, buf, sizeof(buf));
    //int threads = std::max(1, std::min(atoi(buf[0] ? buf : "8"), 64));
    GetWindowTextA(g_edt_delay, buf, sizeof(buf));
    int delay = std::max(0, atoi(buf[0] ? buf : "10"));
    GetWindowTextA(g_edt_path, buf, sizeof(buf));
    std::string path = buf[0] ? buf : "targets.txt";

    bool random_mode = SendMessageA(g_chk_random, BM_GETCHECK, 0, 0) == BST_CHECKED;

    std::string proxy;
    if (g_lst_proxy) {
        int sel = SendMessageA(g_lst_proxy, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR) {
            char pbuf[256];
            SendMessageA(g_lst_proxy, LB_GETTEXT, sel, (LPARAM)pbuf);
            proxy = pbuf;
        }
    }

    // ⭐ LAMBDA-FREE: Pre-create std::function callbacks
    std::function<void(const std::string&)> log_cb = [](const std::string& line) {
        gui::log_line(line);
        };
    std::function<void(int, int)> progress_cb = [](int cur, int total) {
        gui::update_progress(cur, total);
        };
    std::function<std::string()> exe_dir_cb = []() {
        return gui::exe_dir();
        };
    std::function<void(int, const std::vector<uint16_t>&)> preview_cb =
        [](int cnt, const std::vector<uint16_t>& prts) {
        gui::preview_random_targets(cnt, prts);
        };

    // ⭐ NEW FINISH CALLBACK - Enables controls when scan naturally stops
    std::function<void()> finish_cb = []() {
        g_running.store(false);
        gui::log_line("Finished Scanning...");
        gui::enable_controls();  // ⭐ AUTO-ENABLES CONTROLS
        };


    // Start scanning using a future to track the result of the task
  //  std::future<void> scan_future = std::async(std::launch::async, [&]() {
        scanner::run_scan(count, ports_str, threads, delay, random_mode, proxy, path,
            log_cb, progress_cb, &g_running, g_rng, exe_dir_cb, preview_cb, finish_cb);
       // });

    // ⭐ PERFECT CALL with finish callback
  //  scanner::run_scan(count, ports_str, threads, delay, random_mode, proxy, path,
  //      log_cb, progress_cb, &g_running, g_rng, exe_dir_cb, finish_cb, preview_cb);
}


LRESULT CALLBACK gui::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtrA(h, GWLP_HINSTANCE);

    switch (m) {
    case WM_CREATE: {
        RECT rc{}; GetClientRect(h, &rc);
        const int margin = 10;
        const int section_spacing = 12;
        const int line_height = 22;
        const int ctrl_height = 24;
        const int content_width = rc.right - rc.left - (margin * 2);
        const int col_gap = 12;
        const int column_width = (content_width - col_gap) / 2;

        int top = margin;

        CreateWindowA("STATIC", app_version::TITLE, WS_CHILD | WS_VISIBLE | SS_CENTER,
            margin, top, content_width, line_height + 4, h, NULL, hinst, NULL);
        top += line_height + 8;

        HWND grp_setup = CreateWindowA("BUTTON", "Scan Setup", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin, top, content_width, 98, h, NULL, hinst, NULL);
        int inner_left = margin + 12;
        int inner_top = top + 22;

        (void)grp_setup;

        int x = inner_left;
        CreateWindowA("STATIC", "Target Count:", WS_CHILD | WS_VISIBLE, x, inner_top, 95, line_height, h, NULL, hinst, NULL);
        g_edt_ips = CreateWindowA("EDIT", "10", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            x + 98, inner_top - 2, 80, ctrl_height, h, NULL, hinst, NULL);

        x += 200;
        CreateWindowA("STATIC", "Threads:", WS_CHILD | WS_VISIBLE, x, inner_top, 75, line_height, h, NULL, hinst, NULL);
        g_edt_threads = CreateWindowA("EDIT", "2", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            x + 78, inner_top - 2, 60, ctrl_height, h, NULL, hinst, NULL);

        x += 170;
        CreateWindowA("STATIC", "Delay (ms):", WS_CHILD | WS_VISIBLE, x, inner_top, 80, line_height, h, NULL, hinst, NULL);
        g_edt_delay = CreateWindowA("EDIT", "100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            x + 83, inner_top - 2, 70, ctrl_height, h, NULL, hinst, NULL);

        inner_top += line_height + 10;
        g_chk_random = CreateWindowA("BUTTON", "Random IPs", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            inner_left, inner_top - 2, 110, ctrl_height, h, (HMENU)10, hinst, NULL);
        SendMessageA(g_chk_random, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowA("STATIC", "Target File:", WS_CHILD | WS_VISIBLE, inner_left + 130, inner_top, 80, line_height, h, NULL, hinst, NULL);
        g_edt_path = CreateWindowA("EDIT", "targets.txt", WS_CHILD | WS_VISIBLE | WS_BORDER,
            inner_left + 215, inner_top - 2, 180, ctrl_height, h, NULL, hinst, NULL);
        g_btn_gen = CreateWindowA("BUTTON", "Generate", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            inner_left + 400, inner_top - 2, 90, ctrl_height + 2, h, (HMENU)2, hinst, NULL);
        g_btn_load = CreateWindowA("BUTTON", "Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            inner_left + 495, inner_top - 2, 80, ctrl_height + 2, h, (HMENU)3, hinst, NULL);
        g_btn_save = CreateWindowA("BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            inner_left + 580, inner_top - 2, 80, ctrl_height + 2, h, (HMENU)4, hinst, NULL);

        top += 98 + section_spacing;

        HWND grp_proxies = CreateWindowA("BUTTON", "Proxy Management", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin, top, column_width, 240, h, NULL, hinst, NULL);
        int proxy_left = margin + 12;
        int proxy_top = top + 24;

        (void)grp_proxies;

        CreateWindowA("STATIC", "Available Proxies:", WS_CHILD | WS_VISIBLE, proxy_left, proxy_top, column_width - 30, line_height, h, NULL, hinst, NULL);
        proxy_top += line_height;
        g_lst_proxy = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            proxy_left, proxy_top, column_width - 24, 120, h, (HMENU)20, hinst, NULL);
        proxy_top += 126;

        CreateWindowA("STATIC", "Random Range:", WS_CHILD | WS_VISIBLE, proxy_left, proxy_top, 95, line_height, h, NULL, hinst, NULL);
        g_edt_proxy_range = CreateWindowA("EDIT", "1000", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            proxy_left + 100, proxy_top - 2, 70, ctrl_height, h, NULL, hinst, NULL);
        g_btn_proxy_find = CreateWindowA("BUTTON", "Find Proxies", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left + 175, proxy_top - 2, 115, ctrl_height + 2, h, (HMENU)29, hinst, NULL);
        proxy_top += line_height + 12;

        CreateWindowA("STATIC", "Proxy:", WS_CHILD | WS_VISIBLE, proxy_left, proxy_top, 50, line_height, h, NULL, hinst, NULL);
        g_edt_proxy_test = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            proxy_left + 55, proxy_top - 2, 150, ctrl_height, h, NULL, hinst, NULL);
        g_btn_proxy_add = CreateWindowA("BUTTON", "Add", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left + 210, proxy_top - 2, 75, ctrl_height + 2, h, (HMENU)21, hinst, NULL);
        g_btn_proxy_remove = CreateWindowA("BUTTON", "Remove", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left + 290, proxy_top - 2, 85, ctrl_height + 2, h, (HMENU)22, hinst, NULL);
        proxy_top += line_height + 12;

        g_btn_proxy_scan = CreateWindowA("BUTTON", "Scan", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left, proxy_top - 2, 80, ctrl_height + 2, h, (HMENU)23, hinst, NULL);
        g_btn_proxy_save = CreateWindowA("BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left + 85, proxy_top - 2, 80, ctrl_height + 2, h, (HMENU)27, hinst, NULL);
        g_btn_proxy_load = CreateWindowA("BUTTON", "Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            proxy_left + 170, proxy_top - 2, 80, ctrl_height + 2, h, (HMENU)28, hinst, NULL);

        HWND grp_ports = CreateWindowA("BUTTON", "Port Management", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin + column_width + col_gap, top, column_width, 240, h, NULL, hinst, NULL);
        int port_left = margin + column_width + col_gap + 12;
        int port_top = top + 24;

        (void)grp_ports;

        CreateWindowA("STATIC", "Ports to Scan:", WS_CHILD | WS_VISIBLE, port_left, port_top, column_width - 30, line_height, h, NULL, hinst, NULL);
        port_top += line_height;
        g_lst_ports = CreateWindowA("LISTBOX", std::string(ports::COMMON_PORTS).c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            port_left, port_top, column_width - 24, 120, h, (HMENU)24, hinst, NULL);
        port_top += 126;

        g_btn_common_ports = CreateWindowA("BUTTON", "Load Common", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            port_left, port_top - 2, 110, ctrl_height + 2, h, (HMENU)12, hinst, NULL);
        port_top += line_height + 10;

        CreateWindowA("STATIC", "Port:", WS_CHILD | WS_VISIBLE, port_left, port_top, 45, line_height, h, NULL, hinst, NULL);
        g_edt_ports = CreateWindowA("EDIT", "8080", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            port_left + 50, port_top - 2, 80, ctrl_height, h, NULL, hinst, NULL);
        g_btn_ports_add = CreateWindowA("BUTTON", "+", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            port_left + 135, port_top - 2, 45, ctrl_height + 2, h, (HMENU)25, hinst, NULL);
        g_btn_ports_remove = CreateWindowA("BUTTON", "-", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            port_left + 185, port_top - 2, 45, ctrl_height + 2, h, (HMENU)26, hinst, NULL);

        top += 240 + section_spacing;

        HWND grp_scan = CreateWindowA("BUTTON", "Scan Controls", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin, top, content_width, 74, h, NULL, hinst, NULL);
        int scan_left = margin + 12;
        int scan_top = top + 26;

        (void)grp_scan;

        g_btn_scan = CreateWindowA("BUTTON", "START SCAN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
            scan_left, scan_top - 2, 120, ctrl_height + 2, h, (HMENU)5, hinst, NULL);
        g_btn_stop = CreateWindowA("BUTTON", "STOP SCAN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            scan_left + 130, scan_top - 2, 120, ctrl_height + 2, h, (HMENU)1, hinst, NULL);
        g_btn_clear = CreateWindowA("BUTTON", "Clear Log", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            scan_left + 260, scan_top - 2, 120, ctrl_height + 2, h, (HMENU)6, hinst, NULL);
        EnableWindow(g_btn_stop, FALSE);

        top += 74 + section_spacing;

        g_prog_y = top;
        g_status_y = g_prog_y + 28;
        g_log_y = g_status_y + 26;

        g_prog = CreateWindowA(PROGRESS_CLASS, "", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            margin, g_prog_y, content_width, 22, h, NULL, hinst, NULL);
        g_lbl_status = CreateWindowA("STATIC", "Ready - Configure targets, proxies, and ports", WS_CHILD | WS_VISIBLE,
            margin, g_status_y, content_width, line_height, h, NULL, hinst, NULL);

        int log_height = rc.bottom - g_log_y - margin;
        if (log_height < 120) log_height = 120;
        g_log = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT",
            "PROXIES: Add/Remove/Scan/Save/Load listbox\r\n"
            "PORTS: Add/Remove from listbox (+/- buttons)\r\n"
            "TARGETS: Configure counts, delays, and files above",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            margin, g_log_y, content_width, log_height, h, NULL, hinst, NULL);

        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(w);
        char buf[256];

        // MAIN CONTROLS
        if (id == 1) {  // STOP
            g_running.store(false);
            gui::log_line("Scan STOPPED by user");
            gui::enable_controls();
            return 0;
        }

        if (id == 5 && !g_running.load()) {  // START SCAN
            g_running.store(true);
            gui::disable_controls();
            std::thread(gui::start_scan).detach();
            return 0;
        }


        if (id == 6) {  // CLEAR
            clear_log();
            return 0;
        }

        if (id == 12) {  // COMMON PORTS
            if (!g_lst_ports) {
                log_line("ERROR: Ports listbox not ready");
                return 0;
            }
            SendMessageA(g_lst_ports, LB_RESETCONTENT, 0, 0);
            std::vector<std::string> common_ports = {
                "21","22","23","25","53","80","110","135","139",
                "443","993","995","3389","8080"
            };
            for (const auto& port : common_ports) {
                SendMessageA(g_lst_ports, LB_ADDSTRING, 0, (LPARAM)port.c_str());
            }
            log_line("Loaded 14 common ports to listbox");
            return 0;
        }

        // PROXY CONTROLS
        if (id == 21) {  // ADD PROXY
            if (!g_lst_proxy || !g_edt_proxy_test) return 0;
            GetWindowTextA(g_edt_proxy_test, buf, sizeof(buf));
            if (buf[0]) {
                int idx = (int)SendMessageA(g_lst_proxy, LB_ADDSTRING, 0, (LPARAM)buf);
                SendMessageA(g_lst_proxy, LB_SETCURSEL, idx, 0);
                SetWindowTextA(g_edt_proxy_test, "");
                log_line(std::string("Added proxy: ") + buf);
            }
            return 0;
        }

        if (id == 22) {  // REMOVE PROXY
            if (!g_lst_proxy) return 0;
            int sel = (int)SendMessageA(g_lst_proxy, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR || sel < 0) return 0;
            char proxy[256] = { 0 };
            SendMessageA(g_lst_proxy, LB_GETTEXT, sel, (LPARAM)proxy);
            SendMessageA(g_lst_proxy, LB_DELETESTRING, sel, 0);
            log_line(std::string("Removed proxy: ") + proxy);
            return 0;
        }

        if (id == 23) {  // SCAN PROXIES
            if (g_lst_proxy)
                scanner::test_proxies(g_lst_proxy,
                    [](const std::string& line) { gui::log_line(line); });
            return 0;
        }

        // PORTS CONTROLS
        if (id == 25) {  // ADD PORT
            if (!g_lst_ports || !g_edt_ports) return 0;
            GetWindowTextA(g_edt_ports, buf, sizeof(buf));
            if (buf[0] && atoi(buf)) {
                int idx = (int)SendMessageA(g_lst_ports, LB_ADDSTRING, 0, (LPARAM)buf);
                SendMessageA(g_lst_ports, LB_SETCURSEL, idx, 0);
                SetWindowTextA(g_edt_ports, "");
                log_line(std::string("Added port: ") + buf);
            }
            return 0;
        }

        if (id == 26) {  // REMOVE PORT
            if (!g_lst_ports) {
                log_line("ERROR: Ports listbox not initialized");
                return 0;
            }
            int sel = (int)SendMessageA(g_lst_ports, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR || sel < 0) {
                log_line("No port selected");
                return 0;
            }
            char port[32] = { 0 };
            int len = (int)SendMessageA(g_lst_ports, LB_GETTEXTLEN, sel, 0);
            if (len < 0 || len >= (int)sizeof(port)) {
                log_line("Invalid port text length");
                return 0;
            }
            SendMessageA(g_lst_ports, LB_GETTEXT, sel, (LPARAM)port);
            SendMessageA(g_lst_ports, LB_DELETESTRING, sel, 0);
            log_line(std::string("Removed port: ") + port);
            return 0;
        }

        if (id == 27) {  // SAVE PROXIES
            if (g_lst_proxy)
                scanner::save_proxies(g_lst_proxy, exe_dir() + "\\proxies.txt",
                    [](const std::string& line) { gui::log_line(line); });
            return 0;
        }

        if (id == 28) {  // LOAD PROXIES
            if (g_lst_proxy)
                scanner::load_proxies(g_lst_proxy, exe_dir() + "\\proxies.txt",
                    [](const std::string& line) { gui::log_line(line); });
            return 0;
        }

        if (id == 29) {  // FIND PROXIES
            gui::find_proxies();
            return 0;
        }

        break;
    }

    case WM_SIZE: {
        RECT rc; GetClientRect(h, &rc);
        if (g_prog) MoveWindow(g_prog, 10, g_prog_y, rc.right - 20, 22, TRUE);
        if (g_lbl_status) MoveWindow(g_lbl_status, 10, g_status_y, rc.right - 20, 22, TRUE);
        if (g_log) MoveWindow(g_log, 10, g_log_y, rc.right - 20, rc.bottom - g_log_y - 10, TRUE);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(h, m, w, l);
    }
    return 0;
}
