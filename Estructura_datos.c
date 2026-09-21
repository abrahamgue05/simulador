#include <stdint.h>

// 1. FLAGS -- "Bit-fields" ( : 1 ). No ocupa un byte completo, sino exactamente 1 solo bit de memoria; Registro de estado del procesador

typedef struct {
    uint8_t Z : 1; // Zero Flag: 1 si el resultado de una operacion es 0.
    uint8_t N : 1; // Negative Flag: 1 si el resultado es negativo.
    uint8_t C : 1; // Carry Flag: 1 si hay un desbordamiento (overflow) en la suma.
    uint8_t P : 1; // Parity Flag: 1 si el resultado tiene un numero par de bits en '1'.
} CPU_Flags;


// 2. ESTRUCTURA PRINCIPAL DE LA CPU -- Agrupa todos los registros internos que hacen funcionar al procesador

 typedef struct {
    // Registros de Control
    uint8_t  PC;   // Program Counter: Guarda direccion de la siguiente instruccion a ejecutar
    uint8_t  MAR; // Memory Address Register: Guarda direccion de RAM que se va a leer o escribir
    
    // Registro de Instruccion
    uint16_t IR;  // Instruction Register: Guarda la instruccion actual que se esta ejecutando
    
    // Registros de Datos
    int16_t  ACC;    // Acumulador: Donde se hacen todas las sumas y restas
    int16_t  R[4];  //  BX, CX, DX, EX
    
    // Anidado estructura de banderas
    CPU_Flags flags;    // Guarda el estado actual de Z, N, C y P.
} CPU;


// 3. INSTANCIAS Y MEMORIA RAM -- Instanciamos nuestro procesador y creamos la memoria RAM del sistema.

CPU cpu; // Creamos la CPU basada en la estructura anterior.

// RAM. Es un arreglo de 256 posiciones (por eso nuestro PC y MAR son de 8 bits, 2^8 = 256).Cada posicion guarda 16 bits (uint16_t), porque nuestra arquitectura es de 16 bits.
uint16_t memory[256];