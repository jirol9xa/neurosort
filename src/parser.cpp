#include "parser.hpp"

bool Parser::isPIC() { return pic; }

char **Parser::getStrArray() { return strArray; }

Elf64_Sym_Arr *Parser::getSymArr() { return symArr; }

Parser::~Parser() {
  delete[] strArray;
  delete[] symArr->symbols;
  delete symArr;
  free(addrs);
  free(binary);
}

void Parser::dumpRanges() {
  for (auto &range : Ranges) {
    std::cout << std::hex << range[0] << ' ' << range[1] << ' '
              << range[2] << '\n';
  }
}

Parser::Parser(const char *elfFileName, const char *addrsFile,
               const char *mapsFile) {
  assert(elfFileName);
  assert(addrsFile);

  binary = createBuffer(elfFileName);

  addrs = reinterpret_cast<char *>(createBuffer(addrsFile, true));

  strArray = new char *[numOfLinesInAddrs];
  initializeArrOfPointers(strArray, numOfLinesInAddrs, addrs);

  Elf64_Ehdr *elf = reinterpret_cast<Elf64_Ehdr *>(binary);
  symArr = getSymbols(elf);

  pic = elf->e_type == ET_DYN ? true : false;

  if (pic) {
    parseMapsFile(mapsFile);
  }
}

uint8_t *Parser::createBuffer(const char *inputFileName, bool areLinesNeeded) {
  assert(inputFileName);

  /// This is legacy. Please, do not touch.

  FILE *elfFile = fopen(inputFileName, "r");
  if (!elfFile) {
    std::cout << "Error: cannot open " << inputFileName << "\n";
    return nullptr;
  }

  size_t fileSize = getFileSize(elfFile);

  if (areLinesNeeded) {
    numOfLinesInAddrs = getNumberOfStrings(elfFile);
  }

  uint8_t *binary = (uint8_t *)calloc(fileSize + 1, sizeof(uint8_t));
  if (!binary) {
    std::cout << "Unable to allocate memory!\n";
    return nullptr;
  }

  size_t numberOfReadBytes = fread(binary, sizeof(uint8_t), fileSize, elfFile);
  if (numberOfReadBytes != fileSize) {
    std::cout << "Incorrect file reading occured!\n";
    return nullptr;
  }

  return binary;
}

size_t Parser::getNumOfLines() { return numOfLinesInAddrs; }

Elf64_Sym_Arr *Parser::getSymbols(Elf64_Ehdr *elfHeader) {
  assert(elfHeader);

  uint8_t *binary = (uint8_t *)elfHeader;

  Elf64_Shdr *sections = (Elf64_Shdr *)(binary + elfHeader->e_shoff);
  Elf64_Shdr *shStrSect = &sections[elfHeader->e_shstrndx];

  uint8_t *shStrSectPtr = binary + shStrSect->sh_offset;

  std::optional<int> symTabIndex;
  std::optional<int> strTabIndex;

  for (int index = 0; index < elfHeader->e_shnum; index++) {
    if (symTabIndex && strTabIndex) {
      break;
    }

    int strOffset = sections[index].sh_name;

    if (strcmp(".symtab", (char *)(shStrSectPtr + strOffset)) == 0) {
      symTabIndex = index;
    }

    if (strcmp(".strtab", (char *)(shStrSectPtr + strOffset)) == 0) {
      strTabIndex = index;
    }
  }

  if (!symTabIndex || !strTabIndex) {
    std::cout << "File is corrupted!\n";
    return nullptr;
  }

  Elf64_Shdr *symTabSect = &sections[*symTabIndex];
  size_t numberOfSymbols = symTabSect->sh_size / symTabSect->sh_entsize;
  Elf64_Sym *symbols = (Elf64_Sym *)(binary + symTabSect->sh_offset);

  Elf64_Sym_Arr *symArray = new Elf64_Sym_Arr;
  symArray->symbols = new Elf64_Sym_W_Name[numberOfSymbols];

  symArray->size = numberOfSymbols;

  Elf64_Shdr *strTabSect = &sections[*strTabIndex];
  char *strTabPtr = (char *)(binary + strTabSect->sh_offset);

  for (size_t i = 0; i < numberOfSymbols; i++) {
    symArray->symbols[i].symbol = &symbols[i];
    symArray->symbols[i].symName = strTabPtr + symbols[i].st_name;
  }

  return symArray;
}

int symbolComp(const void *symbol1, const void *symbol2) {
  assert(symbol1);
  assert(symbol2);

  Elf64_Sym_W_Name *sym1 = (Elf64_Sym_W_Name *)symbol1;
  Elf64_Sym_W_Name *sym2 = (Elf64_Sym_W_Name *)symbol2;

  return sym1->symbol->st_value - sym2->symbol->st_value;
}

void printSymbolsValues(Elf64_Sym_Arr *symArr) {
  assert(symArr);

  for (size_t index = 0; index < symArr->size; index++) {
    std::cout << "Value: " << symArr->symbols[index].symbol->st_value
              << "      " << symArr->symbols[index].symName << '\n';
  }

  std::cout << '\n';
}

