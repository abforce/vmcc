#ifndef VMVIXHELPER_H
#define VMVIXHELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <QString>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include "vix.h"

#define  CONNTYPE VIX_SERVICEPROVIDER_VMWARE_WORKSTATION
#define  HOSTNAME ""
#define  HOSTPORT 0
#define  USERNAME ""
#define  PASSWORD ""

#define  VMPOWEROPTIONS   VIX_VMPOWEROP_NORMAL

class VmVixHelper
{
public:
    class OperationResult{
    public:
        static const int SUCCESS = 0;
        static const int COULD_NOT_CONNECT = 1;
        static const int COULD_NOT_OPEN_MACHINE = 2;
        static const int COULD_NOT_POWER_ON = 3;
        static const int COULD_NOT_POWER_OFF = 4;
        static const int COULD_NOT_RETRIEVE_NUMBER_OF_SNAPSHOTS = 5;
        static const int COULD_NOT_CREATE_SNAPSHOT = 6;
        static const int COULD_NOT_RETRIEVE_SNAPSHOT = 7;
        static const int COULD_NOT_REVERT_TO_SNAPSHOT = 8;
        static const int COULD_NOT_SET_IP_ADDRESS = 9;
        static const int COULD_NOT_WAIT_FOR_TOOLS = 10;
        static const int COULD_NOT_LOGIN = 11;
        static const int COULD_NOT_RUN_PROGRAM_IN_GUEST = 12;
        static const int COULD_NOT_COPY_FILE = 13;
        static const int COULD_NOT_OPEN_FILE = 14;
        static const int COULD_NOT_REMOVE_FILE_IN_GUEST = 15;
    };
    class ShellOutput{
    public:
        QString txtStdout;
        QString txtStderr;
        ShellOutput(QString txtStdout, QString txtStderr = QString()):txtStdout(txtStdout), txtStderr(txtStderr){}
    };
    VmVixHelper();
    static int connectToWorkstation();
    static int disconnectFromWorkstation();
    static int powerOn(QString vmxPath);
    static int powerOff();
    static int getNumberOfSnapshots(QString vmxPath, int *num);
    static int createSnapshot();
    static int revertToSnapshot(QString vmxPath, int snapshotIndex);
    static int setIpAddress(QString interface, QString ip, QString mask, QString gateway = QString());
    static void cleanup();
    static int runProgramInGuest(QString programName, QString cmdArgs, ShellOutput **output);

protected:
    static int openMachine(QString vmxPath);
    static int waitForToolsAndLogin();
    static int login(QString username, QString password);
    static int nativeRunProgramInGuest(QString program, QString params, bool returnImmediately = true);
    static int copyFromGuestToHost(QString guestFile, QString hostFile);
    static int removeFileInGuest(QString filename);
    static VixHandle jobHandle;
    static VixHandle hostHandle;
    static VixHandle vmHandle;
    static VixHandle snapshotHandle;
    static VixError err;
    static bool isMachineOpened;
};

#endif // VMVIXHELPER_H
