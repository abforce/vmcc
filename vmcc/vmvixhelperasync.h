#ifndef VMVIXHELPERASYNC_H
#define VMVIXHELPERASYNC_H

#include <QList>
#include <QStack>
#include "vmvixhelper.h"

class UserData{
public:
    void *context;
    int callee;
    int requestCode;
    VixHandle vmHandle;
    UserData(void *context, int callee, int requestCode, VixHandle vmHandle = VIX_INVALID_HANDLE):
        context(context),
        callee(callee),
        requestCode(requestCode),
        vmHandle(vmHandle){}
};

typedef void VixHelperCallback(UserData *, int);

class VmVixHelperAsync : public VmVixHelper
{
public:

    enum{
        CALL_POWER_ON,
        CALL_OPEN_MACHINE,
        CALL_POWER_OFF,
        CALL_RUN_PROGRAM_IN_GUEST,
        CALL_WAIT_FOR_TOOLS,
        CALL_LOGIN_GUEST
    };

    class ClientData{
        public:
        void *callerContext;
        int requestCode;
        VixHandle vmHandle;
        QStack<int> *callStack;
        void *slot1;
        void *slot2;
        void *slot3;
        void *slot4;

        ClientData(void *context, int requestCode):
        callerContext(context),
        requestCode(requestCode),
        vmHandle(VIX_INVALID_HANDLE),
        callStack(new QStack<int>())
    {}
    };

    static void registerCallback(VixHelperCallback* callback);
    static void asyncPowerOn(void *context, QString vmxPath, int requestCode, bool machineIsOpen = false, VixHandle vmHandle = VIX_INVALID_HANDLE);
    static void asyncPowerOff(void *context, VixHandle vmHandle, int requestCode);
    static void asyncRunProgramInGuest(void *context, VixHandle vmHandle, QString username, QString password, int requestCode, QString program, QString args, bool toolsIsReady = false, bool isLoggedIn = false);
private:
    static VixHelperCallback* userCallback;
    static void callback(VixHandle jobHandle, VixEventType eventType, VixHandle moreEventInfo, void *clientData);
    static void asyncOpenMachine(ClientData *data);
    static void asyncWaitForTools(ClientData *data);
    static void asyncLoginInGuest(ClientData *data);
};

#endif // VMVIXHELPERASYNC_H
