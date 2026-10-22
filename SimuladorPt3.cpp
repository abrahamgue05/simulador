#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>
#include <iomanip>
#include <limits>

using namespace std;

// ============================================================
// Configuracion de la CPU
// ============================================================
constexpr int TAM_RAM = 256;
constexpr int LIMITE_INSTRUCCIONES = 128;
constexpr int INICIO_DATOS = 128;
constexpr int DIRECCION_DATOS_MAX = 127; // Direccion logica: 000-07F

// Cambia a 500 para mostrar una instruccion cada medio segundo.
#ifndef RETRASO_MS
#define RETRASO_MS 2000
#endif

uint8_t RAM[TAM_RAM][2] = {};

int8_t AX = 0;
int8_t BX = 0;
int8_t CX = 0;

int PC = 0;
uint8_t IR_OpCode = 0;
uint8_t IR_Operando = 0;
int SP = 255;
int BP = 248;
uint8_t MAR = 0;
uint8_t MDR = 0;

// Banderas: Z (cero), N (negativo), C (acarreo), O (desbordamiento).
bool FLAGS[4] = {false, false, false, false};

bool isRunning = true;
bool errorEjecucion = false;

struct Instruccion {
    string op;
    string destino;
    string origen;
    string textoOriginal;
};

vector<Instruccion> programa;

// ============================================================
// Conversiones y utilidades de 8 bits
// ============================================================

// Convierte cualquier entero a su representacion de 8 bits (0-255).
uint8_t codificarByte(int valor) {
    int resultado = valor % 256;
    if (resultado < 0) resultado += 256;
    return static_cast<uint8_t>(resultado);
}

// Interpreta un byte como entero con signo de 8 bits (-128 a 127).
int convertirByteConSigno(uint8_t valor) {
    return (valor < 128) ? static_cast<int>(valor)
                         : static_cast<int>(valor) - 256;
}

// Envuelve el resultado en el rango de un registro con signo de 8 bits.
int convertirAInt8(int valor) {
    int byte = valor % 256;
    if (byte < 0) byte += 256;
    if (byte >= 128) byte -= 256;
    return byte;
}

bool convertirEntero(const string& texto, int& valor) {
    if (texto.empty()) return false;

    try {
        size_t usados = 0;
        long long numero = stoll(texto, &usados, 10);
        if (usados != texto.size()) return false;
        if (numero < numeric_limits<int>::min() ||
            numero > numeric_limits<int>::max()) return false;
        valor = static_cast<int>(numero);
        return true;
    } catch (...) {
        return false;
    }
}

bool esEntero8ConSigno(const string& texto, int& valor) {
    return convertirEntero(texto, valor) && valor >= -128 && valor <= 127;
}

bool esRegistro(const string& texto) {
    return texto == "AX" || texto == "BX" || texto == "CX";
}

int obtenerIdRegistro(const string& texto) {
    if (texto == "AX") return 1;
    if (texto == "BX") return 2;
    if (texto == "CX") return 3;
    return 0;
}

string nombreRegistro(int id) {
    if (id == 1) return "AX";
    if (id == 2) return "BX";
    if (id == 3) return "CX";
    return "?";
}

int leerRegistro(int id) {
    if (id == 1) return static_cast<int>(AX);
    if (id == 2) return static_cast<int>(BX);
    if (id == 3) return static_cast<int>(CX);
    return 0;
}

void escribirRegistro(int id, int valor) {
    const int valor8 = convertirAInt8(valor);
    if (id == 1) AX = static_cast<int8_t>(valor8);
    else if (id == 2) BX = static_cast<int8_t>(valor8);
    else if (id == 3) CX = static_cast<int8_t>(valor8);
}

// ============================================================
// Direccionamiento de memoria
// Direccion logica 000-07F -> direccion fisica RAM[128-255]
// Para evitar ambiguedad con inmediatos numericos, una direccion
// usada como fuente en MOV/CMP puede escribirse [002] o 0x002.
// DIV 002 y MOV 002, BX conservan la sintaxis original.
// ============================================================

