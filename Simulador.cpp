// Permite mostrar mensajes en pantalla con cout y leer texto con cin/getline.
#include <iostream>
// Permite utilizar el tipo string para guardar instrucciones y operandos como texto.
#include <string>
// Biblioteca para representar bits; se conserva entre las bibliotecas permitidas.
// En esta version no se utiliza directamente.
#include <bitset>
// Permite dar formato a los numeros que se muestran, por ejemplo con setprecision.
#include <iomanip>
// Permiten pausar la ejecucion durante un tiempo sin bloquear manualmente el programa.
#include <thread>
#include <chrono>

// Evita escribir std:: antes de cout, string, endl, etc.
using namespace std;

// Memoria total: 256 posiciones.
// Las posiciones 0-127 son para instrucciones.
// Las posiciones 128-255 son para datos.
// Cantidad total de posiciones de memoria RAM simulada.
const int TAM_RAM = 256;
// La zona de instrucciones tiene espacio para 128 instrucciones (indices 0 a 127).
const int MAX_INSTRUCCIONES = 128;
// La zona de datos comienza en la posicion fisica 128.
const int INICIO_DATOS = 128;

// RAM es una tabla de 256 filas y 2 columnas.
// En instrucciones, columna 0 guarda el codigo de operacion y columna 1 el operando.
// Para datos, se usa principalmente la columna 0 para guardar el valor.
// = {} inicializa todos los espacios en cero.
int RAM[TAM_RAM][2] = {};

// Arreglo de registros generales: posicion 0 = AX, 1 = BX y 2 = CX.
// Todos empiezan en cero.
int registros[3] = {0, 0, 0}; // 0=AX, 1=BX, 2=CX
// PC (Program Counter): indica el indice de la siguiente instruccion que se ejecutara.
int PC = 0;
// MAR (Memory Address Register): guarda la direccion de memoria consultada.
int MAR = 0;
// MDR (Memory Data Register): guarda el dato leido de memoria.
int MDR = 0;
// IR: registro de instruccion. IR_OpCode guarda el codigo de la instruccion actual.
int IR_OpCode = 0;
// IR_Operando guarda el valor o la direccion asociada a la instruccion actual.
int IR_Operando = 0;
// BP (Base Pointer): referencia usada para representar la base de la pila.
int BP = 248;
// SP (Stack Pointer): apunta al espacio superior disponible de la pila.
int SP = 255;

// Banderas: 0=Z, 1=N, 2=C, 3=O (desbordamiento)
// Arreglo de banderas: Z = cero, N = negativo, C = acarreo y O = desbordamiento.
// Cada bandera utiliza 0 (falsa) o 1 (verdadera).
int banderas[4] = {0, 0, 0, 0};

// Estructura sencilla para guardar una instruccion antes de convertirla a codigo numerico.
struct Instruccion {
    // Nombre de la operacion, por ejemplo MOV, DIV o HLT.
    string op;
    // Primer operando: suele ser el registro o la direccion de destino.
    string destino;
    // Segundo operando: suele ser el valor, registro o direccion de origen.
    string origen;
};

// Arreglo donde se guarda el programa escrito por el usuario, ya separado en instrucciones.
Instruccion programa[MAX_INSTRUCCIONES];
// Numero de instrucciones validas que se han leido.
int cantidadInstrucciones = 0;
// Se vuelve true si ocurre un error mientras se ejecuta la CPU.
bool errorEjecucion = false;

// Convierte letras minusculas a mayusculas y cambia las comas por espacios.
// Asi MOV ax, 5 y mov AX 5 se pueden procesar de manera similar.
string mayusculas(string texto) {
    for (int i = 0; i < (int)texto.length(); i++) {
        if (texto[i] >= 'a' && texto[i] <= 'z') {
            texto[i] = texto[i] - 'a' + 'A';
        }
        if (texto[i] == ',') texto[i] = ' ';
    }
    return texto;
}

