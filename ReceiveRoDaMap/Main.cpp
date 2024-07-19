#include <iostream>
#include <string>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
    
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/core/mat.hpp"

static constexpr int s_ImageSize = 256;


int main()
{

    std::cout << std::fixed << std::setprecision(3);

    ///////////////////////////////////////////////////////////////////////////
    WSADATA wsaData;

    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

#define DEFAULT_PORT "27015"

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    // Create a SOCKET for the server to listen for client connections

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);


    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }




    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }


    freeaddrinfo(result);


    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }



    SOCKET ClientSocket;
    ClientSocket = INVALID_SOCKET;

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }





    char* recvbuf = new char[s_ImageSize * s_ImageSize * 4];
    char* RGBBuffer = recvbuf;
    char* GrayscaleBuffer = recvbuf + s_ImageSize * s_ImageSize * 3;

    int iReceive, iSendResult;
    int recvbuflen = s_ImageSize * s_ImageSize * 4;

    cv::Mat RGBImage = cv::Mat(s_ImageSize, s_ImageSize, CV_8UC3, RGBBuffer);
    cv::Mat GrayImage = cv::Mat(s_ImageSize, s_ImageSize, CV_8UC1, GrayscaleBuffer);

    int currentBufferHead = 0;
    auto currentTime = std::chrono::high_resolution_clock::now();

    static constexpr int relativePositions[] = { -257, -256, -255, -1, 0, 1, 255, 256, 257 };
    // Receive until the peer shuts down the connection
    do {

        iReceive = recv(ClientSocket, recvbuf + currentBufferHead, recvbuflen - currentBufferHead, 0);
        if (iReceive > 0) {
            std::cout << iReceive << '\n';

            if (iReceive + currentBufferHead != recvbuflen)
            {
                currentBufferHead += iReceive;
                continue;
            }

            currentBufferHead = 0;

            double milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - currentTime).count();
            currentTime = std::chrono::high_resolution_clock::now();
            std::cout << milliseconds << " ms ( " << 1000.0 / milliseconds <<  "FPS )\n";

            for (int i = 0; i < s_ImageSize * s_ImageSize; i++)
            {
                bool foundWhite = false;
                bool foundBlack = false;
                for (int j = 0; j < 9 && !(foundWhite && foundBlack); j++)
                {
                    if (!(i + relativePositions[j] > -1 && i + relativePositions[j] < s_ImageSize * s_ImageSize))
                        continue;

                    if (GrayscaleBuffer[i + relativePositions[j]] == 0)
                        foundBlack = true;
                    else
                        foundWhite = true;
                }

                if (foundWhite && foundBlack)
                {
                    RGBBuffer[i * 3] = (char)0;
                    RGBBuffer[i * 3 + 1] = (char)0;
                    RGBBuffer[i * 3 + 2] = (char)255;
                }
            }

            cv::imshow("RGB", RGBImage);
            cv::imshow("Grayscale", GrayImage);

            cv::waitKey(10);
        }
        else if (iReceive == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iReceive > 0);

    return 0;
}