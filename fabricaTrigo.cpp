#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <iostream>
#include <string.h>

using namespace std;

struct Molino {
    pthread_t thread;
    int id;
    float harinaPorMin;
    float harinaProducida;
    float trigo;
    float agua;
    bool necesitaAgua;
    bool necesitaTrigo;  
};

struct Silo {
    pthread_t thread;
    int id;
    float trigo;
};

struct Dispensadora {
    float agua;
};

struct Empacadora {
    pthread_t thread;
    int id;
    int costalesAProducir; 
    int costalesProducidos; 
};

Molino molinos[3];
Silo silos[3];
Dispensadora dispensadora;
Empacadora empacadoras[3];

pthread_cond_t cond_agua;
pthread_cond_t cond_trigo;
pthread_cond_t cond_empacado;
pthread_mutex_t mutex_agua;
pthread_mutex_t mutex_trigo;
pthread_mutex_t mutex_empacadora;
pthread_mutex_t mutex_out;
pthread_t thrd[3];

void* molerHarina(void* args) {
    Molino *m = (Molino*) args;
    int empacadoraIndex = m->id - 1;

    while (empacadoras[empacadoraIndex].costalesProducidos < empacadoras[empacadoraIndex].costalesAProducir) {
        float usoAgua = (m->harinaPorMin) / 20;  

        pthread_mutex_lock(&mutex_agua);

        while (m->agua < usoAgua) {
            pthread_mutex_lock(&mutex_out);
            cout << "Molino " << m->id << " está esperando agua..." << endl;
            pthread_mutex_unlock(&mutex_out);
            m->necesitaAgua = true;
            pthread_cond_wait(&cond_agua, &mutex_agua); 
        }
        
        while (m->trigo <= 50) {
            pthread_mutex_lock(&mutex_trigo);
            m->necesitaTrigo = true; 
            pthread_cond_signal(&cond_trigo); 
            pthread_mutex_unlock(&mutex_trigo);
        }

        Sleep(1000);
        m->agua -= usoAgua;
        m->trigo -= m->harinaPorMin;
        m->harinaProducida += m->harinaPorMin;

        pthread_mutex_lock(&mutex_out);
        cout << "Molino " << m->id << " ha producido " << m->harinaPorMin << " lbs de harina, agua restante: " << m->agua << ", trigo restante: " << m->trigo << endl;
        pthread_mutex_unlock(&mutex_out);

        pthread_mutex_unlock(&mutex_agua);

        if (m->harinaProducida >= 220) {
            pthread_mutex_lock(&mutex_empacadora);
            empacadoras[empacadoraIndex].costalesProducidos++;
            m->harinaProducida -= 220; 
            pthread_mutex_lock(&mutex_out);
            cout << "Empacadora " << m->id << " ha guardado 1 costal de harina. Costales producidos: " << empacadoras[empacadoraIndex].costalesProducidos << "/" << empacadoras[empacadoraIndex].costalesAProducir << endl;
            pthread_mutex_unlock(&mutex_out);
            pthread_mutex_unlock(&mutex_empacadora);
        }
    }

    pthread_mutex_lock(&mutex_out);
    cout << "Molino " << m->id << " ha completado la producción de trigo " << endl;
    pthread_mutex_unlock(&mutex_out);
    pthread_exit(NULL);
    return 0;
}

void* entregarTrigo(void* args) {
    Silo* s = (Silo*) args;
    int index = s->id-1;

    while (empacadoras[index].costalesProducidos < empacadoras[index].costalesAProducir) {
        pthread_mutex_lock(&mutex_trigo);

        if (molinos[index].necesitaTrigo) {
            molinos[index].trigo += s->trigo;
            molinos[index].necesitaTrigo = false;
            pthread_mutex_lock(&mutex_out);
            cout << "Silo " << s->id << " entrega 1 quintal de trigo al molino " << molinos[index].id << "." << endl;
            pthread_mutex_unlock(&mutex_out);
            
            pthread_cond_signal(&cond_trigo);
        }

        pthread_mutex_unlock(&mutex_trigo);        
    }
    pthread_exit(NULL);
    return 0;
}

