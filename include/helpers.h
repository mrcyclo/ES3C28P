#pragma once

#include <cstdint>
#include <string>
#include <lvgl.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define LV_OBJ_EVENT_CB(Class, Method) [](lv_event_t *e) { static_cast<Class *>(lv_event_get_user_data(e))->Method(); }

#define FREERTOS_TASK_CB(Class, Method) [](void *parameter) { static_cast<Class *>(parameter)->Method(); }

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target_obj(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static std::string fa(uint32_t cp)
{
    std::string s;
    if (cp <= 0x7FU)
    {
        s += static_cast<char>(cp);
    }
    else if (cp <= 0x7FFU)
    {
        s += static_cast<char>(0xC0U | (cp >> 6));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    else if (cp <= 0xFFFFU)
    {
        if (cp >= 0xD800U && cp <= 0xDFFFU)
            return s; // surrogate — invalid đứng một mình
        s += static_cast<char>(0xE0U | (cp >> 12));
        s += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    else if (cp <= 0x10FFFFU)
    {
        s += static_cast<char>(0xF0U | (cp >> 18));
        s += static_cast<char>(0x80U | ((cp >> 12) & 0x3FU));
        s += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    return s;
}

static String http_get(WiFiClientSecure &client, String url, CookieJar *cookieJar = nullptr)
{
    HTTPClient http;

    if (!http.begin(client, url))
    {
        Serial.println("[Helpers] Unable to connect");
        http.end();
        return {};
    }

    if (cookieJar)
    {
        http.setCookieJar(cookieJar);
    }

    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36");

    auto httpCode = http.GET();
    if (httpCode <= 0)
    {
        Serial.printf("[Helpers] GET %s failed, error: %s\n", url.c_str(), http.errorToString(httpCode).c_str());
        http.end();
        return {};
    }

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("[Helpers] GET %s failed, HTTP code: %d\n", url.c_str(), httpCode);
        http.end();
        return {};
    }

    auto body = http.getString();

    http.end();

    return body;
}

static String http_post_form(WiFiClientSecure &client, String url, String payload, CookieJar *cookieJar = nullptr)
{
    HTTPClient http;

    if (!http.begin(client, url))
    {
        Serial.println("[Helpers] Unable to connect");
        http.end();
        return {};
    }

    if (cookieJar)
    {
        http.setCookieJar(cookieJar);
    }

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36");

    auto httpCode = http.POST(payload);
    if (httpCode <= 0)
    {
        Serial.printf("[Helpers] POST %s failed, error: %s\n", url.c_str(), http.errorToString(httpCode).c_str());
        http.end();
        return {};
    }

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("[Helpers] POST %s failed, HTTP code: %d\n", url.c_str(), httpCode);
        http.end();
        return {};
    }

    auto body = http.getString();

    http.end();

    return body;
}
