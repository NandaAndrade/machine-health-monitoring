#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp" 
#include "mqtt/client.h" 

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define GRAPHITE_HOST "graphite" //usando o BD do graphite
#define GRAPHITE_PORT 2003 //porta do BD

using namespace std;

void post_metric(const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {
    cout<<"[post_metric] machine_id: "<<machine_id<<endl;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    cout<<"[post_metric] criado socket"<<endl;
    cout<<"[post_metric] GRAPHITE_HOST: "<<GRAPHITE_HOST<<endl;

    // Configuração do endereço do servidor Graphite
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(GRAPHITE_PORT);
    inet_pton(AF_INET, GRAPHITE_HOST, &serverAddress.sin_addr);

    cout<<"[post_metric] configurado endereço do servidor"<<endl;

    // Conexão ao servidor Graphite
    if (connect(client_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Erro ao conectar ao servidor Graphite." << std::endl;
        close(client_socket);
        return;
    }

    cout<<"[post_metric] conectado ao servidor"<<endl;
    
    // Montagem da mensagem
    //<metric path> <metric value> <metric timestamp>
    string metric_path = machine_id + "." + sensor_id;
    string metric_value = to_string(value);
    string metric_timestamp = timestamp_str;
    
    string metric = metric_path + " " + metric_value + " " + metric_timestamp + "\n";
    std::cout << "metric: " << metric << std::endl;

    // Envio da mensagem
    if (send(client_socket, metric.c_str(), metric.size(), 0) == -1) {
        std::cerr << "Erro ao enviar mensagem para o servidor Graphite." << std::endl;
        close(client_socket);
        return;
    }

    // Recebimento da resposta
    char response[1024];
    if (recv(client_socket, &response, sizeof(response), 0) == -1) {
        std::cerr << "Erro ao receber resposta do servidor Graphite." << std::endl;
        close(client_socket);
        return;
    }

    // Fechamento do socket
    close(client_socket);

    // Impressão da resposta
    std::cout << "response: " << response << std::endl;

}

std::vector<std::string> split(const std::string &str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    std::string clientId = "clientId";
    mqtt::async_client client(BROKER_ADDRESS, clientId);

    // Create an MQTT callback.
    class callback : public virtual mqtt::callback {
    public:

        void message_arrived(mqtt::const_message_ptr msg) override {
            auto j = nlohmann::json::parse(msg->get_payload());

            std::string topic = msg->get_topic();
            auto topic_parts = split(topic, '/');
            std::string machine_id = topic_parts[2];
            std::string sensor_id = topic_parts[3];

            std::string timestamp = j["timestamp"];
            int value = j["value"];
            post_metric(machine_id, sensor_id, timestamp, value);
        }
    };

    callback cb;
    client.set_callback(cb);

    // Connect to the MQTT broker.
    // mqtt::connect_options connOpts;
    // connOpts.set_keep_alive_interval(20);
    // connOpts.set_clean_session(true);


    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    connOpts.set_keep_alive_interval(11);

    mqtt::token_ptr conntok = client.connect(connOpts);
    conntok->wait();

    try {
        client.disconnect()->wait();
        cout << "Connecting to the MQTT broker..." << endl;
        //client.connect(connOpts)->wait_for(10);
        mqtt::token_ptr conntok = client.connect(connOpts);
        conntok->wait();
        cout<<"[main] conectado ao broker"<<endl;
        client.subscribe("/sensors/#", QOS);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    client.disconnect()->wait();
    return EXIT_SUCCESS;
}
