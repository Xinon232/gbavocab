#include <stddef.h>

// GBAWriter's freestanding string routines needed by its input/layout modules.
int strcmp(const char* first, const char* second) {
  while (*first && *first == *second) { ++first; ++second; }
  return (unsigned char)*first - (unsigned char)*second;
}

char* strcpy(char* destination, const char* source) {
  char* result = destination;
  while ((*destination++ = *source++)) {}
  return result;
}


size_t strlen(const char* s) {
  const char* p = s;
  while (*p) ++p;
  return (size_t)(p - s);
}

char* strchr(const char* s, int c) {
  char ch = (char)c;
  while (*s) {
    if (*s == ch) return (char*)s;
    ++s;
  }
  return ch == 0 ? (char*)s : 0;
}

// Row scanning looks for line ends and TABs in whole I/O chunks.
void* memchr(const void* s, int c, size_t n) {
  const unsigned char* p = (const unsigned char*)s;
  const unsigned char ch = (unsigned char)c;
  for (; n; --n, ++p) {
    if (*p == ch) return (void*)p;
  }
  return 0;
}

// V4.1 Import and the GBA dictionary builder.
int strncmp(const char* first, const char* second, size_t n) {
  for (; n; --n, ++first, ++second) {
    if (*first != *second) return (unsigned char)*first - (unsigned char)*second;
    if (!*first) return 0;
  }
  return 0;
}

char* strncpy(char* destination, const char* source, size_t n) {
  size_t i = 0;
  for (; i < n && source[i]; ++i) destination[i] = source[i];
  for (; i < n; ++i) destination[i] = 0;
  return destination;
}

char* strcat(char* destination, const char* source) {
  strcpy(destination + strlen(destination), source);
  return destination;
}

char* strrchr(const char* s, int c) {
  const char* found = 0;
  char ch = (char)c;
  for (;; ++s) {
    if (*s == ch) found = s;
    if (!*s) return (char*)found;
  }
}
