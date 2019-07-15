#include <kernel.h>
#include <klib.h>

int main() {
  _ioe_init();
  _cte_init(os->trap);
  
  // call sequential init code
  os->init(); 
  //os->run();
  _mpe_init(os->run); // all cores call os->run()
  
//  printf("got here\n");
  return 1;
}
