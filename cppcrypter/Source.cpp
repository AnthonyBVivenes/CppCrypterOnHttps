#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")

// Constantes para la encriptación
#define AES_KEY_SIZE 256
#define RSA_KEY_SIZE 2048
#define BUFFER_SIZE 1024
#define OPENSSL_API_COMPAT 10100
#define OPENSSL_NO_DEPRECATED 1


#define AES_BLOCK_SIZE 16  // Tamaño de bloque AES (128 bits)
#define AES_KEY_SIZE 256   // Tamaño de clave AES (256 bits)
#define RSA_KEY_SIZE 2048  // Tamaño de clave RSA (2048 bits)
#define BUFFER_SIZE 1024 


// Estructura para mantener el contexto de encriptación
struct CryptoContext {
    EVP_PKEY* pkey;          // Clave local (par de claves)
    EVP_PKEY* peer_pkey;     // Clave pública del peer
    unsigned char aes_key[AES_KEY_SIZE / 8];
    unsigned char iv[AES_BLOCK_SIZE];
    bool keys_exchanged;
};

// Función para generar un par de claves RSA usando EVP
EVP_PKEY* generate_rsa_key() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        std::cerr << "Error creando contexto de generación de clave\n";
        return nullptr;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        std::cerr << "Error inicializando generación de clave\n";
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEY_SIZE) <= 0) {
        std::cerr << "Error configurando tamaño de clave RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        std::cerr << "Error generando par de claves RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    // Mostrar la clave pública generada
    std::cout << "\n--- Clave RSA generada ---\n";
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, pkey);
    char* pub_key;
    long pub_len = BIO_get_mem_data(bio, &pub_key);
    std::cout << "Clave PÚBLICA:\n" << std::string(pub_key, pub_len) << "\n";
    BIO_free(bio);

    // Mostrar la clave privada generada (solo para depuración)
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL);
    char* priv_key;
    long priv_len = BIO_get_mem_data(bio, &priv_key);
    std::cout << "Clave PRIVADA:\n" << std::string(priv_key, priv_len) << "\n";
    BIO_free(bio);

    EVP_PKEY_CTX_free(ctx);
    return pkey;
}
// Función para serializar una clave pública EVP a string
std::string evp_pubkey_to_string(EVP_PKEY* pkey) {
    if (!pkey) return "";

    BIO* bio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(bio, pkey)) {
        BIO_free(bio);
        return "";
    }

    char* data;
    long len = BIO_get_mem_data(bio, &data);
    std::string result(data, len);

    BIO_free(bio);
    return result;
}

// Función para cargar una clave pública EVP desde string
EVP_PKEY* string_to_evp_pubkey(const std::string& str) {
    BIO* bio = BIO_new_mem_buf(str.c_str(), str.length());
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return pkey;
}

// Función para cifrar con RSA usando EVP
std::vector<unsigned char> rsa_encrypt(EVP_PKEY* pkey, const unsigned char* data, size_t data_len) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        std::cerr << "Error creando contexto de cifrado RSA\n";
        return {};
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        std::cerr << "Error inicializando cifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, data, data_len) <= 0) {
        std::cerr << "Error obteniendo tamaño de cifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    std::vector<unsigned char> result(outlen);
    if (EVP_PKEY_encrypt(ctx, result.data(), &outlen, data, data_len) <= 0) {
        std::cerr << "Error en cifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    result.resize(outlen);
    EVP_PKEY_CTX_free(ctx);
    return result;
}

// Función para descifrar con RSA usando EVP
std::vector<unsigned char> rsa_decrypt(EVP_PKEY* pkey, const unsigned char* data, size_t data_len) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        std::cerr << "Error creando contexto de descifrado RSA\n";
        return {};
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        std::cerr << "Error inicializando descifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, data, data_len) <= 0) {
        std::cerr << "Error obteniendo tamaño de descifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    std::vector<unsigned char> result(outlen);
    if (EVP_PKEY_decrypt(ctx, result.data(), &outlen, data, data_len) <= 0) {
        std::cerr << "Error en descifrado RSA\n";
        EVP_PKEY_CTX_free(ctx);
        return {};
    }

    result.resize(outlen);
    EVP_PKEY_CTX_free(ctx);
    return result;
}