bool descomponerDireccionHex(const string& texto, string& hexadecimal,
                             bool& explicita) {
    string t = texto;
    explicita = false;

    if (t.size() >= 3 && t.front() == '[' && t.back() == ']') {
        hexadecimal = t.substr(1, t.size() - 2);
        explicita = true;
    } else if (t.size() > 2 && t[0] == '0' && t[1] == 'X') {
        hexadecimal = t.substr(2);
        explicita = true;
    } else if (t.size() == 3 && t[0] == '0') {
        hexadecimal = t;
    } else {
        return false;
    }

    if (hexadecimal.empty() || hexadecimal.size() > 3) return false;
    for (unsigned char c : hexadecimal) {
        if (!isxdigit(c)) return false;
    }
    return true;
}

bool pareceDireccionMemoria(const string& texto) {
    string hexadecimal;
    bool explicita = false;
    return descomponerDireccionHex(texto, hexadecimal, explicita);
}

bool esMemoriaComoFuente(const string& texto) {
    string hexadecimal;
    bool explicita = false;
    if (!descomponerDireccionHex(texto, hexadecimal, explicita)) return false;
    if (explicita) return true;

    // Direcciones como 00A no se confunden con literales decimales.
    for (unsigned char c : hexadecimal) {
        if (c >= 'A' && c <= 'F') return true;
    }
    return false;
}

bool obtenerDireccionMemoria(const string& texto, int& desplazamiento,
                             string& error) {
    string hexadecimal;
    bool explicita = false;

    if (!descomponerDireccionHex(texto, hexadecimal, explicita)) {
        error = "Direccion de memoria invalida '" + texto +
                "'. Usa una direccion hexadecimal entre 000 y 07F.";
        return false;
    }

    unsigned long valor = 0;
    try {
        valor = stoul(hexadecimal, nullptr, 16);
    } catch (...) {
        error = "No se pudo interpretar la direccion de memoria '" + texto + "'.";
        return false;
    }

    if (valor > DIRECCION_DATOS_MAX) {
        error = "Direccion de memoria fuera de rango: '" + texto +
                "'. El rango valido es 000-07F.";
        return false;
    }

    desplazamiento = static_cast<int>(valor);
    return true;
}

int direccionFisica(const string& texto) {
    int desplazamiento = 0;
    string error;
    obtenerDireccionMemoria(texto, desplazamiento, error); // La linea ya fue validada.
    return INICIO_DATOS + desplazamiento;
}

// ============================================================
// Limpieza y validacion de instrucciones
// ============================================================

string limpiarLinea(string linea) {
    size_t comentario = string::npos;
    size_t puntoComa = linea.find(';');
    size_t dobleBarra = linea.find("//");

    if (puntoComa != string::npos) comentario = puntoComa;
    if (dobleBarra != string::npos &&
        (comentario == string::npos || dobleBarra < comentario)) {
        comentario = dobleBarra;
    }
    if (comentario != string::npos) linea.erase(comentario);

    for (char& c : linea) {
        unsigned char uc = static_cast<unsigned char>(c);
        c = static_cast<char>(toupper(uc));
        if (c == ',') c = ' ';
    }
    return linea;
}

vector<string> tokenizar(const string& linea) {
    vector<string> tokens;
    string token;
    stringstream ss(linea);
    while (ss >> token) tokens.push_back(token);
    return tokens;
}

bool validarCantidad(const vector<string>& tokens, size_t esperada,
                     const string& op, string& error) {
    if (tokens.size() != esperada) {
        error = "La instruccion " + op + " requiere " +
                to_string(esperada - 1) +
                (esperada == 2 ? " operando." : " operandos.");
        return false;
    }
    return true;
}

