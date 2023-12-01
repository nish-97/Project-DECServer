#include <iostream>
#define THREAD_POOL_MAX 16
using namespace std;

void CompileAndRun(const string *);
string generateUniqueID();
void handleNewRequest(int);
void handleStatusRequest(int , string );
void crashControl();
void *workerThread(void *);