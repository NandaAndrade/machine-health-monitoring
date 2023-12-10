#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "json.hpp" 
#include "mqtt/client.h" 

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
const std::string TOPIC("test/topic");
const std::string TOPIC2("test/topic");
const std::string CLIENT_ID("Temperatura_AltoForno");
const std::string CLIENT_ID2("Velocidade_Motor");
#define GRAPHITE_HOST "graphite" //usando o BD do graphite
#define GRAPHITE_PORT 2003 //porta do BD

void post_metric(const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {

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
    //std::string clientId = "clientId";
    mqtt::async_client client1(BROKER_ADDRESS, CLIENT_ID);//criando um cliente assincrono
    mqtt::async_client client2(BROKER_ADDRESS, CLIENT_ID2);//criando um cliente assincrono



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
    client1.set_callback(cb);
    client2.set_callback(cb);

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    try {
        mqtt::token_ptr conntok1 = client1.connect(connOpts);
         mqtt::token_ptr conntok2 = client2.connect(connOpts);
        // Esperando pela conexão
        conntok1->wait();
        conntok2->wait();
        //client.connect(connOpts);
        client1.subscribe("/sensors/", QOS);//fazendo um subscribe 
        client2.subscribe("/sensors/", QOS);//fazendo um subscribe 
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    client1.disconnect()->wait();
    client2.disconnect()->wait();


    return EXIT_SUCCESS;
}
