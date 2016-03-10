#include <iostream>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <pthread.h>

#include "PracticalSocket.h"
#include "jsonxx.h"
#include "messageforger.h"

using namespace std;

// Function prototypes
void logcat(const char *tag, const char *message);
void blockThread(int min, int max);
bool extractInputArguments(int argc, char *argv[], char *&serverAddress, int &serverPort);
void send(TCPSocket &socket) throw(SocketException);
void receive(TCPSocket &socket) throw(SocketException);
void initSiteDataStructures();
void initTokenDataStructures();
void cleanup();
void requestForCriticalSection(TCPSocket &socket);
void doCriticalSection(TCPSocket &socket);
void exitCriticalSection(TCPSocket &socket);
void *receiveEventHandler(void *sock);
void sendRequest(TCPSocket &socket, int receiver);
void sendToken(TCPSocket &socket, int receiver);

// Buffer for receive operation
#define RCV_BUF_SIZE 1024
char buffer[RCV_BUF_SIZE];
// Number of messages received by each call of socket's recv
int messagesReceived = 0;
// Index to next pending message to be read
int toBeReadMessage = 0;

// Pointer to the data that is intended to be sent
const char *data;

// Singhal site arrays
int *SN;
int *SV;

// Singhal token arrays
int *TSN;
int *TSV;

// Number of all sites
int sitesCount;

// ID of this site
int myId;

// File stream for storing logcat's logs
fstream log;

// Mutex locks
pthread_mutex_t lockLogcat =            PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockSend =              PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockReceive =           PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockTurn =              PTHREAD_MUTEX_INITIALIZER;

// Condition variables
pthread_cond_t condToken =              PTHREAD_COND_INITIALIZER;

// Should I stop?
bool stop = false;

// ==============================================================================================================
// ==============================================================================================================
//
//                                          M A I N --- T H R E A D
//
// ==============================================================================================================
// ==============================================================================================================