// Función para cifrar con AES usando EVP
std::vector<unsigned char> aes_encrypt(const unsigned char* key, const unsigned char* iv,
    const unsigned char* data, size_t data_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Error creando contexto de cifrado AES\n";
        return {};
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        std::cerr << "Error inicializando cifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    std::vector<unsigned char> result(data_len + EVP_MAX_BLOCK_LENGTH);
    int outlen1 = 0, outlen2 = 0;

    if (EVP_EncryptUpdate(ctx, result.data(), &outlen1, data, data_len) != 1) {
        std::cerr << "Error actualizando cifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (EVP_EncryptFinal_ex(ctx, result.data() + outlen1, &outlen2) != 1) {
        std::cerr << "Error finalizando cifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    result.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

// Función para descifrar con AES usando EVP
std::vector<unsigned char> aes_decrypt(const unsigned char* key, const unsigned char* iv,
    const unsigned char* data, size_t data_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Error creando contexto de descifrado AES\n";
        return {};
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        std::cerr << "Error inicializando descifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    std::vector<unsigned char> result(data_len + EVP_MAX_BLOCK_LENGTH);
    int outlen1 = 0, outlen2 = 0;

    if (EVP_DecryptUpdate(ctx, result.data(), &outlen1, data, data_len) != 1) {
        std::cerr << "Error actualizando descifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (EVP_DecryptFinal_ex(ctx, result.data() + outlen1, &outlen2) != 1) {
        std::cerr << "Error finalizando descifrado AES\n";
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    result.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

// Función para realizar el intercambio de claves (host)
bool perform_key_exchange_host(SOCKET socket, CryptoContext& ctx) {
    // 1. Enviar nuestra clave pública al cliente
    std::string pubkey_str = evp_pubkey_to_string(ctx.pkey);
    if (pubkey_str.empty()) {
        std::cerr << "Error serializando clave pública\n";
        return false;
    }

    // Enviar longitud primero
    uint32_t len = htonl(pubkey_str.size());
    if (send(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error enviando longitud de clave pública\n";
        return false;
    }

    // Luego enviar la clave
    if (send(socket, pubkey_str.c_str(), pubkey_str.size(), 0) != pubkey_str.size()) {
        std::cerr << "Error enviando clave pública\n";
        return false;
    }

    // 2. Recibir clave pública del cliente
    if (recv(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error recibiendo longitud de clave pública\n";
        return false;
    }
    len = ntohl(len);

    std::vector<char> peer_pubkey(len);
    if (recv(socket, peer_pubkey.data(), len, 0) != len) {
        std::cerr << "Error recibiendo clave pública\n";
        return false;
    }

    ctx.peer_pkey = string_to_evp_pubkey(std::string(peer_pubkey.data(), len));
    if (!ctx.peer_pkey) {
        std::cerr << "Error cargando clave pública del peer\n";
        return false;
    }

    // MOSTRAR CLAVE PÚBLICA RECIBIDA DEL CLIENTE
    std::cout << "\n--- Clave PÚBLICA RECIBIDA (del cliente) ---\n";
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, ctx.peer_pkey);
    char* pubkey_data;
    long pubkey_len = BIO_get_mem_data(bio, &pubkey_data);
    std::cout << std::string(pubkey_data, pubkey_len) << "\n";
    BIO_free(bio);

    // 3. Generar clave AES y IV aleatorios
    if (!RAND_bytes(ctx.aes_key, sizeof(ctx.aes_key)) || !RAND_bytes(ctx.iv, sizeof(ctx.iv))) {
        std::cerr << "Error generando clave/IV aleatorios\n";
        return false;
    }

    // 4. Enviar clave AES e IV cifrados con RSA
    auto encrypted_key = rsa_encrypt(ctx.peer_pkey, ctx.aes_key, sizeof(ctx.aes_key));
    auto encrypted_iv = rsa_encrypt(ctx.peer_pkey, ctx.iv, sizeof(ctx.iv));

    if (encrypted_key.empty() || encrypted_iv.empty()) {
        std::cerr << "Error cifrando clave AES/IV\n";
        return false;
    }

    // Enviar longitud y luego datos cifrados para la clave
    len = htonl(encrypted_key.size());
    if (send(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error enviando longitud de clave cifrada\n";
        return false;
    }
    if (send(socket, (char*)encrypted_key.data(), encrypted_key.size(), 0) != encrypted_key.size()) {
        std::cerr << "Error enviando clave cifrada\n";
        return false;
    }

    // Enviar longitud y luego datos cifrados para el IV
    len = htonl(encrypted_iv.size());
    if (send(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error enviando longitud de IV cifrado\n";
        return false;
    }
    if (send(socket, (char*)encrypted_iv.data(), encrypted_iv.size(), 0) != encrypted_iv.size()) {
        std::cerr << "Error enviando IV cifrado\n";
        return false;
    }

    // MOSTRAR CLAVE AES GENERADA
    std::cout << "\n--- Clave AES generada para la comunicación ---\n";
    std::cout << "Clave AES: ";
    for (size_t i = 0; i < sizeof(ctx.aes_key); ++i) {
        printf("%02x", ctx.aes_key[i]);
    }
    std::cout << "\nIV: ";
    for (size_t i = 0; i < sizeof(ctx.iv); ++i) {
        printf("%02x", ctx.iv[i]);
    }
    std::cout << "\n";

    ctx.keys_exchanged = true;
    return true;
}


// Función para realizar el intercambio de claves (cliente)
bool perform_key_exchange_client(SOCKET socket, CryptoContext& ctx) {
    // 1. Recibir clave pública del host
    uint32_t len;
    if (recv(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error recibiendo longitud de clave pública\n";
        return false;
    }
    len = ntohl(len);

    std::vector<char> peer_pubkey(len);
    if (recv(socket, peer_pubkey.data(), len, 0) != len) {
        std::cerr << "Error recibiendo clave pública\n";
        return false;
    }

    ctx.peer_pkey = string_to_evp_pubkey(std::string(peer_pubkey.data(), len));
    if (!ctx.peer_pkey) {
        std::cerr << "Error cargando clave pública del host\n";
        return false;
    }

    // MOSTRAR CLAVE PÚBLICA RECIBIDA DEL SERVIDOR
    std::cout << "\n--- Clave PÚBLICA RECIBIDA (del servidor) ---\n";
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, ctx.peer_pkey);
    char* pubkey_data;
    long pubkey_len = BIO_get_mem_data(bio, &pubkey_data);
    std::cout << std::string(pubkey_data, pubkey_len) << "\n";
    BIO_free(bio);

    // 2. Enviar nuestra clave pública al host
    std::string pubkey_str = evp_pubkey_to_string(ctx.pkey);
    if (pubkey_str.empty()) {
        std::cerr << "Error serializando clave pública\n";
        return false;
    }

    len = htonl(pubkey_str.size());
    if (send(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error enviando longitud de clave pública\n";
        return false;
    }
    if (send(socket, pubkey_str.c_str(), pubkey_str.size(), 0) != pubkey_str.size()) {
        std::cerr << "Error enviando clave pública\n";
        return false;
    }

    // 3. Recibir clave AES e IV cifrados
    // Recibir clave AES
    if (recv(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error recibiendo longitud de clave cifrada\n";
        return false;
    }
    len = ntohl(len);

    std::vector<unsigned char> encrypted_key(len);
    if (recv(socket, (char*)encrypted_key.data(), len, 0) != len) {
        std::cerr << "Error recibiendo clave cifrada\n";
        return false;
    }

    // Recibir IV
    if (recv(socket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
        std::cerr << "Error recibiendo longitud de IV cifrado\n";
        return false;
    }
    len = ntohl(len);

    std::vector<unsigned char> encrypted_iv(len);
    if (recv(socket, (char*)encrypted_iv.data(), len, 0) != len) {
        std::cerr << "Error recibiendo IV cifrado\n";
        return false;
    }

    // Descifrar clave AES e IV
    auto decrypted_key = rsa_decrypt(ctx.pkey, encrypted_key.data(), encrypted_key.size());
    auto decrypted_iv = rsa_decrypt(ctx.pkey, encrypted_iv.data(), encrypted_iv.size());

    if (decrypted_key.size() != sizeof(ctx.aes_key) || decrypted_iv.size() != sizeof(ctx.iv)) {
        std::cerr << "Error en tamaño de clave/IV descifrados\n";
        return false;
    }

    memcpy(ctx.aes_key, decrypted_key.data(), sizeof(ctx.aes_key));
    memcpy(ctx.iv, decrypted_iv.data(), sizeof(ctx.iv));

    // MOSTRAR CLAVE AES RECIBIDA
    std::cout << "\n--- Clave AES recibida para la comunicación ---\n";
    std::cout << "Clave AES: ";
    for (size_t i = 0; i < sizeof(ctx.aes_key); ++i) {
        printf("%02x", ctx.aes_key[i]);
    }
    std::cout << "\nIV: ";
    for (size_t i = 0; i < sizeof(ctx.iv); ++i) {
        printf("%02x", ctx.iv[i]);
    }
    std::cout << "\n";

    ctx.keys_exchanged = true;
    return true;
}
// Función para recibir mensajes en un hilo separado
void receiveMessages(SOCKET socket, CryptoContext& ctx) {
    std::vector<unsigned char> buffer(BUFFER_SIZE);

    while (true) {
        // Primero recibir la longitud del mensaje cifrado
        uint32_t encrypted_len;
        int bytesReceived = recv(socket, (char*)&encrypted_len, sizeof(encrypted_len), 0);
        if (bytesReceived <= 0) {
            std::cerr << "\nLa conexión se ha perdido.\n";
            break;
        }
        encrypted_len = ntohl(encrypted_len);

        // Recibir el mensaje cifrado
        std::vector<unsigned char> encrypted_data(encrypted_len);
        bytesReceived = recv(socket, (char*)encrypted_data.data(), encrypted_len, 0);
        if (bytesReceived <= 0) {
            std::cerr << "\nLa conexión se ha perdido.\n";
            break;
        }

        // Descifrar el mensaje
        auto decrypted = aes_decrypt(ctx.aes_key, ctx.iv, encrypted_data.data(), encrypted_data.size());
        if (decrypted.empty()) {
            std::cerr << "\nError descifrando mensaje\n";
            break;
        }

        std::string message(decrypted.begin(), decrypted.end());

        // Eliminar padding si es necesario
        size_t null_pos = message.find('\0');
        if (null_pos != std::string::npos) {
            message.resize(null_pos);
        }

        std::cout << "\nMensaje recibido: " << message << "\n> ";
    }
}



void print_rsa_keys(EVP_PKEY* pkey, const std::string& label) {
    std::cout << "\n=== " << label << " ===\n";

    // Obtener parámetros de la clave
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        std::cerr << "Error creando contexto de clave\n";
        return;
    }

    // Obtener el módulo (n)
    BIGNUM* n = nullptr;
    if (EVP_PKEY_get_bn_param(pkey, "n", &n) != 1) {
        std::cerr << "Error obteniendo módulo (n)\n";
        EVP_PKEY_CTX_free(ctx);
        return;
    }

    std::cout << "Modulo (n):\n";
    BN_print_fp(stdout, n);
    std::cout << "\n";

    // Obtener exponente público (e)
    BIGNUM* e = nullptr;
    if (EVP_PKEY_get_bn_param(pkey, "e", &e) != 1) {
        std::cerr << "Error obteniendo exponente público (e)\n";
        BN_free(n);
        EVP_PKEY_CTX_free(ctx);
        return;
    }

    std::cout << "Exponente público (e):\n";
    BN_print_fp(stdout, e);
    std::cout << "\n";

    // Intentar obtener exponente privado (d) - solo si es clave privada
    BIGNUM* d = nullptr;
    if (EVP_PKEY_get_bn_param(pkey, "d", &d) == 1) {
        std::cout << "Exponente privado (d):\n";
        BN_print_fp(stdout, d);
        std::cout << "\n";
        BN_free(d);
    }
    else {
        std::cout << "No se incluye exponente privado (es una clave pública)\n";
    }

    // Liberar recursos
    BN_free(n);
    BN_free(e);
    EVP_PKEY_CTX_free(ctx);
}















int main() {
    // Inicializar OpenSSL
    OpenSSL_add_all_algorithms();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error al inicializar Winsock\n";
        return 1;
    }

    // Configurar contexto de encriptación
    CryptoContext ctx;
    ctx.pkey = generate_rsa_key();
    ctx.peer_pkey = nullptr;
    ctx.keys_exchanged = false;

    if (!ctx.pkey) {
        std::cerr << "Error generando claves RSA\n";
        WSACleanup();
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
            EVP_PKEY_free(ctx.pkey);
            WSACleanup();
            return 1;
        }

        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(8080);
        peerAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenSocket, (sockaddr*)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR) {
            std::cerr << "Error al vincular el socket\n";
            closesocket(listenSocket);
            EVP_PKEY_free(ctx.pkey);
            WSACleanup();
            return 1;
        }

        if (listen(listenSocket, 1) == SOCKET_ERROR) {
            std::cerr << "Error al escuchar\n";
            closesocket(listenSocket);
            EVP_PKEY_free(ctx.pkey);
            WSACleanup();
            return 1;
        }

        std::cout << "Esperando conexión entrante en el puerto 8080...\n";
        int peerAddrSize = sizeof(peerAddr);
        peerSocket = accept(listenSocket, (sockaddr*)&peerAddr, &peerAddrSize);
        if (peerSocket == INVALID_SOCKET) {
            std::cerr << "Error al aceptar la conexión\n";
            closesocket(listenSocket);
            EVP_PKEY_free(ctx.pkey);
            WSACleanup();
            return 1;
        }

        closesocket(listenSocket);
        std::cout << "¡Conexión establecida!\n";

        // Realizar intercambio de claves (lado host)
        if (!perform_key_exchange_host(peerSocket, ctx)) {
            std::cerr << "Error en el intercambio de claves\n";
            closesocket(peerSocket);
            EVP_PKEY_free(ctx.pkey);
            if (ctx.peer_pkey) EVP_PKEY_free(ctx.peer_pkey);
            WSACleanup();
            return 1;
        }
    }
    else if (choice == 2) { // Modo CLIENTE (conectarse a un host)
        peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (peerSocket == INVALID_SOCKET) {
            std::cerr << "Error al crear el socket\n";
            EVP_PKEY_free(ctx.pkey);
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
            EVP_PKEY_free(ctx.pkey);
            WSACleanup();
            return 1;
        }

        std::cout << "¡Conectado al host!\n";

        // Realizar intercambio de claves (lado cliente)
        if (!perform_key_exchange_client(peerSocket, ctx)) {
            std::cerr << "Error en el intercambio de claves\n";
            closesocket(peerSocket);
            EVP_PKEY_free(ctx.pkey);
            if (ctx.peer_pkey) EVP_PKEY_free(ctx.peer_pkey);
            WSACleanup();
            return 1;
        }
    }
    else {
        std::cerr << "Opción no válida\n";
        EVP_PKEY_free(ctx.pkey);
        WSACleanup();
        return 1;
    }

    // Iniciar hilo para recibir mensajes
    std::thread receiver(receiveMessages, peerSocket, std::ref(ctx));
    receiver.detach();

    // Enviar mensajes
    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message == "exit") break;

        if (ctx.keys_exchanged) {
            // Cifrar el mensaje con AES
            auto encrypted = aes_encrypt(ctx.aes_key, ctx.iv,
                (const unsigned char*)message.c_str(), message.size());
            if (encrypted.empty()) {
                std::cerr << "Error cifrando mensaje\n";
                break;
            }

            // Enviar longitud del mensaje cifrado primero
            uint32_t len = htonl(encrypted.size());
            if (send(peerSocket, (char*)&len, sizeof(len), 0) != sizeof(len)) {
                std::cerr << "Error enviando longitud de mensaje cifrado\n";
                break;
            }

            // Luego enviar el mensaje cifrado
            if (send(peerSocket, (char*)encrypted.data(), encrypted.size(), 0) != encrypted.size()) {
                std::cerr << "Error enviando mensaje cifrado\n";
                break;
            }
        }
        else {
            // Enviar sin cifrar (no debería ocurrir)
            send(peerSocket, message.c_str(), message.size(), 0);
        }
    }

    // Limpieza
    closesocket(peerSocket);
    EVP_PKEY_free(ctx.pkey);
    if (ctx.peer_pkey) EVP_PKEY_free(ctx.peer_pkey);
    WSACleanup();
    return 0;
}