#include "../common.h"
// Inclua o bloco de funções acima aqui ou copie e cole as funções send_file/receive_file/ack_listener aqui

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha no Bind"); return 1;
    }

    cout << "Servidor rodando na porta " << SERVER_PORT << endl;

    while (true) {
        Request req;
        socklen_t len = sizeof(client_addr);
        
        // Aguarda pedido do cliente
        cout << "Aguardando cliente..." << endl;
        recvfrom(sockfd, &req, sizeof(req), 0, (struct sockaddr*)&client_addr, &len);

        cout << "Cliente pediu: " << req.mode << " Arquivo: " << req.filename << endl;

        if (strcmp(req.mode, "UPLOAD") == 0) {
            // Se cliente quer fazer upload, servidor recebe
            receive_file(sockfd, string("server_") + req.filename);
        } else {
            // Se cliente quer download, servidor envia
            send_file(sockfd, client_addr, req.filename);
        }
    }
    return 0;
}