// Elimina comentarios que comiencen con ; o //.
// Solo se conserva el texto que aparece antes del marcador de comentario.
string quitarComentarios(string linea) {
    int pos = (int)linea.find(';');
    int pos2 = (int)linea.find("//");
    if (pos == -1 || (pos2 != -1 && pos2 < pos)) pos = pos2;
    if (pos != -1) linea = linea.substr(0, pos);
    return linea;
}

// Divide una linea en palabras separadas por espacios.
// Ejemplo: "MOV AX 5" se convierte en tres tokens: MOV, AX y 5.
// Devuelve cuantas palabras se guardaron en el arreglo tokens.
int separarTokens(string linea, string tokens[], int maxTokens) {
    int cantidad = 0;
    int i = 0;
    while (i < (int)linea.length()) {
        while (i < (int)linea.length() && linea[i] == ' ') i++;
        if (i >= (int)linea.length()) break;
        string palabra = "";
        while (i < (int)linea.length() && linea[i] != ' ') {
            palabra += linea[i];
            i++;
        }
        if (cantidad < maxTokens) {
            tokens[cantidad] = palabra;
            cantidad++;
        }
    }
    return cantidad;
}

// Intenta convertir un texto a un numero entero decimal.
// Acepta numeros positivos, negativos y un signo + opcional.
// Devuelve true si todo el texto es un numero valido; si no, devuelve false.
// El simbolo & permite guardar el numero convertido en la variable original.
bool convertirNumero(string texto, int &numero) {
    if (texto.length() == 0) return false;
    int signo = 1;
    int i = 0;
    if (texto[0] == '-') {
        signo = -1;
        i = 1;
    } else if (texto[0] == '+') {
        i = 1;
    }
    if (i == (int)texto.length()) return false;
    numero = 0;
    for (; i < (int)texto.length(); i++) {
        if (texto[i] < '0' || texto[i] > '9') return false;
        numero = numero * 10 + texto[i] - '0';
    }
    numero *= signo;
    return true;
}

// Ajusta un valor al rango de un byte sin signo: 0 a 255.
// Los valores que se salen del rango dan la vuelta, como ocurre en una CPU de 8 bits.
int byteSinSigno(int valor) {
    valor = valor % 256;
    if (valor < 0) valor += 256;
    return valor;
}

// Ajusta el valor al rango de un byte con signo: -128 a 127.
// Primero lo convierte a 0-255 y luego interpreta 128-255 como -128 a -1.
int byteConSigno(int valor) {
    valor = byteSinSigno(valor);
    if (valor >= 128) valor -= 256;
    return valor;
}

// Comprueba si el texto corresponde a uno de los registros disponibles.
bool esRegistro(string texto) {
    return texto == "AX" || texto == "BX" || texto == "CX";
}

// Convierte el nombre de un registro en un identificador numerico.
// AX=1, BX=2, CX=3. El cero significa que el nombre no es valido.
int idRegistro(string texto) {
    if (texto == "AX") return 1;
    if (texto == "BX") return 2;
    if (texto == "CX") return 3;
    return 0;
}

// Convierte el identificador numerico de un registro a su nombre.
// Esta funcion puede servir para mostrar el nombre en mensajes.
string nombreRegistro(int id) {
    if (id == 1) return "AX";
    if (id == 2) return "BX";
    if (id == 3) return "CX";
    return "?";
}

// Lee un registro usando su identificador AX=1, BX=2 o CX=3.
// Si el identificador no es valido, devuelve cero como valor de seguridad.
int leerRegistro(int id) {
    if (id >= 1 && id <= 3) return registros[id - 1];
    return 0;
}

// Escribe un valor en el registro indicado.
// El valor se adapta al rango de 8 bits con signo antes de guardarse.
void escribirRegistro(int id, int valor) {
    if (id >= 1 && id <= 3) {
        registros[id - 1] = byteConSigno(valor);
    }
}

