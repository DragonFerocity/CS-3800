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

#define SERVER_PORT 10580     /* define a server port number */

using namespace std;

bool connected = false;
/////////////
//*writeThread: Used to send any messages from this client to the server
void *writeThread(void *arg) {
  char buf[512] = "";
  int sd;
  sd = (long)arg;
  cin.ignore();

  while (connected) { //Start a near infinite loop while the client is connected
    if (cin.getline(buf, 512)) { //Get anything the client types in the console
      write(sd, buf, sizeof(buf));
      if (strcmp(buf, "/exit") != 0 && strcmp(buf, "/quit") != 0 && strcmp(buf, "/part") != 0) { //If the client does not type an exit command
        //cout << "Me: " << buf << endl;
      }
      else
        connected = false;
    }
  }
  pthread_exit(NULL);
}
/////////////
//*readThread: Used to recieve incoming messages
void *readThread(void *arg) {
  char buf[512] = "";
  int sd;
  sd = (long)arg;
  while (connected) {
    read(sd, buf, sizeof(buf));
    if (strcmp(buf, "##///##") != 0 && connected) //Check to see if the server has sent the shutdown message and if we are still connected
      cout << buf << endl;
    else
      connected = false;
  }
  pthread_exit(NULL);
}

void interrupt(int signo) { //If the user hits ctrl+C
  cout << endl << "SYSTEM: If you would like to exit, please type '/exit', '/quit', or '/part'." << endl;
}

int main(int argc, char* argv[])
{
  int sd;
  struct sockaddr_in server_addr = { AF_INET, htons(SERVER_PORT) };
  struct hostent *hp;
  char name[512] = "";
  pthread_t readTh;
  pthread_t writeTh;
  signal(SIGINT, interrupt);

  if (argc != 2) {
    cout << "Usage: " << argv[0] << " hostname" << endl;
    return 1;
  }

  //get the host
  if ((hp = gethostbyname(argv[1])) == NULL) {
    cout << argv[0] << ": " << argv[1] << " uknown host!" << endl;
    return 1;
  }
  bcopy(hp->h_addr_list[0], (char*)&server_addr.sin_addr, hp->h_length);

  //create a socket
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cout << "Client: socket failed!" << endl;
    return 1;
  }

  //Prompt the user for a username
  cout << "What is your name? ";
  cin >> name;

  //connect a socket
  if (connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    cout << "Client: failed to connect!" << endl;
    return 1;
  }

  connected = true;
  cout << "Connected to " << argv[1] << "!" << endl << endl;
  write(sd, name, sizeof(name));

  if (pthread_create(&writeTh, NULL, writeThread, (void *)(sd))) {
    cout << "Failed to create thread!";
    return 1;
  }
  if (pthread_create(&readTh, NULL, readThread, (void *)(sd))) {
    cout << "Failed to create thread!";
    return 1;
  }

  while (connected) {
    //Wait
  }

  close(sd);
  return 0;
}
