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
    inline HWND g_lbl_proxy_header = nullptr, g_lbl_proxy_range = nullptr;
    inline HWND g_lbl_proxy_input = nullptr, g_lbl_proxy_scan = nullptr;
    inline HWND g_lbl_ports_header = nullptr, g_lbl_port_label = nullptr;
    inline HWND g_lbl_target_count = nullptr, g_lbl_target_threads = nullptr, g_lbl_target_delay = nullptr, g_lbl_target_file = nullptr;

    inline HWND g_grp_info = nullptr, g_grp_targets = nullptr, g_grp_proxies = nullptr, g_grp_ports = nullptr;
    inline HWND g_grp_output = nullptr;

    inline int g_prog_y = 0;
    inline int g_status_y = 0;
    inline int g_log_y = 0;
    inline int g_output_top = 0;
    inline int g_output_group_height = 0;
    inline constexpr int GROUP_PADDING = 12;

    inline std::atomic<bool> g_running{ false };
    inline std::atomic<bool> g_proxy_searching{ false };
    inline std::mt19937 g_rng(std::random_device{}());

    std::string exe_dir();
    void log_line(const std::string& s);
    void clear_log();
    void update_progress(int current, int total);
    void preview_random_targets(int count, const std::vector<uint16_t>& ports);
    void find_proxies();
    void start_scan();
    void layout_controls(HWND h, HINSTANCE hinst, bool creating);
    bool handle_command(int id);
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

    struct UiMetrics {
        int margin = 12;
        int section_spacing = 12;
        int row_spacing = 10;
        int col_gap = 12;
        int line_height = 22;
        int ctrl_height = 24;
        int content_width = 0;
        int column_width = 0;
        int client_height = 0;
    };

    struct RowLayout {
        int y{};
        int spacing{};
        int take(int height) {
            int current = y;
            y += height + spacing;
            return current;
        }
    };

    struct PortLayout {
        int left{};
        int inner_width{};
        int list_label_top{};
        int list_top{};
        int list_height{};
        int load_top{};
        int load_width{};
        int port_row_top{};
        int label_width{};
        int button_width{};
        int button_height{};
        int input_width{};
        int entry_height{};
        int hgap{};
    };

    struct ProxyLayout {
        int left{};
        int top{};
        int inner_width{};
        int list_label_top{};
        int list_top{};
        int list_height{};
        int range_row_top{};
        int proxy_row_top{};
        int actions_top{};
        int hgap{};
        int button_width{};
        int button_height{};
        int small_button_width{};
        int range_label_width{};
        int range_input_left{};
        int range_input_width{};
        int find_left{};
        int find_width{};
        int proxy_label_width{};
        int proxy_input_left{};
        int proxy_input_width{};
        int add_left{};
        int remove_left{};
        int scan_label_width{};
        int scan_left{};
        int save_left{};
        int load_left{};
    };

    UiMetrics calculate_metrics(HWND h) {
        UiMetrics metrics{};
        HDC dc = GetDC(h);
        if (dc) {
            TEXTMETRICA tm{};
            if (GetTextMetricsA(dc, &tm)) {
                metrics.line_height = std::max(18, static_cast<int>(tm.tmHeight + tm.tmExternalLeading));
                metrics.ctrl_height = metrics.line_height + 2;
                metrics.section_spacing = std::max(12, metrics.line_height / 2);
                metrics.row_spacing = std::max(8, metrics.line_height / 2);
            }
            ReleaseDC(h, dc);
        }

        RECT rc{};
        GetClientRect(h, &rc);
        metrics.client_height = rc.bottom - rc.top;
        metrics.content_width = (rc.right - rc.left) - (metrics.margin * 2);
        metrics.column_width = (metrics.content_width - metrics.col_gap) / 2;
        return metrics;
    }

    int compute_header_height(const UiMetrics& metrics) {
        return (metrics.line_height * 2) + (GROUP_PADDING * 2);
    }

    int compute_target_height(const UiMetrics& metrics) {
        int rows = 3;
        return (GROUP_PADDING * 2) + (metrics.ctrl_height * rows) + (metrics.row_spacing * 2) + metrics.line_height;
    }

    int compute_manage_height(const UiMetrics& metrics) {
        int desired_list_height = std::max(metrics.line_height * 6, 120);
        int button_stack = metrics.ctrl_height * 3;
        int spacing_stack = metrics.row_spacing * 3;
        return (GROUP_PADDING * 2) + metrics.line_height + desired_list_height + button_stack + spacing_stack;
    }

    int compute_min_output_height(const UiMetrics& metrics) {
        return std::max(metrics.line_height * 6, 150);
    }

    PortLayout compute_port_layout(const RECT& group_rc, const UiMetrics& metrics) {
        PortLayout layout{};
        layout.left = group_rc.left + GROUP_PADDING;
        int right = group_rc.right - GROUP_PADDING;
        int inner_top = group_rc.top + GROUP_PADDING + 2;
        int inner_bottom = group_rc.bottom - GROUP_PADDING;
        layout.inner_width = right - layout.left;

        layout.hgap = metrics.margin / 2;
        layout.label_width = 50;
        layout.button_width = 60;
        layout.button_height = metrics.ctrl_height;
        layout.entry_height = metrics.ctrl_height;

        layout.input_width = layout.inner_width - layout.label_width - (layout.button_width * 2) - (layout.hgap * 2);
        if (layout.input_width < 60) layout.input_width = 60;

        int inner_height = inner_bottom - inner_top;
        int list_height = inner_height - layout.button_height - layout.entry_height - (metrics.row_spacing * 2) - metrics.line_height;
        if (list_height < metrics.line_height * 5) list_height = metrics.line_height * 5;

        layout.load_width = layout.inner_width / 3;
        if (layout.load_width < 110) layout.load_width = 110;
        if (layout.load_width > layout.inner_width) layout.load_width = layout.inner_width;

        RowLayout rows{ inner_top, metrics.row_spacing };
        layout.list_label_top = rows.take(metrics.line_height + list_height);
        layout.list_top = layout.list_label_top + metrics.line_height;
        layout.list_height = list_height;
        layout.load_top = rows.take(layout.button_height);
        layout.port_row_top = rows.take(layout.entry_height);
        return layout;
    }

    ProxyLayout compute_proxy_layout(const RECT& group_rc, const UiMetrics& metrics) {
        ProxyLayout layout{};
        layout.left = group_rc.left + GROUP_PADDING;
        layout.top = group_rc.top + GROUP_PADDING + 2;
        int right = group_rc.right - GROUP_PADDING;
        int inner_bottom = group_rc.bottom - GROUP_PADDING;
        layout.inner_width = right - layout.left;

        layout.hgap = metrics.margin / 2;
        layout.button_width = 90;
        layout.button_height = metrics.ctrl_height;
        layout.small_button_width = 50;
        layout.range_label_width = 115;
        layout.range_input_width = 60;
        layout.proxy_label_width = 50;

        int inner_height = inner_bottom - layout.top;
        int list_height = inner_height - (layout.button_height * 3) - (metrics.row_spacing * 3) - metrics.line_height;
        if (list_height < metrics.line_height * 5) list_height = metrics.line_height * 5;

        RowLayout rows{ layout.top, metrics.row_spacing };
        layout.list_label_top = rows.take(metrics.line_height + list_height);
        layout.list_top = layout.list_label_top + metrics.line_height;
        layout.list_height = list_height;
        layout.range_row_top = rows.take(layout.button_height);
        layout.proxy_row_top = rows.take(layout.button_height);
        layout.actions_top = rows.take(layout.button_height);

        layout.range_input_left = layout.left + layout.range_label_width + layout.hgap;
        layout.find_left = layout.range_input_left + layout.range_input_width + layout.hgap;
        layout.find_width = layout.inner_width - (layout.range_label_width + layout.hgap + layout.range_input_width + layout.hgap);
        if (layout.find_width < 120) layout.find_width = 120;

        layout.proxy_input_left = layout.left + layout.proxy_label_width + layout.hgap;
        layout.proxy_input_width = layout.inner_width - layout.proxy_label_width - layout.hgap - (layout.button_width * 2) - (layout.hgap * 2);
        if (layout.proxy_input_width < 140) layout.proxy_input_width = 140;
        layout.add_left = layout.proxy_input_left + layout.proxy_input_width + layout.hgap;
        layout.remove_left = layout.add_left + layout.button_width + layout.hgap;

        layout.scan_label_width = layout.inner_width - (layout.button_width + layout.hgap + layout.small_button_width + layout.hgap + layout.small_button_width);
        if (layout.scan_label_width < 140) layout.scan_label_width = 140;
        layout.scan_left = layout.left + layout.scan_label_width + layout.hgap;
        layout.save_left = layout.scan_left + layout.button_width + layout.hgap;
        layout.load_left = layout.save_left + layout.small_button_width + layout.hgap;
        return layout;
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

void gui::layout_controls(HWND hwnd, HINSTANCE hinst, bool creating) {
    UiMetrics metrics = calculate_metrics(hwnd);

    auto place = [&](HWND& handle, const char* cls, const char* text, DWORD style,
        int x, int y, int w, int h, HMENU id = nullptr, DWORD ex_style = 0) {
            if (handle) {
                MoveWindow(handle, x, y, w, h, TRUE);
            }
            else if (creating) {
                handle = CreateWindowExA(ex_style, cls, text, style,
                    x, y, w, h, hwnd, id, hinst, NULL);
            }
        };

    int top = metrics.margin;
    int header_height = compute_header_height(metrics);
    int header_text_height = header_height - (GROUP_PADDING * 2);

    std::string section_header = app_version::title() + "  \r\n  Targets • Proxies • Ports • Output";
    int info_left = metrics.margin + GROUP_PADDING;
    place(g_grp_info, "STATIC", section_header.c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER,
        info_left, top, metrics.content_width - (GROUP_PADDING * 2), header_text_height);

    top += header_height + metrics.section_spacing;

    int target_height = compute_target_height(metrics);
    place(g_grp_targets, "BUTTON", "Target Setup", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        metrics.margin, top, metrics.content_width, target_height, nullptr);

    int inner_left = metrics.margin + GROUP_PADDING;
    int inner_top = top + GROUP_PADDING + 2;
    int label_width = 95;
    int input_left = inner_left + label_width + metrics.row_spacing;

    place(g_lbl_target_count, "STATIC", "Target Count:", WS_CHILD | WS_VISIBLE,
        inner_left, inner_top, label_width, metrics.line_height, nullptr);
    place(g_edt_ips, "EDIT", "10", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        input_left, inner_top - 2, 80, metrics.ctrl_height, nullptr);

    int x = inner_left + (metrics.content_width / 3);
    place(g_lbl_target_threads, "STATIC", "Threads:", WS_CHILD | WS_VISIBLE,
        x, inner_top, 75, metrics.line_height, nullptr);
    place(g_edt_threads, "EDIT", "2", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        x + 78, inner_top - 2, 60, metrics.ctrl_height, nullptr);

    x += metrics.content_width / 3;
    place(g_lbl_target_delay, "STATIC", "Delay (ms):", WS_CHILD | WS_VISIBLE,
        x, inner_top, 80, metrics.line_height, nullptr);
    place(g_edt_delay, "EDIT", "100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        x + 83, inner_top - 2, 70, metrics.ctrl_height, nullptr);

    inner_top += metrics.line_height + metrics.row_spacing;
    place(g_chk_random, "BUTTON", "Random IPs", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        inner_left, inner_top - 2, 110, metrics.ctrl_height, (HMENU)10);
    if (creating) SendMessageA(g_chk_random, BM_SETCHECK, BST_CHECKED, 0);

    place(g_lbl_target_file, "STATIC", "Target File:", WS_CHILD | WS_VISIBLE,
        inner_left + 130, inner_top, 80, metrics.line_height, nullptr);
    place(g_edt_path, "EDIT", "targets.txt", WS_CHILD | WS_VISIBLE | WS_BORDER,
        inner_left + 215, inner_top - 2, 180, metrics.ctrl_height, nullptr);
    place(g_btn_gen, "BUTTON", "Generate", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inner_left + 400, inner_top - 2, 90, metrics.ctrl_height, (HMENU)2);
    place(g_btn_load, "BUTTON", "Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inner_left + 495, inner_top - 2, 80, metrics.ctrl_height, (HMENU)3);
    place(g_btn_save, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inner_left + 580, inner_top - 2, 80, metrics.ctrl_height, (HMENU)4);

    inner_top += metrics.line_height + metrics.row_spacing;
    place(g_btn_scan, "BUTTON", "START SCAN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        inner_left, inner_top - 2, 120, metrics.ctrl_height, (HMENU)5);
    place(g_btn_stop, "BUTTON", "STOP SCAN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inner_left + 130, inner_top - 2, 120, metrics.ctrl_height, (HMENU)1);
    if (creating) EnableWindow(g_btn_stop, FALSE);
    place(g_btn_clear, "BUTTON", "Clear Log", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inner_left + 260, inner_top - 2, 120, metrics.ctrl_height, (HMENU)6);

    top += target_height + metrics.section_spacing;

    int manage_height = compute_manage_height(metrics);
    place(g_grp_proxies, "BUTTON", "Proxy Management", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        metrics.margin, top, metrics.column_width, manage_height, nullptr);

    RECT proxy_group_rc{ metrics.margin, top, metrics.margin + metrics.column_width, top + manage_height };
    ProxyLayout proxy_layout = compute_proxy_layout(proxy_group_rc, metrics);

    place(g_lbl_proxy_header, "STATIC", "Available Proxies:", WS_CHILD | WS_VISIBLE,
        proxy_layout.left, proxy_layout.list_label_top, proxy_layout.inner_width, metrics.line_height, nullptr);
    place(g_lst_proxy, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        proxy_layout.left, proxy_layout.list_top, proxy_layout.inner_width, proxy_layout.list_height, (HMENU)20);

    place(g_lbl_proxy_range, "STATIC", "Random Range:", WS_CHILD | WS_VISIBLE,
        proxy_layout.left, proxy_layout.range_row_top, proxy_layout.range_label_width, metrics.line_height, nullptr);
    place(g_edt_proxy_range, "EDIT", "100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        proxy_layout.range_input_left, proxy_layout.range_row_top - 2, proxy_layout.range_input_width, metrics.ctrl_height, nullptr);
    place(g_btn_proxy_find, "BUTTON", "Find Proxies", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.find_left, proxy_layout.range_row_top - 2, proxy_layout.find_width, proxy_layout.button_height, (HMENU)29);

    place(g_lbl_proxy_input, "STATIC", "Proxy:", WS_CHILD | WS_VISIBLE,
        proxy_layout.left, proxy_layout.proxy_row_top, proxy_layout.proxy_label_width, metrics.line_height, nullptr);
    place(g_edt_proxy_test, "EDIT", "192.168.0.1:8080", WS_CHILD | WS_VISIBLE | WS_BORDER,
        proxy_layout.proxy_input_left, proxy_layout.proxy_row_top - 2, proxy_layout.proxy_input_width, metrics.ctrl_height, nullptr);
    place(g_btn_proxy_add, "BUTTON", "+", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.add_left, proxy_layout.proxy_row_top - 2, proxy_layout.button_width, proxy_layout.button_height, (HMENU)21);
    place(g_btn_proxy_remove, "BUTTON", "-", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.remove_left, proxy_layout.proxy_row_top - 2, proxy_layout.button_width, proxy_layout.button_height, (HMENU)22);

    place(g_lbl_proxy_scan, "STATIC", "Scan Selected Proxy:", WS_CHILD | WS_VISIBLE,
        proxy_layout.left, proxy_layout.actions_top, proxy_layout.scan_label_width, metrics.line_height, nullptr);
    place(g_btn_proxy_scan, "BUTTON", "Test Latency", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.scan_left, proxy_layout.actions_top - 2, proxy_layout.button_width, proxy_layout.button_height, (HMENU)23);
    place(g_btn_proxy_save, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.save_left, proxy_layout.actions_top - 2, proxy_layout.small_button_width, proxy_layout.button_height, (HMENU)27);
    place(g_btn_proxy_load, "BUTTON", "Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        proxy_layout.load_left, proxy_layout.actions_top - 2, proxy_layout.small_button_width, proxy_layout.button_height, (HMENU)28);

    place(g_grp_ports, "BUTTON", "Port Management", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        metrics.margin + metrics.column_width + metrics.col_gap, top, metrics.column_width, manage_height, nullptr);

    RECT ports_group_rc{ metrics.margin + metrics.column_width + metrics.col_gap, top,
        metrics.margin + metrics.column_width + metrics.col_gap + metrics.column_width, top + manage_height };
    PortLayout ports_layout = compute_port_layout(ports_group_rc, metrics);

    place(g_lbl_ports_header, "STATIC", "Ports to Scan:", WS_CHILD | WS_VISIBLE,
        ports_layout.left, ports_layout.list_label_top, ports_layout.inner_width, metrics.line_height, nullptr);
    place(g_lst_ports, "LISTBOX", std::string(ports::COMMON_PORTS).c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        ports_layout.left, ports_layout.list_top, ports_layout.inner_width, ports_layout.list_height, (HMENU)24);
    place(g_btn_common_ports, "BUTTON", "Load Common Ports", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        ports_layout.left, ports_layout.load_top - 2, ports_layout.load_width, ports_layout.button_height, (HMENU)12);
    place(g_lbl_port_label, "STATIC", "Port:", WS_CHILD | WS_VISIBLE,
        ports_layout.left, ports_layout.port_row_top, ports_layout.label_width, metrics.line_height, nullptr);
    place(g_edt_ports, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        ports_layout.left + ports_layout.label_width + ports_layout.hgap, ports_layout.port_row_top - 2, ports_layout.input_width, metrics.ctrl_height, nullptr);

    int ports_add_left = ports_layout.left + ports_layout.label_width + ports_layout.hgap + ports_layout.input_width + ports_layout.hgap;
    place(g_btn_ports_add, "BUTTON", "+", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        ports_add_left, ports_layout.port_row_top - 2, ports_layout.button_width, ports_layout.button_height, (HMENU)25);
    place(g_btn_ports_remove, "BUTTON", "-", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        ports_add_left + ports_layout.button_width + ports_layout.hgap, ports_layout.port_row_top - 2, ports_layout.button_width, ports_layout.button_height, (HMENU)26);

    top += manage_height + metrics.section_spacing;

    g_output_top = top;
    g_output_group_height = metrics.client_height - g_output_top - metrics.margin;
    int min_output_height = compute_min_output_height(metrics);
    if (g_output_group_height < min_output_height) g_output_group_height = min_output_height;

    place(g_grp_output, "BUTTON", "Output", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        metrics.margin, g_output_top, metrics.content_width, g_output_group_height, nullptr);

    int output_left = metrics.margin + GROUP_PADDING;
    int progress_height = metrics.ctrl_height;
    g_prog_y = g_output_top + GROUP_PADDING;
    g_status_y = g_prog_y + metrics.line_height + (metrics.row_spacing / 2);
    g_log_y = g_status_y + metrics.line_height + (metrics.row_spacing / 2);

    place(g_prog, PROGRESS_CLASS, "", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        output_left, g_prog_y, metrics.content_width - (GROUP_PADDING * 2), progress_height, nullptr);
    std::string status_text = "Ready - Version " + app_version::version() + " | Configure targets, proxies, and ports";
    place(g_lbl_status, "STATIC", status_text.c_str(), WS_CHILD | WS_VISIBLE,
        output_left, g_status_y, metrics.content_width - (GROUP_PADDING * 2), metrics.line_height, nullptr);

    int log_height = g_output_group_height - (g_log_y - g_output_top) - GROUP_PADDING;
    if (log_height < metrics.line_height * 5) log_height = metrics.line_height * 5;
    if (creating && !g_log) {
        g_log = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT",
            "PROXIES: Add/Remove/Scan/Save/Load listbox\r\n"
            "PORTS: Add/Remove from listbox (+/- buttons)\r\n"
            "TARGETS: Configure counts, delays, and files above\r\n",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            output_left, g_log_y, metrics.content_width - (GROUP_PADDING * 2), log_height, hwnd, NULL, hinst, NULL);
    }
    else if (g_log) {
        MoveWindow(g_log, output_left, g_log_y, metrics.content_width - (GROUP_PADDING * 2), log_height, TRUE);
    }
}

bool gui::handle_command(int id) {
    char buf[256];

    if (id == 1) {  // STOP
        g_running.store(false);
        gui::log_line("Scan STOPPED by user");
        gui::enable_controls();
        return true;
    }

    if (id == 5 && !g_running.load()) {  // START SCAN
        g_running.store(true);
        gui::disable_controls();
        std::thread(gui::start_scan).detach();
        return true;
    }

    if (id == 6) {  // CLEAR
        clear_log();
        return true;
    }

    if (id == 12) {  // COMMON PORTS
        if (!g_lst_ports) {
            log_line("ERROR: Ports listbox not ready");
            return true;
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
        return true;
    }

    if (id == 21) {  // ADD PROXY
        if (!g_lst_proxy || !g_edt_proxy_test) return true;
        GetWindowTextA(g_edt_proxy_test, buf, sizeof(buf));
        if (buf[0]) {
            int idx = (int)SendMessageA(g_lst_proxy, LB_ADDSTRING, 0, (LPARAM)buf);
            SendMessageA(g_lst_proxy, LB_SETCURSEL, idx, 0);
            SetWindowTextA(g_edt_proxy_test, "");
            log_line(std::string("Added proxy: ") + buf);
        }
        return true;
    }

    if (id == 22) {  // REMOVE PROXY
        if (!g_lst_proxy) return true;
        int sel = (int)SendMessageA(g_lst_proxy, LB_GETCURSEL, 0, 0);
        if (sel == LB_ERR || sel < 0) return true;
        char proxy[256] = { 0 };
        SendMessageA(g_lst_proxy, LB_GETTEXT, sel, (LPARAM)proxy);
        SendMessageA(g_lst_proxy, LB_DELETESTRING, sel, 0);
        log_line(std::string("Removed proxy: ") + proxy);
        return true;
    }

    if (id == 23) {  // SCAN PROXIES
        if (g_lst_proxy)
            scanner::test_proxies(g_lst_proxy,
                [](const std::string& line) { gui::log_line(line); });
        return true;
    }

    if (id == 25) {  // ADD PORT
        if (!g_lst_ports || !g_edt_ports) return true;
        GetWindowTextA(g_edt_ports, buf, sizeof(buf));
        if (buf[0] && atoi(buf)) {
            int idx = (int)SendMessageA(g_lst_ports, LB_ADDSTRING, 0, (LPARAM)buf);
            SendMessageA(g_lst_ports, LB_SETCURSEL, idx, 0);
            SetWindowTextA(g_edt_ports, "");
            log_line(std::string("Added port: ") + buf);
        }
        return true;
    }

    if (id == 26) {  // REMOVE PORT
        if (!g_lst_ports) {
            log_line("ERROR: Ports listbox not initialized");
            return true;
        }
        int sel = (int)SendMessageA(g_lst_ports, LB_GETCURSEL, 0, 0);
        if (sel == LB_ERR || sel < 0) {
            log_line("No port selected");
            return true;
        }
        char port[32] = { 0 };
        int len = (int)SendMessageA(g_lst_ports, LB_GETTEXTLEN, sel, 0);
        if (len < 0 || len >= (int)sizeof(port)) {
            log_line("Invalid port text length");
            return true;
        }
        SendMessageA(g_lst_ports, LB_GETTEXT, sel, (LPARAM)port);
        SendMessageA(g_lst_ports, LB_DELETESTRING, sel, 0);
        log_line(std::string("Removed port: ") + port);
        return true;
    }

    if (id == 27) {  // SAVE PROXIES
        if (g_lst_proxy)
            scanner::save_proxies(g_lst_proxy, exe_dir() + "\\proxies.txt",
                [](const std::string& line) { gui::log_line(line); });
        return true;
    }

    if (id == 28) {  // LOAD PROXIES
        if (g_lst_proxy)
            scanner::load_proxies(g_lst_proxy, exe_dir() + "\\proxies.txt",
                [](const std::string& line) { gui::log_line(line); });
        return true;
    }

    if (id == 29) {  // FIND PROXIES
        gui::find_proxies();
        return true;
    }

    return false;
}

void gui::find_proxies() {
    if (g_proxy_searching.exchange(true)) {
        log_line("[PROXY FIND] Discovery already running");
        return;
    }

    if (!g_lst_proxy || !g_edt_proxy_range) {
        log_line("[PROXY FIND] Controls not ready");
        g_proxy_searching.store(false);
        return;
    }

    if (g_btn_proxy_find) EnableWindow(g_btn_proxy_find, FALSE);

    std::thread([] {
        struct ProxyCleanup {
            ~ProxyCleanup() {
                g_proxy_searching.store(false);
                if (g_btn_proxy_find) EnableWindow(g_btn_proxy_find, TRUE);
            }
        } cleanup;

        char buf[32]{};
        GetWindowTextA(g_edt_proxy_range, buf, sizeof(buf));
        int scan_count = std::max(1, std::min(5000, atoi(buf[0] ? buf : "100")));

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

        auto wait_for_connect = [](SOCKET s, DWORD timeout_ms) {
            constexpr DWORD SLICE_MS = 300;
            DWORD waited = 0;
            while (waited < timeout_ms && g_proxy_searching.load()) {
                WSAPOLLFD pfd{};
                pfd.fd = s;
                pfd.events = POLLOUT;
                int poll_res = WSAPoll(&pfd, 1, static_cast<INT>(std::min<DWORD>(SLICE_MS, timeout_ms - waited)));
                if (poll_res > 0) {
                    if (pfd.revents & POLLOUT) {
                        int so_error = 0;
                        int optlen = sizeof(so_error);
                        if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&so_error, &optlen) == 0 && so_error == 0) {
                            return true;
                        }
                        return false;
                    }
                    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                        return false;
                    }
                }
                else if (poll_res == SOCKET_ERROR) {
                    return false;
                }

                waited += SLICE_MS;
            }
            return false;
        };

        auto worker = [&](int start, int end) {
            thread_local std::mt19937 rng_local{ std::random_device{}() };
            std::uniform_int_distribution<int> dist(1, 254);
            for (int i = start; i < end && g_proxy_searching.load(); ++i) {
                char ip[16];
                wsprintfA(ip, "%d.%d.%d.%d", dist(rng_local), dist(rng_local), dist(rng_local), dist(rng_local));

                for (uint16_t port : proxy_ports) {
                    if (!g_proxy_searching.load()) break;
                    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (s != INVALID_SOCKET) {
                        u_long mode = 1;
                        if (ioctlsocket(s, FIONBIO, &mode) == SOCKET_ERROR) {
                            ::closesocket(s);
                            continue;
                        }

                        sockaddr_in addr{};
                        addr.sin_family = AF_INET;
                        addr.sin_port = htons(port);
                        inet_pton(AF_INET, ip, &addr.sin_addr);

                        bool connected = false;
                        int result = ::connect(s, (sockaddr*)&addr, sizeof(addr));
                        if (result == 0) {
                            connected = true;
                        }
                        else {
                            int err = WSAGetLastError();
                            if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
                                connected = wait_for_connect(s, 2500);
                            }
                        }

                        if (connected) {
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
    }).detach();
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
    case WM_CREATE:
        layout_controls(h, hinst, true);
        break;

    case WM_COMMAND: {
        int id = LOWORD(w);
        if (handle_command(id)) return 0;
        break;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(l);
        if (mmi) {
            UiMetrics metrics = calculate_metrics(h);
            int min_width = metrics.margin * 2 + (metrics.line_height * 30) + metrics.col_gap;
            int min_height = metrics.margin * 2 + compute_header_height(metrics) + compute_target_height(metrics)
                + compute_manage_height(metrics) + compute_min_output_height(metrics) + (metrics.section_spacing * 3);
            mmi->ptMinTrackSize.x = min_width;
            mmi->ptMinTrackSize.y = min_height;
        }
        break;
    }

    case WM_SIZE:
        layout_controls(h, hinst, false);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(h, m, w, l);
    }
    return 0;
}

