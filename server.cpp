#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <map>
#include <fcntl.h>
#include "getArgument.hpp"

class MyServer
{
  sockaddr_in hint;
  std::map<int, std::string> MyClients;

public:
  MyServer(std::string address, unsigned short port)
  {
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &hint.sin_addr);
  }
  

  int setNonBlocking(int socket)
  {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
    {
      std::cerr << "fcntl(F_GETFL) failed" << std::endl;
      return -1;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      std::cerr << "fcntl(F_SETFL) failed" << std::endl;
      return -1;
    }
    return 0;
  }

  int runServer()
  {
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
      std::cerr << "Creating socket failed" << std::endl;
      return -1;
    }

    if (bind(listening, (sockaddr *)&hint, sizeof(hint)) == -1)
    {
      std::cerr << "Binding to this IP/Port failed" << std::endl;
      return -1;
    }

    if (listen(listening, SOMAXCONN) == -1)
    {
      std::cerr << "Listening failed" << std::endl;
      return -1;
    }

    if (setNonBlocking(listening) == -1)
    {
      std::cerr << "Setting listening socket to non-blocking mode failed" << std::endl;
      return -1;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listening, &master);
    FD_SET(STDIN_FILENO, &master);
    int maxFd = listening;

    std::cout << "Server is running on port: " << ntohs(hint.sin_port) << std::endl;

    while (true)
    {
      fd_set copy = master;
      int socketCount = select(maxFd + 1, &copy, nullptr, nullptr, nullptr);

      if (socketCount == -1)
      {
        std::cerr << "Select error" << std::endl;
        break;
      }

      for (int i = 0; i <= maxFd; ++i)
      {
        if (FD_ISSET(i, &copy))
        {
          if (i == listening)
          {
            sockaddr_in client;
            socklen_t clientSize = sizeof(client);

            int clientSocket = accept(listening, (sockaddr *)&client, &clientSize);
            if (clientSocket == -1)
            {
              std::cerr << "Client couldn't connect" << std::endl;
              continue;
            }

            MyClients[clientSocket] = "Client" + std::to_string(clientSocket);

            if (setNonBlocking(clientSocket) == -1)
            {
              std::cerr << "Setting client socket to non-blocking mode failed" << std::endl;
              close(clientSocket);
              continue;
            }

            FD_SET(clientSocket, &master);
            if (clientSocket > maxFd)
            {
              maxFd = clientSocket;
            }

            // Receive client name from client
            char clientName[1024];
            std::string name = "";
            memset(clientName, 0, sizeof(clientName));
            int bytesReceived = recv(clientSocket, clientName, sizeof(clientName), 0);
            if (bytesReceived > 0)
            {
              size_t pos = std::string(clientName).find("--name:");
              std::cout << name << std::endl;
              if (pos != std::string::npos)
              {
                name = std::string(clientName).substr(pos + 7); // Start extracting after "--name:"
              }
            }
            MyClients[clientSocket] = name.empty() ? std::string("unknown-" + std::to_string(clientSocket)) : name;
            std::cout << "[Only Server] Client " << MyClients[clientSocket] << " connected\n";
          }
          else if (i == STDIN_FILENO)
          {
            std::string serverInput;
            std::getline(std::cin, serverInput);
            for (const std::pair<const int, std::string> &client : MyClients)
            {
              if (client.first != i)
              {
                int bytesSent = send(client.first, ("[SERVER]: " + serverInput).c_str(), ("[SERVER]: " + serverInput).size(), 0);
                if (bytesSent == -1)
                {
                  std::cerr << "[Only Server] Failed to send message to client " << client.first << std::endl;
                }
              }
            }
          }
          else
          {
            char buf[4096];
            memset(buf, 0, sizeof(buf));

            int bytesRecv = recv(i, buf, sizeof(buf), 0);
            if (bytesRecv <= 0)
            {
              if (bytesRecv == 0)
              {
                std::cout << "[Only Server] Client " << MyClients[i] << " disconnected" << std::endl;
              }
              else
              {
                std::cerr << "recv error" << std::endl;
              }

              close(i);
              FD_CLR(i, &master);
              MyClients.erase(i);
            }
            else
            {
              std::string msg = std::string(buf, 0, bytesRecv);
              size_t pos = std::string(msg).find("--name:");
              if (pos != std::string::npos)
              {
                std::string name = std::string(msg).substr(pos + 7);
                if (!name.empty())
                {
                  MyClients[i] = name;
                  continue;
                }
              }

              std::cout << MyClients[i] << ": " << msg << std::endl;

              // Broadcast message to all clients
              for (const std::pair<const int, std::string> &client : MyClients)
              {
                if (client.first != i)
                {
                  int bytesSent = send(client.first, (MyClients[i] + ": " + msg).c_str(), (MyClients[i] + ": " + msg).size(), 0);
                  if (bytesSent == -1)
                  {
                    std::cerr << "[Only Server] Failed to send message to client " << client.first << std::endl;
                  }
                }
              }
            }
          }
        }
      }
    }

    close(listening);
    return 0;
  }
};

int main(int argc, char *argv[])
{
  std::string portNumberString = getArgument(argc, argv, "--port");
  int portNumber = portNumberString.empty() ? 61111 : std::stoi(portNumberString);

  MyServer serv("127.0.0.1", portNumber);
  serv.runServer();
  return 0;
}
