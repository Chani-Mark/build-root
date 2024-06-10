#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

int count_lines_in_file(int fd)
{
    // Seek to the end of the file
    if (lseek(fd, 0, SEEK_END) == -1)
    {
        perror("Error seeking to the end of the file");
        return -1;
    }

    int lines = 0;
    char ch;
    ssize_t bytesRead;

    // Read the file backwards
    while (lseek(fd, -1, SEEK_CUR) != -1)
    {
        bytesRead = read(fd, &ch, 1);
        if (bytesRead == -1)
        {
            perror("Error reading the file");
            return -1;
        }
        else if (bytesRead == 0)
        {
            break; // Reached the beginning of the file
        }

        // Check for newline characters
        if (ch == '\n')
        {
            lines++;
        }

        // Move back again since read moved the pointer forward
        if (lseek(fd, -1, SEEK_CUR) == -1)
        {
            perror("Error seeking backwards");
            return -1;
        }
    }

    // Check if the file does not end with a newline and is not empty
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1)
    {
        perror("Error getting file status");
        return -1;
    }

    if (file_stat.st_size > 0)
    {
        // Move to the start of the file to check the first character
        if (lseek(fd, 0, SEEK_SET) == -1)
        {
            perror("Error seeking to the start of the file");
            return -1;
        }
        bytesRead = read(fd, &ch, 1);
        if (bytesRead == 1 && ch != '\n')
        {
            lines++;
        }
    }

    return lines;
}

char* tail(int fd, int n, int line_count)
{
    //char buffer[MAX * 8];
     // Dynamically allocate memory for the buffer
    char *buffer = (char*)malloc(2000 * sizeof(char));
    if (buffer == NULL) {
        perror("Error allocating memory");
        close(fd);
        return NULL;
    }
    int lineindex = 0;
        char ch;

    // Reset the file pointer to the beginning
        // Reset the file pointer to the beginning
    lseek(fd, 0, SEEK_SET);

    // Skip to the start of the last n lines
    int target_line = line_count - n;
    int current_line = 0;
    while (current_line < target_line)
    {
          if (read(fd, &ch, 1) > 0) {
            if (ch == '\n') {
                current_line++;
            }
        }
    }

    // Print the last n lines
   while (read(fd, &ch, 1) > 0) 
    {
        // putchar(ch);
        buffer[lineindex] = ch;
        lineindex++;
    }
    return buffer;
}

// Function designed for chat between client and server.
void func(int connfd)
{
    char buff[MAX];
    char bufflast[MAX];
    char buffexit[10];
    //char bufferlogs[2000];

    int n;
    int fp; // File pointer for writing

    // Open file for writing
    fp = open("/home/chani/bsp/logger", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fp == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    // infinite loop for chat
    for (;;)
    {
        bzero(buff, MAX);
        bzero(bufflast, MAX);
        bzero(buffexit, 10);

        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        strcpy(bufflast, "info");
        strcpy(buffexit, "exit");

        int s = strncmp(buff, bufflast, 4);
        int digit;
        if (s == 0)
        {

            printf("loggggg\n");
            // Check if the character at buff[5] is a space and the next character is a digit
            if (buff[4] == ' ' && buff[5] >= '1' && buff[5] <= '8')
            {
                int i = 0;
                digit = buff[5] - '0'; // Convert character to integer
                //printf("The digit is: %d\n", digit);
                int line_count = count_lines_in_file(fp);
                if (line_count != -1)
                {
                    line_count--;
                    printf("The file has %d lines\n", line_count);
                    if (digit <= line_count - 1)
                    { // there is enogh logs to be printed
                        char * bufferlogs = tail(fp, digit, line_count);
                        while (i < strlen(bufferlogs))
                        {
                             printf("%c",bufferlogs[i]);
                             i++;
                        }
                        
                       

                    }
                }
                else
                {
                    printf("Failed to read the file \n");
                }
            }
            else
            {
                printf("No valid digit found after 'info '\n");
            }
        }
        else
        {

            s = strncmp(buff, buffexit, 4);

            if (s == 0){
                //printf("exittttttt\n");
                break;

            }

            // Write received message to file
            ssize_t w = write(fp, buff, strlen(buff));
            if (w < 0)
            {
                printf("Error writing to file!\n");
            }
        }
        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);
        bzero(buff, MAX);
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // and send that buffer to client
        write(connfd, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0)
        {
            printf("Server Exit...\n");
            break;
        }
    }

    // Close the file
    close(fp);
}

// Driver function
int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0)
    {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    // Function for chatting between client and server
    func(connfd);

    // After chatting close the socket
    close(sockfd);
}