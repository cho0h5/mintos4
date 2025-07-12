#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DWORD               unsigned int
#define BYTE                unsigned char
#define MIN(x, y)           (((x) < (y)) ? (x) : (y))

#define SERIAL_FIFOMAXSIZE  16

int main(int argc, char **argv) {
    char vcFileName[256];
    if (argc < 2) {
        printf("Usage: ./a.out filename\n");
        return 1;
    } else {
        strcpy(vcFileName, argv[1]);
    }

    FILE *fp = fopen(vcFileName, "rb");
    if (fp == NULL) {
        fprintf(stderr, "%s File Open Error\n", vcFileName);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    const DWORD dwDataLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("File Name %s, Data Length %d Byte\n", vcFileName, dwDataLength);

    // Connect socket
    struct sockaddr_in stSocketAddr;
    stSocketAddr.sin_family = AF_INET;
    stSocketAddr.sin_port = htons(4444);
    stSocketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    const int iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(iSocket, (struct sockaddr *)&stSocketAddr, sizeof(stSocketAddr)) == -1) {
        fprintf(stderr, "Socket Connect Error\n");
        return 1;
    }
    printf("Socket Connect Success\n");

    // Send data
    if (send(iSocket, &dwDataLength, 4, 0) != 4) {
        fprintf(stderr, "Data Length Send Fail, [%d] Byte\n", dwDataLength);
        return 1;
    }
    printf("Data Length Send Success, [%d] Byte\n", dwDataLength);

    BYTE bAck;
    if (recv(iSocket, &bAck, 1, 0) != 1) {
        fprintf(stderr, "Ack Receive Error\n");
        return 1;
    }

    printf("Now Data Transfer...");
    DWORD dwSentSize = 0;
    while (dwSentSize < dwDataLength) {
        const DWORD dwTemp = MIN(dwDataLength - dwSentSize, SERIAL_FIFOMAXSIZE);
        dwSentSize += dwTemp;

        char vcDataBuffer[SERIAL_FIFOMAXSIZE];
        if (fread(vcDataBuffer, 1, dwTemp, fp) != dwTemp) {
            fprintf(stderr, "File Read Error\n");
            return 1;
        }

        if (send(iSocket, vcDataBuffer, dwTemp, 0) != dwTemp) {
            fprintf(stderr, "Socket Send Error\n");
            return 1;
        }

        if (recv(iSocket, &bAck, 1, 0) != 1) {
            fprintf(stderr, "Ack Reaceive Error\n");
            return 1;
        }

        printf("#");
    }

    fclose(fp);
    close(iSocket);

    printf("\nSend Complete. [%d] Byte\n", dwSentSize);
    return 0;
}
