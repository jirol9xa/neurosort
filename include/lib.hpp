#pragma once

#include <array>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <unordered_set>
#include <vector>

size_t getFileSize(FILE *input);

size_t getNumberOfStrings(FILE *input);

char *bufferAlloc(size_t *fileS, FILE *input);

char *skipNonLetters(char *src);

void initializeArrOfPointers(char **ptrToStrArr, const size_t numbOfStrings,
                             char *text);

u_int64_t getHash(char *src);

enum ERRORS { OK, CORRUPTED_ELF };
