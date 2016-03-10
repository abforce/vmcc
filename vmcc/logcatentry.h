#ifndef LOGCATENTRY_H
#define LOGCATENTRY_H

#include "QString"

class LogcatEntry
{
public:
    LogcatEntry(QString time, QString tag, QString content);

    QString getTime() const;
    void setTime(const QString &value);

    QString getTag() const;
    void setTag(const QString &value);

    QString getContent() const;
    void setContent(const QString &value);

private:
    QString time;
    QString tag;
    QString content;
};

#endif // LOGCATENTRY_H
