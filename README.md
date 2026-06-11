# Motor de Búsqueda Distribuido con MPI y Ranking TF-IDF

## Descripción del proyecto

Este proyecto implementa un motor de búsqueda distribuido usando MPI en lenguaje C.

El sistema busca una consulta dentro de una colección grande de documentos y devuelve los documentos más relevantes. Para estudiar el rendimiento, se implementaron varias versiones del programa:

* Búsqueda serial.
* Búsqueda distribuida simple con MPI.
* Búsqueda distribuida con MPI y ranking TF-IDF.
* Búsqueda distribuida con MPI, TF-IDF y balanceo de carga por cantidad de palabras.

El objetivo principal del proyecto es analizar cómo el paralelismo con MPI afecta el tiempo de ejecución, el speedup, la eficiencia y el balance de carga.

## Motivación

Buscar información en una colección grande de documentos puede tardar bastante si un solo proceso hace todo el trabajo.

Este problema se puede paralelizar porque cada documento puede analizarse de forma independiente. Con MPI, los documentos se dividen entre varios procesos. Cada proceso analiza una parte del dataset, calcula sus mejores resultados locales y luego el proceso 0 combina esos resultados para obtener el Top 10 global.

Además, se implementó ranking TF-IDF para que la búsqueda sea más realista. TF-IDF permite darle más importancia a las palabras que ayudan a identificar mejor los documentos relevantes.

## Dataset

El dataset se genera usando Python.

Cada documento contiene:

* ID del documento.
* Título.
* Tema.
* Cantidad de palabras.
* Texto completo.

El dataset usado en los experimentos tuvo 100,000 documentos.

El archivo `data/documents.csv` no se incluye en el repositorio porque es grande. Para generarlo, se usa:

```bash
python3 scripts/generate_documents.py 100000
```

## Estructura del proyecto

```text
distributed-document-search-mpi/
│
├── README.md
├── src/
│   ├── search_serial.c
│   ├── search_mpi.c
│   ├── search_mpi_simple.c
│   ├── search_mpi_tfidf.c
│   └── search_mpi_tfidf_balanced.c
│
├── data/
│   └── README.md
│
├── scripts/
│   ├── generate_documents.py
│   ├── plot_performance.py
│   ├── plot_comparison.py
│   └── plot_balance.py
│
├── results/
│   ├── timing_results.csv
│   ├── timing_results_full.csv
│   ├── balance_results.csv
│   └── gráficas de rendimiento
│
├── report/
│   ├── report.tex
│   └── report.pdf
│
└── demo/
    └── demo_link.txt
```

## Archivos principales

### Código fuente

* `src/search_serial.c`: versión serial del motor de búsqueda.
* `src/search_mpi.c`: versión distribuida simple con MPI.
* `src/search_mpi_simple.c`: copia de respaldo de la versión MPI simple.
* `src/search_mpi_tfidf.c`: versión MPI con ranking TF-IDF.
* `src/search_mpi_tfidf_balanced.c`: versión MPI TF-IDF con balanceo de carga por cantidad de palabras.

### Scripts

* `scripts/generate_documents.py`: genera el dataset sintético.
* `scripts/plot_comparison.py`: genera gráficas comparando MPI simple y MPI TF-IDF.
* `scripts/plot_balance.py`: genera gráficas de balance de carga.
* `scripts/plot_performance.py`: genera gráficas básicas de rendimiento.

### Resultados

Los archivos de resultados se encuentran en la carpeta `results/`.

Allí están los tiempos de ejecución, métricas de speedup, eficiencia, balance de carga y gráficas generadas.

## Cómo compilar

Desde la carpeta principal del proyecto, compilar la versión serial:

```bash
gcc -O2 src/search_serial.c -o search_serial
```

Compilar la versión MPI simple:

```bash
mpicc -O2 src/search_mpi.c -o search_mpi
```

Compilar la versión MPI con TF-IDF:

```bash
mpicc -O2 src/search_mpi_tfidf.c -o search_mpi_tfidf -lm
```

Compilar la versión MPI TF-IDF balanceada:

```bash
mpicc -O2 src/search_mpi_tfidf_balanced.c -o search_mpi_tfidf_balanced -lm
```

La opción `-lm` se usa en las versiones TF-IDF porque el programa calcula logaritmos para el valor IDF.

## Cómo generar el dataset

Antes de correr el programa, se debe generar el dataset:

```bash
python3 scripts/generate_documents.py 100000
```

Esto crea el archivo:

```text
data/documents.csv
```

## Cómo ejecutar el proyecto

La consulta usada en los experimentos fue:

```text
mpi speedup
```

### Ejecutar la versión serial

```bash
./search_serial data/documents.csv "mpi speedup"
```

### Ejecutar la versión MPI simple con 4 procesos

```bash
mpirun -np 4 ./search_mpi data/documents.csv "mpi speedup"
```

### Ejecutar la versión MPI TF-IDF con 4 procesos

```bash
mpirun -np 4 ./search_mpi_tfidf data/documents.csv "mpi speedup"
```

### Ejecutar la versión MPI TF-IDF balanceada con 4 procesos

