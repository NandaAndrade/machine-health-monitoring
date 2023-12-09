#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // json handling
#include "mqtt/client.h" // paho mqtt
#include <iomanip>

#define QOS 1   //nivel de qualidade na entrega de msgs sendo que pelo menos 1 msg sera entregue
#define BROKER_ADDRESS "tcp://localhost:1883" //adress do broker

int main(int argc, char* argv[]) {
    std::string clientId = "sensor-monitor";
    mqtt::client client(BROKER_ADDRESS, clientId); //criando o cliente mqtt

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20); //Se não houver atividade dentro desse intervalo, o cliente enviará um "ping" ao broker para mantê-la viva
    connOpts.set_clean_session(true); //Na próxima conexão, ele será tratado como um cliente completamente novo, sem histórico das operações anteriores.

    try {
        client.connect(connOpts); //caso de erro gera uma mensagem  
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::clog << "connected to the broker" << std::endl; //estabeleceu a conexao no broker

    // pegando o identificador unico da maquina local.
    char hostname[1024];
    gethostname(hostname, 1024);
    std::string machineId(hostname);

    while (true) {
       //Pega o formato ISO-8601 para o timestamp dos sensores.Exemplo: 2023-06-01T15:30:00Z.
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        std::stringstream ss;
        ss << std::put_time(now_tm, "%FT%TZ");
        std::string timestamp = ss.str();

        // Gerando valores randomicos para medições.
        int value = rand();

        // Construct the JSON message.
        //abaixo temos o exemplo da estrutura da mensagem que seria enviada pelos sensores
        /* {
   "machine_id": "id_da_maquina",
    "sensors": [
        {
            "sensor_id": "id_do_sensor",
            "data_type": "tipo_do_dado",
            "data_interval": periodicidade
        },
        ...
    ]
    }
    */
        nlohmann::json j; //biblio oferece suporte para análise (parsing)
        j["timestamp"] = timestamp;
        j["value"] = value;

        // Publicando a mensagem JSON no topico apropriado.
        std::string topic = "/sensors/" + machineId; //topico destinado 
        mqtt::message msg(topic, j.dump(), QOS, false);//encapsulo a msg que e publicada no topic sensor_monitors, j e a mensagem json e serializa esse objeto para uma string, que será a mensagem a ser publicada, QoS é o nivel de qualidade da msg e false é false nos diz que a info nao fica retida no broker
        std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl; // nos mostra a mensagem publicada
        client.publish(msg);//publica a msg no client

        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}
