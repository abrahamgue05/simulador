# 🖥️ Simulador de CPU de 8 bits en C++

Simulador educativo de una CPU de 8 bits escrito en C++17. El programa lee instrucciones de un ensamblador sencillo, las valida, las convierte a códigos numéricos y las ejecuta mediante un ciclo de **búsqueda, decodificación y ejecución** (*Fetch–Decode–Execute*).

Muestra el estado de la CPU después de cada instrucción. La ejecución tiene una pausa de **5 segundos** entre las impresiones para que sea más fácil observar los cambios.

---

## ⚙️ Arquitectura simulada

### Memoria

La RAM se representa con una matriz de **256 posiciones y 2 columnas** (`RAM[256][2]`):

- **Índices 0–127:** espacio reservado para instrucciones, con un máximo de 128 instrucciones.
- **Índices físicos 128–255:** espacio de RAM destinado a datos.
- Cada instrucción ocupa una fila: la columna 0 guarda el código de operación y la columna 1 guarda el operando.
- En la zona de datos se utiliza principalmente la columna 0 para almacenar los valores.

Las direcciones lógicas de datos van de `000` a `07F` en hexadecimal (0–127 en decimal). Para acceder a ellas, el simulador suma 128 y obtiene la posición física:

- `000` → `RAM[128]`
- `002` → `RAM[130]`
- `07F` → `RAM[255]`

Para indicar una dirección de memoria, se recomienda escribirla entre corchetes, por ejemplo `MOV AX, [002]`. También se acepta el prefijo hexadecimal, como `MOV AX, 0X002` o `MOV AX, [0X002]`.

Los operandos de origen que contienen solo dígitos, como `002`, se interpretan como valores inmediatos. Para evitar confusiones, escribe las direcciones entre corchetes. Las direcciones hexadecimales que contienen letras de `A` a `F`, como `00A`, también pueden reconocerse como direcciones.

### Registros generales

Los registros almacenan valores de 8 bits interpretados con signo, entre **-128 y 127**:

- `AX`: acumulador principal.
- `BX`: registro general secundario.
- `CX`: registro general auxiliar.

Cuando se guarda un resultado en un registro, el simulador lo ajusta al rango de un byte con signo.

### Registros de control mostrados

- `PC` (*Program Counter*): índice de la siguiente instrucción que se ejecutará.
- `MAR` (*Memory Address Register*): dirección de la instrucción consultada.
- `MDR` (*Memory Data Register*): operando leído.
- `IR`: se muestra como el código de operación y el operando de la instrucción actual.
- `SP` (*Stack Pointer*): controla la pila. Comienza en 255 y disminuye al ejecutar `PUSH`.

La pila tiene **128 espacios en un arreglo independiente**. No comparte su almacenamiento con la RAM de datos, por lo que `PUSH` y `POP` no sobrescriben valores guardados en las direcciones de memoria.

### Banderas de estado

Las banderas se muestran después de cada instrucción:

- `Z` (*Zero*): se activa cuando el resultado de 8 bits es cero.
- `N` (*Negative*): indica que está encendido el bit más significativo del resultado de 8 bits.
- `C` (*Carry*): indica el último bit que sale por la izquierda durante `SHL`; en `CMP` se usa para indicar préstamo en la resta.
- `O` (*Overflow*): indica que el resultado calculado queda fuera del rango con signo de 8 bits, de -128 a 127.

No todas las instrucciones modifican las banderas. Por ejemplo, `CMP`, `DIV` y `SHL` las actualizan según sus operaciones; `MOV`, `PUSH`, `POP` y los saltos no las actualizan.

---

## 🛠️ Instrucciones soportadas

Las instrucciones no distinguen entre mayúsculas y minúsculas. Se pueden separar los operandos con comas o espacios, y también se aceptan tabulaciones.

**Importante:** esta versión no procesa comentarios dentro del programa de instrucciones. No agregues comentarios con `;` ni `//` en las líneas que introduzcas en el simulador.

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
| `SHL` | `SHL AX, 1` | Desplaza los bits del registro a la izquierda entre 0 y 7 posiciones. Si la cantidad es mayor que cero, actualiza las banderas correspondientes. |
| `PUSH` | `PUSH AX` | Guarda un registro en la pila y mueve `SP` hacia una posición menor. |
| `POP` | `POP BX` | Recupera el último valor guardado en la pila y lo coloca en el registro indicado. |
| `JMP` | `JMP 12` | Salta incondicionalmente al índice de instrucción 12. Los índices empiezan en 0. |
| `JMA` | `JMA 8` | Salta al índice 8 únicamente si `Z = 1`. |
| `HLT` | `HLT` | Detiene la ejecución. También se acepta `0` como equivalente. |

**Límites importantes:** `JMP` y `JMA` usan índices decimales de 0 a 127, y el índice indicado debe existir en el programa. `PUSH` y `POP` solo aceptan `AX`, `BX` o `CX`. La pila admite 128 valores. Una división entre cero detiene la ejecución con un error.

---

## 🚀 Compilación y ejecución

Se necesita un compilador de C++ compatible con **C++17**, por ejemplo GCC/MinGW o Clang. El programa utiliza estas bibliotecas estándar: `iostream`, `string`, `iomanip`, `thread` y `chrono`.

### Windows (PowerShell, con MinGW/GCC)

Coloca el archivo `Simulador_CPU_estilo_principiante.cpp` en la carpeta del proyecto y abre una terminal en esa carpeta. Para compilar:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic Simulador_CPU_estilo_principiante.cpp -o cpu_simulator.exe
```

Ejecuta el simulador:

```powershell
.\cpu_simulator.exe
```

### Linux

```bash
g++ -std=c++17 -Wall -Wextra -pedantic Simulador_CPU_estilo_principiante.cpp -o cpu_simulator
./cpu_simulator
```

### Introducir un programa

Escribe o pega las instrucciones en la terminal, una por línea, y termina con `HLT`. Si la entrada termina sin encontrar `HLT`, el simulador lo agrega si hay espacio disponible.

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

El ejemplo muestra desplazamiento de bits, operaciones con registros y memoria, uso de la pila, división, comparación y salto condicional. `CMP CX, 5` deja `Z = 1`, por lo que `JMA 14` salta a la instrucción del índice 14 y no ejecuta las instrucciones de los índices 12 y 13.

### ¿Qué se verá en pantalla?

Después de cada instrucción se imprime el estado actual de la CPU. El primer panel debe ser similar a este:

```text
PC: [1] | MAR: [0] | MDR: [100] | IR: [11, 100]
Z: 0 | N: 0 | C: 0 | O: 0
AX: 100 | BX: 0 | CX: 0
SP: 255 | Uso de Pila: 0.0%
--------------------------------------------------
```

Los valores del panel cambian a medida que se ejecuta el programa. Después de cada impresión transcurren 5 segundos antes de continuar con la siguiente instrucción.

---

## 📌 Notas

- El panel es una salida de texto en la terminal; no es una interfaz gráfica independiente.
- La pausa de 5 segundos sirve para observar el estado paso a paso. Se controla con `chrono::seconds(5)` en el archivo fuente.
- La RAM, los registros, las banderas y la pila forman parte de una simulación educativa; no son componentes físicos reales.