Elf64_Sym_W_Name *Parser::findSymbolByAddress(size_t address) {
  assert(symArr);

  // size_t leftIndex = 0;
  // size_t rightIndex = symArr->size - 1;

  // while (leftIndex < rightIndex) {
  //     size_t midIndex = leftIndex + (rightIndex - leftIndex) / 2;

  //     if (address == symArr->symbols[midIndex].symbol->st_value) {
  //         return &symArr->symbols[midIndex];
  //     }
  //     else if (address < symArr->symbols[midIndex].symbol->st_value) {
  //         rightIndex = midIndex;
  //     }
  //     else {
  //         leftIndex = midIndex;
  //     }
  // }

  size_t index = 1;
  for (; index < symArr->size; index++) {
    if (address >= symArr->symbols[index - 1].symbol->st_value &&
        address < symArr->symbols[index].symbol->st_value) {
      return &symArr->symbols[index - 1];
    }
  }

  return nullptr;
}

uint64_t Parser::truncSymbolAdress(uint64_t addr) {
  assert(addr);

  if (pic) {
    std::optional<std::array<uint64_t, 3>> range1 = findLowerBoundRange(addr);

    addr -= ((*range1)[0] - (*range1)[2]) * pic;
  }

  return addr;
}

void fillHashMap(std::map<std::pair<uint64_t, uint64_t>, int> &funcHashTable,
                 Parser *psr) {
  assert(psr);

  for (size_t index = 0; index < psr->getNumOfLines(); index++) {

    size_t addr1 = 0;
    size_t addr2 = 0;

    int res = sscanf(psr->getStrArray()[index], "%lx %lx", &addr1, &addr2);
    if (res < 2) {
      std::cout << "Line number " << index << " is incorrect, skipping it.\n";
    }

    if (psr->isPIC()) {
      std::optional<std::array<uint64_t, 3>> range1 =
          psr->findLowerBoundRange(addr1);
      std::optional<std::array<uint64_t, 3>> range2 =
          psr->findLowerBoundRange(addr2);

      addr1 -= ((*range1)[0] - (*range1)[2]) * psr->isPIC();
      addr2 -= ((*range2)[0] - (*range2)[2]) * psr->isPIC();

      if (addr1 == addr2) {
        continue;
      }
    }

    Elf64_Sym_W_Name *sym1 = psr->findSymbolByAddress(addr1);
    if (!sym1) {
      std::cout << "Address " << std::hex << addr1 << " is out of range!\n";
      continue;
    }
    Elf64_Sym_W_Name *sym2 = psr->findSymbolByAddress(addr2);
    if (!sym2) {
      std::cout << "Address " << std::hex << addr2 << " is out of range!\n";
      continue;
    }

    std::pair<uint64_t, uint64_t> funcPair{sym1->symbol->st_value,
                                           sym2->symbol->st_value};
    funcHashTable[funcPair]++;
  }
}

void dumpMapToFile(std::map<std::pair<uint64_t, uint64_t>, int> &funcHashTable,
                   Parser *psr) {
  assert(psr);

  std::ofstream output;
  output.open("dump.dot");

  std::unordered_set<Elf64_Sym_W_Name *> uniqueSyms;

  output << "digraph D {\n";

  for (auto &MO : funcHashTable) {
    uint64_t addr1 = MO.first.first;
    uint64_t addr2 = MO.first.second;
    int numberOfCalls = MO.second;

    Elf64_Sym_W_Name *symName1 = psr->findSymbolByAddress(addr1);
    Elf64_Sym_W_Name *symName2 = psr->findSymbolByAddress(addr2);

    if (uniqueSyms.find(symName1) == uniqueSyms.end()) {
      uniqueSyms.insert(symName1);
    }

    if (uniqueSyms.find(symName2) == uniqueSyms.end()) {
      uniqueSyms.insert(symName2);
    }

    output << symName1->symbol->st_value << " -> " << symName2->symbol->st_value
           << "[label = \"" << numberOfCalls << "\"];\n";
  }

  for (auto &SO : uniqueSyms) {
    output << SO->symbol->st_value
           << "[fillcolor=cyan, style=\"filled\", label=\" " << SO->symName
           << "\"];\n";
  }

  output << "}\n";

  output.close();
}

void Parser::parseMapsFile(const char *mapsFile) {
  FILE *mapFile = fopen(mapsFile, "r");
  if (!mapFile) {
    std::cout << "Unable to open " << mapsFile << '\n';
    return;
  }

  std::array<uint64_t, 3> Range;
  char *permissions = new char[32];

  do {
    int res = fscanf(mapFile, "%lx-%lx %s %lx %*x:%*x %*x ", &Range[0], &Range[1], permissions, &Range[2]);
    if (res == EOF) {
        break;
    }
    else if (res < 4) {
        fscanf (mapFile, "%*s");
        continue;
    }

    if (!strcmp(permissions, "r-xp")) {
      Ranges.push_back(Range);
    }

  } while (1);

  delete[] permissions;
}

std::optional<std::array<uint64_t, 3>>
Parser::findLowerBoundRange(const uint64_t Addr) const {
  for (auto &Range : Ranges) {
    if (Range[0] < Addr && Addr < Range[1])
      return Range;
  }

  return std::nullopt;
}
