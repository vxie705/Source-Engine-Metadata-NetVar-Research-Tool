# Source Engine Metadata & NetVar Diagnostic Tool
### [ESP] Descripción Técnica
Herramienta de diagnóstico de bajo nivel desarrollada en C para la extracción automatizada de metadatos del motor **Source (Valve)**. Esta utilidad permite la inspección profunda de estructuras de datos en tiempo real (NetVars) mediante el recorrido programático de la lista enlazada `ClientClass`.
**Características principales:**
- **Recuperación de Interfaces**: Utiliza el patrón `CreateInterface` del motor Source para obtener acceso seguro a los servicios del cliente.
- **VTable Redirection**: Acceso programático a la función virtual `GetAllClasses()`.
- **Análisis de RecvTables**: Recorre de forma recursiva las tablas de red para extraer nombres de variables y sus desplazamientos (offsets) de memoria exactos.
- **Seguridad y Estabilidad**: Implementa Manejo de Excepciones Estructuradas (**SEH - `__try`/`__except`**) para garantizar la estabilidad del proceso durante la lectura de regiones de memoria volátiles.
---
### [ENG] Technical Overview
A low-level metadata extraction utility for the **Source Engine (Valve)** developed in pure C. This tool facilitates deep-memory inspection of networkable variables (NetVars) by programmatically traversing the `ClientClass` internal linked list.
**Key Technical Pillars:**
- **Interface Retrieval**: Leverages the Source Engine `CreateInterface` export for secure engine interaction.
- **VTable Hooking**: Direct access to the `GetAllClasses()` virtual function via pointer redirection.
- **Recursive Table Scanning**: Full traversal of `RecvTable` structures to identify variable names and their corresponding memory offsets.
- **Robustness**: Utilizes Structured Exception Handling (**SEH**) to prevent process termination during the inspection of unpredictable memory regions.
---
