/*
  VERSION EXPLICADA DEL SIMULADOR DE CPU

  Este archivo tiene la misma logica y las mismas instrucciones que
  Simulador_CPU.cpp. La diferencia principal es que aqui se explica como
  se conectan las partes. Los numeros de linea de las referencias corresponden
  al archivo corto, porque los comentarios de este archivo ocupan mas lineas.

  MAPA RAPIDO DEL ARCHIVO CORTO:
  - Lineas 6-85: tamanos, codigos de operacion, memorias, registros y banderas.
  - Lineas 88-186: comprobaciones de texto, registros, numeros y direcciones.
  - Lineas 189-221: codificacion de MOV.
  - Lineas 224-309: codificacion del resto de las instrucciones.
  - Lineas 312-339: errores de ejecucion y banderas.
  - Lineas 342-403: nombre de instruccion y visualizacion del estado.
  - Lineas 406-608: que hace cada instruccion al ejecutarse.
  - Lineas 611-638: ciclo Fetch-Decode-Execute.
  - Lineas 641-657: mensajes de formato esperado.
  - Lineas 660-727: entrada del usuario y carga del programa.

  DECISIONES DE ESTE SIMULADOR:
  - Cada instruccion ocupa una posicion logica de memoria de instrucciones.
    La estructura guarda el opcode y el operando dentro de esa posicion.
  - Las direcciones de instrucciones se escriben en decimal: JMP 10.
  - Las direcciones de datos se escriben en hexadecimal entre corchetes: [060].
    La direccion logica 0x60 es 96 decimal y se muestra en la posicion fisica
    128 + 96 = 224. El arreglo de datos usa el indice logico 96.
  - Los datos con signo tienen rango de -128 a 127.
  - MOV, DIV, CMP y las operaciones logicas permiten las variantes que acepta
    el codificador. DIV, CMP, AND, OR y XOR reciben un destino de registro y
    como origen otro registro o un numero inmediato.
  - JMA se interpreta como salto si el primer valor de una comparacion CMP es
    mayor que el segundo, comparando numeros con signo.
  - Si una instruccion produce un error durante la ejecucion, V pasa a true y
    la CPU se detiene. Las lineas que no pasan la validacion durante la carga
    se rechazan y no ocupan una posicion de memoria.

  Para aprender, conviene seguir el recorrido en este orden:
  main() -> codificarInstruccion() -> memoriaInstrucciones[] -> ejecutarPrograma()
  -> ejecutarInstruccion() -> registros / memoriaDatos[] / pila[] / banderas.
*/

#include <iostream>
#include <string>
#include <sstream>
using namespace std;

/*
  Estas constantes establecen los limites que usan los arreglos. Se usan
  constantes con nombre para evitar repetir numeros por todo el codigo.
  En este diseno hay hasta 128 instrucciones, 128 posiciones logicas de datos
  y una pila de 128 espacios.
*/
// ===== TAMANOS RAM =====

const int TOTAL_INSTRUCCIONES = 128;
const int TOTAL_DATOS = 128;
const int TAM_PILA = 128;

/*
  Cada instruccion se guarda como un numero llamado opcode. Algunos opcodes
  tienen variantes: por ejemplo MOV AX, 10 y MOV BX, 10 necesitan distinguir
  el registro de destino. Por eso se usa una base y se suma el indice del
  registro (AX=0, BX=1, CX=2).

  Estos valores son codigos internos elegidos para este simulador; no son
  codigos universales de procesadores reales.
*/
// ===== CODIGOS DE OPERACION =====

const int OP_HLT = 0x00;// HLT detiene la CPU; no tiene operando.
const int OP_MOV_NUM = 0x10;// MOV registro, numero; operando = numero.
const int OP_MOV_REG = 0x20;//  MOV registro, registro; operando = indice del segundo registro.
const int OP_MOV_MEM_REG = 0x30;// MOV registro, dato de memoria; operando = direccion logica de memoria.
const int OP_MOV_REG_MEM = 0x40;// MOV dato de memoria, registro; operando = direccion logica de memoria.
const int OP_DIV_REG = 0x50;// DIV registro, registro; operando = indice del segundo registro.
const int OP_DIV_NUM = 0x54;// DIV registro, numero; operando = numero.
const int OP_CMP_REG = 0x60;// CMP registro, registro; operando = indice del segundo registro.
const int OP_CMP_NUM = 0x64;// CMP registro, numero; operando = numero.
const int OP_AND_REG = 0x70;// AND registro, registro; operando = indice del segundo registro.
const int OP_AND_NUM = 0x74;// AND registro, numero; operando = numero.
const int OP_OR_REG = 0x80;// OR registro, registro; operando = indice del segundo registro.
const int OP_OR_NUM = 0x84;// OR registro, numero; operando = numero.
const int OP_XOR_REG = 0x90;// XOR registro, registro; operando = indice del segundo registro.
const int OP_XOR_NUM = 0x94;// XOR registro, numero; operando = numero.
const int OP_JMP = 0xA0;// JMP indice; operando = indice de instruccion.
const int OP_JMA = 0xA1;// JMA indice; operando = indice de instruccion.
const int OP_SHL = 0xB0;// SHL registro, numero; operando = numero de bits a desplazar.
const int OP_PUSH = 0xB4;// PUSH registro; operando = indice del registro a guardar en la pila.
const int OP_POP = 0xB8;// POP registro; operando = indice del registro a recuperar de la pila.

/*
  La estructura agrupa los datos que pertenecen a una sola instruccion.
  opcode indica la operacion; operando guarda el numero, el indice de registro
  o la direccion que la operacion necesita; cargada indica si esa posicion
  realmente contiene una instruccion.

  memoriaInstrucciones usa una posicion por instruccion. Es una representacion
  didactica simplificada, no una RAM de bytes individuales.
*/
// ===== INSTRUCCIONES =====

