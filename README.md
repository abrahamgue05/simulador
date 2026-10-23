# 🖥️ Simulador de CPU de 8 bits en C++

Simulador educativo de una CPU de 8 bits escrito en C++17. El programa lee instrucciones de un ensamblador sencillo, las valida, las convierte a códigos numéricos y las ejecuta mediante un ciclo de **búsqueda, decodificación y ejecución** (*Fetch–Decode–Execute*).

---

## ⚙️ Arquitectura simulada

### Memoria

La RAM se representa con una matriz de **256 posiciones y 2 columnas** (`RAM[256][2]`):

- **Índices 0–127:** espacio reservado para instrucciones, con un máximo de 128 instrucciones.
- **Índices físicos 128–255:** espacio para datos y pila.
- Cada instrucción ocupa una fila: la columna 0 guarda el código de operación y la columna 1 guarda el operando.
- En la zona de datos se utiliza principalmente la columna 0 para almacenar el valor.

Las direcciones lógicas de datos van de `000` a `07F` en hexadecimal (0–127 en decimal). Para acceder a ellas, el simulador suma 128 y obtiene la posición física:

- `000` → `RAM[128]`
- `002` → `RAM[130]`
- `07F` → `RAM[255]`

Para indicar una dirección de memoria, se recomienda escribirla entre corchetes, por ejemplo `MOV AX, [002]`.

### Registros generales

Los registros almacenan valores de 8 bits interpretados con signo, entre **-128 y 127**:

- `AX`: acumulador principal.
- `BX`: registro general secundario.
- `CX`: registro general auxiliar.

### Registros de control mostrados

- `PC` (*Program Counter*): índice de la siguiente instrucción que se ejecutará.
- `MAR` (*Memory Address Register*): dirección de la instrucción consultada.
- `MDR` (*Memory Data Register*): operando leído.
- `IR`: se muestra como el código de operación y el operando de la instrucción actual.
- `SP` (*Stack Pointer*): puntero utilizado para administrar la pila, que comienza en 255 y crece hacia posiciones menores al hacer `PUSH`.
- `BP` (*Base Pointer*): se muestra en el panel con el valor inicial 248; en esta versión no se utiliza para administrar las operaciones de la pila.

### Banderas de estado

Las banderas se muestran después de cada instrucción:

- `Z` (*Zero*): se activa cuando el resultado evaluado equivale a cero.
- `N` (*Negative*): indica que está encendido el bit más significativo del resultado de 8 bits.
- `C` (*Carry*): indica acarreo en desplazamientos; en comparaciones se utiliza para señalar préstamo.
- `O` (*Overflow*): indica desbordamiento del rango con signo de 8 bits, de -128 a 127.

---

## 🛠️ Instrucciones soportadas

Las instrucciones no distinguen entre mayúsculas y minúsculas. Se pueden separar los operandos con comas o espacios. Se permiten comentarios que comiencen con `;` o `//`.

| Instrucción | Sintaxis | Funcionamiento |
|---|---|---|
| `MOV` | `MOV AX, 42` | Guarda un valor inmediato en un registro. El valor debe estar entre -128 y 127. |
| `MOV` | `MOV AX, BX` | Copia el valor de un registro a otro. |
| `MOV` | `MOV AX, [002]` | Lee un valor de la zona de datos y lo guarda en un registro. |
| `MOV` | `MOV [001], BX` | Guarda en memoria el valor de un registro. |
| `DIV` | `DIV AX, 2` | Divide el registro indicado entre un divisor inmediato distinto de cero y guarda el resultado en ese registro. |
| `DIV` | `DIV [002]` | Divide `AX` entre el valor almacenado en esa dirección y guarda el resultado en `AX`. |
| `CMP` | `CMP AX, 20` | Compara un registro con un valor inmediato y actualiza las banderas sin modificar el registro. |
| `CMP` | `CMP AX, [001]` | Compara un registro con un valor de memoria y actualiza las banderas. |
| `SHL` | `SHL AX, 1` | Desplaza los bits del registro a la izquierda entre 0 y 7 posiciones y actualiza las banderas correspondientes. |
| `PUSH` | `PUSH AX` | Guarda un registro en la pila y mueve `SP` hacia una posición menor. |
| `POP` | `POP BX` | Recupera el último valor guardado en la pila y lo coloca en el registro indicado. |
| `JMP` | `JMP 12` | Salta incondicionalmente al índice de instrucción 12. Los índices empiezan en 0. |
| `JMA` | `JMA 8` | Salta al índice 8 únicamente si `Z = 1`. |
| `HLT` | `HLT` | Detiene la ejecución. También se acepta `0` como equivalente. |

**Límites importantes:** `JMP` y `JMA` usan índices decimales de 0 a 127, no números de línea empezando desde 1. `PUSH` y `POP` solo aceptan `AX`, `BX` o `CX`. Una división entre cero detiene la ejecución con un error.

---

## 🚀 Compilación y ejecución

Se necesita un compilador de C++ compatible con **C++17**, por ejemplo GCC/MinGW o Clang. No se necesitan dependencias externas.

### Windows (PowerShell, con MinGW/GCC)

Coloca el archivo `.cpp` en la carpeta del proyecto y abre una terminal en esa carpeta. Para compilar la versión comentada con pausa de 5 segundos:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic Simulador_CPU_principiante_comentado_sleep.cpp -o cpu_simulator.exe
```

Ejecuta el simulador:

```powershell
.\cpu_simulator.exe
```

### Linux

```bash
g++ -std=c++17 -Wall -Wextra -pedantic Simulador_CPU_principiante_comentado_sleep.cpp -o cpu_simulator
./cpu_simulator
```

### Introducir un programa

Escribe o pega las instrucciones en la terminal, una por línea, y termina con `HLT`. El programa también puede terminar automáticamente si se alcanza el límite de instrucciones sin encontrar `HLT`.

Para probar un archivo de instrucciones en PowerShell:

```powershell
.\cpu_simulator.exe < .\programa.txt
```

El simulador lee hasta 128 instrucciones. Si no encuentra `HLT` y hay espacio disponible, lo agrega. Si se llenan las 128 posiciones, reemplaza la instrucción del índice 127 por `HLT`; por tanto, en ese caso se ejecutan como máximo las primeras 127 instrucciones originales.

---

## 🧪 Programa de ejemplo

Copia el siguiente bloque en la terminal después de iniciar el simulador. Termina en `HLT`:

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

El ejemplo muestra desplazamiento de bits, operaciones con registros y memoria, uso de la pila, división, comparación y salto condicional. Como `CMP CX, 5` deja `Z = 1`, `JMA 14` salta a la instrucción del índice 14.

### ¿Qué se verá en pantalla?

Después de cada instrucción se imprime el estado actual de la CPU. El primer panel debe ser similar a este:

```text
PC: [1] | MAR: [0] | MDR: [100] | IR: [11, 100]
Z: 0 | N: 0 | C: 0 | O: 0
AX: 100 | BX: 0 | CX: 0
BP: 248 | SP: 255 | Uso de Pila: 0.0%
--------------------------------------------------
```

Los valores del panel cambian a medida que se ejecuta el programa. En la versión con pausa, transcurren 5 segundos entre cada impresión y la siguiente instrucción continúa después de esa espera.

---