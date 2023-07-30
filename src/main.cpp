#include "parser.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Error: missing file name!\n";
    return -1;
  }

  Parser psr(argv[1], argv[2], argv[3]);

  qsort((void *)psr.getSymArr()->symbols, psr.getSymArr()->size,
        sizeof(Elf64_Sym_W_Name), symbolComp);

  std::map<std::pair<uint64_t, uint64_t>, int> funcHashTable;

  fillHashMap(funcHashTable, &psr);

  dumpMapToFile(funcHashTable, &psr);

  return 0;
}
