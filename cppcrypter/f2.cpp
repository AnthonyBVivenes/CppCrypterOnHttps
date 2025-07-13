#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

// Función para recibir mensajes en un hilo separado
void receiveMessages(SOCKET socket) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "\nLa conexión se ha perdido.\n";
            break;
        }
        buffer[bytesReceived] = '\0';
        std::cout << "\nMensaje recibido: " << buffer << "\n> ";
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al inicializar Winsock\n";
        return 1;
    }

    // Preguntar si quiere ser host o cliente
    
    std::cout << "host (1) cliente (2)? ";

    int choice;
    std::cin >> choice;
    std::cin.ignore(); // Limpiar el buffer

    SOCKET peerSocket;
    sockaddr_in peerAddr;

    if (choice == 1) { // Modo HOST (esperar conexión)
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Error al crear el socket\n";
            WSACleanup();
            return 1;
        }

        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(8080);
        peerAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenSocket, (sockaddr*)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR) {
            std::cerr << "Error al vincular el socket\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        if (listen(listenSocket, 1) == SOCKET_ERROR) {
            std::cerr << "Error al escuchar\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Esperando conexión entrante en el puerto 8080...\n";
        int peerAddrSize = sizeof(peerAddr);
        peerSocket = accept(listenSocket, (sockaddr*)&peerAddr, &peerAddrSize);
        if (peerSocket == INVALID_SOCKET) {
            std::cerr << "Error al aceptar la conexión\n";
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        closesocket(listenSocket);
        std::cout << "¡Conexión establecida!\n";
    }
    else if (choice == 2) { // Modo CLIENTE (conectarse a un host)
        peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (peerSocket == INVALID_SOCKET) {
            std::cerr << "Error al crear el socket\n";
            WSACleanup();
            return 1;
        }

        std::string ip;
        std::cout << "Ingresa la IP del host: ";
        std::getline(std::cin, ip);

        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(8080);
        inet_pton(AF_INET, ip.c_str(), &peerAddr.sin_addr);

        if (connect(peerSocket, (sockaddr*)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR) {
            std::cerr << "No se pudo conectar al host\n";
            closesocket(peerSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "¡Conectado al host!\n";
    }
    else {
        std::cerr << "Opción no válida\n";
        WSACleanup();
        return 1;
    }

    // Iniciar hilo para recibir mensajes
    std::thread receiver(receiveMessages, peerSocket);
    receiver.detach();

    // Enviar mensajes
    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message == "exit") break;
        send(peerSocket, message.c_str(), message.size(), 0);
    }

    closesocket(peerSocket);
    WSACleanup();
    return 0;
}