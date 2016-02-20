# SimpleFTP
Simple command-line FTP server and client

# Compile 
Compile with the following commands:

`g++ ftserver.c -o ftserver`

`javac FtClient.java`

# Usage

Server: `./ftserver <PORT>`

Client: `java FtClient <HOST> <CONTROL_PORT> <COMMAND> <DATA_PORT> [<FILE_NAME>]`
