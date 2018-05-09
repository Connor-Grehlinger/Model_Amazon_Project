#include "server.h"

int main(int argc, char** argv) {
  Server s;
  s.run_server("vcm-2741.vm.duke.edu",34567,"vcm-2741.vm.duke.edu",23456);
  while(true);
  return 0;
}

// other team: vcm-2741.vm.duke.edu