// Convierte una direccion logica hexadecimal a una direccion fisica.
// Por ejemplo: 002 -> 2 + 128 = RAM[130].
// Interpreta una direccion logica hexadecimal y calcula su direccion fisica.
// Acepta formatos como 00A, [00A] o 0x00A.
// Solo se permiten direcciones logicas de 000 a 07F (0 a 127).
// Se suma INICIO_DATOS (128) para llegar a la zona de datos de RAM.
bool direccionMemoria(string texto, int &direccionFisica) {
    if (texto.length() >= 3 && texto[0] == '[' && texto[texto.length() - 1] == ']') {
        texto = texto.substr(1, texto.length() - 2);
    } else if (texto.length() > 2 && texto[0] == '0' && texto[1] == 'X') {
        texto = texto.substr(2);
    }

    if (texto.length() < 1 || texto.length() > 3) return false;
    int valor = 0;
    for (int i = 0; i < (int)texto.length(); i++) {
        char c = texto[i];
        int digito;
        if (c >= '0' && c <= '9') digito = c - '0';
        else if (c >= 'A' && c <= 'F') digito = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') digito = c - 'a' + 10;
        else return false;
        valor = valor * 16 + digito;
    }
    if (valor < 0 || valor > 127) return false;
    direccionFisica = INICIO_DATOS + valor;
    return true;
}

// Detecta si el usuario marco una direccion de forma explicita con [ ] o 0x.
bool esDireccionExplicita(string texto) {
    return (texto.length() >= 3 && texto[0] == '[' && texto[texto.length()-1] == ']') ||
           (texto.length() > 2 && texto[0] == '0' && texto[1] == 'X');
}

// Decide si un operando de origen debe interpretarse como una direccion de memoria.
// Las direcciones con letras hexadecimales A-F se reconocen automaticamente.
// Los numeros solo con digitos, como 010, se interpretan como valores inmediatos,
// salvo que se escriban entre corchetes o con el prefijo 0x.
bool esDireccionFuente(string texto) {
    int direccion;
    if (esDireccionExplicita(texto)) return direccionMemoria(texto, direccion);
    // Las direcciones hexadecimales con A-F, como 00A, tambien son memoria.
    for (int i = 0; i < (int)texto.length(); i++) {
        if ((texto[i] >= 'A' && texto[i] <= 'F') || (texto[i] >= 'a' && texto[i] <= 'f'))
            return direccionMemoria(texto, direccion);
    }
    return false;
}