struct Instruccion {
    int opcode;// Codigo de operacion que indica la instruccion.
    int operando;// Numero, indice de registro o direccion de memoria que la instruccion necesita.
    bool cargada;// Indica si esta posicion de memoria contiene una instruccion valida.
};

Instruccion memoriaInstrucciones[TOTAL_INSTRUCCIONES] = {};// Arreglo de instrucciones; cada posicion logica de memoria contiene una instruccion.

/*
  memoriaDatos guarda los valores de datos; pila guarda los valores apilados.
  Ambos arreglos empiezan llenos de ceros. SP empieza en TAM_PILA porque la
  pila esta vacia y apunta al siguiente espacio libre.
*/
// ===== MEMORIA Y PILA =====

int memoriaDatos[TOTAL_DATOS] = {};
int pila[TAM_PILA] = {};

/*
  registros[0] es AX, registros[1] es BX y registros[2] es CX. Se usa un
  arreglo para compartir una misma forma de acceder a los tres registros.
*/
// ===== REGISTROS GENERALES =====

int registros[3] = {}; // AX, BX, CX

// ===== REGISTROS ESPECIALES =====

int PC = 0;// PC (Program Counter): indica el indice de la siguiente instruccion que se ejecutara.
int MAR = 0;// MAR (Memory Address Register): guarda la direccion de memoria consultada.
int MDR = 0;// MDR (Memory Data Register): guarda el dato leido de memoria.
int IR_OpCode = 0;// IR: registro de instruccion. IR_OpCode guarda el codigo de la instruccion actual.
int IR_Operando = 0;// IR_Operando guarda el valor o la direccion asociada a la instruccion actual.
int SP = TAM_PILA;// SP (Stack Pointer): apunta al espacio superior disponible de la pila.

// ===== BANDERAS =====

bool Z = false;// Z: resultado cero
bool N = false;// N: bit de signo
bool C = false;// C: acarreo o prestamo
bool V = false;// V: desbordamiento con signo o error de ejecucion

/*
  comparacionValida solo se activa cuando se ejecuta CMP. JMA exige que esta
  variable sea true para evitar saltar usando una comparacion que no existe.
  cpuDetenida marca el final normal y errorEjecucion marca un fallo fatal.
*/
// ===== ESTADO DE LA CPU =====

bool comparacionValida = false;// Indica si la ultima comparacion realizada es valida
bool cpuDetenida = false;// Indica si la CPU ha terminado su ejecucion normal
bool errorEjecucion = false;// Indica si ha ocurrido un error fatal durante la ejecucion

/*
  Este arreglo es la lista de nombres que el usuario puede escribir.
  Estar en la lista no significa que una instruccion ya sea valida en todos
  sus detalles: despues se comprueban sus operandos y se codifica.
*/
// ===== INSTRUCCIONES VALIDAS =====

const int CANTIDAD_INSTRUCCIONES_VALIDAS = 12;

string instruccionesValidas[CANTIDAD_INSTRUCCIONES_VALIDAS] = {
    "MOV", "DIV", "CMP", "JMP",
    "JMA", "SHL", "PUSH", "POP",
    "AND", "OR", "XOR", "HLT"
};

// ===== VALIDACIONES Y CONVERSIONES =====

/*
  Relacion: main() la llama antes de validar los operandos (archivo corto,
  alrededor de la linea 660). Consulta instruccionesValidas[] y devuelve true
  si encuentra una coincidencia exacta. Por eso se exige escribir MAYUSCULAS.
*/
bool instruccionExiste(string nombre) {
    for (int i = 0; i < CANTIDAD_INSTRUCCIONES_VALIDAS; i++) {
        if (nombre == instruccionesValidas[i]) {
            return true;
        }
    }
    return false;
}

/*
  Comprueba el nombre del registro, pero no lo convierte a indice.
  Para convertir AX/BX/CX a 0/1/2 se usa codigoRegistro(), inmediatamente abajo.
  main() utiliza esta comprobacion al validar MOV (archivo corto, alrededor de
  las lineas 660-725), y codificarMOV() utiliza la conversion a indice.
*/
bool esRegistro(string texto) {
    return texto == "AX" || texto == "BX" || texto == "CX";
}

/*
  Traduce el nombre que escribio el usuario al indice del arreglo registros[].
  AX -> 0, BX -> 1, CX -> 2. Devuelve -1 si no coincide. La funcion no cambia
  ningun registro: solo devuelve un numero que otras funciones pueden utilizar.
  Relacion principal: codificarMOV() y codificarInstruccion(), cerca de las
  lineas 189 y 224 del archivo corto.
*/
int codigoRegistro(string texto) {
    if (texto == "AX") return 0;
    if (texto == "BX") return 1;
    if (texto == "CX") return 2;
    return -1;
}

/*
  Recibe texto porque el usuario aun no ha entregado un numero como int.
  stringstream intenta leer un entero. Despues se intenta leer un caracter
  adicional: si aparece, el texto tenia algo extra (por ejemplo "12abc").
  Solo se aceptan valores entre -128 y 127. Esta funcion comprueba, no convierte
  el texto para guardarlo. Se usa al codificar operandos inmediatos y saltos.
*/
bool esNumero8Bits(string texto) {
    stringstream conversion(texto);
    int numero;
    char sobrante;

    if (!(conversion >> numero)) return false;
    if (conversion >> sobrante) return false;

    return numero >= -128 && numero <= 127;
}

/*
  Convierte texto decimal en int. Se llama despues de comprobar el texto con
  esNumero8Bits(), asi que la funcion se puede mantener sencilla. No deberia
  usarse sola con texto que no haya sido validado.
*/
int convertirNumeroDecimal(string texto) {
    stringstream conversion(texto);
    int numero = 0;
    conversion >> numero;
    return numero;
}

