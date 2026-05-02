#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <lvgl.h>

#include <cstring>
#include <string>

#include "config.h"
#include "modules/common/helpers.h"
#include "modules/common/imodule.h"

class VnIndexClass : public ModuleOnce {
public:
    void loop_ui() override {}

    void loop() override {
        if (last_update == 0 || millis() - last_update > 30000) {
            last_update = millis();
            get_vnindex_data();
        }
    }

    lv_obj_t* get_screen() { return screen; }

protected:
    void setup_impl() override {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 10, LV_PART_MAIN);

        lb_index = lv_label_create(screen);
        lv_obj_set_width(lb_index, lv_pct(100));
        lv_obj_set_style_text_align(lb_index, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(lb_index, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_recolor(lb_index, true);
        lv_label_set_text(lb_index, "#000000 VN-Index#");
    }

private:
    lv_obj_t* screen = nullptr;
    lv_obj_t* lb_index = nullptr;
    CookieJar jar;
    ulong last_update = 0;

    static std::string extract_body_token(const String& body) {
        if (body.length() == 0) return {};

        const int form = body.indexOf("__CHART_AjaxAntiForgeryForm");
        if (form < 0) return {};

        int sectionEnd = body.indexOf("</form>", form);
        if (sectionEnd < 0) {
            const int cap = form + 16384;
            sectionEnd = (int)body.length() < cap ? (int)body.length() : cap;
        }

        const String chunk = body.substring(form, sectionEnd);

        auto value_from_equals = [](const String& c, int eq) -> std::string {
            if (eq < 0 || eq + 1 >= (int)c.length()) return {};
            const int v0 = eq + 1;
            const char q = c.charAt(v0);
            if (q == '"') {
                const int v1 = c.indexOf('"', v0 + 1);
                if (v1 <= v0 + 1) return {};
                const String t = c.substring(v0 + 1, v1);
                return std::string(t.c_str(), static_cast<size_t>(t.length()));
            }
            if (q == '\'') {
                const int v1 = c.indexOf('\'', v0 + 1);
                if (v1 <= v0 + 1) return {};
                const String t = c.substring(v0 + 1, v1);
                return std::string(t.c_str(), static_cast<size_t>(t.length()));
            }
            int v1 = v0;
            while (v1 < (int)c.length()) {
                const char ch = c.charAt(v1);
                if (ch == '>' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') break;
                v1++;
            }
            if (v1 <= v0) return {};
            const String t = c.substring(v0, v1);
            return std::string(t.c_str(), static_cast<size_t>(t.length()));
        };

        auto find_value_after = [&](int from) -> std::string {
            const int k = chunk.indexOf("value=", from);
            if (k < 0 || k > from + 512) return {};
            return value_from_equals(chunk, k + 5);
        };

        int namePos = chunk.indexOf("name=\"__RequestVerificationToken\"");
        if (namePos < 0) namePos = chunk.indexOf("name='__RequestVerificationToken'");
        if (namePos < 0) namePos = chunk.indexOf("name=__RequestVerificationToken");
        if (namePos < 0) return {};

        std::string tok = find_value_after(namePos);
        if (!tok.empty()) return tok;

        const int back = namePos > 500 ? namePos - 500 : 0;
        const String head = chunk.substring(back, namePos);
        const int rel = head.lastIndexOf("value=");
        if (rel < 0) return {};
        return value_from_equals(chunk, back + rel + 5);
    }

    void get_vnindex_data() {
        WiFiClientSecure* client = new WiFiClientSecure();
        if (!client) {
            Serial.println("[VnIndex] Unable to create client");
            return;
        }

        client->setCACert(VIETFINANCE_CA_CERTIFICATE);
        client->setInsecure();

        auto body = http_get(*client, "https://finance.vietstock.vn", &jar);
        auto token = extract_body_token(body);
        if (token.empty()) {
            Serial.println("[VnIndex] Body __RequestVerificationToken not found");
            delete client;
            return;
        }

        body = http_post_form(*client, "https://finance.vietstock.vn/data/getmarketprice", String("__RequestVerificationToken=") + token.c_str(), &jar);

        delete client;

        JsonDocument doc;
        deserializeJson(doc, body);
        if (doc.isNull()) {
            Serial.println("[VnIndex] JSON empty");
            return;
        }

        JsonArray data = doc.as<JsonArray>();
        if (data.isNull()) {
            Serial.println("[VnIndex] Data empty");
            return;
        }

        for (JsonObject item : data) {
            const char* code = item["Code"];
            if (!code || std::strcmp(code, "VNIndex") != 0) continue;

            auto change = item["Change"].as<float>();
            auto per_change = item["PerChange"].as<float>();
            auto price = item["Price"].as<float>();
            Serial.println(String("[VnIndex] VN-Index: ") + String(price, 2) + " " + String(change, 2) + " " + String(per_change, 2));

            const char* hex_chg = (change > 0.f) ? "39ca32" : ((change < 0.f) ? "ff0000" : "ffa60c");
            const char* hex_per = (per_change > 0.f) ? "39ca32" : ((per_change < 0.f) ? "ff0000" : "ffa60c");

            String chg;
            if (change > 0.f) chg += '+';
            chg += String(change, 2);

            String per;
            if (per_change > 0.f) per += '+';
            per += String(per_change, 2);
            per += '%';

            const String line = String("#000000 ") + String(price, 2) + "#\n#" + hex_chg + ' ' + chg + "#    #" + hex_per + ' ' + per + '#';
            lv_label_set_text(lb_index, line.c_str());

            break;
        }
    }
};

extern VnIndexClass VnIndex;
