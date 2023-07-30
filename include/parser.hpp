#pragma once

#include <elf.h>

#include "lib.hpp"

struct Elf64_Sym_W_Name {
  Elf64_Sym *symbol;
  char *symName;
};

struct Elf64_Sym_Arr {
  Elf64_Sym_W_Name *symbols;
  size_t size;
};

class Parser {
  Elf64_Sym_Arr *symArr;
  char **strArray;
  uint8_t *binary;
  char *addrs;
  size_t numOfLinesInAddrs = 0;

  std::vector<std::array<uint64_t, 3>> Ranges;

  bool pic;

public:
  Parser(const char *elfFileName, const char *addrsFile, const char *mapsFile);
  ~Parser();
  Elf64_Sym_W_Name *findSymbolByAddress(size_t address);
  Elf64_Sym_Arr *getSymArr();
  char **getStrArray();
  bool isPIC();
  size_t getNumOfLines();
  std::optional<std::array<uint64_t, 3>>
  findLowerBoundRange(uint64_t Addr) const;

  void dumpRanges();

  uint64_t truncSymbolAdress(uint64_t addr);

private:
  uint8_t *createBuffer(const char *inputFileName, bool areLinesNeeded = false);
  Elf64_Sym_Arr *getSymbols(Elf64_Ehdr *elfHeader);
  void parseMapsFile(const char *mapsFile);
};

void fillHashMap(std::map<std::pair<uint64_t, u_int64_t>, int> &funcHashTable,
                 Parser *psr);

int symbolComp(const void *symbol1, const void *symbol2);

void printSymbolsValues(Elf64_Sym_Arr *symArr);

void dumpMapToFile(
    std::map<std::pair<u_int64_t, u_int64_t>, int> &funcHashTable, Parser *psr);
