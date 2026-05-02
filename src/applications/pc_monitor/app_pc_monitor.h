#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <lvgl.h>

#include <cmath>
#include <memory>
#include <mutex>
#include <string>

#include "config.h"
#include "modules/common/helpers.h"
#include "modules/common/iapplication.h"

#define APP_PC_MONITOR_HTTP_PORT 80
#define APP_PC_MONITOR_MAX_BODY 4096

class AppPcMonitorClass : public Application {
public:
    AppPcMonitorClass() : drawer_icon_utf8(fa(0xf390)) {}

    const char* get_drawer_icon_text() override { return drawer_icon_utf8.c_str(); }

    void drawer_icon_clicked() override {
        if (screen) return;

        screen = lv_obj_create(nullptr);
        lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(screen, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 4, LV_PART_MAIN);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        label_ip = lv_label_create(screen);
        lv_label_set_text(label_ip, "WiFi: ...");
        lv_obj_set_width(label_ip, lv_pct(100));
        lv_label_set_long_mode(label_ip, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);

        static int32_t grid_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static int32_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

        grid_root = lv_obj_create(screen);
        lv_obj_set_width(grid_root, lv_pct(100));
        lv_obj_set_flex_grow(grid_root, 1);
        lv_obj_set_layout(grid_root, LV_LAYOUT_GRID);
        lv_obj_set_grid_dsc_array(grid_root, grid_col_dsc, grid_row_dsc);
        lv_obj_set_style_pad_all(grid_root, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(grid_root, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_column(grid_root, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(grid_root, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(grid_root, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_remove_flag(grid_root, LV_OBJ_FLAG_SCROLLABLE);

        build_metric_cell(0, 0, "CPU", &cell_cpu, &arc_cpu, &lbl_cpu_arc_pct, &lbl_cpu_detail);
        build_metric_cell(1, 0, "Memory", &cell_mem, &arc_mem, &lbl_mem_arc_pct, &lbl_mem_detail);
        build_metric_cell(0, 1, "GPU", &cell_gpu, &arc_gpu, &lbl_gpu_arc_pct, &lbl_gpu_detail);
        build_network_cell(1, 1);

        ensure_web_server();
        refresh_ip_label();
    }

    void app_close() override {
        post_accum.clear();
        post_overflow = false;

        {
            const std::lock_guard<std::mutex> lock(mtx);
            pending = MonitorSnapshot{};
            pending_dirty = false;
        }

        web_server.reset();
        web_server_started = false;

        if (!screen) return;

        lv_obj_delete(screen);
        screen = nullptr;

        label_ip = nullptr;
        grid_root = nullptr;
        cell_cpu = nullptr;
        arc_cpu = nullptr;
        lbl_cpu_arc_pct = nullptr;
        lbl_cpu_detail = nullptr;
        cell_mem = nullptr;
        arc_mem = nullptr;
        lbl_mem_arc_pct = nullptr;
        lbl_mem_detail = nullptr;
        cell_gpu = nullptr;
        arc_gpu = nullptr;
        lbl_gpu_arc_pct = nullptr;
        lbl_gpu_detail = nullptr;
        cell_net = nullptr;
        lbl_net_detail = nullptr;
    }

    void loop_ui() override {
        if (!screen) return;

        const unsigned long now = millis();
        if (now - last_ip_refresh_ms >= 1000U) {
            last_ip_refresh_ms = now;
            refresh_ip_label();
            if (!web_server_started && WiFi.status() == WL_CONNECTED) ensure_web_server();
        }

        MonitorSnapshot snap;
        bool do_render = false;
        {
            const std::lock_guard<std::mutex> lock(mtx);
            if (pending_dirty) {
                pending_dirty = false;
                snap = pending;
                do_render = snap.valid;
            }
        }

        if (!do_render) return;

        if (arc_cpu) lv_arc_set_value(arc_cpu, snap.cpu_pct);
        if (lbl_cpu_arc_pct) {
            char pbuf[24];
            std::snprintf(
                pbuf,
                sizeof(pbuf),
                "%d%%\n%.0f\xC2\xB0"
                "C",
                static_cast<int>(snap.cpu_pct),
                snap.cpu_temp
            );
            lv_label_set_text(lbl_cpu_arc_pct, pbuf);
        }
        if (lbl_cpu_detail) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.0f MHz", snap.cpu_clock);
            lv_label_set_text(lbl_cpu_detail, buf);
        }

        if (arc_mem) lv_arc_set_value(arc_mem, snap.mem_pct);
        if (lbl_mem_arc_pct) {
            char pbuf[16];
            std::snprintf(pbuf, sizeof(pbuf), "%d%%", static_cast<int>(snap.mem_pct));
            lv_label_set_text(lbl_mem_arc_pct, pbuf);
        }
        if (lbl_mem_detail) {
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%.1f / %.1f GB", snap.mem_used / 1e9, snap.mem_total / 1e9);
            lv_label_set_text(lbl_mem_detail, buf);
        }

        if (arc_gpu) lv_arc_set_value(arc_gpu, snap.gpu_pct);
        if (lbl_gpu_arc_pct) {
            char pbuf[24];
            std::snprintf(
                pbuf,
                sizeof(pbuf),
                "%d%%\n%.0f\xC2\xB0"
                "C",
                static_cast<int>(snap.gpu_pct),
                snap.gpu_temp
            );
            lv_label_set_text(lbl_gpu_arc_pct, pbuf);
        }
        if (lbl_gpu_detail) {
            char buf[40];
            std::snprintf(buf, sizeof(buf), "%.0f / %.0f MB", snap.gpu_mem_used, snap.gpu_mem_total);
            lv_label_set_text(lbl_gpu_detail, buf);
        }

        if (lbl_net_detail) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "TX: %.2f MB\nRX: %.2f MB", snap.net_sent / 1e6, snap.net_recv / 1e6);
            lv_label_set_text(lbl_net_detail, buf);
        }
    }

    void loop() override {}

private:
    std::string drawer_icon_utf8;

    struct MonitorSnapshot {
        bool valid = false;
        int32_t cpu_pct = 0;
        double cpu_clock = 0;
        double cpu_temp = 0;
        int32_t mem_pct = 0;
        double mem_used = 0;
        double mem_total = 0;
        int32_t gpu_pct = 0;
        double gpu_mem_used = 0;
        double gpu_mem_total = 0;
        double gpu_temp = 0;
        double net_sent = 0;
        double net_recv = 0;
    };

    std::mutex mtx;
    MonitorSnapshot pending{};
    bool pending_dirty = false;

    std::unique_ptr<AsyncWebServer> web_server;
    bool web_server_started = false;

    std::string post_accum;
    bool post_overflow = false;

    unsigned long last_ip_refresh_ms = 0;

    lv_obj_t* label_ip = nullptr;
    lv_obj_t* grid_root = nullptr;

    lv_obj_t* cell_cpu = nullptr;
    lv_obj_t* arc_cpu = nullptr;
    lv_obj_t* lbl_cpu_arc_pct = nullptr;
    lv_obj_t* lbl_cpu_detail = nullptr;

    lv_obj_t* cell_mem = nullptr;
    lv_obj_t* arc_mem = nullptr;
    lv_obj_t* lbl_mem_arc_pct = nullptr;
    lv_obj_t* lbl_mem_detail = nullptr;

    lv_obj_t* cell_gpu = nullptr;
    lv_obj_t* arc_gpu = nullptr;
    lv_obj_t* lbl_gpu_arc_pct = nullptr;
    lv_obj_t* lbl_gpu_detail = nullptr;

    lv_obj_t* cell_net = nullptr;
    lv_obj_t* lbl_net_detail = nullptr;

    static int32_t clamp_pct(double v) {
        if (std::isnan(v)) return 0;
        if (v < 0.0) return 0;
        if (v > 100.0) return 100;
        return static_cast<int32_t>(v + 0.5);
    }

    static int32_t mem_pct_from_used_total(double used, double total) {
        if (std::isnan(used) || std::isnan(total) || total <= 0.0) return 0;
        const double r = (used / total) * 100.0;
        return clamp_pct(r);
    }

    void refresh_ip_label() {
        if (!label_ip) return;

        if (WiFi.status() != WL_CONNECTED) {
            lv_label_set_text(label_ip, "WiFi: not connected");
            return;
        }

        const String ip = WiFi.localIP().toString();
        char buf[96];
        std::snprintf(buf, sizeof(buf), "POST http://%s/monitor", ip.c_str());
        lv_label_set_text(label_ip, buf);
    }

    void ensure_web_server() {
        if (web_server_started) return;
        if (WiFi.status() != WL_CONNECTED) return;

        web_server.reset(new AsyncWebServer(APP_PC_MONITOR_HTTP_PORT));

        web_server->on(
            "/monitor",
            AsyncWebRequestMethod::HTTP_POST,
            [this](AsyncWebServerRequest* request) { on_monitor_request_done(request); },
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { on_monitor_post_body(request, data, len, index, total); }
        );

        web_server->begin();
        web_server_started = true;
    }

    void on_monitor_post_body(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        (void)request;
        if (total > APP_PC_MONITOR_MAX_BODY) {
            post_overflow = true;
            return;
        }

        if (index == 0) {
            post_accum.clear();
            post_overflow = false;
            post_accum.reserve(static_cast<size_t>(total));
        }

        if (post_overflow) return;

        post_accum.append(reinterpret_cast<const char*>(data), len);
    }

    void on_monitor_request_done(AsyncWebServerRequest* request) {
        if (post_overflow) {
            post_accum.clear();
            post_overflow = false;
            request->send(413, "text/plain", "Body too large");
            return;
        }

        JsonDocument doc;
        const DeserializationError err = deserializeJson(doc, post_accum);
        post_accum.clear();

        if (err) {
            request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
            return;
        }

        MonitorSnapshot s;
        s.valid = true;

        if (JsonObject cpu = doc["cpu"].as<JsonObject>()) {
            s.cpu_pct = clamp_pct(cpu["percentage"].as<double>());
            s.cpu_clock = cpu["clock"].as<double>();
            if (!cpu["temperature"].isNull())
                s.cpu_temp = cpu["temperature"].as<double>();
            else if (!cpu["temp"].isNull())
                s.cpu_temp = cpu["temp"].as<double>();
        }

        if (JsonObject mem = doc["mem"].as<JsonObject>()) {
            s.mem_used = mem["used"].as<double>();
            s.mem_total = mem["total"].as<double>();
            s.mem_pct = mem_pct_from_used_total(s.mem_used, s.mem_total);
        }

        if (JsonObject gpu = doc["gpu"].as<JsonObject>()) {
            s.gpu_pct = clamp_pct(gpu["percentage"].as<double>());
            s.gpu_mem_used = gpu["memory_used"].as<double>();
            s.gpu_mem_total = gpu["memory_total"].as<double>();
            if (!gpu["temperature"].isNull())
                s.gpu_temp = gpu["temperature"].as<double>();
            else if (!gpu["temp"].isNull())
                s.gpu_temp = gpu["temp"].as<double>();
        }

        if (JsonObject net = doc["network"].as<JsonObject>()) {
            s.net_sent = net["sent"].as<double>();
            s.net_recv = net["received"].as<double>();
        }

        {
            const std::lock_guard<std::mutex> lock(mtx);
            pending = s;
            pending_dirty = true;
        }

        request->send(200, "application/json", "{\"ok\":true}");
    }

    static void style_arc_gauge(lv_obj_t* arc) {
        lv_arc_set_range(arc, 0, 100);
        lv_arc_set_bg_angles(arc, 135, 45);
        lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
        lv_obj_set_width(arc, 72);
        lv_obj_set_height(arc, 72);
        lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    }

    void build_metric_cell(int32_t col, int32_t row, const char* title, lv_obj_t** out_cell, lv_obj_t** out_arc, lv_obj_t** out_arc_pct, lv_obj_t** out_detail) {
        lv_obj_t* cell = lv_obj_create(grid_root);
        *out_cell = cell;
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell, 2, LV_PART_MAIN);
        lv_obj_set_style_border_width(cell, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(cell, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cell, LV_OPA_30, LV_PART_MAIN);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_t = lv_label_create(cell);
        lv_label_set_text(lbl_t, title);

        lv_obj_t* arc = lv_arc_create(cell);
        *out_arc = arc;
        style_arc_gauge(arc);

        lv_obj_t* arc_pct = lv_label_create(arc);
        *out_arc_pct = arc_pct;
        lv_label_set_text(
            arc_pct,
            "--%\n--\xC2\xB0"
            "C"
        );
        lv_obj_set_width(arc_pct, lv_pct(100));
        lv_label_set_long_mode(arc_pct, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_align(arc_pct, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_align(arc_pct, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_remove_flag(arc_pct, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* det = lv_label_create(cell);
        *out_detail = det;
        lv_label_set_text(det, "-");
        lv_obj_set_width(det, lv_pct(100));
        lv_label_set_long_mode(det, LV_LABEL_LONG_MODE_WRAP);
        lv_obj_set_style_text_align(det, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }

    void build_network_cell(int32_t col, int32_t row) {
        cell_net = lv_obj_create(grid_root);
        lv_obj_set_grid_cell(cell_net, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_set_layout(cell_net, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cell_net, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell_net, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell_net, 2, LV_PART_MAIN);
        lv_obj_set_style_border_width(cell_net, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(cell_net, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(cell_net, LV_OPA_30, LV_PART_MAIN);
        lv_obj_remove_flag(cell_net, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_t = lv_label_create(cell_net);
        lv_label_set_text(lbl_t, "Network");

        lbl_net_detail = lv_label_create(cell_net);
        lv_label_set_text(lbl_net_detail, "TX / RX");
        lv_obj_set_style_text_align(lbl_net_detail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
};

extern AppPcMonitorClass AppPcMonitor;