bool analizarLinea(const string& linea, Instruccion& instruccion,
                   string& error) {
    vector<string> tokens = tokenizar(limpiarLinea(linea));
    if (tokens.empty()) return false;

    const string& op = tokens[0];
    instruccion.op = op;
    instruccion.destino = (tokens.size() > 1) ? tokens[1] : "";
    instruccion.origen = (tokens.size() > 2) ? tokens[2] : "";
    instruccion.textoOriginal = linea;

    if (op == "HLT" || op == "0") {
        if (tokens.size() != 1) {
            error = "HLT no acepta operandos.";
            return false;
        }
        instruccion.op = "HLT";
        return true;
    }

    if (op == "MOV") {
        if (!validarCantidad(tokens, 3, op, error)) return false;
        const string& d = instruccion.destino;
        const string& o = instruccion.origen;

        if (esRegistro(d) && esRegistro(o)) return true;

        if (esRegistro(d) && esMemoriaComoFuente(o)) {
            int dir = 0;
            return obtenerDireccionMemoria(o, dir, error);
        }

        int inmediato = 0;
        if (esRegistro(d) && convertirEntero(o, inmediato)) {
            if (inmediato < -128 || inmediato > 127) {
                error = "El inmediato de MOV debe estar entre -128 y 127.";
                return false;
            }
            return true;
        }

        if (pareceDireccionMemoria(d) && esRegistro(o)) {
            int dir = 0;
            return obtenerDireccionMemoria(d, dir, error);
        }

        error = "Operandos invalidos para MOV. Usa MOV registro, inmediato; "
                "MOV registro, registro; MOV registro, [memoria]; o MOV memoria, registro.";
        return false;
    }

    if (op == "DIV") {
        if (tokens.size() == 2) {
            int dir = 0;
            if (!pareceDireccionMemoria(instruccion.destino)) {
                error = "DIV con un operando requiere una direccion de memoria, por ejemplo: DIV 002.";
                return false;
            }
            return obtenerDireccionMemoria(instruccion.destino, dir, error);
        }
        if (!validarCantidad(tokens, 3, op, error)) return false;
        int divisor = 0;
        if (!esRegistro(instruccion.destino) ||
            !esEntero8ConSigno(instruccion.origen, divisor)) {
            error = "Usa DIV registro, inmediato con inmediato entre -128 y 127, o DIV direccion_de_memoria.";
            return false;
        }
        return true;
    }

    if (op == "CMP") {
        if (!validarCantidad(tokens, 3, op, error)) return false;
        if (!esRegistro(instruccion.destino)) {
            error = "El primer operando de CMP debe ser AX, BX o CX.";
            return false;
        }

        if (esMemoriaComoFuente(instruccion.origen)) {
            int dir = 0;
            return obtenerDireccionMemoria(instruccion.origen, dir, error);
        }

        int inmediato = 0;
        if (!esEntero8ConSigno(instruccion.origen, inmediato)) {
            error = "El segundo operando de CMP debe ser un inmediato entre -128 y 127 o una direccion de memoria como [002].";
            return false;
        }
        return true;
    }

    if (op == "JMP" || op == "JMA") {
        if (!validarCantidad(tokens, 2, op, error)) return false;
        int destino = 0;
        if (!convertirEntero(instruccion.destino, destino) ||
            destino < 0 || destino >= LIMITE_INSTRUCCIONES) {
            error = "El destino de " + op + " debe ser un indice entre 0 y 127.";
            return false;
        }
        return true;
    }

    if (op == "SHL") {
        if (!validarCantidad(tokens, 3, op, error)) return false;
        int desplazamiento = 0;
        if (!esRegistro(instruccion.destino) ||
            !convertirEntero(instruccion.origen, desplazamiento) ||
            desplazamiento < 0 || desplazamiento > 7) {
            error = "Usa SHL registro, cantidad; la cantidad debe estar entre 0 y 7.";
            return false;
        }
        return true;
    }

    if (op == "PUSH" || op == "POP") {
        if (!validarCantidad(tokens, 2, op, error)) return false;
        if (!esRegistro(instruccion.destino)) {
            error = op + " solo acepta los registros AX, BX o CX.";
            return false;
        }
        return true;
    }

    error = "Instruccion desconocida '" + op + "'.";
    return false;
}

bool validarDestinosSaltos() {
    for (const Instruccion& ins : programa) {
        if (ins.op != "JMP" && ins.op != "JMA") continue;
        int destino = 0;
        convertirEntero(ins.destino, destino);
        if (destino < 0 || destino >= static_cast<int>(programa.size())) {
            cout << "\n[!] Error: " << ins.op << " apunta al indice " << destino
                 << ", pero el programa solo tiene indices de 0 a "
                 << (programa.empty() ? 0 : static_cast<int>(programa.size()) - 1)
                 << ".\n";
            return false;
        }
    }
    return true;
}

