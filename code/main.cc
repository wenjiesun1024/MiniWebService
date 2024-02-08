#include <iostream>
#include "Webservice/webservice.h"

int main() {
  WebService ws(9006, LogWriteMode::Async, TriggerMode::LEVEL, 8, 8);

  
  return 0;
}