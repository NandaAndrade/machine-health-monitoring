#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // json handling
#include "mqtt/client.h" // paho mqtt
#include <iomanip>
#include <mutex>

#include "sensor.hpp"

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define PERIODICIDADE_MSG_CONFIG 10

bool envioAtivo = true;
int qtdeSensores = 6;
Sensor *sensors[6];

mutex mtx;

using namespace std;


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


void envioLeiturasBroker(Sensor *sensor, string machineId, mqtt::client *client){
    while(envioAtivo){
        cout<<"[envioLeiturasBroker] sensor_id: "<<sensor->getIdSensor()<<endl;

        // Get the current time in ISO 8601 format.
        string timestamp = currenteTimeISO();

        // Construct the JSON message.
        nlohmann::json j;
        j["timestamp"] = timestamp;

        // Generate a random value.
        mtx.lock();
            sensor->FazerLeitura();
            if(sensor->getTipoSensor() == "int"){
                j["value"] = sensor->getLeituraInt();
            }else if(sensor->getTipoSensor() == "float"){
                j["value"] = sensor->getLeituraFloat();
            }else{
                std::clog << "tipo do sensor incompatível parando o envio e inativando o sensor"<< std::endl;
                sensor->setStatus(false);
                return;
            }
        

            // Publish the JSON message to the appropriate topic.
            std::string topic = "/sensors/" + machineId + "/" + sensor->getIdSensor();
            mqtt::message msg(topic, j.dump(), QOS, false);
            std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl;
            client->publish(msg);

        mtx.unlock();
        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(sensor->getTempoLeitura()));
    }
};

void envioMensagemPeriodica(string machineId, mqtt::client *client){
    while(envioAtivo){
        // Construct the JSON message.
        nlohmann::json j;
        j["machine_id"] = machineId;
        j["sensors"] = nlohmann::json::array(); // Inicializando um array para os sensores
        
        mtx.lock();
        //imprime tudo de sensors
        for(int i = 0; i < qtdeSensores ; i++){
            cout<<"[envioMensagemPeriodica] sensor_id: "<<sensors[i]->getIdSensor()<<endl;
            cout<<"[envioMensagemPeriodica] sensor_tipo: "<<sensors[i]->getTipoSensor()<<endl;
            cout<<"[envioMensagemPeriodica] sensor_tempo: "<<sensors[i]->getTempoLeitura()<<endl;
        }
        mtx.unlock();
        
            for(int i = 0; i < qtdeSensores ; i++){
                mtx.lock();
                //imprime id da thread
                cout<<"[envioMensagemPeriodica] thread_id: "<<this_thread::get_id()<<endl;

                cout<<"[envioMensagemPeriodica] sensor_id: "<<sensors[i]->getIdSensor()<<endl;
                cout<<"[envioMensagemPeriodica] sensor_tipo: "<<sensors[i]->getTipoSensor()<<endl;
                cout<<"[envioMensagemPeriodica] sensor_tempo: "<<sensors[i]->getTempoLeitura()<<endl;

                string sensor_id = sensors[i]->getIdSensor();
                string sensor_tipo = sensors[i]->getTipoSensor();
                //time to string so tempo leitura
                time_t sensor_tempo = sensors[i]->getTempoLeitura();
                tm *sensor_tempo_tm = localtime(&sensor_tempo);

                std::stringstream sensor_tempo_ss;
                sensor_tempo_ss << std::put_time(sensor_tempo_tm, "%FT%TZ");
                std::string tempo_leitura_s = sensor_tempo_ss.str();

                cout<<"[envioMensagemPeriodica] sensor_id: "<<sensor_id
                    <<"  sensor_tipo: "<<sensor_tipo
                    <<"  sensor_tempo: "<<tempo_leitura_s
                    <<endl;

                if(sensors[i]->getStatus() == true){
                    nlohmann::json sensor;
                    sensor["sensor_id"] = sensor_id;
                    sensor["data_type"] = sensor_tipo;
                    sensor["data_interval"] = tempo_leitura_s;
                    
                    // Adicionando o sensor ao array de sensores
                    j["sensors"].push_back(sensor);
                    cout<<"[envioMensagemPeriodica] "<< j.dump()<<endl;
                }
                mtx.unlock();
            }
        
        mtx.lock();
            // Publish the JSON message to the appropriate topic.
            std::string topic = "/sensor_monitors";

            mqtt::message msg(topic, j.dump(), QOS, false);
            std::clog << "message published - topic: " << topic << " - message: " << j.dump() << std::endl;
            client->publish(msg);
        mtx.unlock();
        // Sleep for some time.
        std::this_thread::sleep_for(std::chrono::seconds(PERIODICIDADE_MSG_CONFIG));
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
    sensors[0] = new Sensor("tempMaquina_01", TIPO_FLOAT, 5);
    sensors[1] = new Sensor("pcDisco_01", TIPO_INT, 2);
    sensors[2] = new Sensor("pcMemoria_01", TIPO_INT, 3);

    sensors[3] = new Sensor("tempMaquina_02", TIPO_FLOAT, 5);
    sensors[4] = new Sensor("pcDisco_02", TIPO_INT, 2);
    sensors[5] = new Sensor("pcMemoria_02", TIPO_INT, 3);

    vector<thread> threads;

    // Create a thread for each sensor.
    for(int i=0; i < qtdeSensores; i++){
        threads.emplace_back(envioLeiturasBroker, sensors[i], machineId, &client);
    }

    // Create a thread for periodic messages.
    thread t(envioMensagemPeriodica, machineId, &client);

    for(int i=0; i < qtdeSensores; i++){
        threads[i].join();
    }

    t.join();

    // Disconnect from the MQTT broker.
    try {
        client.disconnect();
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