/*
  Comprueba dos cosas: que la direccion este entre corchetes y que su contenido
  sea un hexadecimal entre 0x00 y 0x7F. Por ejemplo [060] representa 0x60,
  que equivale a 96 en decimal. La direccion logica se usa como indice de
  memoriaDatos[]; cuando MAR muestra la direccion fisica, se suma 128.
  Se utiliza en codificarMOV() para leer o escribir datos.
*/
bool esDireccionMemoria(string texto) {
    if (texto.size() < 3 || texto[0] != '[' || texto[texto.size() - 1] != ']') {
        return false;
    }

    string contenido = texto.substr(1, texto.size() - 2);
    stringstream conversion(contenido);
    int direccion;
    char sobrante;

    if (!(conversion >> hex >> direccion)) return false;
    if (conversion >> sobrante) return false;

    return direccion >= 0 && direccion <= 0x7F;
}

/*
  Retira los corchetes y convierte el contenido hexadecimal a int. La validacion
  se hace antes mediante esDireccionMemoria(); esta funcion solo transforma.
  Por ejemplo, convertirDireccionHex("[060]") devuelve 96 decimal.
*/
int convertirDireccionHex(string texto) {
    string contenido = texto.substr(1, texto.size() - 2);
    stringstream conversion(contenido);
    int direccion = 0;
    conversion >> hex >> direccion;
    return direccion;
}

/*
  Extrae una sola palabra y la deja en resultado. El & indica que resultado es
  una referencia: la funcion puede escribir en la variable que le paso, sin
  devolver la palabra mediante return. Devuelve true si hay exactamente un
  elemento y false si esta vacio o tiene mas texto.
*/
bool obtenerUnSoloElemento(string texto, string &resultado) {
    stringstream separarTexto(texto);
    string extra;

    if (!(separarTexto >> resultado)) return false;
    if (separarTexto >> extra) return false;

    return true;
}

/*
  Recibe el texto que queda despues del nombre de la instruccion. Primero
  cuenta comas y exige exactamente una. Luego separa ambos lados y comprueba
  que cada lado contenga un solo elemento. Asi se aceptan formas como
  "AX, 10", pero no "AX 10", "AX, 10, BX" o "AX, 10 extra".
  codificarMOV() y codificarInstruccion() reutilizan esta funcion.
*/
bool separarDosOperandos(string texto, string &primero, string &segundo) {
    int cantidadComas = 0;
    for (char caracter : texto) {
        if (caracter == ',') cantidadComas++;
    }

    if (cantidadComas != 1) return false;

    stringstream separar(texto);
    string primerTexto;
    string segundoTexto;

    if (!getline(separar, primerTexto, ',')) return false;
    if (!getline(separar, segundoTexto, ',')) return false;

    if (!obtenerUnSoloElemento(primerTexto, primero)) return false;
    if (!obtenerUnSoloElemento(segundoTexto, segundo)) return false;

    return true;
}

bool sinOperandos(string texto) {
    stringstream separar(texto);
    string extra;
    return !(separar >> extra);
}

/*
  Esta funcion transforma las cuatro formas elegidas para MOV a un par de
  numeros: opcode y operando. No modifica AX/BX/CX ni la memoria; solo prepara
  los datos que despues se guardaran en memoriaInstrucciones[].
*/
// ===== CODIFICAR MOV =====

/*
  Las referencias int & permiten que la funcion escriba los dos resultados
  directamente en las variables de quien la llama. El bool indica si pudo
  codificar la instruccion. Relacion: codificarInstruccion() llama a esta
  funcion y main() llama a codificarInstruccion() (archivo corto, lineas 224
  y 660, respectivamente).

  Familias de opcode: 0x10 = registro/numero; 0x20 = registro/registro;
  0x30 = memoria a registro; 0x40 = registro a memoria. En cada familia se
  suma el indice de AX/BX/CX. El operando contiene el valor o indice restante.
*/
bool codificarMOV(string destino, string origen, int &opcode, int &operando) {
    int idDestino = codigoRegistro(destino);
    int idOrigen = codigoRegistro(origen);

    if (idDestino != -1) {
        if (idOrigen != -1) {
            opcode = OP_MOV_REG + idDestino;
            operando = idOrigen;
            return true;
        }

        if (esNumero8Bits(origen)) {
            opcode = OP_MOV_NUM + idDestino;
            operando = convertirNumeroDecimal(origen);
            return true;
        }

        if (esDireccionMemoria(origen)) {
            opcode = OP_MOV_MEM_REG + idDestino;
            operando = convertirDireccionHex(origen);
            return true;
        }
    }

    if (esDireccionMemoria(destino) && idOrigen != -1) {
        opcode = OP_MOV_REG_MEM + idOrigen;
        operando = convertirDireccionHex(destino);
        return true;
    }

    return false;
}

/*
  Esta es la puerta de entrada para validar y codificar cada instruccion.
  Para MOV delega en codificarMOV(); para DIV/CMP/AND/OR/XOR comparte la
  validacion de dos operandos; JMP/JMA usan una direccion de instruccion decimal;
  SHL/PUSH/POP reciben un solo registro. Si algo no encaja, devuelve false y
  main() rechaza esa linea sin ocupar una posicion de memoria.
*/
// ===== CODIFICAR INSTRUCCIONES =====

