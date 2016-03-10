#include "vmvixhelper.h"

VixHandle VmVixHelper::jobHandle = VIX_INVALID_HANDLE;
VixHandle VmVixHelper::hostHandle = VIX_INVALID_HANDLE;
VixHandle VmVixHelper::vmHandle = VIX_INVALID_HANDLE;
VixHandle VmVixHelper::snapshotHandle = VIX_INVALID_HANDLE;
VixError VmVixHelper::err;
bool VmVixHelper::isMachineOpened = false;

int VmVixHelper::connectToWorkstation()
{
    jobHandle = VixHost_Connect(VIX_API_VERSION,
                                CONNTYPE,
                                HOSTNAME, // *hostName,
                                HOSTPORT, // hostPort,
                                USERNAME, // *userName,
                                PASSWORD, // *password,
                                0, // options,
                                VIX_INVALID_HANDLE, // propertyListHandle,
                                NULL, // *callbackProc,
                                NULL); // *clientData);
    err = VixJob_Wait(jobHandle,
                      VIX_PROPERTY_JOB_RESULT_HANDLE,
                      &hostHandle,
                      VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        Vix_ReleaseHandle(vmHandle);
        VixHost_Disconnect(hostHandle);
        return OperationResult::COULD_NOT_CONNECT;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::disconnectFromWorkstation()
{
    isMachineOpened = false;
    Vix_ReleaseHandle(vmHandle);
    VixHost_Disconnect(hostHandle);
    return OperationResult::SUCCESS;
}

int VmVixHelper::powerOn(QString vmxPath)
{
    int result = openMachine(vmxPath);
    if(result != OperationResult::SUCCESS){
        return result;
    }
    jobHandle = VixVM_PowerOn(vmHandle,
                              VMPOWEROPTIONS,
                              VIX_INVALID_HANDLE,
                              NULL, // *callbackProc,
                              NULL); // *clientData);
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_POWER_ON;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::powerOff()
{
    jobHandle = VixVM_PowerOff(vmHandle,
                               VIX_VMPOWEROP_NORMAL,
                               NULL, // *callbackProc,
                               NULL); // *clientData);
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_POWER_OFF;
    }
    isMachineOpened = false;
    return OperationResult::SUCCESS;
}

int VmVixHelper::getNumberOfSnapshots(QString vmxPath, int *num)
{
    if(!isMachineOpened){
        int result = openMachine(vmxPath);
        if(result != OperationResult::SUCCESS){
            return result;
        }
    }
    err = VixVM_GetNumRootSnapshots(vmHandle, num);
    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_RETRIEVE_NUMBER_OF_SNAPSHOTS;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::createSnapshot()
{
    jobHandle = VixVM_CreateSnapshot(vmHandle,
                                     QString("snapshot created on " + QTime::currentTime().toString()).toStdString().c_str(),
                                     "Snapshot taken by VMCC utility",
                                     VIX_SNAPSHOT_INCLUDE_MEMORY,
                                     VIX_INVALID_HANDLE,
                                     NULL, // *callbackProc,
                                     NULL); // *clientData);
    err = VixJob_Wait(jobHandle,
                      VIX_PROPERTY_JOB_RESULT_HANDLE,
                      &snapshotHandle,
                      VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_CREATE_SNAPSHOT;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::revertToSnapshot(QString vmxPath, int snapshotIndex)
{
    if(!isMachineOpened){
        int result = openMachine(vmxPath);
        if(result != OperationResult::SUCCESS){
            return result;
        }
    }
    err = VixVM_GetRootSnapshot(vmHandle, snapshotIndex, &snapshotHandle);
    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_RETRIEVE_SNAPSHOT;
    }
    jobHandle = VixVM_RevertToSnapshot(vmHandle,
                                       snapshotHandle,
                                       VMPOWEROPTIONS, // options,
                                       VIX_INVALID_HANDLE,
                                       NULL, // *callbackProc,
                                       NULL); // *clientData);
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_REVERT_TO_SNAPSHOT;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::setIpAddress(QString interface, QString ip, QString mask, QString gateway)
{
    int result = waitForToolsAndLogin();
    if(result != OperationResult::SUCCESS){
        return result;
    }
    QString program = "c:\\windows\\system32\\netsh.exe";
    QString params = "interface ip set address \"" + interface + "\" static " + ip + " " + mask;
    if(!gateway.isEmpty()){
        params += " " + gateway;
    }
    result = nativeRunProgramInGuest(program, params);
    if (result != OperationResult::SUCCESS) {
        return OperationResult::COULD_NOT_SET_IP_ADDRESS;
    }
    return OperationResult::SUCCESS;
}

void VmVixHelper::cleanup()
{
    isMachineOpened = false;
    Vix_ReleaseHandle(jobHandle);
    Vix_ReleaseHandle(vmHandle);
    VixHost_Disconnect(hostHandle);
}

int VmVixHelper::runProgramInGuest(QString programName, QString cmdArgs, VmVixHelper::ShellOutput **output)
{
    int result = waitForToolsAndLogin();
    if(result != OperationResult::SUCCESS){
        return result;
    }

    QString cmd = "c:\\windows\\system32\\cmd.exe";
    cmdArgs = "/C \"" + programName + " " + cmdArgs + " 1>c:\\mystuff\\stdout 2>c:\\mystuff\\stderr\"";

    result = nativeRunProgramInGuest(cmd, cmdArgs, true);
    if(result != OperationResult::SUCCESS){
        return result;
    }
    result = copyFromGuestToHost("c:\\mystuff\\stdout", "/home/abforce/stdout");
    if(result != OperationResult::SUCCESS){
        return result;
    }
    result = copyFromGuestToHost("c:\\mystuff\\stderr", "/home/abforce/stderr");
    if(result != OperationResult::SUCCESS){
        return result;
    }

    removeFileInGuest("c:\\mystuff\\stdout");
    removeFileInGuest("c:\\mystuff\\stderr");

    QFile fileStdOut("/home/abforce/stdout");
    QFile fileStdErr("/home/abforce/stderr");
    if (!fileStdOut.open(QFile::ReadOnly | QFile::Text)) {
        return OperationResult::COULD_NOT_OPEN_FILE;
    };
    if (!fileStdErr.open(QFile::ReadOnly | QFile::Text)) {
        return OperationResult::COULD_NOT_OPEN_FILE;
    };
    QTextStream inStdOut(&fileStdOut);
    QTextStream inStdErr(&fileStdErr);

    *output = new ShellOutput(inStdOut.readAll(), inStdErr.readAll());;

    fileStdOut.close();
    fileStdErr.close();
    fileStdOut.remove();
    fileStdErr.remove();

    return OperationResult::SUCCESS;
}

int VmVixHelper::openMachine(QString vmxPath)
{
    jobHandle = VixVM_Open(hostHandle,
                           vmxPath.toStdString().c_str(),
                           NULL, // VixEventProc *callbackProc,
                           NULL); // void *clientData);
    err = VixJob_Wait(jobHandle,
                      VIX_PROPERTY_JOB_RESULT_HANDLE,
                      &vmHandle,
                      VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_OPEN_MACHINE;
    }
    isMachineOpened = true;
    return OperationResult::SUCCESS;
}

int VmVixHelper::waitForToolsAndLogin()
{
    const QString username = "big_apple";
    const QString password = "another_apple";

    jobHandle = VixVM_WaitForToolsInGuest(vmHandle,
                                          300, // timeoutInSeconds
                                          NULL, // callbackProc
                                          NULL); // clientData
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_WAIT_FOR_TOOLS;
    }

    jobHandle = VixVM_LoginInGuest(vmHandle,
                                   username.toStdString().c_str(), // userName
                                   password.toStdString().c_str(), // password
                                   0, //VIX_LOGIN_IN_GUEST_REQUIRE_INTERACTIVE_ENVIRONMENT, // options
                                   NULL, // callbackProc
                                   NULL); // clientData
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_LOGIN;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::nativeRunProgramInGuest(QString program, QString params, bool returnImmediately)
{
    jobHandle = VixVM_RunProgramInGuest(vmHandle,
                                        program.toStdString().c_str(),
                                        params.toStdString().c_str(),
                                        returnImmediately ? VIX_RUNPROGRAM_RETURN_IMMEDIATELY : 0,
                                        VIX_INVALID_HANDLE,
                                        NULL,
                                        NULL);
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_RUN_PROGRAM_IN_GUEST;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::copyFromGuestToHost(QString guestFile, QString hostFile)
{
    jobHandle = VixVM_CopyFileFromGuestToHost(vmHandle,
                                  guestFile.toStdString().c_str(),
                                  hostFile.toStdString().c_str(),
                                  0,
                                  VIX_INVALID_HANDLE,
                                  NULL,
                                  NULL);

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_COPY_FILE;
    }

    return OperationResult::SUCCESS;
}

int VmVixHelper::removeFileInGuest(QString filename)
{
    jobHandle = VixVM_DeleteFileInGuest(vmHandle,
                                        filename.toStdString().c_str(),
                                        NULL,
                                        NULL);

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    Vix_ReleaseHandle(jobHandle);

    if (VIX_FAILED(err)) {
        return OperationResult::COULD_NOT_REMOVE_FILE_IN_GUEST;
    }

    return OperationResult::SUCCESS;
}