// Revisa una linea escrita por el usuario y comprueba que la instruccion sea valida.
// Si es valida, llena el objeto ins. Si no, explica el problema en error.
// Devuelve false cuando la linea esta vacia o contiene una instruccion invalida.
bool analizarInstruccion(string linea, Instruccion &ins, string &error) {
    linea = mayusculas(quitarComentarios(linea));
    string tokens[4];
    int n = separarTokens(linea, tokens, 4);
    if (n == 0) return false;

    ins.op = tokens[0];
    ins.destino = n > 1 ? tokens[1] : "";
    ins.origen = n > 2 ? tokens[2] : "";

    // HLT detiene la CPU. Tambien se acepta 0 como forma equivalente.
    if (ins.op == "HLT" || ins.op == "0") {
        if (n != 1) { error = "HLT no acepta operandos."; return false; }
        ins.op = "HLT";
        return true;
    }

    // MOV copia un valor, un registro o un dato de memoria a un destino.
    if (ins.op == "MOV") {
        if (n != 3) { error = "MOV necesita dos operandos."; return false; }
        if (esRegistro(ins.destino) && esRegistro(ins.origen)) return true;
        int direccion, numero;
        if (esRegistro(ins.destino) && esDireccionFuente(ins.origen)) {
            if (direccionMemoria(ins.origen, direccion)) return true;
        }
        if (esRegistro(ins.destino) && convertirNumero(ins.origen, numero)) {
            if (numero >= -128 && numero <= 127) return true;
            error = "El valor de MOV debe estar entre -128 y 127."; return false;
        }
        if (direccionMemoria(ins.destino, direccion) && esRegistro(ins.origen)) return true;
        error = "Operandos invalidos para MOV."; return false;
    }

    // DIV divide un registro entre un numero o divide AX usando un dato de memoria.
    if (ins.op == "DIV") {
        int direccion, numero;
        if (n == 2 && direccionMemoria(ins.destino, direccion)) return true;
        if (n == 3 && esRegistro(ins.destino) && convertirNumero(ins.origen, numero) && numero >= -128 && numero <= 127) return true;
        error = "Usa DIV registro, numero o DIV direccion_de_memoria."; return false;
    }

    // CMP compara un registro con un valor o dato de memoria y actualiza las banderas.
    if (ins.op == "CMP") {
        int direccion, numero;
        if (n != 3 || !esRegistro(ins.destino)) { error = "CMP necesita un registro y un valor."; return false; }
        if (esDireccionFuente(ins.origen) && direccionMemoria(ins.origen, direccion)) return true;
        if (convertirNumero(ins.origen, numero) && numero >= -128 && numero <= 127) return true;
        error = "Valor invalido para CMP."; return false;
    }

    // JMP salta siempre al indice indicado; JMA salta si la bandera Z vale 1.
    if (ins.op == "JMP" || ins.op == "JMA") {
        int destino;
        if (n != 2 || !convertirNumero(ins.destino, destino) || destino < 0 || destino >= MAX_INSTRUCCIONES) {
            error = "El salto debe tener un indice entre 0 y 127."; return false;
        }
        return true;
    }

    // SHL desplaza los bits de un registro hacia la izquierda de 0 a 7 posiciones.
    if (ins.op == "SHL") {
        int cantidad;
        if (n != 3 || !esRegistro(ins.destino) || !convertirNumero(ins.origen, cantidad) || cantidad < 0 || cantidad > 7) {
            error = "SHL usa un registro y una cantidad de 0 a 7."; return false;
        }
        return true;
    }

    // PUSH guarda un registro en la pila; POP recupera el ultimo valor guardado.
    if (ins.op == "PUSH" || ins.op == "POP") {
        if (n != 2 || !esRegistro(ins.destino)) { error = ins.op + " solo acepta AX, BX o CX."; return false; }
        return true;
    }

    error = "Instruccion desconocida: " + ins.op;
    return false;
}

// Lee las instrucciones desde el teclado hasta recibir HLT o alcanzar el limite.
// Comprueba la sintaxis y valida que los saltos apunten a instrucciones existentes.
bool leerPrograma() {
    cantidadInstrucciones = 0;
    bool encontroHLT = false;
    cout << "Escribe las instrucciones y termina con HLT." << endl;

    // Se leen como maximo 128 instrucciones para no salir del arreglo programa.
    while (cantidadInstrucciones < MAX_INSTRUCCIONES) {
        string linea;
        if (!getline(cin, linea)) break;
        Instruccion ins;
        string error;
        if (!analizarInstruccion(linea, ins, error)) {
            if (quitarComentarios(linea).find_first_not_of(" \t\r\n") == string::npos) continue;
            cout << "Error en la instruccion: " << error << endl;
            return false;
        }
        programa[cantidadInstrucciones] = ins;
        cantidadInstrucciones++;
        if (ins.op == "HLT") { encontroHLT = true; break; }
    }

    // Si el usuario olvido HLT, se agrega uno si hay espacio.
    // Si se llenaron las 128 posiciones, la posicion 127 se reserva para HLT.
    if (!encontroHLT) {
        cout << "[!] No se encontro HLT. Se reservara la ultima posicion para HLT." << endl;
        Instruccion hlt;
        hlt.op = "HLT";
        hlt.destino = "";
        hlt.origen = "";
        if (cantidadInstrucciones < MAX_INSTRUCCIONES) {
            programa[cantidadInstrucciones] = hlt;
            cantidadInstrucciones++;
        } else {
            programa[MAX_INSTRUCCIONES - 1] = hlt;
            cout << "[!] La instruccion del indice 127 se reemplazo por HLT." << endl;
        }
    }

    // Comprueba que los saltos no apunten fuera del programa.
    for (int i = 0; i < cantidadInstrucciones; i++) {
        if (programa[i].op == "JMP" || programa[i].op == "JMA") {
            int destino;
            convertirNumero(programa[i].destino, destino);
            if (destino >= cantidadInstrucciones) {
                cout << "Error: el salto apunta fuera del programa." << endl;
                return false;
            }
        }
    }
    return true;
}

