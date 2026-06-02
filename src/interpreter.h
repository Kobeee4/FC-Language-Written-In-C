#ifndef FC_INTERP_H
#define FC_INTERP_H

#include <stddef.h>
#include <setjmp.h>
#include "parser.h"

struct Interp;
struct Environment;

typedef enum {
    VAL_NULL, VAL_NUMBER, VAL_BOOL, VAL_STRING,
    VAL_LIST, VAL_DICT, VAL_FUNCTION, VAL_NATIVE,
    VAL_CLASS, VAL_OBJECT, VAL_BOUND
} ValueType;

typedef struct ObjString ObjString;
typedef struct ObjList ObjList;
typedef struct ObjDict ObjDict;
typedef struct ObjFunction ObjFunction;
typedef struct ObjNative ObjNative;
typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;
typedef struct ObjBound ObjBound;

typedef struct Value {
    ValueType type;
    union {
        double number;
        int boolean;
        ObjString* str;
        ObjList* list;
        ObjDict* dict;
        ObjFunction* fn;
        ObjNative* nat;
        ObjClass* klass;
        ObjInstance* inst;
        ObjBound* bound;
    } as;
} Value;

typedef Value (*NativeFn)(struct Interp*, Value, int, Value*);

struct ObjString { int refcount; int length; char* chars; };
struct ObjList { int refcount; int count; int cap; Value* items; };
typedef struct { char* key; Value value; } DictEntry;
struct ObjDict { int refcount; int count; int cap; DictEntry* entries; };
struct ObjFunction { int refcount; Node* decl; struct Environment* closure; char* name; };
struct ObjNative { int refcount; NativeFn fn; Value self; char* name; };
struct ObjClass { int refcount; char* name; ObjDict* methods; };
struct ObjInstance { int refcount; ObjClass* klass; ObjDict* fields; };
struct ObjBound { int refcount; Value receiver; ObjFunction* method; };

typedef struct Entry { char* name; Value value; struct Entry* next; } Entry;
typedef struct Environment { struct Environment* parent; Entry* head; } Environment;

enum { FLOW_NORMAL, FLOW_RETURN, FLOW_BREAK, FLOW_CONTINUE };

typedef struct Interp {
    Environment* globals;
    Environment* env;
    int flow;
    Value return_value;
    char error_msg[512];
    int error_line;
    jmp_buf* handlers[64];
    int handler_count;
    int had_error;
    int argc;
    char** argv;
    char** imported;
    int imported_count;
    Node** import_asts;
    int import_ast_count;
} Interp;

static inline Value NULL_VAL(void) { Value v; v.type = VAL_NULL; v.as.number = 0; return v; }
static inline Value NUMBER_VAL(double n) { Value v; v.type = VAL_NUMBER; v.as.number = n; return v; }
static inline Value BOOL_VAL(int b) { Value v; v.type = VAL_BOOL; v.as.boolean = b ? 1 : 0; return v; }

Value make_string(const char* chars, int len);
Value make_string_cstr(const char* s);
Value make_list(void);
Value make_dict(void);
Value make_native(NativeFn fn, Value self, const char* name);

Value value_retain(Value v);
void value_release(Value v);
int value_truthy(Value v);
int value_equals(Value a, Value b);

void list_append_take(ObjList* l, Value v);
void dict_set_take(ObjDict* d, const char* key, Value v);
int dict_get(ObjDict* d, const char* key, Value* out);
int dict_remove(ObjDict* d, const char* key);

Environment* env_new(Environment* parent);
void env_free(Environment* e);
void env_define(Environment* e, const char* name, Value v);
int env_get(Environment* e, const char* name, Value* out);
int env_set(Environment* e, const char* name, Value v);

Interp* interp_new(void);
void interp_free(Interp* I);
void interp_run(Interp* I, Node* program);
Value interp_eval(Interp* I, Node* n);
void interp_exec(Interp* I, Node* n);
Value interp_call_value(Interp* I, Value callee, int line, int argc, Value* argv);
void interp_error(Interp* I, int line, const char* fmt, ...);

char* value_to_string(Value v, int quote);
char* read_file(const char* path);
void fc_sha256(const unsigned char* data, size_t len, char out_hex[65]);
void fc_md5(const unsigned char* data, size_t len, char out_hex[33]);
void fc_sha1(const unsigned char* data, size_t len, char out_hex[41]);

void register_modules(Interp* I);

#endif
