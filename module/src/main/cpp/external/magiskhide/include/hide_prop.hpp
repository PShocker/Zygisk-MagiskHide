#include "_system_properties.h"
#include "log.hpp"

static const char *prop_key[] =
    {"ro.boot.vbmeta.device_state", "ro.boot.verifiedbootstate", "ro.boot.flash.locked",
     "ro.boot.veritymode", "ro.boot.warranty_bit", "ro.warranty_bit",
     "ro.debuggable", "ro.secure", "ro.build.type", "ro.build.tags",
     "ro.vendor.boot.warranty_bit", "ro.vendor.warranty_bit",
     "vendor.boot.vbmeta.device_state", nullptr};

static const char *prop_val[] =
    {"locked", "green", "1",
     "enforcing", "0", "0",
     "0", "1", "user", "release-keys",
     "0", "0",
     "locked", nullptr};

int getprop(const char *name, char *buf)
{
    return __system_property_get(name, buf);
}

int setprop(const char *name, const char *value)
{
    __system_property_delete(name);
    __system_property_add(name, strlen(name), value, strlen(value));
    return 0;
}

void hide_sensitive_props()
{
    LOGD("hide: Hiding sensitive props\n");

    for (int i = 0; prop_key[i]; ++i)
    {
        char buf[256];
        getprop(prop_key[i], buf);
        string value = buf;
        if (!value.empty())
        {
            LOGD("set %s:%s", prop_key[i], prop_val[i]);
            setprop(prop_key[i], prop_val[i]);
        }
    }
}

void do_hide_prop()
{
    //初始化
    __system_properties_init();
    //隐藏敏感信息
    hide_sensitive_props();
}