// 000000000

// Librerias

// 000000000



#include <iostream>  // cin, cout, getline

#include <string>    // Manejar cadenas de texto

#include <vector>    // Arreglos dinamicos

#include <sstream>   // Tokenizacion

#include <cstdint>   // uint8_t, int8_t

#include <algorithm> // Funciones logicas

#include <cctype>    // Funciones de caracteres



using namespace std; // std





// 000000000

// Variables

// 000000000



// uint8_t: Entero sin signo de 8 bits (solo valores positivos: 0 a 255)

// int8_t: Entero con signo de 8 bits (valores positivos y negativos: -128 a 127)



uint8_t RAM[256][2]; // 128 Instr (0-127), 128 Datos (128-255). Col0: OpCode, Col1: Operando



// Registros generales

int8_t AX;

int8_t BX;

int8_t CX;



uint8_t PC = 0;      // Contador programa: Proxima instruccion a ejecutar

uint8_t IR_OpCode;   // Registro instruccion: Codifica e interpreta la instruccion (Instruccion)

uint8_t IR_Operando; // Registro instruccion: Codifica e interpreta la instruccion (Dato)

uint8_t SP;          // Puntero pila: Proxima direccion disponible

uint8_t BP;          // Puntero base: Puntero de base de la pila

uint8_t MAR;         // Registro direccion memoria: Guarda direccion de memoria a acceder

uint8_t MDR;         // Registro de dato de memoria: Intermedio para leer o escribir en RAM



// Registros de control

bool FLAGS[4] = {false, false, false, false}; // Indices: 0=Z, 1=N, 2=C, 3=O (Afectadas por CMP, DIV, etc)



bool isRunning = true;  // Interruptor CPU: true = corriendo, false = detener simulador

string entradaUsuario;  // Almacena la linea bruta escrita por el usuario

string palabraOpCode;   // Guarda la instruccion (ej: "MOV")

string palabraDestino;  // Guarda 1er operando (ej: "AX")

string palabraOrigen;   // Guarda 2do operando (ej: "06A")



vector<string> programaTxt; // Guarda historial de las lineas ya validadas (Global para Paso 2)





// 000000000

// Funciones de Validacion (Helpers)

// 000000000



// Verifica si el texto contiene solo numeros (para valores decimales)

bool esNumero(const string& str) {

    if (str.empty()) return false;

    for (char const &c : str) {

        if (!isdigit(c)) return false;

    }

    return true;

}



// Revisa si el operando es un registro valido del CPU

bool esRegistro(const string& str) {

    return (str == "AX" || str == "BX" || str == "CX");

}



// Asigna un numero identificador unico a cada registro

int obtenerIdRegistro(const string& str) {

    if (str == "AX") return 1;

    if (str == "BX") return 2;

    if (str == "CX") return 3;

    return 0;

}



// Valida si el operando es una direccion de memoria hexadecimal valida (ej: 06A)

bool esMemoria(const string& str) {

    if (str.length() != 3 || str[0] != '0') return false;

    for (char const &c : str) {

        if (!isxdigit(c)) return false;

    }

    return true;

}



// El usuario entro una instruccion valida?

bool validarInstruccion(const string& op) {

    vector<string> validas = {"MOV", "HLT", "DIV", "JMP", "JMA", "CMP", "SHL"};

    if (esNumero(op)) return true;

    for (const string& v : validas) {

        if (op == v) return true;

    }

    return false;

}



// Acepta direcciones de memoria, Registros o numeros decimales

bool validarOperando(const string& op) {

    if (op.empty()) return true;

    if (esMemoria(op)) return true;

    if (esRegistro(op)) return true;

    if (esNumero(op)) return true;

    return false;

}





//000000000000000000000000000000000

// Paso 1. Lectura de instrucciones (Parser)

//000000000000000000000000000000000



void paso1_Lectura() {

    programaTxt.clear(); // Limpiar historial viejo por si se hacen varias pruebas

    bool tieneHLT = false; // Bandera para saber si el programa tiene un final seguro



    // Bucle limitado a 128 lugares (limite de memoria para instrucciones)

    while (programaTxt.size() < 128) {

        getline(cin, entradaUsuario); // Leer linea cruda desde el teclado



        // Ignorar lineas completamente vacias (ej: enter accidental)

        if (entradaUsuario.empty()) continue;



        // Reemplazar comas por espacios para facilitar el corte de operandos

        replace(entradaUsuario.begin(), entradaUsuario.end(), ',', ' ');



        stringstream ss(entradaUsuario); // Crear flujo de texto para extraer palabras

        vector<string> tokens; // Lista temporal para guardar cada pedazo de la linea

        string token;

       

        // Extraer palabra por palabra ignorando espacios extra

        while (ss >> token) {

            tokens.push_back(token);

        }



        // Prevenir crasheo del programa si el usuario solo tecleo espacios

        if (tokens.empty()) continue;



        palabraOpCode = tokens[0]; // 1ra palabra: Instruccion (ej: MOV)

        palabraDestino = (tokens.size() > 1) ? tokens[1] : ""; // 2da palabra: Destino (ej: AX)

        palabraOrigen = (tokens.size() > 2) ? tokens[2] : ""; // 3ra palabra: Origen (ej: 06A o 5)



        // Verificar que la instruccion pertenezca a las permitidas

        if (!validarInstruccion(palabraOpCode)) {

            cout << "\n[!] Error de Sintaxis: Instruccion desconocida" << endl;

            isRunning = false;

            break;

        }



        // Verificar que los operandos tengan sintaxis correcta

        if (!validarOperando(palabraDestino) || !validarOperando(palabraOrigen)) {

            cout << "\n[!] Error de Sintaxis: Operando invalido" << endl;

            isRunning = false;

            break;

        }



        // Si paso los filtros, se guarda en el historial

        programaTxt.push_back(entradaUsuario);



        // Detectar si la instruccion actual es el terminador de la maquina

        if (palabraOpCode == "HLT" || palabraOpCode == "0") {

            tieneHLT = true;

            break;

        }

    }



    // Evitar desbordamiento si el usuario agota las 128 lineas sin poner HLT

    if (!tieneHLT && isRunning) {

        cout << "\n[!] Advertencia: Limite de codigo alcanzado. Forzando HLT." << endl;

    }

}





