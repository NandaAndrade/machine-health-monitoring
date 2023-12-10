#ifndef SENSOR_HPP
#define SENSOR_HPP

#include<iostream>
#include<string.h>

#define TIPO_INT 1
#define TIPO_TEMPO 3
#define TIPO_PORCENTAGEM 4
#define TIPO_FLOAT 5

using namespace std;

class Sensor{
    private:
        string id_sensor;
        int tipo_sensor;
        time_t tempo_leitura;

    public:
        Sensor(string id_sensor, int tipo_sensor, time_t tempo_leitura) : id_sensor(id_sensor), tipo_sensor(tipo_sensor), tempo_leitura(tempo_leitura){};
        ~Sensor(){};
        
        string getIdSensor(){ return id_sensor; };
        time_t getTempoLeitura(){ return tempo_leitura; };

        template <typename T>
        void FazerLeitura(T leitura){

            switch (tipo_sensor)
            {
            case TIPO_INT:
                leitura = rand();
                break;
            case TIPO_TEMPO:
                leitura = rand() % 60;
                break;
            case TIPO_PORCENTAGEM:
                leitura = rand() % 101 /100.0f;
                break;
            case TIPO_FLOAT:
                leitura = rand() % 200;
                break;
            default:
                clog<<"Tipo de sensor invalido"<<endl;
                break;
            }
        };

};

#endif