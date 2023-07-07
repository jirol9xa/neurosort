#ifndef _CONFIG_H__
#define _CONFIG_H__

struct Edge final
{
  int dst; /* Callee's vertex number.  */
  long long w; /* Number of jumps.  */
  double prob; /* Jmp's probability.  */
};

#endif