```bash
mpirun -np 4 ./search_mpi_tfidf_balanced data/documents.csv "mpi speedup"
```

## Explicación de las versiones

### Versión serial

La versión serial usa un solo proceso. Lee todos los documentos y calcula un puntaje para cada uno.

El puntaje simple da más peso a las coincidencias en el título:

```text
score = 3 * apariciones_en_titulo + apariciones_en_texto
```

Esta versión sirve como base para comparar el rendimiento de las versiones paralelas.

### Versión MPI simple

La versión MPI simple divide los documentos entre varios procesos.

Cada proceso:

1. Recibe una parte del dataset.
2. Busca la consulta en sus documentos.
3. Calcula sus mejores 10 resultados locales.
4. Envía esos resultados al proceso 0.

Luego, el proceso 0 combina los resultados locales y obtiene el Top 10 global.

### Versión MPI con TF-IDF

La versión TF-IDF es más completa porque no solo cuenta palabras. También mide qué tan importante es cada término de la consulta dentro de toda la colección de documentos.

Para esto se calcula el IDF:

```text
idf = log((N + 1) / (df + 1)) + 1
```

donde:

* `N` es el número total de documentos.
* `df` es el número de documentos donde aparece el término.

En esta versión se usa `MPI_Allreduce` para sumar los conteos locales de todos los procesos y obtener el valor global de `df`.

### Versión MPI TF-IDF balanceada

En la versión MPI TF-IDF normal, cada proceso recibe aproximadamente la misma cantidad de documentos.

El problema es que no todos los documentos tienen el mismo tamaño. Algunos documentos tienen muchas más palabras que otros. Entonces, dos procesos pueden tener la misma cantidad de documentos, pero no necesariamente la misma cantidad de trabajo.

Por eso se implementó una versión balanceada. Esta versión usa el campo `WordCount` para repartir una cantidad parecida de palabras entre los procesos.

Esto permite estudiar si una mejor distribución del trabajo mejora el rendimiento.

## Resultados de rendimiento

Los experimentos se hicieron con 100,000 documentos y la consulta:

```text
mpi speedup
```

### MPI simple

| Procesos | Tiempo (segundos) | Speedup | Eficiencia |
| -------- | ----------------- | ------- | ---------- |
| 1        | 1.002666          | 1.0000  | 1.0000     |
| 2        | 0.532996          | 1.8812  | 0.9406     |
| 4        | 0.314630          | 3.1868  | 0.7967     |
| 8        | 0.244280          | 4.1046  | 0.5131     |

### MPI TF-IDF

| Procesos | Tiempo (segundos) | Speedup | Eficiencia |
| -------- | ----------------- | ------- | ---------- |
| 1        | 3.809188          | 1.0000  | 1.0000     |
| 2        | 1.818630          | 2.0945  | 1.0473     |
| 4        | 0.973448          | 3.9131  | 0.9783     |
| 8        | 0.801135          | 4.7547  | 0.5943     |

La versión MPI simple es más rápida porque hace menos trabajo. La versión MPI TF-IDF tarda más, pero produce un ranking más realista.

## Resultados de balance de carga

También se comparó la partición estática contra la partición balanceada por cantidad de palabras.

| Versión         | Procesos | Tiempo   | Speedup | Eficiencia | Word Imbalance | Time Imbalance |
| --------------- | -------- | -------- | ------- | ---------- | -------------- | -------------- |
| TF-IDF Static   | 4        | 0.973448 | 3.9131  | 0.9783     | 1.0233         | 1.0656         |
| TF-IDF Balanced | 4        | 0.916161 | 4.1578  | 1.0394     | 1.0001         | 1.0282         |
| TF-IDF Static   | 8        | 0.801135 | 4.7547  | 0.5943     | 1.0286         | 1.1519         |
| TF-IDF Balanced | 8        | 0.795925 | 4.7859  | 0.5982     | 1.0009         | 1.2004         |

Con 4 procesos, la versión balanceada mejoró el balance de palabras y también redujo el tiempo total.

Con 8 procesos, el balance de palabras también mejoró, pero el balance de tiempo no mejoró igual. Esto muestra que el tiempo real también depende de otros factores, como lectura de archivo, caché, overhead de MPI y actividad del sistema operativo.

## Gráficas

El proyecto genera gráficas para:

* Comparación de tiempo de ejecución.
* Comparación de speedup.
* Comparación de eficiencia.
* Comparación de tiempo entre TF-IDF estático y balanceado.
* Desbalance de palabras.
* Desbalance de tiempo.

Para generar las gráficas:

```bash
python3 scripts/plot_comparison.py
python3 scripts/plot_balance.py
```

Las gráficas se guardan en la carpeta:

```text
results/
```

## Reporte

El reporte final se encuentra en:

```text
report/report.pdf
```

El archivo fuente en LaTeX se encuentra en:

```text
report/report.tex
```

## Video demo

El enlace al video demo se encuentra en:

```text
demo/demo_link.txt
```

El video muestra cómo compilar el código, generar el dataset, ejecutar las versiones serial y MPI, y revisar los resultados de rendimiento.

## Autor

Sebastian Alzate Vargas

COMP6786 - Proyecto Final
