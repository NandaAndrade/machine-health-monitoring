#ifndef SENSOR_HPP
#define SENSOR_HPP

#include<iostream>
#include<string.h>
#include<random>

#define TIPO_INT 1
#define TIPO_FLOAT 2

using namespace std;

class Sensor{
    private:
        string id_sensor;
        int tipo_sensor;
        time_t tempo_leitura;
        float leitura_float;
        int leitura_int;
        bool status;

    public:
        Sensor(string id_sensor, int tipo_sensor, time_t tempo_leitura)
            : id_sensor(id_sensor), tipo_sensor(tipo_sensor), tempo_leitura(tempo_leitura){
                status = true;
            };

        ~Sensor(){};
        
        string getIdSensor(){ return id_sensor; };

        string getTipoSensor(){
            if(tipo_sensor = 1) return "int";
            else if(tipo_sensor = 2) return "float";
            else return "erro";
        }

        time_t getTempoLeitura(){ return tempo_leitura; };

        int getLeituraInt(){ return leitura_int; };

        float getLeituraFloat(){ return leitura_float; };

        bool getStatus(){ return status; };

        void setStatus(bool status){ this->status = status; };

        void FazerLeitura(){
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<int> dis_int(0, 100);
            leitura_int = dis_int(gen);

            uniform_real_distribution<float> dis_float(0, 100);
            leitura_float = dis_float(gen);
        };

};

#endif