bool paso1_Lectura() {
    programa.clear();
    bool tieneHLT = false;

    while (programa.size() < LIMITE_INSTRUCCIONES) {
        string linea;
        if (!getline(cin, linea)) break;

        string limpia = limpiarLinea(linea);
        if (tokenizar(limpia).empty()) continue;

        Instruccion ins;
        string error;
        if (!analizarLinea(linea, ins, error)) {
            cout << "\n[!] Error de sintaxis en: " << linea
                 << "\n    " << error << "\n";
            return false;
        }

        programa.push_back(ins);
        if (ins.op == "HLT") {
            tieneHLT = true;
            break;
        }
    }

    if (!tieneHLT) {
        cout << "\n[!] Advertencia: no se encontro HLT. Se insertara uno al final." << endl;
        Instruccion hlt = {"HLT", "", "", "HLT (insertado automaticamente)"};
        if (programa.size() < LIMITE_INSTRUCCIONES) {
            programa.push_back(hlt);
        } else {
            // La memoria de instrucciones admite 128 posiciones; se reserva la ultima para HLT.
            programa.back() = hlt;
            cout << "[!] Se reemplazo la instruccion del indice 127 por HLT." << endl;
        }
    }

    return validarDestinosSaltos();
}

// ============================================================
// Paso 2: ensamblado a opcodes
// ============================================================

void paso2_Ensamblado() {
    for (int i = 0; i < TAM_RAM; ++i) {
        RAM[i][0] = 0;
        RAM[i][1] = 0;
    }

    for (size_t i = 0; i < programa.size(); ++i) {
        const Instruccion& ins = programa[i];
        const string& op = ins.op;
        const string& d = ins.destino;
        const string& o = ins.origen;
        uint8_t opcode = 0;
        uint8_t operando = 0;
        int numero = 0;

        if (op == "HLT") {
            opcode = 0;
        } else if (op == "MOV") {
            if (esRegistro(d) && esRegistro(o)) {
                opcode = static_cast<uint8_t>(40 + obtenerIdRegistro(d));
                operando = static_cast<uint8_t>(obtenerIdRegistro(o));
            } else if (esRegistro(d) && esMemoriaComoFuente(o)) {
                opcode = static_cast<uint8_t>(20 + obtenerIdRegistro(d));
                operando = static_cast<uint8_t>(direccionFisica(o));
            } else if (esRegistro(d)) {
                convertirEntero(o, numero);
                opcode = static_cast<uint8_t>(10 + obtenerIdRegistro(d));
                operando = codificarByte(numero);
            } else {
                int direccion = direccionFisica(d);
                opcode = static_cast<uint8_t>(30 + obtenerIdRegistro(o));
                operando = static_cast<uint8_t>(direccion);
            }
        } else if (op == "DIV") {
            if (esRegistro(d)) {
                convertirEntero(o, numero);
                opcode = static_cast<uint8_t>(50 + obtenerIdRegistro(d));
                operando = codificarByte(numero);
            } else {
                opcode = 60;
                operando = static_cast<uint8_t>(direccionFisica(d));
            }
        } else if (op == "CMP") {
            if (esMemoriaComoFuente(o)) {
                opcode = static_cast<uint8_t>(80 + obtenerIdRegistro(d));
                operando = static_cast<uint8_t>(direccionFisica(o));
            } else {
                convertirEntero(o, numero);
                opcode = static_cast<uint8_t>(70 + obtenerIdRegistro(d));
                operando = codificarByte(numero);
            }
        } else if (op == "JMP") {
            convertirEntero(d, numero);
            opcode = 90;
            operando = static_cast<uint8_t>(numero);
        } else if (op == "JMA") {
            convertirEntero(d, numero);
            opcode = 91;
            operando = static_cast<uint8_t>(numero);
        } else if (op == "SHL") {
            convertirEntero(o, numero);
            opcode = static_cast<uint8_t>(100 + obtenerIdRegistro(d));
            operando = static_cast<uint8_t>(numero);
        } else if (op == "PUSH") {
            opcode = static_cast<uint8_t>(110 + obtenerIdRegistro(d));
        } else if (op == "POP") {
            opcode = static_cast<uint8_t>(120 + obtenerIdRegistro(d));
        }

        RAM[i][0] = opcode;
        RAM[i][1] = operando;
    }
}

// ============================================================
// Banderas y dashboard
// ============================================================

void actualizarFlags(int resultadoMatematico, bool acarreo = false) {
    const uint8_t resultadoByte = codificarByte(resultadoMatematico);
    FLAGS[0] = (resultadoByte == 0); // Z: resultado de 8 bits igual a cero.
    FLAGS[1] = (resultadoByte >= 128); // N: bit mas significativo encendido.
    FLAGS[2] = acarreo; // C: se calcula explicitamente para SHL; otros casos simplificados.
    FLAGS[3] = (resultadoMatematico < -128 || resultadoMatematico > 127); // O
}

