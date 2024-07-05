#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <fstream>
#include "getArgument.hpp"

class MyClient
{
  sockaddr_in hint;
  std::string name;
  std::ofstream fileStream;

public:
  MyClient(std::string address, unsigned short port, std::string streamingFilePath)
  {
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &hint.sin_addr);

    if (!streamingFilePath.empty())
    {
      fileStream.open(streamingFilePath, std::ios::app);
      if (!fileStream.is_open())
      {
        std::cerr << "Failed to open file for streaming: " << streamingFilePath << std::endl;
      }
    }
  }

  ~MyClient()
  {
    if (fileStream.is_open())
    {
      fileStream.close();
    }
  }

  void logMsg(const std::string &msg)
  {
    if (fileStream.is_open())
    {
      fileStream << msg << std::endl;
    }
  }

  int connectToServer(std::string clientName)
  {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
      std::cerr << "Creating socket went wrong" << std::endl;
      return -1;
    }

    int connectRes = connect(sock, (sockaddr *)&hint, sizeof(hint));
    if (connectRes == -1)
    {
      std::cerr << "Server didn't respond on port: " << ntohs(hint.sin_port) << std::endl;
      return -1;
    }

    std::cout << "Connected to the server on port: " << ntohs(hint.sin_port) << std::endl;

    fd_set master;
    FD_ZERO(&master);
    FD_SET(sock, &master);
    FD_SET(STDIN_FILENO, &master);
    int maxFd = std::max(sock, STDIN_FILENO);

    char buf[4096];
    std::string userInput;

    if (!clientName.empty())
    {
      int sendRes = send(sock, ("--name:" + clientName).c_str(), ("--name:" + clientName).size() + 1, 0);

      if (sendRes == -1)
      {
        std::cout << "[Server] didn't get your name" << std::endl;
        clientName = "unknown";
      }
    }

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
          if (i == sock)
          {
            memset(buf, 0, 4096);
            int bytesReceived = recv(sock, buf, 4096, 0);
            if (bytesReceived <= 0)
            {
              if (bytesReceived == 0)
              {
                std::cout << "Server disconnected" << std::endl;
              }
              else
              {
                std::cerr << "Recv error" << std::endl;
              }
              close(sock);
              return -1;
            }
            std::cout << std::string(buf, bytesReceived) << std::endl;
            logMsg(std::string(buf, bytesReceived));
          }
          else if (i == STDIN_FILENO)
          {
            std::getline(std::cin, userInput);
            if (!userInput.empty())
            {
              int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
              if (sendRes == -1)
              {
                std::cout << "Server didn't receive" << std::endl;
              }
              else {
                logMsg(userInput);
              }
            }
          }
        }
      }
    }

    close(sock);
    return -1;
  }
};

int main(int argc, char *argv[])
{
  std::string portNumberString = getArgument(argc, argv, "--port");
  std::string streamingFilePath = getArgument(argc, argv, "--stream");
  std::string clientName = getArgument(argc, argv, "--name");

  int portNumber = portNumberString.empty() ? 61111 : std::stoi(portNumberString);
  MyClient client("127.0.0.1", portNumber, streamingFilePath);
  client.connectToServer(clientName);

  return 0;
}
