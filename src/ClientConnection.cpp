//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//              This class processes an FTP transactions.
//
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"




ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);

    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }

    ok = true;
    data_socket = -1;
    parar = false;//?
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket);

}


int connect_TCP( uint32_t address,  uint16_t  port) {
     // Implement your code to define a socket here
     struct sockaddr_in sin;
     int s;

     s = socket(AF_INET,SOCK_STREAM,0);

     if (s < 0) {
       errexit("No se puede crear el socket: %s\n", strerror(errno));
       return -1;
     }

     memset(&sin, 0, sizeof(sin));
     sin.sin_family = AF_INET;
     sin.sin_addr.s_addr = address;
     sin.sin_port = htons(port);


     if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
       errexit("No se puede conectar con %s: %s\n", inet_ntoa(sin.sin_addr),
               strerror(errno));
       return -1;
     }

    return s; // You must return the socket descriptor.

}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;

}






#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }


  fprintf(fd, "220 Service ready\n");

  while(!parar) {

  fscanf(fd, "%s", command);


  if (COMMAND("USER")) {
	  fscanf(fd, "%s", arg);
	  fprintf(fd, "331 User name ok, need password\n");
  } else if (COMMAND("PWD")) {

  } else if (COMMAND("PASS")) {

    fscanf(fd, "%s", arg);
    if(strcmp(arg,"1234") == 0) {
      fprintf(fd, "230 User logged in\n");
    } else{
      fprintf(fd, "530 Not logged in.\n");
      parar = true;
    }
  } else if (COMMAND("PORT")) {

	  // To be implemented by students
    port();
  } else if (COMMAND("PASV")) {

    // To be implemented by students
    passv();
  } else if (COMMAND("STOR") ) {

    // To be implemented by students
    stor();
  } else if (COMMAND("RETR")) {

    // To be implemented by students
    retr();
  } else if (COMMAND("LIST")) {

    // To be implemented by students
    list();
  } else if (COMMAND("SYST")) {

    fprintf(fd, "215 UNIX Type: L8.\n");
  } else if (COMMAND("TYPE")) {

    fscanf(fd, "%s", arg);
	  fprintf(fd, "200 OK\n");
  } else if (COMMAND("QUIT")) {

    fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
    close(data_socket);
    parar=true;
    break;
  } else  {

    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	  printf("Comando : %s %s\n", command, arg);
	  printf("Error interno del servidor\n");

  }

}

    fclose(fd);


    return;

};

void ClientConnection::port(void) {

  passive = false;

  unsigned host_b[4], port_b[2];

  fscanf(fd, "%d,%d,%d,%d,%d,%d", &host_b[0], &host_b[1], &host_b[2],
         &host_b[3], &port_b[0], &port_b[1]);

  unsigned host = host_b[3]<<24 | host_b[2]<<16 |
      host_b[1]<<8 | host_b[0];
  unsigned port = port_b[0] << 8 | port_b[1];

  data_socket = connect_TCP(host,port);

  fprintf(fd, "200 OK\n");
}

void ClientConnection::retr(void) {
  fscanf(fd, "%s", arg);           // Reads file name
  //printf("Fichero: %s\n", arg);
  FILE* file = fopen(arg, "rb");    //Opens the file

  if (!file) {
    fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
    close(data_socket);
  } else {
    fprintf(fd, "150 File status okay - About to open data connection.\n");
    struct sockaddr_in socket_address;
    socklen_t socket_address_len = sizeof(socket_address);
    char buffer[MAX_BUFF];
    int n;

    if (passive) {
      data_socket = accept(data_socket,(struct sockaddr*)&socket_address,
                                        &socket_address_len);
    }

    do {
      n = fread(buffer, sizeof(char), MAX_BUFF, file);
      send(data_socket, buffer, n, 0);
    } while (n == MAX_BUFF);

    fprintf(fd, "226 Closing data connection. Requested file action successful\n");
    fclose(file);
    close(data_socket);
  }
}

void ClientConnection::passv(void) {
  passive = true;

  struct sockaddr_in sin, sock_address;
  socklen_t sock_address_len = sizeof(sock_address);
  int s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0) {
    errexit("No se puedo crear el socket: %s\n", strerror(errno));
  } else {
      int sv_address = 16777343; // Server address (localhost 127.0.0.1)
      memset(&sin, 0, sizeof(sin));
      sin.sin_family = AF_INET;
      sin.sin_addr.s_addr = sv_address;
      sin.sin_port = 0; //Cualquier puerto abierto (?)
      if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        errexit("No se ha podido hacer bind con el puerto: %s\n", strerror(errno));
      } else {
        if (listen(s, 5) < 0) {
          errexit("Fallo en el listen: %s\n", strerror(errno));
        } else {
          getsockname(s, (struct sockaddr*)&sock_address, &sock_address_len);
          fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
          (unsigned int)(sv_address & 0xff),
          (unsigned int)((sv_address >> 8) & 0xff),
          (unsigned int)((sv_address >> 16) & 0xff),
          (unsigned int)((sv_address >> 24) & 0xff),
          (unsigned int)(sock_address.sin_port & 0xff),
          (unsigned int)(sock_address.sin_port >> 8));
          data_socket = s;
        }
      }
  }
}

void ClientConnection::stor(void) {
  fscanf(fd, "%s", arg);
  FILE* file = fopen(arg, "wb");
  if (!file) {
    fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
    close(data_socket);
  } else {
    fprintf(fd, "150 File status okay; openning data connection.\n");
    fflush(fd);

    struct sockaddr_in socket_address;
    socklen_t socket_address_len = sizeof(socket_address);
    char buffer[MAX_BUFF];
    int n;

    if (passive) {
      data_socket = accept(data_socket, (struct sockaddr*)&socket_address,
                           &socket_address_len);
    }

    do {
      n = recv(data_socket, buffer, MAX_BUFF, 0);
      fwrite(buffer, sizeof(char), n, file);
    } while (n == MAX_BUFF);

    fprintf(fd, "226 Closing data connection. Operation successfully completed.\n");
    fclose(file);
    close(data_socket);
  }
}

void ClientConnection::list(void) {
  struct sockaddr_in socket_address;
  socklen_t socket_address_len = sizeof(socket_address);
  char buffer[MAX_BUFF];
  std::string ls_content = "";
  std::string ls = "ls";


  ls.append(" 2>&1"); //To redirect stderr to stdout

  fprintf(fd, "125 Data connection already open; transfer starting\n");
  FILE* file = popen(ls.c_str(), "r");

  if (!file) {
    fprintf(fd, "450 Requested file action not taken. File unavailable.\n");
    close(data_socket);
  } else {
    if (passive) {
      data_socket = accept(data_socket, (struct sockaddr*)&socket_address,
                           &socket_address_len);
    }
    while (!feof(file))
        if (fgets(buffer, MAX_BUFF, file) != NULL)
          ls_content.append(buffer);

    //  printf("ls_content : %s\n", ls_content.c_str());

    send(data_socket, ls_content.c_str(), ls_content.size(), 0);
    fprintf(fd, "250 Closing data connection. Requested file action successful.\n");
    pclose(file);
    close(data_socket);
  }
}