//000000000000000000000000000000000

// Paso 2. Enviar a memoria (Ensamblado y MMU)

//000000000000000000000000000000000



void paso2_Ensamblado() {

    // Limpiar toda la zona de codigo de la RAM (0 a 127) con ceros por seguridad

    for (int i = 0; i < 128; i++) {

        RAM[i][0] = 0;

        RAM[i][1] = 0;

    }



    // Recorrer el historial linea por linea para traducirlo a la matriz RAM

    for (size_t i = 0; i < programaTxt.size(); i++) {

        string linea = programaTxt[i];

        stringstream ss(linea);

        vector<string> tokens;

        string token;

       

        // Separar la instruccion en sus partes para traducir

        while (ss >> token) tokens.push_back(token);

       

        string op = tokens[0];

        string d = (tokens.size() > 1) ? tokens[1] : "";

        string o = (tokens.size() > 2) ? tokens[2] : "";



        uint8_t opcode_num = 0;

        uint8_t operando_num = 0;



        // Traducir texto a OpCodes numericos y aplicar Offset (+128) si es memoria

        if (op == "HLT" || op == "0") {

            opcode_num = 0;

            operando_num = 0;

        }

        else if (op == "MOV") {

            if (esRegistro(d) && esNumero(o)) {

                opcode_num = 10 + obtenerIdRegistro(d); // Fusionar registro en OpCode (11, 12, 13)

                operando_num = stoi(o);

            }

            else if (esRegistro(d) && esMemoria(o)) {

                opcode_num = 20 + obtenerIdRegistro(d);

                operando_num = stoi(o, nullptr, 16) + 128; // Aplicar Memoria Virtual (+128)

            }

            else if (esMemoria(d) && esRegistro(o)) {

                opcode_num = 30 + obtenerIdRegistro(o);

                operando_num = stoi(d, nullptr, 16) + 128; // Aplicar Memoria Virtual (+128)

            }

            else if (esRegistro(d) && esRegistro(o)) {

                opcode_num = 40 + obtenerIdRegistro(d);

                operando_num = obtenerIdRegistro(o);

            }

        }

        else if (op == "DIV") {

            if (esRegistro(d) && esNumero(o)) {

                opcode_num = 50 + obtenerIdRegistro(d);

                operando_num = stoi(o);

            }

            else if (esMemoria(d)) {

                opcode_num = 60;

                operando_num = stoi(d, nullptr, 16) + 128; // Aplicar Memoria Virtual (+128)

            }

        }

        else if (op == "CMP") {

            if (esRegistro(d) && esNumero(o)) {

                opcode_num = 70 + obtenerIdRegistro(d);

                operando_num = stoi(o);

            }

            else if (esRegistro(d) && esMemoria(o)) {

                opcode_num = 80 + obtenerIdRegistro(d);

                operando_num = stoi(o, nullptr, 16) + 128; // Aplicar Memoria Virtual (+128)

            }

        }

        else if (op == "JMP") {

            opcode_num = 90;

            operando_num = stoi(d); // Los saltos ignoran el offset porque apuntan a codigo

        }

        else if (op == "JMA") {

            opcode_num = 91;

            operando_num = stoi(d); // Los saltos ignoran el offset porque apuntan a codigo

        }

        else if (op == "SHL") {

            if (esRegistro(d) && esNumero(o)) {

                opcode_num = 100 + obtenerIdRegistro(d);

                operando_num = stoi(o);

            }

        }



        // Inyectar los datos traducidos en la fila correspondiente de la RAM

        RAM[i][0] = opcode_num;

        RAM[i][1] = operando_num;

    }



    // Cortafuegos: Si el usuario lleno las 128 lineas, forzar un HLT al final por seguridad

    if (programaTxt.size() == 128) {

        RAM[127][0] = 0;

        RAM[127][1] = 0;

    }

}

//000000000000000000000000000000000

// Main (Modo Pruebas y Desensamblador)

//000000000000000000000000000000000



int main() {

    cout << "--- CPU: FASE DE ENSAMBLADO ---" << endl;

    cout << "Escribe tu codigo. Termina con HLT." << endl;

   

    // Paso 1: Leer y validar el codigo ingresado por el usuario

    paso1_Lectura();



    // Si todo salio bien, proceder a ensamblar en la RAM

    if (isRunning) {

        paso2_Ensamblado();

        cout << "\n--- RAM ---" << endl;

       

        // Imprimir vista previa de la RAM combinando la instruccion original del usuario

        for(int i = 0; i < 5; i++) {

            if (i < programaTxt.size()) {

                cout << "RAM[" << i << "] -> " << programaTxt[i] << endl;

            } else {

                // Si la linea esta vacia, mostrar 0 (relleno de seguridad)

                cout << "RAM[" << i << "] -> 0" << endl;

            }

        }

    }



    return 0;

}