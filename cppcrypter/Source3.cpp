
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <limits>
#pragma comment(lib, "ws2_32.lib")

void RecibirMensajes(SOCKET socket) {
    char buffer[1024];
    while (true) {
        int bytesRecibidos = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRecibidos <= 0) {
            std::cerr << "Conexion cerrada.\n";
            break;
        }
        buffer[bytesRecibidos] = '\0';
        std::cout << "Mensaje recibido: " << buffer << "\n";
    }
}

int mai1n() {
    // Inicializar Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al inicializar Winsock\n";
        return 1;
    }

    // Configuración común
    SOCKET socketPeer;
    sockaddr_in dirPeer = { 0 };
    dirPeer.sin_family = AF_INET;
    dirPeer.sin_port = htons(8080);

    // Selección de modo
    std::cout << "Seleccione modo:\n1. Esperar conexion\n2. Conectar a IP\nOpcion: ";
    int modo;
    std::cin >> modo;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (modo == 1) {
        // Modo servidor
        socketPeer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        dirPeer.sin_addr.s_addr = INADDR_ANY;

        if (bind(socketPeer, (sockaddr*)&dirPeer, sizeof(dirPeer)) == SOCKET_ERROR) {
            std::cerr << "Error al vincular socket\n";
            closesocket(socketPeer);
            WSACleanup();
            return 1;
        }

        listen(socketPeer, 1);
        std::cout << "Esperando conexion en puerto 8080...\n";

        SOCKET socketCliente = accept(socketPeer, NULL, NULL);
        closesocket(socketPeer);
        socketPeer = socketCliente;
    }
    else {
        // Modo cliente
        char ipPeer[16];
        std::cout << "Ingrese IP del peer: ";
        std::cin >> ipPeer;

        if (inet_pton(AF_INET, ipPeer, &dirPeer.sin_addr) != 1) {
            std::cerr << "Direccion IP invalida\n";
            WSACleanup();
            return 1;
        }

        socketPeer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        // Intentar conexión
        while (connect(socketPeer, (sockaddr*)&dirPeer, sizeof(dirPeer)) == SOCKET_ERROR) {
            std::cout << "Intentando conexion...\n";
            Sleep(2000);
        }
    }

    std::cout << "Conexion establecida!\n";

    // Hilo para recibir mensajes
    std::thread hiloRecibir(RecibirMensajes, socketPeer);
    hiloRecibir.detach();

    // Enviar mensajes
    char mensaje[1024];
    while (true) {
        std::cout << "> ";
        std::cin >> mensaje;

        if (strcmp(mensaje, "exit") == 0) break;

        send(socketPeer, mensaje, strlen(mensaje), 0);
    }

    closesocket(socketPeer);
    WSACleanup();
    return 0;
}