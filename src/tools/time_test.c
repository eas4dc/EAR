#include <stdio.h>
#include <stdlib.h>
#include <common/system/time.h>


void fast_call(uint t)
{
  random();
}
void main(int argc,char *argv[])
{
  timestamp_t init, end;
  timestamp_t test_init, test_end;
  ulong elap_test;
  ulong total = 0;

  if (argc != 3) {
    printf("%s repetitions gettime\n", argv[0]);
    exit(0);
  }
  uint run_test = atoi(argv[2]);

  if (run_test) printf("Measuring time\n");

  timestamp_getfast(&init);

  for (uint rep = 0; rep < atoi(argv[1]); rep++){
    if (run_test) timestamp_getfast(&test_init);
    fast_call(rep);
    if (run_test){
      timestamp_getfast(&test_end);
      elap_test = timestamp_diff(&test_end, &test_init, TIME_MSECS);
      total += elap_test;
    }
  }
  timestamp_getfast(&end);

  ulong elap = timestamp_diff(&end, &init, TIME_MSECS);
  printf("Total time %lu total %lu\n", elap, total);
}
