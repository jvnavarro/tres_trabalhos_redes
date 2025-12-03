#include "../common.h"
// Inclua o mesmo bloco de funções send_file/receive_file/ack_listener aqui

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Uso: ./client <nome_arquivo> <UPLOAD|DOWNLOAD>" << endl;
        return 1;
    }

    string filename = argv[1];
    string mode = argv[2];

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Envia requisição inicial
    Request req;
    strncpy(req.filename, filename.c_str(), 255);
    strncpy(req.mode, mode.c_str(), 9);

    cout << "Enviando requisicao..." << endl;
    sendto(sockfd, &req, sizeof(req), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if (mode == "UPLOAD") {
        // Cliente envia arquivo
        send_file(sockfd, server_addr, filename);
    } else {
        // Cliente recebe arquivo
        receive_file(sockfd, "client_" + filename);
    }

    close(sockfd);
    return 0;
}