bool empacadoFinalizado() {
    int empacadorasFinalizadas = 0;

    for (int i = 0; i < 3; i++) {
        if (empacadoras[i].costalesAProducir == empacadoras[i].costalesProducidos) {
            empacadorasFinalizadas++;
        }
    }

    return empacadorasFinalizadas == 3;
}

void* dispensarAgua(void* args) {
    Dispensadora* d = (Dispensadora*) args;

    while (!empacadoFinalizado() && d->agua > 0) {
        pthread_mutex_lock(&mutex_agua);

        for (int i = 0; i < 3; i++) {
            if (molinos[i].necesitaAgua && d->agua > 0) {
                molinos[i].agua += 1;
                d->agua -= 1;
                molinos[i].necesitaAgua = false;
                pthread_mutex_lock(&mutex_out);
                cout << "Molino " << molinos[i].id << " recibe 1 litro de agua. Agua restante en dispensadora: " << d->agua << " litros." << endl;
                pthread_mutex_unlock(&mutex_out);
                
                pthread_cond_signal(&cond_agua);
            }
        }

        pthread_mutex_unlock(&mutex_agua);
    }

    pthread_mutex_lock(&mutex_out);
    if (d->agua <= 0) {
        cout << "La dispensadora se quedó sin agua." << endl;
    } else {
        cout << "La dispensadora ha detenido su operación porque el empacado ha finalizado." << endl;
    }
    pthread_mutex_unlock(&mutex_out);
    pthread_exit(NULL);
    return 0;
}

void* empacarHarina(void* args) {
    Empacadora* e = (Empacadora*) args;
    int molinoIndex = e->id - 1;

    while (e->costalesProducidos < e->costalesAProducir) {
        pthread_mutex_lock(&mutex_empacadora);

        while (molinos[molinoIndex].harinaProducida < 220) {
            pthread_cond_wait(&cond_empacado, &mutex_empacadora);
        }

        Sleep(2000);
        molinos[molinoIndex].harinaProducida -= 220;
        e->costalesProducidos++;
        pthread_mutex_lock(&mutex_out);
        cout << "Empacadora " << e->id << " ha producido 1 costal de harina. Total costales: " << e->costalesProducidos << "/" << e->costalesAProducir << endl;
        pthread_mutex_unlock(&mutex_out);

        pthread_mutex_unlock(&mutex_empacadora);
    }

    pthread_exit(NULL);
    return 0;
}

int main () {
    dispensadora.agua = 10000;

    pthread_mutex_init(&mutex_agua, NULL);
    pthread_mutex_init(&mutex_trigo, NULL);
    pthread_mutex_init(&mutex_empacadora, NULL);
    pthread_mutex_init(&mutex_out, NULL);
    pthread_cond_init(&cond_agua, NULL);
    pthread_cond_init(&cond_trigo, NULL);
    pthread_cond_init(&cond_empacado, NULL);

    molinos[0].harinaPorMin = 18.33;
    molinos[1].harinaPorMin = 27.5;
    molinos[2].harinaPorMin = 14.66;

    pthread_t dispensadora_thread;
    pthread_create(&dispensadora_thread, NULL, dispensarAgua, (void*)&dispensadora);

    for(int i = 0; i < 3; i++) {
        molinos[i].id = i+1;
        silos[i].id = i+1;
        silos[i].trigo = 220;
        empacadoras[i].costalesAProducir = 3;


        pthread_create(&molinos[i].thread, NULL, molerHarina, (void*)&molinos[i]);
        pthread_create(&silos[i].thread, NULL, entregarTrigo, (void*)&silos[i]);
        pthread_create(&empacadoras[i].thread, NULL, empacarHarina, (void*)&empacadoras[i]);
    }

    for(int i = 0; i < 3; i++) {
        pthread_join(molinos[i].thread, NULL);
        pthread_join(silos[i].thread, NULL);
        pthread_join(empacadoras[i].thread, NULL);
    }
    pthread_join(dispensadora_thread, NULL);
    
    return 0;
}