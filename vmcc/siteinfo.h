#ifndef SITEINFO_H
#define SITEINFO_H

#include <QPushButton>
#include "vmvixhelperasync.h"

class SiteInfo : public QObject{
public:
    QString name;
    QString vmxPath;
    QString username;
    QString password;
    bool holder;
    QString SN;
    QString SV;
    bool connected;
    bool running;
    QPushButton *btnAction;
    int id;
    int socketId;
    VixHandle vmHandle;
    int state;
    enum{
        STATE_NEW,
        STATE_HELLO_SAIED
    };

    SiteInfo(QString name,
             QString vmxPath,
             QString username,
             QString password,
             bool initialHolder,
             QString SN =               "N/A",
             QString SV =               "N/A",
             bool connected =           false,
             bool running =             false,
             QPushButton *btnAction =   0,
             int id =                   -1,
             VixHandle vmHandle =       VIX_INVALID_HANDLE,
             int state =                STATE_NEW):

        name(name),
        vmxPath(vmxPath),
        username(username),
        password(password),
        holder(initialHolder),
        SN(SN),
        SV(SV),
        connected(connected),
        running(running),
        btnAction(btnAction),
        id(id),
        vmHandle(vmHandle),
        state(state)
    {}
};

#endif // SITEINFO_H
