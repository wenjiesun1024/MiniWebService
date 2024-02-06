#include "config.h"

Config::Config()
{
    Port = 9006;
    logWriteMode = LogWriteMode::Sync;
    triggerMode = TriggerMode::LEVEL;
    sqlNum = 8;
    threadNum = 8;
}