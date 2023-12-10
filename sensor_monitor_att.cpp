#include <iostream>
#include <cstdlib>
#include <chrono>
#include <random>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // json handling
#include "mqtt/client.h" // paho mqtt
#include "mqtt/async_client.h"
#include <iomanip>

#define QOS 1   //nivel de qualidade na entrega de msgs sendo que pelo menos 1 msg sera entregue
#define BROKER_ADDRESS "tcp://localhost:1883" //adress do broker
const std::string CLIENT_ID("Temperatura_AltoForno");
const std::string CLIENT_ID2("Velocidade_Motor");

using namespace std;

int main(int argc, char* argv[]) {
    //std::string clientId = "sensor";
    //mqtt::client client(BROKER_ADDRESS, clientId); //criando o cliente mqtt
    mqtt::async_client client1(BROKER_ADDRESS, CLIENT_ID);
    mqtt::async_client client2(BROKER_ADDRESS, CLIENT_ID2);


    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20); //Se não houver atividade dentro desse intervalo, o cliente enviará um "ping" ao broker para mantê-la viva
    connOpts.set_clean_session(true); //Na próxima conexão, ele será tratado como um cliente completamente novo, sem histórico das operações anteriores.
   
    try {
        mqtt::token_ptr conntok1 = client1.connect(connOpts);
        mqtt::token_ptr conntok2 = client2.connect(connOpts);

        // Esperando pela conexão
        conntok1->wait();
        conntok2->wait();
        //client.connect(connOpts); //caso de erro gera uma mensagem  
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
        // Construct the JSON message.
        nlohmann::json x;
        //abaixo temos o exemplo da estrutura da mensagem que seria enviada pelos sensores
       /* {
        "machine_id": "id_da_maquina",
            "sensors": [
            {
                "sensor_id": "/sensors/" + machineId + "/temp_forno",
                "data_type": "float",
                "data_interval": "periodicidade"
                },
                {
                "sensor_id": "/sensors/" + machineId + "/vel_motor",
                "data_type": "int",
                "data_interval": "periodicidade"
                },
            ]
        }
    */

       //Pega o formato ISO-8601 para o timestamp dos sensores.Exemplo: 2023-06-01T15:30:00Z.
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        std::stringstream ss;
        ss << std::put_time(now_tm, "%FT%TZ");
        std::string timestamp = ss.str();

        // Gerando valores randomicos para medições.
        float value = rand();
        int value2 = rand();

        
        nlohmann::json j; //biblio oferece suporte para análise (parsing)
        j["timestamp"] = timestamp;
        j["value"] = value;

        nlohmann::json k; //biblio oferece suporte para análise (parsing)
        k["timestamp"] = timestamp;
        k["value"] = value2;

        // Publicando a mensagem JSON no topico apropriado.
        //std::string topic1 = "/sensors/" + machineId + "/temp_forno"; //topico destinado 
        //mqtt::message msg1(topic1, j.dump(), QOS, false);//encapsulo a msg que e publicada no topic sensor_monitors, j e a mensagem json e serializa esse objeto para uma string, que será a mensagem a ser publicada, QoS é o nivel de qualidade da msg e false é false nos diz que a info nao fica retida no broker
        
        //client1.publish(msg1)->wait_for(std::chrono::seconds(10));//publica a msg no client
        
        // Criando e publicando a mensagem JSON no tópico apropriado.
        std::string topic1 = "/sensors/" + machineId + "/temp_forno";
        mqtt::message_ptr pubmsg1 = mqtt::make_message(topic1, j.dump());
        pubmsg1->set_qos(QOS);
        pubmsg1->set_retained(false);

        //std::clog << "message published - topic: " << topic1 << " - message: " << j.dump() << std::endl; // nos mostra a mensagem publicada

        std::string topic2 = "/sensors/" + machineId + "/vel_motor";
        mqtt::message_ptr pubmsg2 = mqtt::make_message(topic2, k.dump());
        pubmsg2->set_qos(QOS);
        pubmsg2->set_retained(false);

        // Publicando as mensagens utilizando os objetos pubmsg1 e pubmsg2
        client1.publish(pubmsg1)->wait_for(chrono::seconds(5));
        client2.publish(pubmsg2)->wait_for(chrono::seconds(5));

        
        // Publicando a mensagem JSON no topico apropriado.
        //std::string topic2 = "/sensors/" + machineId + "/vel_motor"; //topico destinado 
        //mqtt::message msg2(topic2, k.dump(), QOS, false);//encapsulo a msg que e publicada no topic sensor_monitors, j e a mensagem json e serializa esse objeto para uma string, que será a mensagem a ser publicada, QoS é o nivel de qualidade da msg e false é false nos diz que a info nao fica retida no broker
       // std::clog << "message published - topic: " << topic2 << " - message: " << k.dump() << std::endl; // nos mostra a mensagem publicada
        //client2.publish(msg2);//publica a msg no client

        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
    }
    client1.disconnect()->wait();
    client2.disconnect()->wait();
    return EXIT_SUCCESS;
}
