#include <iostream>


#define THREAD_POOL_MAX 16

void* ClientHandler(void*);
std::string CompileAndRun(std::string* ,int);
int if_zero(int);
void* qCheck(void*);
void* reqinqueue(void *);