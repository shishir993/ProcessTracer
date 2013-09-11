
// CHelpLib.h
// Exported symbols from CHelpLib DLL
// Shishir K Prasad (http://www.shishirprasad.net)
// History
//      06/23/13 Initial version
//

#ifndef _CHELPLIBDLL_H
#define _CHELPLIBDLL_H

#ifdef __cplusplus
extern "C" {  
#endif

#ifdef CHELPLIB_EXPORTS
#define DllExpImp __declspec( dllexport )
#else
#define DllExpImp __declspec( dllimport )
#endif // CHELPLIB_EXPORTS

// Defines

// hashtable key types
#define HT_KEY_STR      10
#define HT_KEY_DWORD    11

// hashtable value types
#define HT_VAL_DWORD    12
#define HT_VAL_PTR      13
#define HT_VAL_STR      14

typedef int HT_KEYTYPE;
typedef int HT_VALTYPE;

// Structures
union _key {
    BYTE *skey;
    DWORD dwkey;
};

union _val {
    DWORD dwVal;
    void *pval;
    BYTE *pbVal;
};

// hashtable node
typedef struct _hashTableNode {
    //BOOL fOccupied;     // FALSE = not occupied
    union _key key;
    union _val val;
    int keysize;
    int valsize;
    struct _hashTableNode *pnext;
}HT_NODE;

// hashtable itself
typedef struct _hashtable {
    HT_KEYTYPE htKeyType;
    HT_VALTYPE htValType;
    HT_NODE **phtNodes;
    int nTableSize;
    HANDLE hMuAccess;
}CHL_HTABLE;

// Structure that defines the iterator for the hashtable
// Callers can use this to iterate through the hashtable
// and get all (key,value) pairs one-by-one
typedef struct _hashtableIterator {
    int opType;
    int nCurIndex;              // current position in the main bucket
    HT_NODE *phtCurNodeInList;  // current position in the sibling list
}CHL_HT_ITERATOR;

// Functions exported

// MemFunctions
DllExpImp BOOL ChlfMemAlloc(OUT void **pvAddr, IN size_t uSizeBytes, OPTIONAL DWORD *pdwError);
DllExpImp void ChlvMemFree(IN void **pvToFree);

// IOFunctions
DllExpImp BOOL ChlfReadLineFromStdin(OUT TCHAR *psBuffer, IN DWORD dwBufSize);

// StringFunctions
DllExpImp WCHAR* Chl_GetFilenameFromPath(WCHAR *pwsFilepath, int numCharsInput);

// Hastable functions
DllExpImp BOOL HT_fCreate(CHL_HTABLE **pHTableOut, int nKeyToTableSize, int keyType, int valType);
DllExpImp BOOL HT_fDestroy(CHL_HTABLE *phtable);

DllExpImp BOOL HT_fInsert (CHL_HTABLE *phtable, void *pvkey, int keySize, void *pval, int valSize);
DllExpImp BOOL HT_fFind   (CHL_HTABLE *phtable, void *pvkey, int keySize, OUT void *pval, OUT int *pvalsize);
DllExpImp BOOL HT_fRemove (CHL_HTABLE *phtable, void *pvkey, int keySize);

DllExpImp BOOL HT_fInitIterator(CHL_HT_ITERATOR *pItr);
DllExpImp BOOL HT_fGetNext(CHL_HTABLE *phtable, CHL_HT_ITERATOR *pItr, 
                            OUT void *pkey, OUT int *pkeysize,
                            OUT void *pval, OUT int *pvalsize);

DllExpImp void HT_vDumpTable(CHL_HTABLE *phtable);

// Unit Tests
DllExpImp void vTests();

#ifdef __cplusplus
}
#endif

#endif // _CHELPLIBDLL_H
