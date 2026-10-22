# 🖥️ Simulador de CPU (Arquitectura de 8 bits)

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Arquitectura](https://img.shields.io/badge/Arquitectura-8_Bits-blueviolet?style=for-the-badge)
![Estado](https://img.shields.io/badge/Estado-Fase_2_(Ensamblado_y_MMU)-brightgreen?style=for-the-badge)

Este proyecto es un simulador de una Unidad Central de Procesamiento (CPU) basado en una arquitectura personalizada de 8 bits. Incluye su propio conjunto de instrucciones (Set RISC), un traductor sintáctico (Parser) y una Unidad de Manejo de Memoria (MMU) que traduce el código ensamblador a código máquina ejecutable.

---

## 🛠️ 1. Arquitectura y Variables del Sistema

El simulador se divide en componentes de **Hardware** (piezas físicas emuladas de la CPU) y **Software** (el sistema de control del programa).

### 🧮 Hardware (Registros y Memoria)

| Componente | Tipo de Dato | Descripción |
| :--- | :--- | :--- |
| **`RAM[256][2]`** | `uint8_t` | Memoria principal. **128** espacios para código y **128** para datos.<br>• `Columna 0`: Guarda el OpCode fusionado.<br>• `Columna 1`: Guarda el Operando (Dato o Dirección). |
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

## 🔍 2. Funciones de Validación (Helpers)

Pequeños filtros lógicos que actúan como aduana para verificar la correcta estructura de los datos antes de permitir su paso al hardware:

*   **`esNumero(str)`:** Recorre el texto para confirmar que contenga exclusivamente dígitos numéricos válidos.
*   **`esRegistro(str)`:** Comprueba que el operando coincida exactamente con los registros oficiales (`AX`, `BX` o `CX`).
*   **`obtenerIdRegistro(str)`:** Asigna un identificador numérico único a cada registro (`AX = 1`, `BX = 2`, `CX = 3`).
*   **`esMemoria(str)`:** Valida que una dirección de memoria tenga formato hexadecimal válido (mida 3 caracteres y empiece con el dígito `0`, ej. `06A`).
*   **`validarInstruccion(op)`:** Verifica que la instrucción pertenezca al set permitido (`MOV`, `HLT`, `DIV`, `JMP`, `JMA`, `CMP`, `SHL`).
*   **`validarOperando(op)`:** Asegura que el operando sea un registro válido, una dirección de memoria correcta o un número decimal.

---

## ⚙️ 3. Flujo de Ejecución

### Fase 1: Lectura de Instrucciones (Parser)
El objetivo de esta fase es recibir el código fuente del usuario, limpiarlo, tokenizarlo y aplicar un análisis sintáctico estricto antes de enviarlo a memoria.

1. **Bucle de lectura:** Inicia la recepción de líneas de texto. Existe un límite estricto de seguridad de **128 líneas máximas**.
2. **Limpieza de texto:** Evalúa la línea. Si está vacía, se ignora. Si tiene texto, reemplaza cualquier carácter delimitador (como la coma `,`) por espacios en blanco para facilitar la extracción.
3. **Tokenización (Separación):** Corta la línea usando espacios y extrae las partes lógicas asignándolas a las variables `palabraOpCode`, `palabraDestino` y `palabraOrigen`.
4. **Validación de Instrucción:** Verifica que la instrucción ingresada pertenezca al set oficial: 
    > `MOV`, `HLT`, `DIV`, `JMP`, `JMA`, `CMP`, `SHL`
    *Si no coincide, se lanza un **Error de Sintaxis** y se detiene la máquina.*
5. **Validación de Operandos:** Comprueba que el Destino y el Origen encajen en una de las reglas arquitectónicas (Registro, Memoria o Número). *Si no cumple, se lanza un **Error de Sintaxis**.*
6. **Almacenamiento:** Si la línea supera todos los filtros, se añade al vector `programaTxt` para su posterior ensamblado.
7. **Validación de Terminador (Safe Exit):** Al finalizar la captura de datos, el parser verifica la existencia obligatoria de la instrucción `HLT` (o `0`).

---

## 📖 4. Diccionario de Hardware (OpCodes Oficiales con Fusión de Registros)
Para optimizar la matriz RAM de 2 columnas, el OpCode fusiona la operación con el registro destino usando aritmética de nibbles (4 bits para instrucción, 4 bits para el registro):

*   **HLT:** `0` (Detiene el reloj del sistema)
*   **MOV Reg, Num:** Base `10` + ID Reg (`11` = AX, `12` = BX, `13` = CX)
*   **MOV Reg, Mem:** Base `20` + ID Reg (`21` = AX, `22` = BX, `23` = CX)
*   **MOV Mem, Reg:** Base `30` + ID Reg Origen (`31` = AX, `32` = BX, `33` = CX)
*   **MOV Reg, Reg:** Base `40` + ID Reg (`41`, `42`, `43`)
*   **DIV Reg, Num:** Base `50` + ID Reg (`51`, `52`, `53`)
*   **DIV Mem, Num:** `60` (Fijo, sin registro destino)
*   **CMP Reg, Num:** Base `70` + ID Reg (`71`, `72`, `73`)
*   **CMP Reg, Mem:** Base `80` + ID Reg (`81`, `82`, `83`)
*   **JMP Linea:** `90` (Salto incondicional)
*   **JMA Linea:** `91` (Salto condicional)
*   **SHL Reg, Num:** Base `100` + ID Reg (`101`, `102`, `103`)

---

### Fase 2: Enviar a Memoria (Ensamblador y MMU)
Toma el historial de texto validado en la Fase 1, realiza la traducción numérica y aplica virtualización de memoria:

1. **Limpieza Preventiva (Zero Padding):** Rellena la zona de código de la RAM (0 a 127) con ceros (`0, 0`) para eliminar residuos anteriores.
2. **Traducción y Fusión:** Recorre secuencialmente `programaTxt`, extrayendo los tokens y buscando su equivalente numérico en el Diccionario de Hardware.
3. **Aplicación de Memoria Virtual (MMU - Offset de +128):** 
   * Si el operando es un número o registro, se traduce directamente.
   * Si es una **dirección de memoria**, el ensamblador la convierte de hexadecimal a decimal y **le suma obligatoriamente 128**. Esto empuja el dato a la mitad superior de la RAM (128-255), evitando que sobrescriba el código de instrucciones.
   * **Excepción de Saltos:** Las instrucciones de salto (`JMP`, `JMA`) **ignoran** el offset de +128, ya que sus punteros deben apuntar estrictamente a la zona de código (0-127).
4. **Inyección Secuencial:** Almacena los valores procesados en orden estricto (`RAM[i][0]` y `RAM[i][1]`) respetando el tamaño exacto del programa y blindando el resto de las casillas con el cortafuegos de seguridad.