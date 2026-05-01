#pragma once

#include <vector>
#include "common/iapplication.h"

class AppManagementClass
{
public:
    void register_app(IApplication &app)
    {
        IApplication *p = &app;
        for (IApplication *x : apps)
        {
            if (x == p)
                return;
        }
        apps.push_back(p);
    }

    size_t app_count() const { return apps.size(); }

    IApplication *app_at(size_t index) { return apps.at(index); }

    const IApplication *app_at(size_t index) const { return apps.at(index); }

    void loop_ui()
    {
        for (IApplication *a : apps)
            a->loop_ui();
    }

    void loop()
    {
        for (IApplication *a : apps)
            a->loop();
    }

private:
    std::vector<IApplication *> apps;
};

extern AppManagementClass AppManagement;