void imprimirDashboard() {
    int bytesUsados = 255 - SP;
    if (bytesUsados < 0) bytesUsados = 0;
    if (bytesUsados > 128) bytesUsados = 128;
    double porcentajePila = bytesUsados * 100.0 / 128.0;

    cout << "PC: [" << PC << "] | MAR: [" << static_cast<int>(MAR)
         << "] | MDR: [" << static_cast<int>(MDR) << "] | IR: ["
         << static_cast<int>(IR_OpCode) << ", "
         << static_cast<int>(IR_Operando) << "]\n";

    cout << "Z: " << FLAGS[0] << " | N: " << FLAGS[1]
         << " | C: " << FLAGS[2] << " | O: " << FLAGS[3] << "\n";

    cout << "AX: " << static_cast<int>(AX)
         << " | BX: " << static_cast<int>(BX)
         << " | CX: " << static_cast<int>(CX) << "\n";

    cout << "BP: " << BP << " | SP: " << SP << " | Uso de Pila: "
         << fixed << setprecision(1) << porcentajePila << "%" << defaultfloat << "\n";
    cout << "--------------------------------------------------" << endl;
}

// ============================================================
// Paso 3: Fetch-Decode-Execute
// ============================================================

void paso3_Ejecucion() {
    cout << "\n--- INICIANDO EJECUCION (F-D-E) ---\n" << endl;

    while (isRunning) {
        if (PC < 0 || PC >= static_cast<int>(programa.size()) ||
            PC >= LIMITE_INSTRUCCIONES) {
            cout << "[!] Error: PC fuera del rango del programa (PC=" << PC << ")." << endl;
            isRunning = false;
            errorEjecucion = true;
            break;
        }

        // 1. FETCH
        MAR = static_cast<uint8_t>(PC);
        IR_OpCode = RAM[PC][0];
        IR_Operando = RAM[PC][1];
        MDR = IR_Operando;
        ++PC;

        // 2. DECODE & EXECUTE
        const int familia = IR_OpCode / 10;
        const int regId = IR_OpCode % 10;

        if (IR_OpCode == 0) {
            isRunning = false; // HLT
        } else if (familia == 1) {
            // MOV registro, inmediato con signo de 8 bits.
            escribirRegistro(regId, convertirByteConSigno(IR_Operando));
        } else if (familia == 2) {
            // MOV registro, memoria.
            const int direccion = IR_Operando;
            const int valor = convertirByteConSigno(RAM[direccion][0]);
            escribirRegistro(regId, valor);
        } else if (familia == 3) {
            // MOV memoria, registro.
            const int direccion = IR_Operando;
            RAM[direccion][0] = codificarByte(leerRegistro(regId));
        } else if (familia == 4) {
            // MOV registro, registro.
            const int idOrigen = IR_Operando;
            if (idOrigen < 1 || idOrigen > 3) {
                cout << "[!] Error: registro fuente invalido en MOV." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            escribirRegistro(regId, leerRegistro(idOrigen));
        } else if (familia == 5) {
            // DIV registro, inmediato.
            const int divisor = convertirByteConSigno(IR_Operando);
            const int valorAntes = leerRegistro(regId);
            if (divisor == 0) {
                cout << "[!] Error critico: division entre cero detectada." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            const int resultado = valorAntes / divisor;
            cout << "[ALU] -> Division\n  --> Operacion: " << nombreRegistro(regId)
                 << " (" << valorAntes << ") / " << divisor << " = " << resultado << endl;
            escribirRegistro(regId, resultado);
            actualizarFlags(resultado);
        } else if (IR_OpCode == 60) {
            // DIV memoria: AX = AX / RAM[direccion].
            const int direccion = IR_Operando;
            const int divisor = convertirByteConSigno(RAM[direccion][0]);
            const int valorAntes = static_cast<int>(AX);
            if (divisor == 0) {
                cout << "[!] Error critico: division entre cero detectada en RAM["
                     << direccion << "]." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            const int resultado = valorAntes / divisor;
            cout << "[ALU] -> Division con memoria\n  --> Operacion: AX ("
                 << valorAntes << ") / RAM[" << direccion << "] ("
                 << divisor << ") = " << resultado << endl;
            escribirRegistro(1, resultado);
            actualizarFlags(resultado);
        } else if (familia == 7) {
            // CMP registro, inmediato. No guarda la resta, solo actualiza flags.
            const int valorRegistro = leerRegistro(regId);
            const int inmediato = convertirByteConSigno(IR_Operando);
            actualizarFlags(valorRegistro - inmediato);
        } else if (familia == 8) {
            // CMP registro, memoria.
            const int direccion = IR_Operando;
            const int valorRegistro = leerRegistro(regId);
            const int valorMemoria = convertirByteConSigno(RAM[direccion][0]);
            actualizarFlags(valorRegistro - valorMemoria);
        } else if (familia == 10) {
            // SHL registro, cantidad (0-7).
            const int cantidad = IR_Operando;
            const int valorAntes = leerRegistro(regId);
            cout << "[ALU] -> Desplazamiento SHL\n  --> Operacion: "
                 << nombreRegistro(regId) << " (" << valorAntes << ") << "
                 << cantidad << endl;

            if (cantidad > 0) {
                const unsigned int bitsAntes =
                    static_cast<unsigned int>(codificarByte(valorAntes));
                const bool acarreo = ((bitsAntes >> (8 - cantidad)) & 1U) != 0;
                const int resultadoMatematico = valorAntes * (1 << cantidad);
                escribirRegistro(regId, resultadoMatematico);
                actualizarFlags(resultadoMatematico, acarreo);
            }
        } else if (familia == 11) {
            // PUSH registro. La pila usa RAM[128..255].
            if (SP < INICIO_DATOS) {
                cout << "[!] Error: desbordamiento de pila; no queda espacio." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            const int valor = leerRegistro(regId);
            const int direccion = SP;
            RAM[direccion][0] = codificarByte(valor);
            RAM[direccion][1] = 0;
            --SP;
            cout << "[STACK] -> PUSH " << nombreRegistro(regId)
                 << " (Valor: " << valor << ") en RAM[" << direccion << "]" << endl;
        } else if (familia == 12) {
            // POP registro.
            if (SP >= 255) {
                cout << "[!] Error: subdesbordamiento de pila; no hay datos para POP." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            ++SP;
            const int valor = convertirByteConSigno(RAM[SP][0]);
            escribirRegistro(regId, valor);
            cout << "[STACK] -> POP " << nombreRegistro(regId)
                 << " (Recuperado: " << valor << ") desde RAM[" << SP << "]" << endl;
        } else if (IR_OpCode == 90) {
            // JMP destino: el destino es un indice de instruccion basado en cero.
            if (IR_Operando >= programa.size()) {
                cout << "[!] Error: salto JMP fuera de rango." << endl;
                isRunning = false;
                errorEjecucion = true;
                break;
            }
            PC = IR_Operando;
        } else if (IR_OpCode == 91) {
            // JMA: salto si Z=1 (igualdad).
            if (FLAGS[0]) {
                if (IR_Operando >= programa.size()) {
                    cout << "[!] Error: salto JMA fuera de rango." << endl;
                    isRunning = false;
                    errorEjecucion = true;
                    break;
                }
                PC = IR_Operando;
            }
        } else {
            cout << "[!] Error: opcode no implementado: "
                 << static_cast<int>(IR_OpCode) << "." << endl;
            isRunning = false;
            errorEjecucion = true;
            break;
        }

        // 3. Estado visible despues de cada instruccion ejecutada.
        imprimirDashboard();

        // HLT detiene inmediatamente; no espera despues de imprimir.
        if (isRunning && RETRASO_MS > 0) {
            this_thread::sleep_for(chrono::milliseconds(RETRASO_MS));
        }
    }

    if (errorEjecucion) {
        cout << "\n--- CPU DETENIDA POR ERROR ---" << endl;
    } else {
        cout << "\n--- CPU DETENIDA (HLT) ---" << endl;
    }
}

// ============================================================
// Main
// ============================================================

int main() {
    cout << "--- CPU: FASE DE ENSAMBLADO Y EJECUCION ---" << endl;
    cout << "Escribe tu codigo. Termina con HLT." << endl;
    cout << "Direcciones de datos: 000-07F; indices de salto desde 0.\n" << endl;

    if (!paso1_Lectura()) {
        cout << "\n[FRACASO] No se pudo ensamblar el programa." << endl;
        return 1;
    }

    paso2_Ensamblado();
    cout << "\n[EXITO] Codigo ensamblado. Arrancando la CPU...\n" << endl;
    paso3_Ejecucion();
    return errorEjecucion ? 1 : 0;
}
