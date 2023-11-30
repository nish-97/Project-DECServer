#include <iostream>
#define THREAD_POOL_MAX 16
using namespace std;

void* ClientHandler(void*);
string CompileAndRun(string* ,int);
int if_zero(int);
void* qCheck(void*);
void* reqinqueue(void *);

void crashControl();
void* newfunc(void*);
void handleStatusRequest(int,string);
void handleNewRequest(int*);
void* workerThread(void*);
string generateUniqueID();

