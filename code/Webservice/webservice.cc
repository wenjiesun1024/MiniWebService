#include "./webservice.h"
#include <iostream>

WebService::WebService(int port, LogWriteMode logWriteMode, TriggerMode triggerMode, int sqlNum, int threadNum)
    : Port(port), logWriteMode(logWriteMode), triggerMode(triggerMode), sqlNum(sqlNum), threadNum(threadNum)
{
}