// Convierte las instrucciones de texto a codigos numericos que la CPU puede ejecutar.
// Cada instruccion ocupa una fila de RAM: codigo en columna 0 y operando en columna 1.
void ensamblarPrograma() {
    for (int i = 0; i < TAM_RAM; i++) {
        RAM[i][0] = 0;
        RAM[i][1] = 0;
    }

    for (int i = 0; i < cantidadInstrucciones; i++) {
        Instruccion ins = programa[i];
        // codigo representa la operacion; operando guarda el dato o direccion asociada.
        int codigo = 0;
        int operando = 0;
        int numero = 0;
        int direccion = 0;
        int reg = idRegistro(ins.destino);

        if (ins.op == "HLT") codigo = 0;
        else if (ins.op == "MOV") {
            if (esRegistro(ins.destino) && esRegistro(ins.origen)) {
                codigo = 40 + reg;
                operando = idRegistro(ins.origen);
            } else if (esRegistro(ins.destino) && esDireccionFuente(ins.origen)) {
                direccionMemoria(ins.origen, direccion);
                codigo = 20 + reg;
                operando = direccion;
            } else if (esRegistro(ins.destino)) {
                convertirNumero(ins.origen, numero);
                codigo = 10 + reg;
                operando = byteSinSigno(numero);
            } else {
                direccionMemoria(ins.destino, direccion);
                codigo = 30 + idRegistro(ins.origen);
                operando = direccion;
            }
        } else if (ins.op == "DIV") {
            if (esRegistro(ins.destino)) {
                convertirNumero(ins.origen, numero);
                codigo = 50 + reg;
                operando = byteSinSigno(numero);
            } else {
                direccionMemoria(ins.destino, direccion);
                codigo = 60;
                operando = direccion;
            }
        } else if (ins.op == "CMP") {
            if (esDireccionFuente(ins.origen)) {
                direccionMemoria(ins.origen, direccion);
                codigo = 80 + reg;
                operando = direccion;
            } else {
                convertirNumero(ins.origen, numero);
                codigo = 70 + reg;
                operando = byteSinSigno(numero);
            }
        } else if (ins.op == "JMP") {
            convertirNumero(ins.destino, numero); codigo = 90; operando = numero;
        } else if (ins.op == "JMA") {
            convertirNumero(ins.destino, numero); codigo = 91; operando = numero;
        } else if (ins.op == "SHL") {
            convertirNumero(ins.origen, numero); codigo = 100 + reg; operando = numero;
        } else if (ins.op == "PUSH") codigo = 110 + reg;
        else if (ins.op == "POP") codigo = 120 + reg;

        RAM[i][0] = codigo;
        RAM[i][1] = operando;
    }
}

// Actualiza las banderas Z, N, C y O a partir de un resultado aritmetico.
// Z: resultado de 8 bits igual a cero. N: bit mas significativo encendido.
// C: acarreo recibido como parametro. O: resultado fuera del rango -128 a 127.
void actualizarFlags(int resultado, int acarreo = 0) {
    int byte = byteSinSigno(resultado);
    banderas[0] = (byte == 0);
    banderas[1] = (byte >= 128);
    banderas[2] = acarreo;
    banderas[3] = (resultado < -128 || resultado > 127);
}

// Actualiza las banderas despues de comparar/restar a - b.
// No guarda el resultado en un registro: solo modifica las banderas de comparacion.
void actualizarFlagsResta(int a, int b) {
    int resultado = a - b;
    int byte = byteSinSigno(resultado);
    banderas[0] = (byte == 0);
    banderas[1] = (byte >= 128);
    banderas[2] = (byteSinSigno(a) < byteSinSigno(b));
    banderas[3] = (((a ^ b) & (a ^ resultado) & 128) != 0);
}

