#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>  /* define socket */
#include <netinet/in.h>  /* define internet socket */
#include <netdb.h>       /* define internet socket */
#include <iostream>
#include <cstring>
#include <string>
#include <pthread.h>
#include <vector>
#include <signal.h>

#define SERVER_PORT 10580        /* define a server port number */

using namespace std;

//STRUCT: Client
//Stores information for each client that connects
struct client {
  int id; //This is the ID returned when the client connects to the port the server is being hosted on
  char *name; //This is the name the client enters when they first connect
  pthread_t thread; //This is the thread ID used to reference the thread on the OS in which the connection with a client is contained

  client(int ID, char *NAME, pthread_t THR) : id(ID), name(NAME), thread(THR) {}; //Constructor
  ~client() { //Destructor to remove the name pointer to avoid memory leaks
    delete name;
  }
};

vector<client*> CLIENTS; //Stores a list of all clients that are currently connected.
//vector<pthread_t> THREADS;
bool quit = false; //Used to check whether the server is in the process of shutting down
int sd, ns;
int BUF_SIZE = 512; //Default size of messages to be sent between client/server
struct sockaddr_in server_addr = { AF_INET, htons(SERVER_PORT) };
/////////////
//sendTextAll: Sends text to all connected clients
void sendTextAll(char* reply) {
  int num = CLIENTS.size();
  for (int i = 0; i < num; i++) {
    write(CLIENTS[i]->id, reply, BUF_SIZE); //Send 'reply' to al connected clients
  }
}
/////////////
//sendText: Sends text to all connected clients, except for the one who's name is specified
void sendText(char* reply, char* name) {
  int num = CLIENTS.size();
  for (int i = 0; i < num; i++) {
    if (strcmp(CLIENTS[i]->name, name) != 0)
      write(CLIENTS[i]->id, reply, BUF_SIZE);
  }
}
/////////////
//quitProg: Shuts down the server. This is called immediately after quitInt.
void quitProg() {
  sleep(5); //Wait 5 seconds

  char msg[512] = "";
  strcpy(msg, "The server will shut down in 5 seconds...");
  sendTextAll(msg);
  cout << msg << endl;

  sleep(5); //Wait 5 more seconds

  strcpy(msg, "The server is shutting down!");
  sendTextAll(msg);
  cout << msg << endl;

  strcpy(msg, "##///##"); //This is the special string sent to let the client program know to quit.
  sendTextAll(msg);

  unlink((const char*)&server_addr.sin_addr);
  for (unsigned int i = 0; i < CLIENTS.size(); i++) {
    close(CLIENTS[i]->id); //Close the connection used by each client
    delete CLIENTS[i]; //Delete the client object pointed by the CLIENTS vector
  }
  close(sd); //Close the port
  exit(0); //Quit the program
}
/////////////
//quitInt: Begins the shut down proceedure of the server
void quitInt(int signo) {
  char msg[512] = "";
  strcpy(msg, "The server will shut down in 10 seconds...");
  sendTextAll(msg);
  cout << endl << msg << endl;
  quitProg();
}
/////////////
//*clientThread: This function is used to control each connection to a client in a separate thread
void *clientThread(void *name) {
  client* me = (client*)name;
  char buf[512];
  bool wait = true; //This is used to prevent a blank message from being sent to each client when a new client joins
  char msg[512] = "";

  strcat(msg, me->name);
  strcat(msg, " has joined!");
  sendText(msg, me->name); //Let each connected client know (except this client) know that this client has joined
  cout << me->name << " has joined!" << endl;

  do { //Start a near-infinite loop to keep the thread active
    if (read(me->id, buf, sizeof(buf)) > 0) { //When a message is recieved
      if (!wait && strcmp(buf, "/exit") != 0 && strcmp(buf, "/quit") != 0 && strcmp(buf, "/part") != 0) { //If not one of the exit commands
        char reply[512];
        strcpy(reply, me->name);
        strcat(reply, ": ");
        strcat(reply, buf);
        cout << reply << endl;
        sendText(reply, me->name); //Send the message to all other clients
      }
    }
    wait = false;
  } while (strcmp(buf, "/exit") != 0 && strcmp(buf, "/quit") != 0 && strcmp(buf, "/part") != 0);
  cout << me->name << " has left!" << endl;

  strcpy(msg, me->name);
  strcat(msg, " has left!");
  sendText(msg, me->name); //Let all other clients know this client has left

  close(me->id);
  for (unsigned int i = 0; i < CLIENTS.size(); i++) {
    if (CLIENTS[i]->id == me->id) {
      CLIENTS.erase(CLIENTS.begin() + i); //Remove this client from the list of all clients
      break;
    }
  }
  delete me;
  pthread_exit(NULL); //Exit the thread
}

int main() {
  struct sockaddr_in client_addr = { AF_INET };
  unsigned int client_len = sizeof(client_addr);
  client* newClient;
  signal(SIGINT, quitInt); //If an interupt signal is given, call the quitInt function
  signal(SIGPIPE, SIG_IGN); //When a client disconnects ungracefully (ie network error), an interupt is sent of type SIGPIPE
                            // We want to ignore this, otherwise the server may quit ungracefully

  //create a stream socket
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cout << "Server: Failed to create a socket!" << endl;
    return 1;
  }

  //bind the socket to an internet port
  if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    cout << "Server: Failed to bind the socket!" << endl;
    return 1;
  }

  while (!quit) { //While the server hasn't quit
    if (listen(sd, 1) != -1 && !quit) { //Listen for clients
      if ((ns = accept(sd, (struct sockaddr*)&client_addr, &client_len))) { //If a client wants to connect, accept that connection
        char *newClientName = new char[512];
        pthread_t th;
        read(ns, newClientName, sizeof(newClientName)); //Read the name of the new client
        newClient = new client(ns, newClientName, th);
        CLIENTS.push_back(newClient); //Add this client to the list of clients
        if (pthread_create(&th, NULL, clientThread, (void *)(newClient))) { //Create a new thread for the client
          cout << "Failed to create client thread! ";
          return 1;
        }
      }
    }
  }

  quitInt(1); //Gracefully shutdown the program if we get to here for any reason

  return 0;
}