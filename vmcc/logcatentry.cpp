#include "logcatentry.h"

LogcatEntry::LogcatEntry(QString time, QString tag, QString content):
    time(time),
    tag(tag),
    content(content)
{
}
QString LogcatEntry::getTime() const
{
    return time;
}

void LogcatEntry::setTime(const QString &value)
{
    time = value;
}
QString LogcatEntry::getTag() const
{
    return tag;
}

void LogcatEntry::setTag(const QString &value)
{
    tag = value;
}
QString LogcatEntry::getContent() const
{
    return content;
}

void LogcatEntry::setContent(const QString &value)
{
    content = value;
}



