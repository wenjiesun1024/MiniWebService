#include <iostream>


#include "Webservice/webservice.h"

int main() {
  WebService ws(9006, LogWriteMode::Sync, TriggerMode::LEVEL, 8, 8);

  

  return 0;
}