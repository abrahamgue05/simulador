# 🖥️ Simulador de CPU (Arquitectura de 8 bits)

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Arquitectura](https://img.shields.io/badge/Arquitectura-8_Bits-blueviolet?style=for-the-badge)
![Estado](https://img.shields.io/badge/Estado-Fase_1_(Parser)-brightgreen?style=for-the-badge)

Este proyecto es un simulador de una Unidad Central de Procesamiento (CPU) basado en una arquitectura personalizada de 8 bits. Incluye su propio conjunto de instrucciones (Set RISC) y un traductor (Parser) que convierte código ensamblador en código máquina ejecutable.

---

## 🛠️ 1. Arquitectura y Variables del Sistema

El simulador se divide en componentes de **Hardware** (piezas físicas emuladas de la CPU) y **Software** (el sistema de control del programa).

### 🧮 Hardware (Registros y Memoria)

| Componente | Tipo de Dato | Descripción |
| :--- | :--- | :--- |
| **`RAM[256][2]`** | `uint8_t` | Memoria principal. **128** espacios para código y **128** para datos.<br>• `Columna 0`: Guarda el OpCode.<br>• `Columna 1`: Guarda el Operando. |
| **`AX`, `BX`, `CX`** | `int8_t` | Registros de propósito general (soportan valores de `-128` a `127`). |
| **`PC`** | `uint8_t` | *Program Counter:* Apunta a la línea de la próxima instrucción. |
| **`IR_OpCode`** | `uint8_t` | *Instruction Register:* Código numérico de la instrucción actual. |
| **`IR_Operando`**| `uint8_t` | *Instruction Register:* Dato o dirección de la instrucción actual. |
| **`SP`** | `uint8_t` | *Stack Pointer:* Apunta a la cima de la pila. |
| **`BP`** | `uint8_t` | *Base Pointer:* Puntero base para la pila. |
| **`MAR`** | `uint8_t` | *Memory Address Register:* Dirección de memoria a acceder. |
| **`MDR`** | `uint8_t` | *Memory Data Register:* Dato intermedio leído/escrito en RAM. |
| **`FLAGS[4]`** | `bool` | Banderas de estado. Índices: `0=Z` (Zero), `1=N` (Negative), `2=C` (Carry), `3=O` (Overflow). |

### 💻 Software (Traductor y Control)

| Variable | Tipo | Descripción |
| :--- | :--- | :--- |
| **`isRunning`** | `bool` | Interruptor principal del CPU. Falso = detiene el simulador. |
| **`entradaUsuario`**| `string` | Almacena la línea bruta escrita por el usuario en consola. |
| **`palabraOpCode`** | `string` | Guarda la palabra 1 (Instrucción, ej. `MOV`). |
| **`palabraDestino`**| `string` | Guarda la palabra 2 (Operando 1, ej. `AX`). |
| **`palabraOrigen`** | `string` | Guarda la palabra 3 (Operando 2, ej. `06A`). |
| **`programaTxt`** | `vector<string>` | Historial dinámico de las líneas validadas del usuario. |

---

## ⚙️ 2. Flujo de Ejecución

### Fase 1: Lectura de Instrucciones (Parser)
El objetivo de esta fase es recibir el código fuente del usuario, limpiarlo, tokenizarlo y aplicar un análisis sintáctico estricto antes de enviarlo a memoria.

1. **Bucle de lectura:** Inicia la recepción de líneas de texto. Existe un límite estricto de seguridad de **128 líneas máximas**.
2. **Limpieza de texto:** Evalúa la línea. Si está vacía, se ignora. Si tiene texto, reemplaza cualquier carácter delimitador (como la coma `,`) por espacios en blanco para facilitar la extracción.
3. **Tokenización (Separación):** Corta la línea usando espacios y extrae las partes lógicas asignándolas a las variables `palabraOpCode`, `palabraDestino` y `palabraOrigen`.
4. **Validación de Instrucción:** Verifica que la instrucción ingresada pertenezca al set oficial: 
   > `MOV`, `HLT`, `DIV`, `JMP`, `JMA`, `CMP`, `SHL` (o sus equivalentes numéricos).
   *Si no coincide, se lanza un **Error de Sintaxis** y se detiene la máquina.*
5. **Validación de Operandos:** Comprueba que el Destino y el Origen encajen en una de estas 3 reglas arquitectónicas:
   - **Registro:** Es exactamente `AX`, `BX` o `CX`.
   - **Memoria:** Tiene exactamente 3 caracteres y empieza con el dígito `0` (Ej. `06A`).
   - **Número:** Todos sus caracteres son dígitos del `0` al `9`.
   *Si no cumple ninguna, se lanza un **Error de Sintaxis**.*
6. **Almacenamiento:** Si la línea supera todos los filtros, se añade al vector `programaTxt` para su posterior ensamblado.
7. **Validación de Terminador (Safe Exit):** Al finalizar la captura de datos, el parser verifica la existencia obligatoria de la instrucción `HLT` (o `0`). Si se omite, el simulador aborta la ejecución para prevenir desbordamientos de memoria.g