/*
  Se llama desde main() durante la carga del programa. Es importante separar
  carga de ejecucion: aqui se determina como representar la instruccion, pero
  aun no se cambian registros ni datos. Solo si devuelve true, main() guarda
  opcode, operando y cargada=true en memoriaInstrucciones[].
*/
bool codificarInstruccion(string nombre, string operandosTexto, int &opcode, int &operando) {
    string primero;
    string segundo;
    int idDestino;
    int idOrigen;
    int codigoBase = -1;
    int codigoNumero = -1;

    // HLT se codifica igual que las demas instrucciones, pero exige texto vacio.
    if (nombre == "HLT") {
        if (!sinOperandos(operandosTexto)) return false;
        opcode = OP_HLT;
        operando = 0;
        return true;
    }

    if (nombre == "MOV") {
        if (!separarDosOperandos(operandosTexto, primero, segundo)) return false;
        return codificarMOV(primero, segundo, opcode, operando);
    }

    if (nombre == "DIV") {
        codigoBase = OP_DIV_REG;
        codigoNumero = OP_DIV_NUM;
    } else if (nombre == "CMP") {
        codigoBase = OP_CMP_REG;
        codigoNumero = OP_CMP_NUM;
    } else if (nombre == "AND") {
        codigoBase = OP_AND_REG;
        codigoNumero = OP_AND_NUM;
    } else if (nombre == "OR") {
        codigoBase = OP_OR_REG;
        codigoNumero = OP_OR_NUM;
    } else if (nombre == "XOR") {
        codigoBase = OP_XOR_REG;
        codigoNumero = OP_XOR_NUM;
    }

    if (codigoBase != -1) {
        if (!separarDosOperandos(operandosTexto, primero, segundo)) return false;

        idDestino = codigoRegistro(primero);
        idOrigen = codigoRegistro(segundo);
        if (idDestino == -1) return false;

        if (idOrigen != -1) {
            opcode = codigoBase + idDestino;
            operando = idOrigen;
            return true;
        }

        if (esNumero8Bits(segundo)) {
            opcode = codigoNumero + idDestino;
            operando = convertirNumeroDecimal(segundo);
            return true;
        }

        return false;
    }

    if (nombre == "JMP" || nombre == "JMA") {
        if (!obtenerUnSoloElemento(operandosTexto, primero)) return false;
        if (!esNumero8Bits(primero)) return false;

        operando = convertirNumeroDecimal(primero);
        if (operando < 0 || operando >= TOTAL_INSTRUCCIONES) return false;

        opcode = (nombre == "JMP") ? OP_JMP : OP_JMA;
        return true;
    }

    if (nombre == "SHL" || nombre == "PUSH" || nombre == "POP") {
        if (!obtenerUnSoloElemento(operandosTexto, primero)) return false;
        idDestino = codigoRegistro(primero);
        if (idDestino == -1) return false;

        if (nombre == "SHL") opcode = OP_SHL + idDestino;
        else if (nombre == "PUSH") opcode = OP_PUSH + idDestino;
        else opcode = OP_POP + idDestino;

        operando = 0;
        return true;
    }

    return false;
}

/*
  Los errores de sintaxis se rechazan durante la carga y permiten seguir
  escribiendo. Esta funcion es para errores que aparecen cuando la CPU ya esta
  ejecutando: registra el error, activa V y detiene la ejecucion. La direccion
  que recibe identifica la instruccion que fallo aunque PC ya se haya movido.
*/
// ===== ERRORES DE EJECUCION =====

/*
  Se llama, por ejemplo, cuando DIV recibe cero, una operacion desborda el rango,
  la pila se llena o vacia, o un salto apunta a una posicion sin instruccion.
  Devuelve false para que ejecutarPrograma() sepa que debe detener el ciclo.
*/
bool informarErrorEjecucion(string mensaje, int direccionInstruccion) {
    cout << "\nERROR DE EJECUCION en la instruccion " << direccionInstruccion << ": " << mensaje << endl;
    V = true;
    errorEjecucion = true;
    cpuDetenida = true;
    return false;
}

// ===== BANDERAS =====

/*
  Para las operaciones logicas necesitamos los ocho bits de un numero.
  Esta funcion transforma el valor en un numero de 0 a 255 que representa esos
  bits. Por ejemplo -1 pasa a 255 (11111111 en binario).
*/
int convertirAByte(int valor) {
    int bits = valor % 256;
    if (bits < 0) bits += 256;
    return bits;
}

/*
  Hace el proceso inverso: interpreta un valor de 0 a 255 como un numero con
  signo de 8 bits. Los valores 128-255 pasan a -128..-1. Asi AND/OR/XOR pueden
  trabajar con ocho bits y guardar el resultado con signo.
*/
int convertirByteASigno(int bits) {
    if (bits >= 128) return bits - 256;
    return bits;
}

/*
  Actualiza las banderas comunes despues de un resultado exitoso de DIV o una
  operacion logica/SHL. Z se activa si es cero y N si es negativo. Aqui C y V
  se limpian; SHL vuelve a calcular C despues porque tiene una regla propia.
  MOV, PUSH, POP y los saltos no actualizan las banderas.
*/
void actualizarBanderasResultado(int resultado) {
    Z = (resultado == 0);
    N = (resultado < 0);
    C = false;
    V = false;
}

/*
  Convierte el opcode interno en un nombre legible. No ejecuta la instruccion;
  solo sirve para que la pantalla diga, por ejemplo, "Ejecutando instruccion 2: DIV".
*/
// ===== NOMBRE DE LA INSTRUCCION =====

