#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <iostream>
#include <string.h>
#include <cstring>

using namespace std;

string characters[] = { " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_", "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~" };

struct Threads {
    pthread_t thread;
    int index;
    string value;
    string converted;
};

void* toDecimal(void* args) {
    string dec[] = { "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119", "120", "121", "122", "123", "124", "125", "126" };

    Threads* t;
    t = (Threads*)args;

    t->converted = dec[t->index];

    cout << t->converted << " ";

    return 0;
}

void* toHex(void* args) {
    string hex[] = { "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E" };

    Threads* t;
    t = (Threads*)args;

    t->converted = hex[t->index];

    return 0;
}

int main () {
    int hilos;
    string texto;

    cout << "Ingresa la cadena a imprimir" << endl;
    cin >> texto;

    char* arreglo = new char[texto.size() + 1];
    hilos = texto.size();

    Threads threads[hilos];

    cout << "La cantidad de hilos a usar es: " << hilos << endl;

    for (int i = 0; i < hilos; i++) {
        threads[i].index = 1;
        pthread_create(&threads[i].thread, NULL, toDecimal, (void*)&threads[i]);
    }

    return 0;
}