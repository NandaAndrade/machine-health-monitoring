#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // json handling
#include "mqtt/client.h" // paho mqtt
#include <iomanip>

#include "sensor.hpp"

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"

bool envioAtivo = true;

string currenteTimeISO(){
    // Get the current time in ISO 8601 format.
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%FT%TZ");
    std::string timestamp = ss.str();
    return timestamp;
}

void envioLeiturasTemperaturaBroker(Sensor *sensor, string machineId, mqtt::client *client){
    while(envioAtivo){
        // Get the current time in ISO 8601 format.
        string timestamp = currenteTimeISO();

        
        // Generate a random value.
        float temperatura = 0;
       sensor->FazerLeitura(temperatura);

        // Construct the JSON message.
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["value"] = temperatura;

        // Publish the JSON message to the appropriate topic.
        std::string topic = "/sensors/" + machineId + "/" + sensor->getIdSensor();
        mqtt::message msg(topic, j.dump(), QOS, false);
        std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl;
        client->publish(msg);

        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(sensor->getTempoLeitura()));
    }
};

void envioLeiturasPorcentagemBroker(Sensor *sensor, string machineId, mqtt::client *client){
    while(envioAtivo){
        // Get the current time in ISO 8601 format.
        string timestamp = currenteTimeISO();

        
        // Generate a random value.
        float porcentagem = 0;
       sensor->FazerLeitura(porcentagem);

        // Construct the JSON message.
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["value"] = porcentagem;

        // Publish the JSON message to the appropriate topic.
        std::string topic = "/sensors/" + machineId + "/" + sensor->getIdSensor();
        mqtt::message msg(topic, j.dump(), QOS, false);
        std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl;
        client->publish(msg);

        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(sensor->getTempoLeitura()));
    }
};

int main(int argc, char* argv[]) {
    std::string clientId = "sensor-monitor";
    mqtt::client client(BROKER_ADDRESS, clientId);

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    try {
        client.connect(connOpts);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::clog << "connected to the broker" << std::endl;

    // Get the unique machine identifier, in this case, the hostname.
    char hostname[1024];
    gethostname(hostname, 1024);
    std::string machineId(hostname);

    //Criar sensores
    Sensor *tempMaquina_01 = new Sensor("tempMaquina_01", TIPO_FLOAT, 5);
    Sensor *pcDisco_01 = new Sensor("pcDisco_01", TIPO_PORCENTAGEM, 2);
    Sensor *pcMemoria_01 = new Sensor("pcMemoria_01", TIPO_PORCENTAGEM, 3);

    Sensor *tempMaquina_02 = new Sensor("tempMaquina_02", TIPO_FLOAT, 5);
    Sensor *pcDisco_02 = new Sensor("pcDisco_02", TIPO_PORCENTAGEM, 2);
    Sensor *pcMemoria_02 = new Sensor("pcMemoria_02", TIPO_PORCENTAGEM, 3);


    // Create a thread for each sensor.
    std::thread tmp01(envioLeiturasTemperaturaBroker, tempMaquina_01, machineId, &client);
    std::thread dsc01(envioLeiturasPorcentagemBroker, pcDisco_01, machineId, &client);
    std::thread mem01(envioLeiturasPorcentagemBroker, pcMemoria_01, machineId, &client);

    std::thread tmp02(envioLeiturasTemperaturaBroker, tempMaquina_02, machineId, &client);
    std::thread dsc02(envioLeiturasPorcentagemBroker, pcDisco_02, machineId, &client);
    std::thread mem02(envioLeiturasPorcentagemBroker, pcMemoria_02, machineId, &client);

    // Wait for the threads to finish.
    tmp01.join();
    dsc01.join();
    mem01.join();

    tmp02.join();
    dsc02.join();
    mem02.join();

    // Disconnect from the MQTT broker.
    try {
        client.disconnect();
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
