#include "../common.h"
// Inclua o bloco de funções acima aqui ou copie e cole as funções send_file/receive_file/ack_listener aqui

// --- Variáveis Globais de Controle de Fluxo ---
int base = 0;           // Base da janela
int next_seq_num = 0;   // Próximo a enviar
mutex mtx;              // Para proteger acesso à 'base'

// Thread que fica escutando ACKs (Req 3 e 4)
void ack_listener(int sockfd, bool &running) {
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    Ack ack_pkt;

    while (running) {
        // Configura timeout no socket para esta thread não travar pra sempre
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 100000; // 100ms
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr*)&sender_addr, &addr_len);
        
        if (n > 0) {
            lock_guard<mutex> lock(mtx);
            // ACK Cumulativo: Se receber ACK 5, confirma tudo até o 5
            if (ack_pkt.seq_num >= base) {
                // Req 4: Elimina da tabela (move a base)
                base = ack_pkt.seq_num + 1;
                // cout << "[ACK] Recebido: " << ack_pkt.seq_num << " | Nova Base: " << base << endl;
            }
        }
    }
}

// Função para ENVIAR arquivo (Sender)
void send_file(int sockfd, struct sockaddr_in addr, string filepath) {
    FILE *f = fopen(filepath.c_str(), "rb");
    if (!f) { cout << "Erro ao abrir arquivo!" << endl; return; }

    // Carregar arquivo na memória (Tabela de transmissão - Req 1)
    vector<Packet> packets;
    int seq = 0;
    while (true) {
        Packet p;
        p.seq_num = seq++;
        p.size = fread(p.data, 1, PACKET_SIZE, f);
        p.is_last = (p.size < PACKET_SIZE); // Se leu menos que o total, é o último
        packets.push_back(p);
        if (p.is_last) break;
    }
    fclose(f);

    cout << "Iniciando envio de " << packets.size() << " frames..." << endl;

    bool running = true;
    base = 0;
    next_seq_num = 0;

    // Inicia thread paralela para ouvir ACKs
    thread ack_thread(ack_listener, sockfd, ref(running));

    socklen_t addr_len = sizeof(addr);
    auto last_time = chrono::steady_clock::now();

    // Loop Principal de Envio
    while (base < packets.size()) {
        
        // Req 2: Transmitir frames dentro da janela permitida
        mtx.lock();
        while (next_seq_num < base + WINDOW_SIZE && next_seq_num < packets.size()) {
            sendto(sockfd, &packets[next_seq_num], sizeof(Packet), 0, (struct sockaddr*)&addr, addr_len);
            // cout << "[Envio] Frame " << next_seq_num << endl;
            next_seq_num++;
        }
        mtx.unlock();

        // Req 5, 6 e 7: Verificação de Timeout
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - last_time).count() > TIMEOUT_MS) {
            mtx.lock();
            if (base < next_seq_num) {
                cout << "[Timeout] Retransmitindo a partir de " << base << endl;
                next_seq_num = base; // Retransmite (Go-Back-N)
            }
            mtx.unlock();
            last_time = now; // Reseta timer
        } else {
            // Se avançou a base (recebeu ack), reseta timer
            // Nota: Lógica simplificada. Idealmente timer é por pacote, aqui é pela janela.
            if (base == next_seq_num) last_time = now;
        }
        
        this_thread::sleep_for(chrono::milliseconds(1)); // Evita uso 100% CPU
    }

    running = false; // Para thread de ACK
    ack_thread.join();
    cout << "Envio Concluido!" << endl;
}

// Função para RECEBER arquivo (Receiver)
void receive_file(int sockfd, string filepath) {
    FILE *f = fopen(filepath.c_str(), "wb");
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    
    int expected_seq = 0;
    Packet pkt;
    Ack ack;

    cout << "Aguardando dados..." << endl;

    while (true) {
        // Reseta timeout para leitura normal
        struct timeval tv; tv.tv_sec = 10; tv.tv_usec = 0; 
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&sender_addr, &addr_len);
        
        if (n < 0) break; // Erro ou timeout longo

        // Req 3: Verifica se é o frame correto
        if (pkt.seq_num == expected_seq) {
            // cout << "[Recv] Frame " << pkt.seq_num << " aceito." << endl;
            fwrite(pkt.data, 1, pkt.size, f);
            
            ack.seq_num = expected_seq;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*)&sender_addr, addr_len);
            
            expected_seq++;
            if (pkt.is_last) break;
        } else {
            // Se vier fora de ordem, reenvia ACK do ultimo sucesso
            // cout << "[Recv] Frame ignorado. Esperado: " << expected_seq << endl;
            ack.seq_num = expected_seq - 1;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*)&sender_addr, addr_len);
        }
    }
    fclose(f);
    cout << "Recebimento Concluido!" << endl;
}



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