/*
  Se usa en ejecutarPrograma() justo despues de buscar una instruccion. Los
  rangos corresponden a las familias definidas al principio del archivo.
*/
string nombreInstruccionOpcode(int opcode) {
    if (opcode == OP_HLT) return "HLT";
    if ((opcode >= OP_MOV_NUM && opcode < OP_MOV_NUM + 3) ||
        (opcode >= OP_MOV_REG && opcode < OP_MOV_REG + 3) ||
        (opcode >= OP_MOV_MEM_REG && opcode < OP_MOV_MEM_REG + 3) ||
        (opcode >= OP_MOV_REG_MEM && opcode < OP_MOV_REG_MEM + 3)) return "MOV";
    if ((opcode >= OP_DIV_REG && opcode < OP_DIV_REG + 3) ||
        (opcode >= OP_DIV_NUM && opcode < OP_DIV_NUM + 3)) return "DIV";
    if ((opcode >= OP_CMP_REG && opcode < OP_CMP_REG + 3) ||
        (opcode >= OP_CMP_NUM && opcode < OP_CMP_NUM + 3)) return "CMP";
    if ((opcode >= OP_AND_REG && opcode < OP_AND_REG + 3) ||
        (opcode >= OP_AND_NUM && opcode < OP_AND_NUM + 3)) return "AND";
    if ((opcode >= OP_OR_REG && opcode < OP_OR_REG + 3) ||
        (opcode >= OP_OR_NUM && opcode < OP_OR_NUM + 3)) return "OR";
    if ((opcode >= OP_XOR_REG && opcode < OP_XOR_REG + 3) ||
        (opcode >= OP_XOR_NUM && opcode < OP_XOR_NUM + 3)) return "XOR";
    if (opcode == OP_JMP) return "JMP";
    if (opcode == OP_JMA) return "JMA";
    if (opcode >= OP_SHL && opcode < OP_SHL + 3) return "SHL";
    if (opcode >= OP_PUSH && opcode < OP_PUSH + 3) return "PUSH";
    if (opcode >= OP_POP && opcode < OP_POP + 3) return "POP";
    return "DESCONOCIDA";
}

/*
  La salida muestra los registros especiales, registros generales, banderas,
  memoria de datos no nula y contenido ocupado de la pila. Se muestra despues
  de cada instruccion para que se pueda observar como cambia la CPU.
*/
// ===== MOSTRAR ESTADO =====

/*
  Esta funcion solo muestra informacion: no modifica el estado del simulador.
  La direccion fisica de los datos se imprime como 128 + indice logico. Para
  evitar una pantalla enorme, solo se muestran las posiciones de memoria de
  datos distintas de cero. En la pila se muestran las posiciones ocupadas,
  desde SP hasta el final del arreglo.
*/
void mostrarEstadoCPU() {
    cout << "\n--- ESTADO DE LA CPU ---" << endl;
    cout << "PC = " << PC << " | MAR = " << MAR << " | MDR = " << MDR << endl;
    cout << "IR_OpCode = 0x" << hex << IR_OpCode << dec
         << " | IR_Operando = " << IR_Operando << endl;
    cout << "AX = " << registros[0] << " | BX = " << registros[1]
         << " | CX = " << registros[2] << endl;
    cout << "SP = " << SP << endl;
    cout << "Z = " << Z << " | N = " << N << " | C = " << C << " | V = " << V << endl;
    cout << "Comparacion valida = " << comparacionValida << endl;

    bool hayDatos = false;
    for (int i = 0; i < TOTAL_DATOS; i++) {
        if (memoriaDatos[i] != 0) {
            if (!hayDatos) {
                cout << "Memoria de datos (direccion fisica = 128 + direccion):" << endl;
                hayDatos = true;
            }
            cout << "  Direccion " << TOTAL_INSTRUCCIONES + i
                 << " (hexadecimal " << hex << (i < 16 ? "0" : "") << i << dec
                 << ") = " << memoriaDatos[i] << endl;
        }
    }
    if (!hayDatos) cout << "Memoria de datos: todas las posiciones contienen 0." << endl;

    if (SP < TAM_PILA) {
        cout << "Pila (de la cima hacia abajo):" << endl;
        for (int i = SP; i < TAM_PILA; i++) {
            cout << "  Posicion " << i << " = " << pila[i] << endl;
        }
    } else {
        cout << "Pila vacia." << endl;
    }
    cout << "------------------------" << endl;
}

/*
  Aqui ocurre el trabajo real de las instrucciones. ejecutarPrograma() coloca
  el opcode y el operando actuales en IR_OpCode e IR_Operando, y despues llama
  a esta funcion. Cada bloque reconoce una familia de opcode y realiza la
  operacion correspondiente sobre registros, memoria, pila o banderas.
*/
// ===== EJECUTAR INSTRUCCIONES =====

