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

uint8_t RAM[256][2]; // 128 Instr, 128 Datos. Col0: OpCode, Col1: Operando

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


// 000000000
// Funciones
// 000000000

// Verifica si el texto contiene solo numeros (para valores decimales)
bool esNumero(const string& str) {
    if (str.empty()) return false;
    for (char const &c : str) {
        if (!isdigit(c)) return false;
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

// Acepta direcciones de memoria (inicia con 0 y 3 chars), Registros o numeros decimales
bool validarOperando(const string& op) {
    if (op.empty()) return true;
    if (op.length() == 3 && op[0] == '0') return true;
    if (op == "AX" || op == "BX" || op == "CX") return true;
    if (esNumero(op)) return true;
    return false;
}


//000000000000000000000000000000000
// Paso 1. Lectura de instrucciones
//000000000000000000000000000000000

void paso1_Lectura() {
    vector<string> programaTxt; // Guarda historial de las lineas ya validadas
    bool tieneHLT = false; // Bandera para saber si el programa tiene un final seguro

    // Bucle limitado a 128 lugares (limite de memoria para instrucciones)
    while (programaTxt.size() < 128) {
        getline(cin, entradaUsuario); // Leer linea cruda desde el teclado

        // Ignorar lineas completamente vacias (ej: enter accidental)
        if (entradaUsuario.empty()) {
            continue;
        }

        // Reemplazar comas por espacios para facilitar el corte de operandos
        replace(entradaUsuario.begin(), entradaUsuario.end(), ',', ' ');

        stringstream ss(entradaUsuario); // Crear flujo de texto para extraer palabras
        vector<string> tokens; // Lista temporal para guardar cada pedazo de la linea
        string token;
        
        // Extraer palabra por palabra ignorando espacios extra
        while (ss >> token) {
            tokens.push_back(token);
        }

        // Prevenir crasheo del programa si el usuario solo tecleo espacios en blanco
        if (tokens.empty()) {
            continue;
        }

        palabraOpCode = tokens[0]; // 1ra palabra: Instruccion (ej: MOV)
        palabraDestino = (tokens.size() > 1) ? tokens[1] : ""; // 2da palabra: Destino (ej: AX)
        palabraOrigen = (tokens.size() > 2) ? tokens[2] : ""; // 3ra palabra: Origen (ej: 06A o 5)

        // Verificar que la instruccion pertenezca a las 7 permitidas
        if (!validarInstruccion(palabraOpCode)) {
            cout << "Error de Sintaxis" << endl;
            isRunning = false;
            break;
        }

        // Verificar que los operandos tengan sintaxis correcta (Reg, Mem o Num)
        if (!validarOperando(palabraDestino) || !validarOperando(palabraOrigen)) {
            cout << "Error de Sintaxis" << endl;
            isRunning = false;
            break;
        }

        // Si paso los filtros, se guarda en el historial
        programaTxt.push_back(entradaUsuario);

        // Detectar si la instruccion actual es el terminador de la maquina
        if (palabraOpCode == "HLT" || palabraOpCode == "0") {
            tieneHLT = true;
            break; // Romper bucle de lectura, el programa termino
        }
    }

    // Evitar desbordamiento si el usuario agota las 128 lineas sin poner HLT
    if (!tieneHLT && isRunning) {
        cout << "Error" << endl;
        isRunning = false;
    }
}


//000000000000000000000000000000000
// Menu Principal
//000000000000000000000000000000000

void menuPrincipal() {
    string opcion; // Guarda la eleccion del usuario
    bool enMenu = true; // Control del ciclo del menu

    // Mantener menu activo hasta que se elija Salir
    while (enMenu) {
        cout << "\n--- MENU SIMULADOR CPU ---" << endl;
        cout << "1. Ver instrucciones permitidas" << endl;
        cout << "2. Iniciar programa (Escribir codigo)" << endl;
        cout << "3. Ver memoria actual" << endl;
        cout << "4. Salir" << endl;
        cout << "5. Imprimir un hola mundo en C++ yupiiii" << endl;
        cout << "Elige una opcion: ";
        
        getline(cin, opcion); // Leer opcion como texto para evitar bug de Enter atorado

        if (opcion == "1") {
            cout << "\n[Instrucciones]: MOV, HLT, DIV, JMP, JMA, CMP, SHL" << endl;
            cout << "[Sintaxis]: INSTRUCCION DESTINO, ORIGEN (Ej: MOV AX, 0FF)" << endl;
        } 
        else if (opcion == "2") {
            cout << "\n[Modo Programa]: Escribe tu codigo (Termina con HLT):" << endl;
            paso1_Lectura(); // Llamar al traductor (Paso 1)
        } 
        else if (opcion == "3") {
            cout << "\n[Memoria]: (Aun en construccion...)" << endl;
        } 
        else if (opcion == "4") {
            cout << "\nSaliendo del simulador..." << endl;
            enMenu = false; // Romper ciclo y cerrar programa
        } 
        else if (opcion == "5") {
            cout << "\nHola mundo en C++ yupiiii" << endl;
        } 
        else {
            cout << "\nOpcion no valida. Intenta de nuevo." << endl;
        }
    }
}


//000000000000000000000000000000000
// Main
//000000000000000000000000000000000

int main() {
    menuPrincipal(); // Iniciar programa desde el menu
    return 0;
}