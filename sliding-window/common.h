#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

// Configurações
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define WINDOW_SIZE 4       // Tamanho da Janela
#define PACKET_SIZE 1024    // Tamanho do payload
#define TIMEOUT_MS 200      // Tempo de Timeout (ms)

// Estrutura do Frame (Dados)
struct Packet {
    int seq_num;            // Número de sequência
    int size;               // Tamanho útil dos dados
    bool is_last;           // Flag de fim de arquivo
    char data[PACKET_SIZE]; // Dados do arquivo
};

// Estrutura de Confirmação
struct Ack {
    int seq_num; // Confirma recebimento ATÉ este número
};

// Estrutura para handshake inicial (nome do arquivo e modo)
struct Request {
    char filename[256];
    char mode[10]; // "UPLOAD" ou "DOWNLOAD"
};

#endif