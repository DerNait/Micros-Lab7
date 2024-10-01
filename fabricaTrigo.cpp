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
    string nombre;
    float harinaPorMin;
    float harinaProducida;
    float trigo;
    float agua;
    bool necesitaAgua;
    bool necesitaTrigo;  
    bool produccionTerminada = false;

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
    string nombre;
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
pthread_cond_t cond_empacado[3];
pthread_mutex_t mutex_agua;
pthread_mutex_t mutex_trigo;
pthread_mutex_t mutex_empacadora[3];
pthread_mutex_t mutex_out;
pthread_t thrd[3];

void* molerHarina(void* args) {
    Molino *m = (Molino*) args;
    int empacadoraIndex = m->id - 1;

    while (empacadoras[empacadoraIndex].costalesProducidos < empacadoras[empacadoraIndex].costalesAProducir) {
        float usoAgua = (m->harinaPorMin) / 20;  

        pthread_mutex_lock(&mutex_agua);
        while (m->agua < usoAgua) {
            m->necesitaAgua = true; 
            pthread_cond_wait(&cond_agua, &mutex_agua);
        }
        pthread_mutex_unlock(&mutex_agua);
        
        pthread_mutex_lock(&mutex_trigo);
        while (m->trigo <= 50) { 
            pthread_cond_wait(&cond_trigo, &mutex_trigo);
        }
        pthread_mutex_unlock(&mutex_trigo);

        Sleep(1000);
        m->agua -= usoAgua;
        m->trigo -= m->harinaPorMin;

        pthread_mutex_lock(&mutex_empacadora[empacadoraIndex]);
        m->harinaProducida += m->harinaPorMin;

        bool hasEnoughHarina = (m->harinaProducida >= 220);
        if (hasEnoughHarina) {
            pthread_cond_signal(&cond_empacado[empacadoraIndex]);
        }
        pthread_mutex_unlock(&mutex_empacadora[empacadoraIndex]);

        pthread_mutex_lock(&mutex_out);
        cout << "Molino " << m->nombre << " ha producido " << m->harinaPorMin << " lbs de harina, agua restante: " << m->agua << ", trigo restante: " << m->trigo << endl;
        pthread_mutex_unlock(&mutex_out);

    }

    pthread_mutex_lock(&mutex_empacadora[empacadoraIndex]);
    m->produccionTerminada = true;
    pthread_cond_signal(&cond_empacado[empacadoraIndex]);
    pthread_mutex_unlock(&mutex_empacadora[empacadoraIndex]);

    pthread_mutex_lock(&mutex_out);
    cout << "Molino " << m->nombre << " ha completado la producción de trigo " << endl;
    pthread_mutex_unlock(&mutex_out);
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

void* entregarTrigo(void* args) {
    Silo* s = (Silo*) args;
    int index = s->id - 1;

    while (!empacadoFinalizado())
    {
        pthread_mutex_lock(&mutex_trigo);
        if (molinos[index].trigo <= 50) {
            molinos[index].trigo += s->trigo;
            pthread_mutex_lock(&mutex_out);
            cout << "Silo " << s->id << " entrega 1 quintal de trigo al molino " << molinos[index].nombre << "." << endl;
            pthread_mutex_unlock(&mutex_out);

            pthread_cond_signal(&cond_trigo);
        }
        pthread_mutex_unlock(&mutex_trigo);
    }
    pthread_exit(NULL);
    return 0;
}

void* dispensarAgua(void* args) {
    Dispensadora* d = (Dispensadora*) args;

    while(!empacadoFinalizado() && d->agua > 0) {
        pthread_mutex_lock(&mutex_agua);
        for (int i = 0; i < 3; i++) {
            if (molinos[i].necesitaAgua && d->agua > 0) {  // Si algún molino necesita agua
                molinos[i].agua += 1;
                molinos[i].necesitaAgua = false;
                d->agua -= 1;
                pthread_mutex_lock(&mutex_out);
                cout << "Molino " << molinos[i].nombre << " recibe 1 litro de agua. Agua restante en dispensadora: " << d->agua << " litros." << endl;
                pthread_mutex_unlock(&mutex_out);

                pthread_cond_broadcast(&cond_agua);
            }
        }
        pthread_mutex_unlock(&mutex_agua);
    }
    
    pthread_mutex_lock(&mutex_out);
    if (d->agua <= 0) {
        cout << "La dispensadora se quedó sin agua." << endl;
    } else {
        cout << "La dispensadora ha terminado su operación." << endl;
    }
    pthread_mutex_unlock(&mutex_out);

    pthread_exit(NULL);
    return 0;
}


void* empacarHarina(void* args) {
    Empacadora* e = (Empacadora*) args;
    int molinoIndex = e->id - 1;
    Molino *m = &molinos[molinoIndex];

    while (e->costalesProducidos < e->costalesAProducir) {
        pthread_mutex_lock(&mutex_empacadora[molinoIndex]);

        while (m->harinaProducida < 220 && !m->produccionTerminada) {
            pthread_cond_wait(&cond_empacado[molinoIndex], &mutex_empacadora[molinoIndex]);
        }

        bool shouldBreak = false;

        if (m->harinaProducida >= 220) {
            Sleep(2000);
            m->harinaProducida -= 220;
            e->costalesProducidos++;
            pthread_mutex_lock(&mutex_out);
            cout << "Empacadora " << e->nombre << " ha producido 1 costal de harina. Total costales: "
                 << e->costalesProducidos << "/" << e->costalesAProducir << endl;
            pthread_mutex_unlock(&mutex_out);
        } else if (m->produccionTerminada && m->harinaProducida < 220) {
            shouldBreak = true;
        }

        pthread_mutex_unlock(&mutex_empacadora[molinoIndex]);

        if (shouldBreak) {
            break;
        }
    }

    pthread_exit(NULL);
    return 0;
}


int main () {
    dispensadora.agua = 10000;
    int costalesAProducir;

    cout << "Ingresa la cantidad de costales que quieres que cada una de las empacadoras produzca: " << endl;
    cin >> costalesAProducir;
    cout << "Comendando operación... " << endl << endl;

    pthread_mutex_init(&mutex_agua, NULL);
    pthread_mutex_init(&mutex_trigo, NULL);
    pthread_mutex_init(&mutex_out, NULL);
    pthread_cond_init(&cond_agua, NULL);
    pthread_cond_init(&cond_trigo, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&mutex_empacadora[i], NULL);
        pthread_cond_init(&cond_empacado[i], NULL);
    }

    molinos[0].harinaPorMin = 18.33;
    molinos[0].nombre = "A";

    molinos[1].harinaPorMin = 27.5;
    molinos[1].nombre = "B";

    molinos[2].harinaPorMin = 14.66;
    molinos[2].nombre = "C";

    empacadoras[0].nombre = "harina soft";
    empacadoras[1].nombre = "harina HDW";
    empacadoras[2].nombre = "harina integral";

    for(int i = 0; i < 3; i++) {
        // Inicializar molinos
        molinos[i].id = i+1;
        molinos[i].harinaProducida = 0;
        molinos[i].trigo = 0;
        molinos[i].agua = 0;
        molinos[i].necesitaAgua = false;
        molinos[i].necesitaTrigo = false;
        molinos[i].produccionTerminada = false;

        // Inicializar silos
        silos[i].id = i+1;
        silos[i].trigo = 220;

        // Inicializar empacadoras
        empacadoras[i].id = i+1;
        empacadoras[i].costalesAProducir = costalesAProducir;
        empacadoras[i].costalesProducidos = 0;

        // Crear hilos
        pthread_create(&molinos[i].thread, NULL, molerHarina, (void*)&molinos[i]);
        pthread_create(&silos[i].thread, NULL, entregarTrigo, (void*)&silos[i]);
        pthread_create(&empacadoras[i].thread, NULL, empacarHarina, (void*)&empacadoras[i]);
    }

    pthread_t dispensadora_thread;
    pthread_create(&dispensadora_thread, NULL, dispensarAgua, (void*)&dispensadora);

    for(int i = 0; i < 3; i++) {
        pthread_join(molinos[i].thread, NULL);
        pthread_join(silos[i].thread, NULL);
        pthread_join(empacadoras[i].thread, NULL);
    }
    pthread_join(dispensadora_thread, NULL);

    // Destruir mutexes y variables de condición
    pthread_mutex_destroy(&mutex_agua);
    pthread_mutex_destroy(&mutex_trigo);
    pthread_mutex_destroy(&mutex_out);
    pthread_cond_destroy(&cond_agua);
    pthread_cond_destroy(&cond_trigo);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&mutex_empacadora[i]);
        pthread_cond_destroy(&cond_empacado[i]);
    }

    return 0;
}