/*
  El parametro direccionInstruccion es la posicion que se esta ejecutando, no
  el PC actual: PC normalmente ya se incremento durante Fetch. Se pasa por
  separado para que los errores indiquen la direccion correcta.
*/
bool ejecutarInstruccion(int direccionInstruccion) {
    int idDestino;
    int idOrigen;
    int valorOrigen;
    int resultado;
    int bitsA;
    int bitsB;
    int bitsResultado;
    int valorA;
    int valorB;
    int direccionDatos;
    bool mayor;

    if (IR_OpCode == OP_HLT) {
        cout << "HLT: la CPU ha terminado." << endl;
        cpuDetenida = true;
        return true;
    }

    /*
       MOV con numero: el opcode identifica el registro destino y el operando ya es el numero validado durante la carga.
    */
    // MOV registro, numero
    if (IR_OpCode >= OP_MOV_NUM && IR_OpCode < OP_MOV_NUM + 3) {
        idDestino = IR_OpCode - OP_MOV_NUM;
        registros[idDestino] = IR_Operando;
        return true;
    }

    /*
       MOV entre registros: el opcode indica el destino y el operando contiene el indice del registro de origen.
    */
    // MOV registro, registro
    if (IR_OpCode >= OP_MOV_REG && IR_OpCode < OP_MOV_REG + 3) {
        idDestino = IR_OpCode - OP_MOV_REG;
        idOrigen = IR_Operando;
        if (idOrigen < 0 || idOrigen >= 3) return informarErrorEjecucion("registro de origen invalido en MOV", direccionInstruccion);
        registros[idDestino] = registros[idOrigen];
        return true;
    }

    /*
       Lectura de memoria: el operando es una direccion logica; MAR muestra 128 + direccion y MDR recibe el dato.
    */
    // MOV registro, memoria
    if (IR_OpCode >= OP_MOV_MEM_REG && IR_OpCode < OP_MOV_MEM_REG + 3) {
        idDestino = IR_OpCode - OP_MOV_MEM_REG;
        direccionDatos = IR_Operando;
        if (direccionDatos < 0 || direccionDatos >= TOTAL_DATOS) return informarErrorEjecucion("direccion de memoria invalida en MOV", direccionInstruccion);
        MAR = TOTAL_INSTRUCCIONES + direccionDatos;
        MDR = memoriaDatos[direccionDatos];
        registros[idDestino] = MDR;
        return true;
    }

    /*
       Escritura de memoria: MAR recibe la direccion fisica y MDR toma el valor del registro antes de guardarlo.
    */
    // MOV memoria, registro
    if (IR_OpCode >= OP_MOV_REG_MEM && IR_OpCode < OP_MOV_REG_MEM + 3) {
        idOrigen = IR_OpCode - OP_MOV_REG_MEM;
        direccionDatos = IR_Operando;
        if (direccionDatos < 0 || direccionDatos >= TOTAL_DATOS) return informarErrorEjecucion("direccion de memoria invalida en MOV", direccionInstruccion);
        MAR = TOTAL_INSTRUCCIONES + direccionDatos;
        MDR = registros[idOrigen];
        memoriaDatos[direccionDatos] = MDR;
        return true;
    }

    /*
       DIV guarda el cociente en el primer registro. Divisor cero o resultado fuera de -128..127 detiene la CPU y activa V.
    */
    // DIV registro, registro o numero
    if ((IR_OpCode >= OP_DIV_REG && IR_OpCode < OP_DIV_REG + 3) ||
        (IR_OpCode >= OP_DIV_NUM && IR_OpCode < OP_DIV_NUM + 3)) {
        bool origenEsRegistro = IR_OpCode < OP_DIV_NUM;
        idDestino = origenEsRegistro ? IR_OpCode - OP_DIV_REG : IR_OpCode - OP_DIV_NUM;
        if (origenEsRegistro) {
            idOrigen = IR_Operando;
            if (idOrigen < 0 || idOrigen >= 3) return informarErrorEjecucion("registro de origen invalido en DIV", direccionInstruccion);
            valorOrigen = registros[idOrigen];
        } else {
            valorOrigen = IR_Operando;
        }
        if (valorOrigen == 0) return informarErrorEjecucion("division entre cero", direccionInstruccion);
        resultado = registros[idDestino] / valorOrigen;
        if (resultado < -128 || resultado > 127) return informarErrorEjecucion("desbordamiento en DIV: el resultado no cabe en 8 bits con signo", direccionInstruccion);
        registros[idDestino] = resultado;
        actualizarBanderasResultado(resultado);
        comparacionValida = false;
        return true;
    }

    /*
       CMP compara sin modificar los registros. Calcula las banderas que JMA usa para saber si el primer valor es mayor con signo.
    */
    // CMP registro, registro o numero
    if ((IR_OpCode >= OP_CMP_REG && IR_OpCode < OP_CMP_REG + 3) ||
        (IR_OpCode >= OP_CMP_NUM && IR_OpCode < OP_CMP_NUM + 3)) {
        bool origenEsRegistro = IR_OpCode < OP_CMP_NUM;
        idDestino = origenEsRegistro ? IR_OpCode - OP_CMP_REG : IR_OpCode - OP_CMP_NUM;
        valorA = registros[idDestino];
        if (origenEsRegistro) {
            idOrigen = IR_Operando;
            if (idOrigen < 0 || idOrigen >= 3) return informarErrorEjecucion("registro de origen invalido en CMP", direccionInstruccion);
            valorB = registros[idOrigen];
        } else {
            valorB = IR_Operando;
        }
        resultado = valorA - valorB;
        bitsResultado = convertirAByte(resultado);
        Z = (valorA == valorB);
        N = (bitsResultado >= 128);
        C = (convertirAByte(valorA) >= convertirAByte(valorB));
        V = ((valorA >= 0 && valorB < 0 && resultado > 127) ||
             (valorA < 0 && valorB >= 0 && resultado < -128));
        comparacionValida = true;
        return true;
    }

    /*
       Las operaciones logicas usan los ocho bits de los operandos y sustituyen el primer registro por el resultado.
    */
    // AND, OR y XOR
    bool esOperacionLogica = false;
    bool origenEsRegistro = false;
    string nombreLogico;
    int baseRegistro = -1;
    int baseNumero = -1;

    if (IR_OpCode >= OP_AND_REG && IR_OpCode < OP_AND_REG + 3) {
        esOperacionLogica = true; nombreLogico = "AND"; baseRegistro = OP_AND_REG; baseNumero = OP_AND_NUM;
    } else if (IR_OpCode >= OP_AND_NUM && IR_OpCode < OP_AND_NUM + 3) {
        esOperacionLogica = true; nombreLogico = "AND"; baseRegistro = OP_AND_REG; baseNumero = OP_AND_NUM;
    } else if (IR_OpCode >= OP_OR_REG && IR_OpCode < OP_OR_REG + 3) {
        esOperacionLogica = true; nombreLogico = "OR"; baseRegistro = OP_OR_REG; baseNumero = OP_OR_NUM;
    } else if (IR_OpCode >= OP_OR_NUM && IR_OpCode < OP_OR_NUM + 3) {
        esOperacionLogica = true; nombreLogico = "OR"; baseRegistro = OP_OR_REG; baseNumero = OP_OR_NUM;
    } else if (IR_OpCode >= OP_XOR_REG && IR_OpCode < OP_XOR_REG + 3) {
        esOperacionLogica = true; nombreLogico = "XOR"; baseRegistro = OP_XOR_REG; baseNumero = OP_XOR_NUM;
    } else if (IR_OpCode >= OP_XOR_NUM && IR_OpCode < OP_XOR_NUM + 3) {
        esOperacionLogica = true; nombreLogico = "XOR"; baseRegistro = OP_XOR_REG; baseNumero = OP_XOR_NUM;
    }

    if (esOperacionLogica) {
        origenEsRegistro = (IR_OpCode >= baseRegistro && IR_OpCode < baseRegistro + 3);
        idDestino = origenEsRegistro ? IR_OpCode - baseRegistro : IR_OpCode - baseNumero;
        if (origenEsRegistro) {
            idOrigen = IR_Operando;
            if (idOrigen < 0 || idOrigen >= 3) return informarErrorEjecucion("registro de origen invalido en operacion logica", direccionInstruccion);
            valorOrigen = registros[idOrigen];
        } else {
            valorOrigen = IR_Operando;
        }
        bitsA = convertirAByte(registros[idDestino]);
        bitsB = convertirAByte(valorOrigen);

        if (nombreLogico == "AND") bitsResultado = bitsA & bitsB;
        else if (nombreLogico == "OR") bitsResultado = bitsA | bitsB;
        else bitsResultado = bitsA ^ bitsB;

        resultado = convertirByteASigno(bitsResultado);
        registros[idDestino] = resultado;
        actualizarBanderasResultado(resultado);
        comparacionValida = false;
        return true;
    }

    /*
       JMP cambia PC a la direccion decimal indicada; solo acepta una posicion que contenga una instruccion cargada.
    */
    // JMP
    if (IR_OpCode == OP_JMP) {
        if (IR_Operando < 0 || IR_Operando >= TOTAL_INSTRUCCIONES || !memoriaInstrucciones[IR_Operando].cargada) {
            return informarErrorEjecucion("JMP apunta a una posicion sin instruccion", direccionInstruccion);
        }
        PC = IR_Operando;
        return true;
    }

    /*
       JMA necesita CMP. Para valores con signo, salta si Z es falsa y N coincide con V.
    */
    // JMA: salto si el resultado de CMP indica que el primer valor es mayor
    if (IR_OpCode == OP_JMA) {
        if (!comparacionValida) return informarErrorEjecucion("JMA necesita una comparacion previa con CMP", direccionInstruccion);
        mayor = (!Z && N == V);
        if (mayor) {
            if (IR_Operando < 0 || IR_Operando >= TOTAL_INSTRUCCIONES || !memoriaInstrucciones[IR_Operando].cargada) {
                return informarErrorEjecucion("JMA apunta a una posicion sin instruccion", direccionInstruccion);
            }
            PC = IR_Operando;
        }
        return true;
    }

    /*
       SHL multiplica por dos y produce un error si el resultado sale del rango con signo de 8 bits. C recibe el bit alto original.
    */
    // SHL: desplaza un registro un bit a la izquierda
    if (IR_OpCode >= OP_SHL && IR_OpCode < OP_SHL + 3) {
        idDestino = IR_OpCode - OP_SHL;
        valorA = registros[idDestino];
        resultado = valorA * 2;
        if (resultado < -128 || resultado > 127) return informarErrorEjecucion("desbordamiento en SHL: el resultado no cabe en 8 bits con signo", direccionInstruccion);
        registros[idDestino] = resultado;
        actualizarBanderasResultado(resultado);
        C = (convertirAByte(valorA) >= 128);
        comparacionValida = false;
        return true;
    }

    /*
       PUSH reduce SP antes de guardar el registro. Si SP ya esta en cero, la pila esta llena.
    */
    // PUSH
    if (IR_OpCode >= OP_PUSH && IR_OpCode < OP_PUSH + 3) {
        idOrigen = IR_OpCode - OP_PUSH;
        if (SP <= 0) return informarErrorEjecucion("la pila esta llena", direccionInstruccion);
        SP--;
        pila[SP] = registros[idOrigen];
        return true;
    }

    /*
       POP recupera el valor de la cima y luego aumenta SP. Si SP ya vale TAM_PILA, la pila esta vacia.
    */
    // POP
    if (IR_OpCode >= OP_POP && IR_OpCode < OP_POP + 3) {
        idDestino = IR_OpCode - OP_POP;
        if (SP >= TAM_PILA) return informarErrorEjecucion("la pila esta vacia", direccionInstruccion);
        registros[idDestino] = pila[SP];
        SP++;
        return true;
    }

    return informarErrorEjecucion("codigo de operacion desconocido", direccionInstruccion);
}

