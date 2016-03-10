#include "vmvixhelperasync.h"

VixHelperCallback* VmVixHelperAsync::userCallback;

void VmVixHelperAsync::registerCallback(VixHelperCallback *callback)
{
    userCallback = callback;
}

void VmVixHelperAsync::asyncPowerOn(void *context, QString vmxPath, int requestCode, bool machineIsOpen, VixHandle vmHandle)
{
    ClientData *data = new ClientData(context, requestCode);
    data->callStack->push(CALL_POWER_ON);
    if(machineIsOpen){
        data->vmHandle = vmHandle;
        VixVM_PowerOn(vmHandle, VMPOWEROPTIONS, VIX_INVALID_HANDLE, VmVixHelperAsync::callback, data);
    } else {
        data->slot1 = new QString(vmxPath);
        asyncOpenMachine(data);
    }
}

void VmVixHelperAsync::asyncPowerOff(void *context, VixHandle vmHandle, int requestCode)
{
    ClientData *data = new ClientData(context, requestCode);
    data->callStack->push(CALL_POWER_OFF);
    data->vmHandle = vmHandle;
    VixVM_PowerOff(vmHandle, VIX_VMPOWEROP_NORMAL, VmVixHelperAsync::callback, data);
}

void VmVixHelperAsync::asyncRunProgramInGuest(void *context, VixHandle vmHandle, QString username, QString password, int requestCode, QString program, QString args, bool toolsIsReady, bool isLoggedIn)
{
    ClientData *data = new ClientData(context, requestCode);
    data->callStack->push(CALL_RUN_PROGRAM_IN_GUEST);
    data->vmHandle = vmHandle;
    if(isLoggedIn){
        VixVM_RunProgramInGuest(vmHandle,
                                program.toStdString().c_str(),
                                args.toStdString().c_str(),
                                VIX_RUNPROGRAM_RETURN_IMMEDIATELY,
                                VIX_INVALID_HANDLE,
                                VmVixHelperAsync::callback,
                                data);
    } else {
        data->slot1 = new QString(program);
        data->slot2 = new QString(args);
        data->slot3 = new QString(username);
        data->slot4 = new QString(password);

        if(toolsIsReady){

            asyncLoginInGuest(data);
        } else {
            asyncWaitForTools(data);
        }
    }
}

void VmVixHelperAsync::callback(VixHandle jobHandle, VixEventType eventType, VixHandle moreEventInfo, void *clientData)
{
    if(eventType != VIX_EVENTTYPE_JOB_COMPLETED || clientData == 0){
        return;
    }
    ClientData *cdata = (ClientData *)clientData;
    int callee = cdata->callStack->pop();
    VixError error = VixJob_GetError(jobHandle);

    switch (callee) {
    case CALL_OPEN_MACHINE:{
        Vix_GetProperties(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &cdata->vmHandle, VIX_PROPERTY_NONE);
        if (VIX_FAILED(error)) {
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_OPEN_MACHINE);
        } else {
            QString vmxPath = *((QString *) cdata->slot1);
            if(cdata->callStack->isEmpty()){
                return;
            }
            callee = cdata->callStack->pop();
            switch (callee) {
            case CALL_POWER_ON:
                asyncPowerOn(cdata->callerContext, vmxPath, cdata->requestCode, true, cdata->vmHandle);
                break;
            }
        }
        break;
    }

    case CALL_POWER_ON:{
        if (VIX_FAILED(error)) {
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_POWER_ON);
        } else {
            UserData *udata = new UserData(cdata->callerContext, CALL_POWER_ON, cdata->requestCode, cdata->vmHandle);
            userCallback(udata, OperationResult::SUCCESS);
        }
        break;
    }

    case CALL_POWER_OFF:{
        if (VIX_FAILED(error)) {
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_POWER_OFF);
        } else {
            UserData *udata = new UserData(cdata->callerContext, CALL_POWER_OFF, cdata->requestCode, cdata->vmHandle);
            userCallback(udata, OperationResult::SUCCESS);
        }
        break;
    }

    case CALL_WAIT_FOR_TOOLS:{
        if(VIX_FAILED(error)){
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_WAIT_FOR_TOOLS);
        } else {
            if(cdata->callStack->isEmpty()){
                return;
            }
            callee = cdata->callStack->pop();
            switch (callee) {
            case CALL_RUN_PROGRAM_IN_GUEST:{
                QString program = *((QString *) cdata->slot1);
                QString args = *((QString *) cdata->slot2);
                QString username = *((QString *) cdata->slot3);
                QString password = *((QString *) cdata->slot4);
                asyncRunProgramInGuest(cdata->callerContext, cdata->vmHandle, username, password, cdata->requestCode, program, args, true);
                break;
            }
            }
        }
        break;
    }

    case CALL_LOGIN_GUEST:{
        if(VIX_FAILED(error)){
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_LOGIN);
        } else {
            if(cdata->callStack->isEmpty()){
                return;
            }
            callee = cdata->callStack->pop();
            switch (callee) {
            case CALL_RUN_PROGRAM_IN_GUEST:{
                QString program = *((QString *) cdata->slot1);
                QString args = *((QString *) cdata->slot2);
                QString username = *((QString *) cdata->slot3);
                QString password = *((QString *) cdata->slot4);
                asyncRunProgramInGuest(cdata->callerContext, cdata->vmHandle, username, password, cdata->requestCode, program, args, true, true);
                break;
            }
            }
        }
        break;
    }

    case CALL_RUN_PROGRAM_IN_GUEST:{
        if(VIX_FAILED(error)){
            while(!cdata->callStack->isEmpty()){
                callee = cdata->callStack->pop();
            }
            UserData *udata = new UserData(cdata->callerContext, callee, cdata->requestCode);
            userCallback(udata, OperationResult::COULD_NOT_RUN_PROGRAM_IN_GUEST);
        } else {
            UserData *udata = new UserData(cdata->callerContext, CALL_RUN_PROGRAM_IN_GUEST, cdata->requestCode);
            userCallback(udata, OperationResult::SUCCESS);
        }
        break;
    }
    }
    Vix_ReleaseHandle(jobHandle);
}

void VmVixHelperAsync::asyncOpenMachine(ClientData *data)
{
    data->callStack->push(CALL_OPEN_MACHINE);
    const char *vmxPath = ((QString *) data->slot1)->toStdString().c_str();
    VixVM_Open(hostHandle, vmxPath, VmVixHelperAsync::callback, data);
}

void VmVixHelperAsync::asyncWaitForTools(ClientData *data)
{
    data->callStack->push(CALL_WAIT_FOR_TOOLS);
    VixVM_WaitForToolsInGuest(data->vmHandle, 300, VmVixHelperAsync::callback, data);
}

void VmVixHelperAsync::asyncLoginInGuest(VmVixHelperAsync::ClientData *data)
{
    const char *str;

    str = ((QString *) data->slot3)->toStdString().c_str();
    char *username = new char[strlen(str) + 1];
    memcpy(username, str, strlen(str) + 1);

    str = ((QString *) data->slot4)->toStdString().c_str();
    char *password = new char[strlen(str) + 1];
    memcpy(password, str, strlen(str) + 1);

    data->callStack->push(CALL_LOGIN_GUEST);
    VixVM_LoginInGuest(data->vmHandle, username, password, 0, VmVixHelperAsync::callback, data);

    delete[] username;
    delete[] password;
}
