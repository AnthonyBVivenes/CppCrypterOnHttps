#include <string> 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> conexionActiva(false);
std::atomic<bool> terminarPrograma(false);

void ManejarConexion(SOCKET socketCliente) {
    char buffer[1024];
    while (conexionActiva) {
        int bytesRecibidos = recv(socketCliente, buffer, sizeof(buffer) - 1, 0);
        if (bytesRecibidos <= 0) {
            std::cerr << "Conexión perdida. Reconectando..." << std::endl;
            conexionActiva = false;
            closesocket(socketCliente);
            break;
        }
        buffer[bytesRecibidos] = '\0';
        std::cout << "\nMensaje recibido: " << buffer << "\n> ";
    }
}

void IntentarConexion(const char* ip, int puerto) {
    while (!terminarPrograma) {
        if (!conexionActiva) {
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in remoto = { 0 };
            remoto.sin_family = AF_INET;
            remoto.sin_port = htons(puerto);
            inet_pton(AF_INET, ip, &remoto.sin_addr);

            if (connect(s, (sockaddr*)&remoto, sizeof(remoto)) == 0) {
                conexionActiva = true;
                std::cout << "¡Conexión establecida con " << ip << "!" << std::endl;
                std::thread(ManejarConexion, s).detach();

                // Envío de mensajes
                std::string mensaje;
                while (conexionActiva) {
                    std::cout << "> ";
                    std::getline(std::cin, mensaje);
                    if (mensaje == "exit") {
                        terminarPrograma = true;
                        break;
                    }
                    send(s, mensaje.c_str(), mensaje.size(), 0);
                }
                closesocket(s);
            }
            else {
                std::cout << "Error de conexión. Reintentando en 3 segundos..." << std::endl;
                closesocket(s);
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void EscucharConexiones(int puerto) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in local = { 0 };
    local.sin_family = AF_INET;
    local.sin_port = htons(puerto);
    local.sin_addr.s_addr = INADDR_ANY;

    bind(s, (sockaddr*)&local, sizeof(local));
    listen(s, 1);

    while (!terminarPrograma) {
        if (!conexionActiva) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(s, &set);
            timeval timeout = { 1, 0 }; // Timeout de 1 segundo

            if (select(0, &set, NULL, NULL, &timeout) > 0) {
                SOCKET cliente = accept(s, NULL, NULL);
                conexionActiva = true;
                std::cout << "¡Conexión entrante establecida!" << std::endl;
                std::thread(ManejarConexion, cliente).detach();
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    closesocket(s);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::string ipRemota;
    std::cout << "IP del otro nodo: ";
    std::getline(std::cin, ipRemota);

    // Hilo para conexión saliente
    std::thread(IntentarConexion, ipRemota.c_str(), 54321).detach();

    // Hilo para conexión entrante
    std::thread(EscucharConexiones, 54321).detach();

    // Esperar hasta que el usuario quiera salir
    while (!terminarPrograma) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    conexionActiva = false;
    WSACleanup();
    return 0;
}