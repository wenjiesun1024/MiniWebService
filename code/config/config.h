#ifndef MINIWEBSERVER_CONFIG_H
#define MINIWEBSERVER_CONFIG_H

enum class TriggerMode
{
    LEVEL,
    EDGE
};

enum class LogWriteMode
{
    Sync,
    Async
};

struct Config
{
public:
    Config();

public:
    int Port;
    LogWriteMode logWriteMode;
    TriggerMode triggerMode;

    int sqlNum;    // number of sql connections
    int threadNum; // number of threads
};

#endif // MINIWEBSERVER_CONFIG_H