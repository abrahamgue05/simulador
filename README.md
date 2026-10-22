# 🖥️ Simulador de CPU de 8 bits en C++

Simulador de microprocesador de arquitectura de 8 bits basado en un ciclo clásico **Fetch-Decode-Execute (F-D-E)**. Cuenta con un ensamblador de texto personalizado, gestión de memoria virtual (MMU), una Unidad Aritmética y Lógica (ALU) con banderas de estado en vivo, gestión de pila (`PUSH`/`POP`), y un panel de control interactivo (*Dashboard*) en tiempo real.

---

## ⚙️ Características de la Arquitectura

* **Espacio de Memoria RAM (256 bytes):**
  * **00 - 127:** Zona exclusiva para Código (hasta 128 instrucciones).
  * **128 - 255:** Zona exclusiva para Datos y Pila (crece de manera descendente desde la posición `255`).
* **Registros Generales (8 bits con signo):**
  * `AX` (Acumulador principal)
  * `BX` (Registro general secundario)
  * `CX` (Registro general auxiliar)
* **Punteros y Registros de Control:**
  * `PC` (Program Counter / Instruction Pointer)
  * `MAR` (Memory Address Register)
  * `MDR` (Memory Data Register)
  * `IR` (Instruction Register)
  * `SP` (Stack Pointer - Puntero de cima de pila)
  * `BP` (Base Pointer - Puntero base)
* **Banderas de Estado (`FLAGS`):**
  * `Z` (Zero) — Se activa si el resultado es `0`.
  * `N` (Negative) — Se activa si el bit más significativo está encendido.
  * `C` (Carry) — Refleja acarreos en operaciones de desplazamiento.
  * `O` (Overflow) — Se activa si el resultado excede el rango de 8 bits con signo (`-128` a `127`).

---

## 🛠️ Instrucciones Soportadas

| Instrucción | Sintaxis de Ejemplo | Descripción |
| :--- | :--- | :--- |
| **`MOV`** | `MOV AX, 42`<br>`MOV AX, [002]`<br>`MOV [001], BX`<br>`MOV AX, BX` | Copia valores inmediatos, transfiere datos entre registros o mapea celdas de la zona de datos virtual (`000-07F`). |
| **`DIV`** | `DIV AX, 2`<br>`DIV [002]` | Realiza división entera. En la variante de un operando, divide implícitamente el acumulador `AX` entre el valor de memoria. |
| **`CMP`** | `CMP AX, 20`<br>`CMP AX, [001]` | Compara un registro contra un valor inmediato o en memoria mediante resta lógica, actualizando las banderas sin alterar el registro. |
| **`SHL`** | `SHL AX, 1` | Desplaza los bits del registro hacia la izquierda (multiplica por potencias de 2) y evalúa el desbordamiento/acarreo. |
| **`PUSH`** | `PUSH AX` | Empuja el valor del registro a la cima de la pila (`SP--`) y actualiza el medidor de uso. |
| **`POP`** | `POP BX` | Extrae el valor superior de la pila (`SP++`) y lo deposita en el registro indicado. |
| **`JMP`** | `JMP 12` | Salto incondicional absoluto hacia el índice de línea indicado. |
| **`JMA`** | `JMA 8` | Salto condicional: salta a la línea especificada si la bandera `Z` (Zero) se encuentra activa (`1`). |
| **`HLT`** | `HLT` | Detiene de forma segura el ciclo de reloj y apaga el procesador. |

---

## 🚀 Compilación y Ejecución

El código está escrito en **C++17** estándar y utiliza librerías nativas (`<thread>`, `<chrono>`, `<vector>`, etc.), por lo que no requiere dependencias externas ni gestores de paquetes complejos.

### Compilar con GCC / Clang:
```bash
g++ -std=c++17 -O3 SimuladorPt3.cpp -o cpu_simulator
```

### Ejecutar el Simulador:
```bash
./cpu_simulator
```

## 🧪 Programa de Prueba Sugerido

Al iniciar el programa, puedes copiar y pegar este bloque completo directamente en tu terminal:

```text
MOV AX, 100
SHL AX, 1
PUSH AX
MOV BX, 50
PUSH BX
MOV [002], BX
POP AX
POP BX
MOV CX, 10
DIV CX, 2
CMP CX, 5
JMA 14
MOV CX, 99
JMP 15
MOV CX, 12
DIV [002]
CMP AX, 5
HLT
```

### ¿Qué verás en pantalla?
Un **Dashboard de Hardware en cascada** impreso instrucción por instrucción que se ve así:
```text
PC: [1] | MAR: [0] | MDR: [100] | IR: [11, 100]
Z: 0 | N: 0 | C: 0 | O: 0
AX: 100 | BX: 0 | CX: 0
BP: 248 | SP: 255 | Uso de Pila: 0.0%
--------------------------------------------------
```
Y reportes en tiempo real cada vez que la **ALU** procese matemáticas o la **Pila** mueva datos (`[STACK] -> PUSH / POP`).