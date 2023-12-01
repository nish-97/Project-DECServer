#include <iostream>
#define THREAD_POOL_MAX 16
using namespace std;


string CompileAndRun(const string*);
string generateUniqueID();
void handleStatusRequest(int,string);
void handleNewRequest(int*);
void crashControl();
void* workerThread(void*);
bool Config(const string *);