// Muestra el estado actual de la CPU: registros internos, banderas, registros y pila.
// Se llama despues de cada instruccion ejecutada correctamente.
void imprimirDashboard() {
    // SP baja cada vez que se guarda un valor en la pila.
    // Esta resta calcula cuantos espacios se han utilizado.
    int bytesUsados = 255 - SP;
    if (bytesUsados < 0) bytesUsados = 0;
    if (bytesUsados > 128) bytesUsados = 128;
    double uso = bytesUsados * 100.0 / 128.0;

    cout << "PC: [" << PC << "] | MAR: [" << MAR << "] | MDR: [" << MDR
         << "] | IR: [" << IR_OpCode << ", " << IR_Operando << "]" << endl;
    cout << "Z: " << banderas[0] << " | N: " << banderas[1]
         << " | C: " << banderas[2] << " | O: " << banderas[3] << endl;
    cout << "AX: " << registros[0] << " | BX: " << registros[1]
         << " | CX: " << registros[2] << endl;
    cout << "BP: " << BP << " | SP: " << SP << " | Uso de Pila: "
         << fixed << setprecision(1) << uso << "%" << endl;
    cout << "--------------------------------------------------" << endl;
}

// Ciclo principal de ejecucion de la CPU simulada.
// Repite el ciclo buscar instruccion -> decodificar -> ejecutar hasta HLT o un error.
void ejecutarPrograma() {
    bool ejecutando = true;
    while (ejecutando) {
        if (PC < 0 || PC >= cantidadInstrucciones || PC >= MAX_INSTRUCCIONES) {
            cout << "Error: PC fuera del rango del programa." << endl;
            errorEjecucion = true;
            break;
        }

        // 1. BUSQUEDA: MAR recibe la direccion de la instruccion actual.
        MAR = PC;
        // La CPU lee de RAM el codigo de operacion y el operando.
        IR_OpCode = RAM[PC][0];
        IR_Operando = RAM[PC][1];
        MDR = IR_Operando;
        // Se avanza el PC para apuntar a la siguiente instruccion.
        // Un salto puede cambiarlo mas adelante durante la ejecucion.
        PC++;

        // Los codigos se agrupan por familias: 1=MOV inmediato, 2=MOV memoria, etc.
        // La division entera obtiene la familia y el modulo identifica el registro.
        int familia = IR_OpCode / 10;
        int reg = IR_OpCode % 10;

        // 2. DECODIFICACION Y EJECUCION: se decide que operacion realizar.
        if (IR_OpCode == 0) {
            ejecutando = false;
        // Familia 1: MOV registro, valor inmediato.
        } else if (familia == 1) {
            escribirRegistro(reg, byteConSigno(IR_Operando));
        // Familia 2: MOV registro, dato guardado en memoria.
        } else if (familia == 2) {
            escribirRegistro(reg, byteConSigno(RAM[IR_Operando][0]));
        // Familia 3: MOV direccion_de_memoria, registro.
        } else if (familia == 3) {
            RAM[IR_Operando][0] = byteSinSigno(leerRegistro(reg));
        // Familia 4: MOV registro, otro registro.
        } else if (familia == 4) {
            if (IR_Operando < 1 || IR_Operando > 3) {
                cout << "Error: registro invalido en MOV." << endl;
                errorEjecucion = true; break;
            }
            escribirRegistro(reg, leerRegistro(IR_Operando));
        // Familia 5: DIV registro, divisor inmediato.
        } else if (familia == 5) {
            int divisor = byteConSigno(IR_Operando);
            if (divisor == 0) {
                cout << "Error: division entre cero." << endl;
                errorEjecucion = true; break;
            }
            int resultado = leerRegistro(reg) / divisor;
            escribirRegistro(reg, resultado);
            actualizarFlags(resultado);
        // Codigo 60: divide AX entre el dato indicado por una direccion de memoria.
        } else if (IR_OpCode == 60) {
            int divisor = byteConSigno(RAM[IR_Operando][0]);
            if (divisor == 0) {
                cout << "Error: division entre cero en memoria." << endl;
                errorEjecucion = true; break;
            }
            int resultado = registros[0] / divisor;
            escribirRegistro(1, resultado);
            actualizarFlags(resultado);
        // Familia 7: CMP registro, valor inmediato.
        } else if (familia == 7) {
            actualizarFlagsResta(leerRegistro(reg), byteConSigno(IR_Operando));
        // Familia 8: CMP registro, dato de memoria.
        } else if (familia == 8) {
            actualizarFlagsResta(leerRegistro(reg), byteConSigno(RAM[IR_Operando][0]));
        // Familia 10: SHL, desplazamiento de bits a la izquierda.
        } else if (familia == 10) {
            int cantidad = IR_Operando;
            int antes = leerRegistro(reg);
            if (cantidad > 0) {
                int byte = byteSinSigno(antes);
                banderas[2] = (byte >> (8 - cantidad)) & 1;
                int resultado = byte * (1 << cantidad);
                escribirRegistro(reg, resultado);
                actualizarFlags(resultado, banderas[2]);
            }
        // Familia 11: PUSH, guarda el registro en la posicion actual de la pila.
        } else if (familia == 11) {
            if (SP < INICIO_DATOS) {
                cout << "Error: pila llena." << endl;
                errorEjecucion = true; break;
            }
            RAM[SP][0] = byteSinSigno(leerRegistro(reg));
            RAM[SP][1] = 0;
            SP--;
        // Familia 12: POP, recupera el valor que esta en la cima de la pila.
        } else if (familia == 12) {
            if (SP >= 255) {
                cout << "Error: no hay datos en la pila." << endl;
                errorEjecucion = true; break;
            }
            SP++;
            escribirRegistro(reg, byteConSigno(RAM[SP][0]));
        // Codigo 90: JMP; cambia PC siempre al indice indicado.
        } else if (IR_OpCode == 90) {
            if (IR_Operando >= cantidadInstrucciones) {
                cout << "Error: salto JMP fuera de rango." << endl;
                errorEjecucion = true; break;
            }
            PC = IR_Operando;
        // Codigo 91: JMA; solo salta si Z=1, es decir, si la comparacion dio igualdad.
        } else if (IR_OpCode == 91) {
            if (banderas[0] == 1) {
                if (IR_Operando >= cantidadInstrucciones) {
                    cout << "Error: salto JMA fuera de rango." << endl;
                    errorEjecucion = true; break;
                }
                PC = IR_Operando;
            }
        } else {
            cout << "Error: instruccion no implementada." << endl;
            errorEjecucion = true; break;
        }

        imprimirDashboard();

        // Espera 5 segundos antes de ejecutar la siguiente instruccion y mostrar
        // la siguiente actualizacion del panel. Cambia seconds(5) por seconds(10)
        // si prefieres una pausa de 10 segundos.
        this_thread::sleep_for(chrono::seconds(5));
    }

    if (errorEjecucion) cout << "\n--- CPU DETENIDA POR ERROR ---" << endl;
    else cout << "\n--- CPU DETENIDA (HLT) ---" << endl;
}

// Punto de entrada del programa: aqui comienza la ejecucion del simulador.
int main() {
    cout << "--- SIMULADOR DE CPU ---" << endl;
    cout << "Memoria: 256 posiciones. Instrucciones: 0-127. Datos: 128-255." << endl;
    cout << "Direcciones de datos 000-07F se desplazan sumando 128." << endl;

    // Primero se leen y validan las instrucciones escritas por el usuario.
    if (!leerPrograma()) {
        cout << "No se pudo ensamblar el programa." << endl;
        return 1;
    }

    // Si el programa es valido, se convierte a codigos numericos en la RAM.
    ensamblarPrograma();
    cout << "\nPrograma ensamblado. Iniciando CPU...\n" << endl;
    // Finalmente, se ejecuta el programa instruccion por instruccion.
    ejecutarPrograma();
    return errorEjecucion ? 1 : 0;
}