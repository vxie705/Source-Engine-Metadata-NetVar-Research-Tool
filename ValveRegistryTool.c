#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ============================================================
   L4D2 NETVAR DUMPER v2 - PROYECTO KERNEL
   
   Corregido: usa __fastcall para simular __thiscall en C
   y SEH (__try/__except) para evitar crasheos.
   ============================================================ */

typedef struct RecvProp RecvProp;
typedef struct RecvTable RecvTable;

struct RecvTable {
    RecvProp*   m_pProps;
    int         m_nProps;
    void*       m_pDecoder;
    char*       m_pNetTableName;
    int         m_bInitialized;
    int         m_bInMainList;
};

struct RecvProp {
    char*       m_pVarName;
    int         m_RecvType;
    int         m_Flags;
    int         m_StringBufferSize;
    int         m_bInsideArray;
    void*       m_pExtraData;
    RecvProp*   m_pArrayProp;
    void*       m_ArrayLengthProxy;
    void*       m_ProxyFn;
    void*       m_DataTableProxyFn;
    RecvTable*  m_pDataTable;
    int         m_Offset;
    int         m_ElementStride;
    int         m_nElements;
    char*       m_pParentArrayPropName;
};

typedef struct ClientClass {
    void*               m_pCreateFn;
    void*               m_pCreateEventFn;
    char*               m_pNetworkName;
    RecvTable*          m_pRecvTable;
    struct ClientClass* m_pNext;
    int                 m_ClassID;
} ClientClass;

/* __fastcall simula __thiscall en C puro (ECX = this, EDX = basura) */
typedef ClientClass* (__fastcall *GetAllClassesFn)(void* thisptr, void* edx);

void DumpTable(FILE* f, RecvTable* table, int depth) {
    int i, d;
    if (!table || !table->m_pProps) return;
    if (table->m_nProps <= 0 || table->m_nProps > 200) return;

    for (i = 0; i < table->m_nProps; i++) {
        __try {
            RecvProp* prop = &table->m_pProps[i];
            if (!prop || !prop->m_pVarName) continue;
            
            if (IsBadReadPtr(prop->m_pVarName, 1)) continue;
            if (strcmp(prop->m_pVarName, "baseclass") == 0) continue;

            for (d = 0; d < depth; d++) fprintf(f, "  ");
            fprintf(f, "%-40s -> 0x%X\n", prop->m_pVarName, prop->m_Offset);

            if (prop->m_pDataTable && !IsBadReadPtr(prop->m_pDataTable, sizeof(RecvTable))) {
                DumpTable(f, prop->m_pDataTable, depth + 1);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }
    }
}

DWORD WINAPI DumperThread(LPVOID lpParam) {
    HMODULE clientDll = GetModuleHandleA("client.dll");
    if (!clientDll) {
        MessageBoxA(NULL, "ERROR: client.dll no encontrado", "DUMPER", MB_OK);
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    typedef void* (*CreateInterfaceFn)(const char*, int*);
    CreateInterfaceFn pCreateInterface = (CreateInterfaceFn)GetProcAddress(clientDll, "CreateInterface");
    if (!pCreateInterface) {
        MessageBoxA(NULL, "ERROR: CreateInterface no encontrado", "DUMPER", MB_OK);
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    void* pClient = NULL;
    const char* foundVersion = NULL;
    const char* versions[] = { "VClient016", "VClient015", "VClient017", "VClient014", NULL };
    int vi = 0;
    while (versions[vi] && !pClient) {
        pClient = pCreateInterface(versions[vi], NULL);
        if (pClient) foundVersion = versions[vi];
        vi++;
    }

    if (!pClient) {
        MessageBoxA(NULL, "ERROR: VClient interface no encontrado", "DUMPER", MB_OK);
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    /* Probar indices de vtable 6-10 para encontrar GetAllClasses */
    ClientClass* head = NULL;
    int foundIndex = -1;
    uintptr_t* vtable = *(uintptr_t**)pClient;
    
    int idx;
    for (idx = 6; idx <= 10; idx++) {
        __try {
            GetAllClassesFn fn = (GetAllClassesFn)vtable[idx];
            ClientClass* test = fn(pClient, NULL);
            if (test && !IsBadReadPtr(test, sizeof(ClientClass)) && 
                test->m_pNetworkName && !IsBadReadPtr(test->m_pNetworkName, 4)) {
                head = test;
                foundIndex = idx;
                break;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }
    }

    if (!head) {
        MessageBoxA(NULL, "ERROR: No se encontro GetAllClasses en vtable[6-10]", "DUMPER", MB_OK);
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    FILE* f;
    fopen_s(&f, "c:\\Users\\USER\\Desktop\\PROYECTO_KERNEL\\L4D2_NETVAR_DUMP.txt", "w");
    if (!f) {
        MessageBoxA(NULL, "ERROR: No se pudo crear archivo", "DUMPER", MB_OK);
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    fprintf(f, "================================================================\n");
    fprintf(f, "  L4D2 NETVAR DUMP - PROYECTO KERNEL\n");
    fprintf(f, "  Interfaz: %s | vtable[%d]\n", foundVersion, foundIndex);
    fprintf(f, "  client.dll Base: 0x%X\n", (unsigned int)clientDll);
    fprintf(f, "================================================================\n\n");

    int classCount = 0;
    ClientClass* current = head;
    while (current && classCount < 500) {
        __try {
            if (IsBadReadPtr(current, sizeof(ClientClass))) break;
            if (!current->m_pNetworkName || IsBadReadPtr(current->m_pNetworkName, 1)) {
                current = current->m_pNext;
                continue;
            }

            fprintf(f, "------------------------------------------------------------\n");
            fprintf(f, "CLASS: %-30s (ID: %d)\n", current->m_pNetworkName, current->m_ClassID);
            fprintf(f, "------------------------------------------------------------\n");

            if (current->m_pRecvTable && !IsBadReadPtr(current->m_pRecvTable, sizeof(RecvTable))) {
                if (current->m_pRecvTable->m_pNetTableName && 
                    !IsBadReadPtr(current->m_pRecvTable->m_pNetTableName, 1)) {
                    fprintf(f, "  [Table: %s]\n", current->m_pRecvTable->m_pNetTableName);
                }
                DumpTable(f, current->m_pRecvTable, 1);
            }
            fprintf(f, "\n");
            classCount++;
            current = current->m_pNext;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            break;
        }
    }

    fprintf(f, "================================================================\n");
    fprintf(f, "  TOTAL CLASES: %d\n", classCount);
    fprintf(f, "================================================================\n");
    fclose(f);

    char msg[256];
    sprintf_s(msg, 256, "EXTRACCION OK!\n%d clases.\n\nArchivo: L4D2_NETVAR_DUMP.txt", classCount);
    MessageBoxA(NULL, msg, "DUMPER", MB_OK);

    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DumperThread, hModule, 0, NULL);
    }
    return TRUE;
}
