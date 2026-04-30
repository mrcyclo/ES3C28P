#pragma once

class IModule
{
public:
    virtual ~IModule() = default;
    virtual void setup() = 0;
    virtual void loop_ui() = 0;
    virtual void loop() = 0;
};

class ModuleOnce : public IModule
{
public:
    void setup() final
    {
        if (inited)
            return;
        inited = true;
        setup_impl();
    }

protected:
    virtual void setup_impl() = 0;

private:
    bool inited = false;
};
