#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string>

#include "hide_mount.hpp"
#include "hide_prop.hpp"
#include "log.hpp"
#include "zygisk.hpp"


using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

//要隐藏的app包名
const char *hook_pkg_name = "com.shocker.zygiskdetect";

class MyModule : public zygisk::ModuleBase
{
public:
    void onLoad(Api *api, JNIEnv *env) override
    {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override
    {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (strcmp(hook_pkg_name, process) == 0)
        {
            int pid=getpid();
            int fd = api->connectCompanion();
            write(fd, &pid, sizeof(pid));
            close(fd);
        }
        env->ReleaseStringUTFChars(args->nice_name, process);
        return;
    }

private:
    Api *api;
    JNIEnv *env;
};

static void companion_handler(int i)
{
    int pid;
    read(i, &pid, sizeof(pid));

    unshare(CLONE_NEWNS);
    mount(nullptr, "/", nullptr, MS_PRIVATE | MS_REC, nullptr);
    LOGD("daemon pid:%d,do_hide start pid:%d\n",getpid(),pid);
    do_hide(pid);
    do_hide_prop();
    return;
}

REGISTER_ZYGISK_MODULE(MyModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
