
# Author's Name

1. Nishkarsh Gautam
2. Naman Sharma


## Instructions to run each versions

#### a> lab 06
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### Run :-
```bash
./client <serverIP:port> <sourceCodeFileTobeGraded>

```

#### SERVER 
##### Compile :- 
```bash
g++ server.cpp -o server
```
##### Run :-
```bash
./server <port>

```
### 1. Version 1
#### b> lab 07
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### Run :-
```bash
./client <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>

```

##### Loadtest :-
```bash
./script.sh <numClients> <loopNum> <sleepTimeSeconds>
```
#### SERVER 
##### Compile :- 
```bash
make
```
##### Run :-
```bash
./server <port>
```

### 2. Version 2
#### a> lab 08
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### Run :-
```bash
./client <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>

```
##### Loadtest :-
```bash
./script.sh <numClients> <loopNum> <sleepTimeSeconds>
```

#### SERVER 
##### Compile :- 
```bash
make
```
##### Run :-
```bash
./server <port>

```

### 3. Version 3
#### a> lab 09
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### Run :-
```bash
./client <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>

```
##### Loadtest :-
```bash
./script.sh <numClients> <loopNum> <sleepTimeSeconds>
```

#### SERVER 
##### Compile :- 
```bash
make
```
##### Run :-
```bash
./server <port> <thread_pool_size>

```

### 4. Version 4
#### a> lab 10
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### To run only client:-
```bash
./client  <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID>

```

##### To run client with polling:-
```bash
python3  clientPolling.py  <serverIP:port> <sourceCodeFile> <polling_interval>

```

##### Loadtest :-
```bash
./loadtest.sh <serverIP:port> <sourceCodeFile>  <num_clients> <polling_interval> 
```

#### SERVER 
##### Compile :- 
```bash
make
```
##### Run :-
```bash
./server <port> <thread_pool_size>

```

### 4. Version 5
#### a> lab 11
####  CLIENT

##### Compile :- 
```bash
g++ client.cpp -o client
```
##### To run only client:-
```bash
./client  <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID>

```

##### To run client with polling:-
```bash
python3  clientPolling.py  <serverIP:port> <sourceCodeFile> <polling_interval>

```

##### Loadtest :-
```bash
./loadtest.sh <serverIP:port> <sourceCodeFile>  <num_clients> <polling_interval> 
```

#### SERVER 
##### Compile :- 
```bash
make
```
##### Run :-
```bash
./server <port> <thread_pool_size>  <config_file>
```


##GitHub Link: https://github.com/VedantK11/Project_DECS_SERVER

