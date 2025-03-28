#include <stdlib.h>

//malloc wrapper to memory debug
void* debugMalloc(size_t size, const char* label) {
  Serial.printf("[malloc] %s: 要求サイズ = %u bytes\n", label, (unsigned int)size);
  Serial.printf("  Before malloc: Heap = %u, Max Alloc = %u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  void* ptr = malloc(size);
  if (ptr) {
    Serial.println("  malloc 成功！");
  } else {
    Serial.println("  malloc 失敗！！");
  }
  Serial.printf("  After malloc: Heap = %u, Max Alloc = %u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  return ptr;
}

