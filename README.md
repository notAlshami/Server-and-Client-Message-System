# Server and Client Message System

This application is a simple server and client message system implemented in C++. It allows multiple clients to connect to a server, send messages to each other, and receive broadcasts from the server.

## Features

### Server
- Non-blocking sockets for asynchronous I/O
- Handles multiple clients using the `select` system call
- Server can broadcast messages to all connected clients

#### Server argument

- --port <port_number> (optional default is 61111): Specifies the port number on which the server will listen for incoming connections.

### Client
- Allows clients to set their names
- Clients can send messages to each other
- Clients can save the chat to an external file

#### Client arguments

- --port <port_number> (optional default is 61111): Specifies the port number on which the server will listen for incoming connections.
- --name <client_name> (optional): Specifies the name of the client. If not provided, a default name will be assigned.
- --stream <file_path> (optional): Specifies a file path to stream messages to an external file.

## Prerequisites

- POSIX-compliant operating system (e.g., Linux)


### Note
- This code serves as a basic example and may contain bugs and limitations.
