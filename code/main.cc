#include <iostream>

#include "Webservice/webservice.h"

int main() {
  WebService ws(9006, LogWriteMode::Async, TriggerMode::LEVEL,
                TriggerMode::LEVEL, 8, 8, 500);

  ws.Start();
  return 0;
}