/*
  Este ciclo recorre el programa cargado. Fetch busca la instruccion indicada
  por PC y la coloca en los registros IR. PC se incrementa para apuntar a la
  siguiente posicion. Decode queda representado por reconocer el opcode dentro
  de ejecutarInstruccion(); Execute es el cambio de estado de esa funcion.
  La implementacion es intencionalmente sencilla: no simula cada micro-paso
  electrico de un procesador real.
*/
// ===== CICLO FETCH - DECODE - EXECUTE =====

/*
  Solo se llama cuando ya se cargo un HLT correctamente. Comprueba que PC apunte
  a una instruccion cargada, copia sus campos a IR, ejecuta y muestra el estado.
  JMP/JMA pueden reemplazar el valor de PC y cambiar el recorrido normal.
*/
void ejecutarPrograma() {
    while (!cpuDetenida && !errorEjecucion) {
        if (PC < 0 || PC >= TOTAL_INSTRUCCIONES || !memoriaInstrucciones[PC].cargada) {
            informarErrorEjecucion("PC apunta a una posicion sin instruccion", PC);
            mostrarEstadoCPU();
            break;
        }

        int direccionInstruccion = PC;
        MAR = PC;
        IR_OpCode = memoriaInstrucciones[PC].opcode;
        IR_Operando = memoriaInstrucciones[PC].operando;
        PC++;

        cout << "\nEjecutando instruccion " << direccionInstruccion
             << ": " << nombreInstruccionOpcode(IR_OpCode) << endl;
        bool ejecucionCorrecta = ejecutarInstruccion(direccionInstruccion);
        mostrarEstadoCPU();
        if (!ejecucionCorrecta) break;
    }

    if (errorEjecucion) {
        cout << "Ejecucion detenida por un error." << endl;
    } else {
        cout << "Ejecucion finalizada." << endl;
    }
}