int main(int argc, char *argv[])
{
    // Setting up log output stream
    log.open("/var/log/singhal_client_logcat.log", ios_base::out);

    logcat("App", "Application started");

    // Seed random number generator
    srand(time(NULL));

    char *serverAddress;
    int serverPort;

    if(!extractInputArguments(argc, argv, serverAddress, serverPort)){
        logcat("App", "Invalid or insufficient input arguments.");
        logcat("App", "Application halted unexpectedly");
        return 1;
    }

    sprintf(buffer, "Server entry point: %s:%d", serverAddress, serverPort);
    logcat("App", buffer);

    sprintf(buffer, "My site ID is %d", myId);
    logcat("App", buffer);

    pthread_t receiveThreadId;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

    TCPSocket socket;

    // We have to wait until network is reachable.
    int remainingTry = 20;
    while(remainingTry){
        try{
            // Try to establish connection with the server
            socket.connect(serverAddress, serverPort);
        } catch(SocketException &e){
            sleep(1);
            remainingTry -= 1;
            continue;
        }
        break;
    }
    if(remainingTry == 0){
        // Connection timed out
        logcat("Network", "Connection timed out");
        return 1;
    }

    logcat("Network", "Socket is now connected to the server");

    try {
        // Forge and send a hello message to the server
        MessageForger::Message clientHello(MessageForger::HELLO, myId, MessageForger::CONTROLER_SITE_ID);
        data = MessageForger::serialize(clientHello);
        send(socket);
        delete[] data;
        logcat("Network", "Hello message sent to the server");

        // Receive a hello message from the server
        receive(socket);
        MessageForger::Message serverHello = MessageForger::parse(buffer);

        // At this point only hello message is accepatble
        if(serverHello.type != MessageForger::HELLO){
            // Anti-manner detected or an error occurred
            logcat("Network", "The server did not say hello");
            return 1;
        }

        // Retrieve necessary fields from the hello message
        sitesCount = serverHello.sitesCount;

        logcat("Network", "Hello message received from the server");

        sprintf(buffer, "Am I the initial token holder? %s", myId == 0 ? "Yes" : "No");
        logcat("Singhal", buffer);

        sprintf(buffer, "Number of sites: %d", sitesCount);
        logcat("Singhal", buffer);

        // Initialize Singhal algorithm's data structures
        initSiteDataStructures();
        initTokenDataStructures();

        // Receive a Launch message from the server
        receive(socket);
        MessageForger::Message launchMessage = MessageForger::parse(buffer);

        // At this point only launch message is acceptable
        if(launchMessage.type != MessageForger::LAUNCH){
            // Anti-manner detected or an error occurred
            logcat("Network", "The server did not send the launch message as it should");
            logcat("App", "Application halted unexpectedly");
            return 1;
        }

        logcat("Network", "Launch message received from the server");
        logcat("Singhal", "----------- ALGORITHM STARTED -----------");

        // Unleash receiving thread
        pthread_create(&receiveThreadId, &attrs, receiveEventHandler, &socket);

        // Infinite loop, through which sites compete for critical section
        while (!stop) {

            logcat("Singhal", "I am doing non-CS stuff");
            // Do non-critical section stuff
            blockThread(1, 5);

            pthread_mutex_lock(&lockTurn);

            // If a stop message has been received while I was sleep
            if(stop){
                break;
            }

            if(SV[myId] == STATE_N){
                // Request for critical section
                requestForCriticalSection(socket);

                // Wait for the token
                pthread_cond_wait(&condToken, &lockTurn);
            }

            // If a stop message has been received while I was sleep
            if(stop){
                break;
            }

            // Update my state
            SV[myId] = STATE_E;

            pthread_mutex_unlock(&lockTurn);

            logcat("Singhal", "I am doing CS stuff");
            // Do critical section stuff, i.e. sending request for writing a file in host OS
            doCriticalSection(socket);

            // Exit critical section
            exitCriticalSection(socket);
        }

    } catch(SocketException &e) {
        logcat("Network", e.what());
        logcat("App", "Application halted unexpectedly");
        exit(1);
    }

    pthread_join(receiveThreadId, NULL);
    logcat("App", "Receiveing thread joined to the main thread");
    logcat("App", "Application normally finished");
    cleanup();
    return 0;
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                   R E C E I V I N G --- T H R E A D
//
// ==============================================================================================================
// ==============================================================================================================

/**
 * @brief receiveEvenHandler
 */
void *receiveEventHandler(void *sock){
    logcat("App", "Receive handler thread spawned");
    TCPSocket &socket = *((TCPSocket *) sock);

    while(true){
        // Try to receive a message from the socket
        try {
            receive(socket);
        } catch(SocketException &e){
            logcat("Network", e.what());
            logcat("App", "A network error occurred. Application will be automatically stopped");
            stop = true;
            break;
        }

        // Parse data
        MessageForger::Message message = MessageForger::parse(buffer);

        if(message.type == MessageForger::STOP){
            stop = true;
            logcat("Network", "Stop message received");
            break;
        }

        pthread_mutex_lock(&lockTurn);
        char buff[100];

        if(message.type == MessageForger::REQUEST){
            if(SN[message.sender] >= message.requesterSN){
                // Ignore the request, it's out-of-date.
                sprintf(buff, "An out-dated request received from site %d", message.sender);
                logcat("Singhal", buff);
            } else {
                sprintf(buff, "A request received from site %d", message.sender);
                logcat("Singhal", buff);
                SN[message.sender] = message.requesterSN;
                int myState = SV[myId];

                if(myState == STATE_N){
                    SV[message.sender] = STATE_R;
                    sprintf(buff, "My state is N");
                    logcat("Singhal", buff);
                }
                if(myState == STATE_R){
                    sprintf(buff, "My state is R");
                    logcat("Singhal", buff);

                    if(SV[message.sender] != STATE_R){
                        SV[message.sender] = STATE_R;

                        // Send a request message back to the sender
                        sendRequest(socket, message.sender);
                        sprintf(buff, "The request is valid and a request is sent back");
                        logcat("Singhal", buff);
                    } else {
                        sprintf(buff, "This request has been previously logged. Ignore it");
                        logcat("Singhal", buff);
                    }
                }
                if(myState == STATE_E){
                    SV[message.sender] = STATE_R;
                    sprintf(buff, "My state is E");
                    logcat("Singhal", buff);
                }
                if(myState == STATE_H){
                    sprintf(buff, "My state is H");
                    logcat("Singhal", buff);
                    SV[message.sender] = STATE_R;
                    TSV[message.sender] = STATE_R;
                    TSN[message.sender] = message.requesterSN;
                    SV[myId] = STATE_N;

                    sendToken(socket, message.sender);
                    sprintf(buff, "Token was sent to site %d", message.sender);
                    logcat("Singhal", buff);
                }
            }
        }
        if(message.type == MessageForger::TOKEN){
            sprintf(buff, "Token arrived from site %d", message.sender);
            logcat("Singhal", buff);

            memcpy(TSN, message.TSN, sitesCount * sizeof(int));
            memcpy(TSV, message.TSV, sitesCount * sizeof(int));
            delete[] message.TSN;
            delete[] message.TSV;

            // Update my state
            SV[myId] = STATE_E;

            pthread_cond_signal(&condToken);
        }

        pthread_mutex_unlock(&lockTurn);
    }

    pthread_mutex_lock(&lockTurn);
    pthread_cond_signal(&condToken);
    pthread_mutex_unlock(&lockTurn);

    pthread_exit(NULL);
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                 S I N G H A L --- S U B R O U T I N E S
//
// ==============================================================================================================
// ==============================================================================================================

void requestForCriticalSection(TCPSocket &socket){
    // Update my state
    SV[myId] = STATE_R;
    SN[myId] += 1;

    // Send a request message to all currently requesting sites
    for(int i = 0; i < sitesCount; i += 1){
        // Skip myself
        if(i == myId){
            continue;
        }

        if(SV[i] == STATE_R){
            sendRequest(socket, i);
            char buff[100];
            sprintf(buff, "Request for CS sent to site %d", i);
            logcat("Singhal", buff);
        }
    }
}

void doCriticalSection(TCPSocket &socket){
    const char *filename = "/home/abforce/big_apple";
    char buff[100];
    sprintf(buff, "I'm the site with index %d and overwriting this file for %d times", myId, SN[myId]);

    // Send request for writing on the protected file
    MessageForger::Message message(MessageForger::WRITE, myId, MessageForger::CONTROLER_SITE_ID);
    message.fileName = filename;
    message.fileContent = buff;
    pthread_mutex_lock(&lockSend);
    data = MessageForger::serialize(message);
    send(socket);
    delete[] data;
    pthread_mutex_unlock(&lockSend);
    logcat("Network", "Write message sent to the VMCC");

    // Block this thread for 1 or 2 second(s)
    blockThread(1, 2);
}

void exitCriticalSection(TCPSocket &socket){
    pthread_mutex_lock(&lockTurn);
    SV[myId] = STATE_N;
    TSV[myId] = STATE_N;

    for(int j = 0; j < sitesCount; j += 1){
        if(SN[j] > TSN[j]){
            TSV[j] = SV[j];
            TSN[j] = SN[j];
        } else {
            SV[j] = TSV[j];
            SN[j] = TSN[j];
        }
    }

    bool allIsNone = true;
    for(int j = 0; j < sitesCount; j += 1){
        if(SV[j] != STATE_N){
            allIsNone = false;
            break;
        }
    }
    if(allIsNone){
        SV[myId] = STATE_H;
        logcat("Singhal", "I've exited CS. All sites are of state N and I'm now of state H");
    } else {
        for(int j = 0; j < sitesCount; j += 1){
            if(SV[j] == STATE_R){
                sendToken(socket, j);
                char buff[100];
                sprintf(buff, "I've exited CS and sent the token to site %d", j);
                logcat("Singhal", buff);
                break;
            }
        }
    }
    pthread_mutex_unlock(&lockTurn);
}

/**
 * @brief initSiteDataStructures
 */
void initSiteDataStructures(){
    SN = new int[sitesCount];
    SV = new int[sitesCount];

    for(int i = 0; i < sitesCount; i += 1){
        SN[i] = 0;
        if(i < myId){
            SV[i] = STATE_R;
        } else {
            SV[i] = STATE_N;
        }
    }

    if(myId == 0){
        SV[0] = STATE_H;
    }
}

/**
 * @brief initTokenDataStructures
 */
void initTokenDataStructures(){
    TSN = new int[sitesCount];
    TSV = new int[sitesCount];

    if(myId == 0){
        for(int i = 0; i < sitesCount; i += 1){
            TSN[i] = 0;
            TSV[i] = STATE_N;
        }
    }
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                      N E T W O R K I N G --- S T U F F
//
// ==============================================================================================================
// ==============================================================================================================

void sendRequest(TCPSocket &socket, int receiver){
    MessageForger::Message request(MessageForger::REQUEST, myId, receiver);
    request.requesterSN = SN[myId];
    request.sitesCount = sitesCount;
    request.SN = SN;
    request.SV = SV;

    pthread_mutex_lock(&lockSend);
    data = MessageForger::serialize(request);
    send(socket);
    delete[] data;
    pthread_mutex_unlock(&lockSend);
}

void sendToken(TCPSocket &socket, int receiver){
    MessageForger::Message token(MessageForger::TOKEN, myId, receiver);
    token.sitesCount = sitesCount;
    token.TSN = TSN;
    token.TSV = TSV;
    token.SN = SN;
    token.SV = SV;

    pthread_mutex_lock(&lockSend);
    data = MessageForger::serialize(token);
    send(socket);
    delete[] data;
    pthread_mutex_unlock(&lockSend);
}

/**
 * @brief send
 */
void send(TCPSocket &socket) throw(SocketException){
    socket.send(data, strlen(data));
}

/**
 * @brief receive
 *  This function receives raw bytes from the socket and store them in the buffer.
 *  And finally the buffer will be null-terminated.
 */
void receive(TCPSocket &socket) throw(SocketException){
    pthread_mutex_lock(&lockReceive);
    int bytesReceived = socket.recv(buffer, RCV_BUF_SIZE);
    if (bytesReceived <= 0) {
        logcat("Network", "The socket was unexpectedly disconnected");
        logcat("App", "Application halted unexpectedly");
        exit(1);
    }
    buffer[bytesReceived] = '\0';
    pthread_mutex_unlock(&lockReceive);
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                                  M I S C
//
// ==============================================================================================================
// ==============================================================================================================

/**
 * @brief logcat
 * @param tag
 * @param message
 */
void logcat(const char *tag, const char *message){
    // Lock mutex
    pthread_mutex_lock(&lockLogcat);

    // Retrieve current time
    time_t now = time(0);
    struct tm tstruct;
    char currentTime[80];
    tstruct = *localtime(&now);
    strftime(currentTime, sizeof(currentTime), "%X", &tstruct);

    // Log
    log << currentTime << "  " << tag << "   \t" << message << endl;

    // Flush to file
    log.flush();

    // Unlock mutex
    pthread_mutex_unlock(&lockLogcat);
}

bool extractInputArguments(int argc, char *argv[], char *&serverAddress, int &serverPort){
    // A valid input argument list is as follows:
    // <server ip> <server port> <site id>
    if(argc != 4){
        return false;
    }

    // IP address of the server in dotted notation
    serverAddress = argv[1];

    // TCP port of the server
    serverPort = atoi(argv[2]);

    // My site ID
    myId = atoi(argv[3]);

    return true;
}

void blockThread(int min, int max){
    // Generates a random number between (min) and (max)
    int diff = rand() % (max - min);
    int wait = diff + min + (diff ? 1 : 0);

    // Block current thread for (wait) seconds
    sleep(wait);
}

/**
 * @brief cleanup
 */
void cleanup(){
    delete[] SN;
    delete[] SV;
    delete[] TSN;
    delete[] TSV;
    log.close();
}