/*
  Estos ejemplos se imprimen cuando una linea no cumple el formato esperado.
  Ayudan a corregir la entrada y volver a intentarlo sin perder las instrucciones
  que ya se habian cargado correctamente.
*/
// ===== FORMATO DE INSTRUCCIONES =====

/*
  La funcion recibe el nombre ya reconocido de la instruccion y enseña una o
  dos formas validas de escribirla. Los formatos aqui son los mismos que acepta
  codificarInstruccion(); si se cambia uno, hay que actualizar el otro tambien.
*/
void mostrarFormatoEsperado(string nombre) {
    cout << "Formato esperado: ";
    if (nombre == "MOV") cout << "MOV AX, 10 | MOV AX, BX | MOV AX, [060] | MOV [060], AX";
    else if (nombre == "DIV") cout << "DIV AX, BX o DIV AX, 2";
    else if (nombre == "CMP") cout << "CMP AX, BX o CMP AX, 2";
    else if (nombre == "JMP") cout << "JMP 10 (direccion decimal de instruccion)";
    else if (nombre == "JMA") cout << "JMA 10 (salta si CMP indica que el primer valor es mayor)";
    else if (nombre == "SHL") cout << "SHL AX";
    else if (nombre == "PUSH") cout << "PUSH AX";
    else if (nombre == "POP") cout << "POP AX";
    else if (nombre == "AND") cout << "AND AX, BX o AND AX, 10";
    else if (nombre == "OR") cout << "OR AX, BX o OR AX, 10";
    else if (nombre == "XOR") cout << "XOR AX, BX o XOR AX, 10";
    else if (nombre == "HLT") cout << "HLT";
    cout << endl;
}

/*
  main() organiza las fases generales: leer texto, reconocer el nombre, validar
  y codificar, guardar la instruccion y finalmente iniciar la CPU. Las lineas
  invalidas no incrementan cantidadInstrucciones, por lo que no dejan huecos.
  HLT se guarda como la ultima instruccion; no ejecuta la CPU en este primer
  ciclo de entrada, sino que marca el final de la carga.
*/
// ===== PROGRAMA PRINCIPAL =====

/*
  Esta es la unica funcion que arranca el programa. codificarInstruccion() se
  usa durante la carga; despues ejecutarPrograma() recorre lo que quedo guardado.
  Si el usuario termina la entrada o se llena la memoria sin escribir HLT, el
  programa no empieza a ejecutar, para evitar correr una carga incompleta.
*/
int main() {
    string linea;
    int cantidadInstrucciones = 0;
    bool programaCargado = false;

    cout << "===== SIMULADOR DE CPU =====" << endl;
    cout << "Escribe las instrucciones en MAYUSCULAS." << endl;
    cout << "Direccion de instruccion: decimal de 0 a 127." << endl;
    cout << "Direccion de datos: hexadecimal entre corchetes, por ejemplo [060]." << endl;
    cout << "Escribe HLT para terminar de cargar y ejecutar el programa." << endl;

    while (cantidadInstrucciones < TOTAL_INSTRUCCIONES) {
        cout << "\n[" << cantidadInstrucciones << "] > ";
        if (!getline(cin, linea)) {
            cout << "Entrada finalizada sin HLT. No se ejecutara el programa." << endl;
            break;
        }

        stringstream entrada(linea);
        string nombre;
        if (!(entrada >> nombre)) {
            cout << "Error: linea vacia." << endl;
            continue;
        }

        string operandosTexto;
        getline(entrada, operandosTexto);

        if (!instruccionExiste(nombre)) {
            cout << "Error: instruccion no encontrada: " << nombre << endl;
            continue;
        }

        int opcode = 0;
        int operando = 0;
        if (!codificarInstruccion(nombre, operandosTexto, opcode, operando)) {
            if (nombre == "HLT") cout << "Error: HLT no acepta operandos." << endl;
            else cout << "Error: operandos invalidos para " << nombre << "." << endl;
            mostrarFormatoEsperado(nombre);
            continue;
        }

        // Solo guardamos la instruccion cuando todas sus comprobaciones pasaron.
        memoriaInstrucciones[cantidadInstrucciones].opcode = opcode;
        memoriaInstrucciones[cantidadInstrucciones].operando = operando;
        memoriaInstrucciones[cantidadInstrucciones].cargada = true;
        cantidadInstrucciones++;

        cout << "Instruccion cargada correctamente." << endl;

        // HLT queda guardada y marca el final de la carga del programa.
        if (nombre == "HLT") {
            programaCargado = true;
            break;
        }
    }

    if (!programaCargado) {
        if (cantidadInstrucciones >= TOTAL_INSTRUCCIONES) {
            cout << "Error: la memoria de instrucciones esta llena y no hay espacio para HLT." << endl;
        }
        cout << "Fin de la carga. El programa no se ejecutara porque falta HLT." << endl;
        return 0;
    }

    cout << "\nPrograma cargado: " << cantidadInstrucciones << " instrucciones." << endl;
    cout << "===== INICIO DE EJECUCION =====" << endl;
    ejecutarPrograma();
    return 0;
}
