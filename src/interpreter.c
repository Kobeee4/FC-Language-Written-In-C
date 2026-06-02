#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include "interpreter.h"

Value make_string(const char* chars, int len) {
    ObjString* s = (ObjString*)malloc(sizeof(ObjString));
    s->refcount = 1;
    s->length = len;
    s->chars = (char*)malloc(len + 1);
    if (len > 0) memcpy(s->chars, chars, len);
    s->chars[len] = 0;
    Value v;
    v.type = VAL_STRING;
    v.as.str = s;
    return v;
}

Value make_string_cstr(const char* s) {
    return make_string(s, (int)strlen(s));
}

Value make_list(void) {
    ObjList* l = (ObjList*)malloc(sizeof(ObjList));
    l->refcount = 1;
    l->count = 0;
    l->cap = 0;
    l->items = NULL;
    Value v;
    v.type = VAL_LIST;
    v.as.list = l;
    return v;
}

Value make_dict(void) {
    ObjDict* d = (ObjDict*)malloc(sizeof(ObjDict));
    d->refcount = 1;
    d->count = 0;
    d->cap = 0;
    d->entries = NULL;
    Value v;
    v.type = VAL_DICT;
    v.as.dict = d;
    return v;
}

Value make_native(NativeFn fn, Value self, const char* name) {
    ObjNative* n = (ObjNative*)malloc(sizeof(ObjNative));
    n->refcount = 1;
    n->fn = fn;
    n->self = value_retain(self);
    n->name = strdup(name);
    Value v;
    v.type = VAL_NATIVE;
    v.as.nat = n;
    return v;
}

static Value make_function(Node* decl, Environment* closure) {
    ObjFunction* f = (ObjFunction*)malloc(sizeof(ObjFunction));
    f->refcount = 1;
    f->decl = decl;
    f->closure = closure;
    f->name = strdup(decl->str ? decl->str : "<fn>");
    Value v;
    v.type = VAL_FUNCTION;
    v.as.fn = f;
    return v;
}

static Value make_bound(Value receiver, ObjFunction* method) {
    ObjBound* b = (ObjBound*)malloc(sizeof(ObjBound));
    b->refcount = 1;
    b->receiver = value_retain(receiver);
    method->refcount++;
    b->method = method;
    Value v;
    v.type = VAL_BOUND;
    v.as.bound = b;
    return v;
}

Value value_retain(Value v) {
    switch (v.type) {
        case VAL_STRING: v.as.str->refcount++; break;
        case VAL_LIST: v.as.list->refcount++; break;
        case VAL_DICT: v.as.dict->refcount++; break;
        case VAL_FUNCTION: v.as.fn->refcount++; break;
        case VAL_NATIVE: v.as.nat->refcount++; break;
        case VAL_CLASS: v.as.klass->refcount++; break;
        case VAL_OBJECT: v.as.inst->refcount++; break;
        case VAL_BOUND: v.as.bound->refcount++; break;
        default: break;
    }
    return v;
}

void value_release(Value v) {
    switch (v.type) {
        case VAL_STRING:
            if (--v.as.str->refcount == 0) {
                free(v.as.str->chars);
                free(v.as.str);
            }
            break;
        case VAL_LIST:
            if (--v.as.list->refcount == 0) {
                for (int i = 0; i < v.as.list->count; i++) value_release(v.as.list->items[i]);
                free(v.as.list->items);
                free(v.as.list);
            }
            break;
        case VAL_DICT:
            if (--v.as.dict->refcount == 0) {
                for (int i = 0; i < v.as.dict->count; i++) {
                    free(v.as.dict->entries[i].key);
                    value_release(v.as.dict->entries[i].value);
                }
                free(v.as.dict->entries);
                free(v.as.dict);
            }
            break;
        case VAL_FUNCTION:
            if (--v.as.fn->refcount == 0) {
                free(v.as.fn->name);
                free(v.as.fn);
            }
            break;
        case VAL_NATIVE:
            if (--v.as.nat->refcount == 0) {
                value_release(v.as.nat->self);
                free(v.as.nat->name);
                free(v.as.nat);
            }
            break;
        case VAL_CLASS:
            if (--v.as.klass->refcount == 0) {
                Value md;
                md.type = VAL_DICT;
                md.as.dict = v.as.klass->methods;
                value_release(md);
                free(v.as.klass->name);
                free(v.as.klass);
            }
            break;
        case VAL_OBJECT:
            if (--v.as.inst->refcount == 0) {
                Value fd;
                fd.type = VAL_DICT;
                fd.as.dict = v.as.inst->fields;
                value_release(fd);
                Value kv;
                kv.type = VAL_CLASS;
                kv.as.klass = v.as.inst->klass;
                value_release(kv);
                free(v.as.inst);
            }
            break;
        case VAL_BOUND:
            if (--v.as.bound->refcount == 0) {
                value_release(v.as.bound->receiver);
                Value mv;
                mv.type = VAL_FUNCTION;
                mv.as.fn = v.as.bound->method;
                value_release(mv);
                free(v.as.bound);
            }
            break;
        default:
            break;
    }
}

int value_truthy(Value v) {
    switch (v.type) {
        case VAL_NULL: return 0;
        case VAL_BOOL: return v.as.boolean;
        case VAL_NUMBER: return v.as.number != 0;
        case VAL_STRING: return v.as.str->length > 0;
        case VAL_LIST: return v.as.list->count > 0;
        case VAL_DICT: return v.as.dict->count > 0;
        default: return 1;
    }
}

int value_equals(Value a, Value b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case VAL_NULL: return 1;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_NUMBER: return a.as.number == b.as.number;
        case VAL_STRING: return strcmp(a.as.str->chars, b.as.str->chars) == 0;
        case VAL_LIST: return a.as.list == b.as.list;
        case VAL_DICT: return a.as.dict == b.as.dict;
        case VAL_OBJECT: return a.as.inst == b.as.inst;
        case VAL_CLASS: return a.as.klass == b.as.klass;
        case VAL_FUNCTION: return a.as.fn == b.as.fn;
        default: return 0;
    }
}

void list_append_take(ObjList* l, Value v) {
    if (l->count >= l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->items = (Value*)realloc(l->items, sizeof(Value) * l->cap);
    }
    l->items[l->count++] = v;
}

void dict_set_take(ObjDict* d, const char* key, Value v) {
    for (int i = 0; i < d->count; i++) {
        if (strcmp(d->entries[i].key, key) == 0) {
            value_release(d->entries[i].value);
            d->entries[i].value = v;
            return;
        }
    }
    if (d->count >= d->cap) {
        d->cap = d->cap ? d->cap * 2 : 8;
        d->entries = (DictEntry*)realloc(d->entries, sizeof(DictEntry) * d->cap);
    }
    d->entries[d->count].key = strdup(key);
    d->entries[d->count].value = v;
    d->count++;
}

int dict_get(ObjDict* d, const char* key, Value* out) {
    for (int i = 0; i < d->count; i++) {
        if (strcmp(d->entries[i].key, key) == 0) {
            *out = value_retain(d->entries[i].value);
            return 1;
        }
    }
    return 0;
}

int dict_remove(ObjDict* d, const char* key) {
    for (int i = 0; i < d->count; i++) {
        if (strcmp(d->entries[i].key, key) == 0) {
            free(d->entries[i].key);
            value_release(d->entries[i].value);
            for (int j = i; j < d->count - 1; j++) d->entries[j] = d->entries[j + 1];
            d->count--;
            return 1;
        }
    }
    return 0;
}

Environment* env_new(Environment* parent) {
    Environment* e = (Environment*)malloc(sizeof(Environment));
    e->parent = parent;
    e->head = NULL;
    return e;
}

void env_free(Environment* e) {
    Entry* cur = e->head;
    while (cur) {
        Entry* next = cur->next;
        value_release(cur->value);
        free(cur->name);
        free(cur);
        cur = next;
    }
    free(e);
}

void env_define(Environment* e, const char* name, Value v) {
    for (Entry* cur = e->head; cur; cur = cur->next) {
        if (strcmp(cur->name, name) == 0) {
            value_release(cur->value);
            cur->value = value_retain(v);
            return;
        }
    }
    Entry* en = (Entry*)malloc(sizeof(Entry));
    en->name = strdup(name);
    en->value = value_retain(v);
    en->next = e->head;
    e->head = en;
}

int env_get(Environment* e, const char* name, Value* out) {
    for (Environment* env = e; env; env = env->parent) {
        for (Entry* cur = env->head; cur; cur = cur->next) {
            if (strcmp(cur->name, name) == 0) {
                *out = value_retain(cur->value);
                return 1;
            }
        }
    }
    return 0;
}

int env_set(Environment* e, const char* name, Value v) {
    for (Environment* env = e; env; env = env->parent) {
        for (Entry* cur = env->head; cur; cur = cur->next) {
            if (strcmp(cur->name, name) == 0) {
                value_release(cur->value);
                cur->value = value_retain(v);
                return 1;
            }
        }
    }
    return 0;
}

void interp_error(Interp* I, int line, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(I->error_msg, sizeof(I->error_msg), fmt, ap);
    va_end(ap);
    I->error_line = line;
    if (I->handler_count > 0) longjmp(*I->handlers[I->handler_count - 1], 1);
    fprintf(stderr, "FC Fatal Error on Line %d: %s\n", line, I->error_msg);
    exit(1);
}

Interp* interp_new(void) {
    Interp* I = (Interp*)calloc(1, sizeof(Interp));
    I->globals = env_new(NULL);
    I->env = I->globals;
    I->flow = FLOW_NORMAL;
    I->return_value = NULL_VAL();
    I->handler_count = 0;
    I->had_error = 0;
    srand((unsigned int)time(NULL));
    return I;
}

void interp_free(Interp* I) {
    for (int i = 0; i < I->import_ast_count; i++) free_node(I->import_asts[i]);
    free(I->import_asts);
    for (int i = 0; i < I->imported_count; i++) free(I->imported[i]);
    free(I->imported);
    env_free(I->globals);
    free(I);
}

static void do_import(Interp* I, const char* name, int line) {
    char cands[8][1024];
    int nc = 0;
    if (strchr(name, '/') || strstr(name, ".fc")) {
        snprintf(cands[nc++], 1024, "%s", name);
    } else {
        snprintf(cands[nc++], 1024, "%s.fc", name);
        snprintf(cands[nc++], 1024, "packages/%s.fc", name);
        const char* home = getenv("HOME");
        if (home) snprintf(cands[nc++], 1024, "%s/.fc/packages/%s.fc", home, name);
        const char* fp = getenv("FCPATH");
        if (fp && nc < 8) snprintf(cands[nc++], 1024, "%s/%s.fc", fp, name);
    }
    char* src = NULL;
    int found = -1;
    for (int i = 0; i < nc; i++) {
        src = read_file(cands[i]);
        if (src) { found = i; break; }
    }
    if (found < 0) interp_error(I, line, "import: cannot find module '%s'", name);

    for (int i = 0; i < I->imported_count; i++) {
        if (strcmp(I->imported[i], cands[found]) == 0) { free(src); return; }
    }
    I->imported = (char**)realloc(I->imported, sizeof(char*) * (I->imported_count + 1));
    I->imported[I->imported_count++] = strdup(cands[found]);

    TokenList tl = lex_source(src);
    free(src);
    if (tl.had_error) {
        int el = tl.error_line;
        char em[256];
        strncpy(em, tl.error_msg, sizeof(em) - 1);
        em[sizeof(em) - 1] = 0;
        free_tokens(&tl);
        interp_error(I, line, "import '%s': syntax error on line %d: %s", name, el, em);
    }
    Parser p;
    Node* prog = parse_program(&tl, &p);
    free_tokens(&tl);
    if (!prog || p.had_error) {
        int el = p.error_line;
        char em[256];
        strncpy(em, p.error_msg, sizeof(em) - 1);
        em[sizeof(em) - 1] = 0;
        if (prog) free_node(prog);
        interp_error(I, line, "import '%s': parse error on line %d: %s", name, el, em);
    }
    I->import_asts = (Node**)realloc(I->import_asts, sizeof(Node*) * (I->import_ast_count + 1));
    I->import_asts[I->import_ast_count++] = prog;

    Environment* saved_env = I->env;
    int saved_flow = I->flow;
    I->env = I->globals;
    I->flow = FLOW_NORMAL;
    for (int i = 0; i < prog->item_count; i++) {
        interp_exec(I, prog->items[i]);
        if (I->flow != FLOW_NORMAL) break;
    }
    I->env = saved_env;
    I->flow = saved_flow;
}

static Value call_function(Interp* I, ObjFunction* fn, Value thisv, int argc, Value* argv) {
    Node* decl = fn->decl;
    Environment* env = env_new(fn->closure);
    for (int i = 0; i < decl->param_count; i++) {
        Value pv = (i < argc) ? argv[i] : NULL_VAL();
        env_define(env, decl->params[i], pv);
    }
    if (thisv.type != VAL_NULL) env_define(env, "this", thisv);

    Environment* saved_env = I->env;
    int saved_flow = I->flow;
    Value saved_ret = I->return_value;
    I->env = env;
    I->flow = FLOW_NORMAL;
    I->return_value = NULL_VAL();

    Node* body = decl->a;
    for (int i = 0; i < body->item_count; i++) {
        interp_exec(I, body->items[i]);
        if (I->flow != FLOW_NORMAL) break;
    }

    Value result = (I->flow == FLOW_RETURN) ? I->return_value : NULL_VAL();
    I->env = saved_env;
    I->flow = saved_flow;
    I->return_value = saved_ret;
    env_free(env);
    return result;
}

static Value instantiate(Interp* I, ObjClass* k, int line, int argc, Value* argv) {
    ObjInstance* inst = (ObjInstance*)malloc(sizeof(ObjInstance));
    inst->refcount = 1;
    inst->klass = k;
    k->refcount++;
    Value fd = make_dict();
    inst->fields = fd.as.dict;
    Value instv;
    instv.type = VAL_OBJECT;
    instv.as.inst = inst;

    Value initm;
    if (dict_get(k->methods, "init", &initm)) {
        if (initm.type == VAL_FUNCTION) {
            Value r = call_function(I, initm.as.fn, instv, argc, argv);
            value_release(r);
        }
        value_release(initm);
    }
    (void)line;
    return instv;
}

static Value nat_list_append(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1) interp_error(I, 0, "append expects 1 argument");
    list_append_take(self.as.list, value_retain(argv[0]));
    return NULL_VAL();
}
static Value nat_list_pop(Interp* I, Value self, int argc, Value* argv) {
    (void)argc; (void)argv;
    ObjList* l = self.as.list;
    if (l->count == 0) interp_error(I, 0, "pop from empty list");
    return l->items[--l->count];
}
static Value nat_list_length(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    return NUMBER_VAL(self.as.list->count);
}
static Value nat_list_get(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_NUMBER) interp_error(I, 0, "get expects a numeric index");
    ObjList* l = self.as.list;
    int i = (int)argv[0].as.number;
    if (i < 0 || i >= l->count) interp_error(I, 0, "list index out of range");
    return value_retain(l->items[i]);
}
static Value nat_list_set(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_NUMBER) interp_error(I, 0, "set expects index and value");
    ObjList* l = self.as.list;
    int i = (int)argv[0].as.number;
    if (i < 0 || i >= l->count) interp_error(I, 0, "list index out of range");
    value_release(l->items[i]);
    l->items[i] = value_retain(argv[1]);
    return NULL_VAL();
}

static Value nat_dict_keys(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    Value list = make_list();
    ObjDict* d = self.as.dict;
    for (int i = 0; i < d->count; i++) list_append_take(list.as.list, make_string_cstr(d->entries[i].key));
    return list;
}
static Value nat_dict_has(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "has expects a string key");
    Value tmp;
    int found = dict_get(self.as.dict, argv[0].as.str->chars, &tmp);
    if (found) value_release(tmp);
    return BOOL_VAL(found);
}
static Value nat_dict_get(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "get expects a string key");
    Value out;
    if (dict_get(self.as.dict, argv[0].as.str->chars, &out)) return out;
    return NULL_VAL();
}
static Value nat_dict_set(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_STRING) interp_error(I, 0, "set expects a string key and a value");
    dict_set_take(self.as.dict, argv[0].as.str->chars, value_retain(argv[1]));
    return NULL_VAL();
}
static Value nat_dict_remove(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "remove expects a string key");
    return BOOL_VAL(dict_remove(self.as.dict, argv[0].as.str->chars));
}
static Value nat_dict_length(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    return NUMBER_VAL(self.as.dict->count);
}

static Value nat_str_length(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    return NUMBER_VAL(self.as.str->length);
}
static Value nat_str_upper(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    ObjString* s = self.as.str;
    Value r = make_string(s->chars, s->length);
    for (int i = 0; i < r.as.str->length; i++) {
        char c = r.as.str->chars[i];
        if (c >= 'a' && c <= 'z') r.as.str->chars[i] = c - 32;
    }
    return r;
}
static Value nat_str_lower(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    ObjString* s = self.as.str;
    Value r = make_string(s->chars, s->length);
    for (int i = 0; i < r.as.str->length; i++) {
        char c = r.as.str->chars[i];
        if (c >= 'A' && c <= 'Z') r.as.str->chars[i] = c + 32;
    }
    return r;
}
static Value nat_str_contains(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "contains expects a string");
    return BOOL_VAL(strstr(self.as.str->chars, argv[0].as.str->chars) != NULL);
}
static Value nat_str_split(Interp* I, Value self, int argc, Value* argv) {
    const char* sep = (argc >= 1 && argv[0].type == VAL_STRING) ? argv[0].as.str->chars : " ";
    (void)I;
    Value list = make_list();
    const char* s = self.as.str->chars;
    int seplen = (int)strlen(sep);
    if (seplen == 0) {
        list_append_take(list.as.list, make_string_cstr(s));
        return list;
    }
    const char* start = s;
    const char* found;
    while ((found = strstr(start, sep)) != NULL) {
        list_append_take(list.as.list, make_string(start, (int)(found - start)));
        start = found + seplen;
    }
    list_append_take(list.as.list, make_string_cstr(start));
    return list;
}

static Value nat_list_insert(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_NUMBER) interp_error(I, 0, "insert expects index and value");
    ObjList* l = self.as.list;
    int idx = (int)argv[0].as.number;
    if (idx < 0 || idx > l->count) interp_error(I, 0, "insert index out of range");
    list_append_take(l, NULL_VAL());
    for (int i = l->count - 1; i > idx; i--) l->items[i] = l->items[i - 1];
    l->items[idx] = value_retain(argv[1]);
    return NULL_VAL();
}
static Value nat_list_remove_at(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_NUMBER) interp_error(I, 0, "remove_at expects an index");
    ObjList* l = self.as.list;
    int idx = (int)argv[0].as.number;
    if (idx < 0 || idx >= l->count) interp_error(I, 0, "remove_at index out of range");
    Value v = l->items[idx];
    for (int i = idx; i < l->count - 1; i++) l->items[i] = l->items[i + 1];
    l->count--;
    return v;
}
static Value nat_list_index_of(Interp* I, Value self, int argc, Value* argv) {
    (void)I;
    if (argc < 1) return NUMBER_VAL(-1);
    ObjList* l = self.as.list;
    for (int i = 0; i < l->count; i++) if (value_equals(l->items[i], argv[0])) return NUMBER_VAL(i);
    return NUMBER_VAL(-1);
}
static Value nat_list_contains(Interp* I, Value self, int argc, Value* argv) {
    (void)I;
    if (argc < 1) return BOOL_VAL(0);
    ObjList* l = self.as.list;
    for (int i = 0; i < l->count; i++) if (value_equals(l->items[i], argv[0])) return BOOL_VAL(1);
    return BOOL_VAL(0);
}
static Value nat_list_reverse(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    ObjList* l = self.as.list;
    for (int i = 0, j = l->count - 1; i < j; i++, j--) {
        Value t = l->items[i];
        l->items[i] = l->items[j];
        l->items[j] = t;
    }
    return NULL_VAL();
}
static Value nat_list_sort(Interp* I, Value self, int argc, Value* argv) {
    (void)argc; (void)argv;
    ObjList* l = self.as.list;
    for (int i = 1; i < l->count; i++) {
        Value key = l->items[i];
        int j = i - 1;
        while (j >= 0) {
            Value b = l->items[j];
            int gt = 0;
            if (key.type == VAL_NUMBER && b.type == VAL_NUMBER) gt = b.as.number > key.as.number;
            else if (key.type == VAL_STRING && b.type == VAL_STRING) gt = strcmp(b.as.str->chars, key.as.str->chars) > 0;
            else interp_error(I, 0, "sort requires all numbers or all strings");
            if (!gt) break;
            l->items[j + 1] = l->items[j];
            j--;
        }
        l->items[j + 1] = key;
    }
    return NULL_VAL();
}
static Value nat_list_slice(Interp* I, Value self, int argc, Value* argv) {
    ObjList* l = self.as.list;
    int start = (argc >= 1 && argv[0].type == VAL_NUMBER) ? (int)argv[0].as.number : 0;
    int end = (argc >= 2 && argv[1].type == VAL_NUMBER) ? (int)argv[1].as.number : l->count;
    if (start < 0) start += l->count;
    if (end < 0) end += l->count;
    if (start < 0) start = 0;
    if (end > l->count) end = l->count;
    (void)I;
    Value out = make_list();
    for (int i = start; i < end; i++) list_append_take(out.as.list, value_retain(l->items[i]));
    return out;
}
static Value nat_list_join(Interp* I, Value self, int argc, Value* argv) {
    (void)I;
    const char* sep = (argc >= 1 && argv[0].type == VAL_STRING) ? argv[0].as.str->chars : "";
    ObjList* l = self.as.list;
    int cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    int seplen = (int)strlen(sep);
    for (int i = 0; i < l->count; i++) {
        char* s = value_to_string(l->items[i], 0);
        int sl = (int)strlen(s);
        int need = len + sl + (i ? seplen : 0) + 1;
        if (need > cap) { while (need > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        if (i) { memcpy(buf + len, sep, seplen); len += seplen; }
        memcpy(buf + len, s, sl); len += sl;
        buf[len] = 0;
        free(s);
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}

static Value nat_dict_values(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    Value list = make_list();
    ObjDict* d = self.as.dict;
    for (int i = 0; i < d->count; i++) list_append_take(list.as.list, value_retain(d->entries[i].value));
    return list;
}
static Value nat_dict_items(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    Value list = make_list();
    ObjDict* d = self.as.dict;
    for (int i = 0; i < d->count; i++) {
        Value pair = make_list();
        list_append_take(pair.as.list, make_string_cstr(d->entries[i].key));
        list_append_take(pair.as.list, value_retain(d->entries[i].value));
        list_append_take(list.as.list, pair);
    }
    return list;
}
static Value nat_dict_merge(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_DICT) interp_error(I, 0, "merge expects a dict");
    ObjDict* o = argv[0].as.dict;
    for (int i = 0; i < o->count; i++) dict_set_take(self.as.dict, o->entries[i].key, value_retain(o->entries[i].value));
    return NULL_VAL();
}
static Value nat_dict_clear(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    ObjDict* d = self.as.dict;
    for (int i = 0; i < d->count; i++) { free(d->entries[i].key); value_release(d->entries[i].value); }
    d->count = 0;
    return NULL_VAL();
}

static Value nat_str_replace(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING)
        interp_error(I, 0, "replace expects two strings");
    const char* s = self.as.str->chars;
    const char* old = argv[0].as.str->chars;
    const char* neu = argv[1].as.str->chars;
    int oldlen = (int)strlen(old);
    int newlen = (int)strlen(neu);
    if (oldlen == 0) return value_retain(self);
    int cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    const char* p = s;
    const char* f;
    while ((f = strstr(p, old)) != NULL) {
        int chunk = (int)(f - p);
        if (len + chunk + newlen + 1 > cap) { while (len + chunk + newlen + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf + len, p, chunk); len += chunk;
        memcpy(buf + len, neu, newlen); len += newlen;
        p = f + oldlen;
    }
    int tail = (int)strlen(p);
    if (len + tail + 1 > cap) { cap = len + tail + 1; buf = (char*)realloc(buf, cap); }
    memcpy(buf + len, p, tail); len += tail;
    buf[len] = 0;
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value nat_str_trim(Interp* I, Value self, int argc, Value* argv) {
    (void)I; (void)argc; (void)argv;
    const char* s = self.as.str->chars;
    int n = self.as.str->length;
    int start = 0, end = n;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) end--;
    return make_string(&s[start], end - start);
}
static Value nat_str_find(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "find expects a string");
    const char* hit = strstr(self.as.str->chars, argv[0].as.str->chars);
    if (!hit) return NUMBER_VAL(-1);
    return NUMBER_VAL((double)(hit - self.as.str->chars));
}
static Value nat_str_substr(Interp* I, Value self, int argc, Value* argv) {
    ObjString* s = self.as.str;
    int start = (argc >= 1 && argv[0].type == VAL_NUMBER) ? (int)argv[0].as.number : 0;
    int count = (argc >= 2 && argv[1].type == VAL_NUMBER) ? (int)argv[1].as.number : (s->length - start);
    if (start < 0) start = 0;
    if (start > s->length) start = s->length;
    if (count < 0) count = 0;
    if (start + count > s->length) count = s->length - start;
    (void)I;
    return make_string(&s->chars[start], count);
}
static Value nat_str_repeat(Interp* I, Value self, int argc, Value* argv) {
    int n = (argc >= 1 && argv[0].type == VAL_NUMBER) ? (int)argv[0].as.number : 0;
    if (n < 0) n = 0;
    ObjString* s = self.as.str;
    long total = (long)s->length * n;
    if (total > 50000000L) interp_error(I, 0, "repeat result too large");
    char* buf = (char*)malloc(total + 1);
    for (int i = 0; i < n; i++) memcpy(buf + (long)i * s->length, s->chars, s->length);
    buf[total] = 0;
    Value v = make_string(buf, (int)total);
    free(buf);
    return v;
}
static Value nat_str_starts_with(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "starts_with expects a string");
    int pl = argv[0].as.str->length;
    if (pl > self.as.str->length) return BOOL_VAL(0);
    return BOOL_VAL(strncmp(self.as.str->chars, argv[0].as.str->chars, pl) == 0);
}
static Value nat_str_ends_with(Interp* I, Value self, int argc, Value* argv) {
    if (argc < 1 || argv[0].type != VAL_STRING) interp_error(I, 0, "ends_with expects a string");
    int pl = argv[0].as.str->length;
    int sl = self.as.str->length;
    if (pl > sl) return BOOL_VAL(0);
    return BOOL_VAL(strcmp(self.as.str->chars + (sl - pl), argv[0].as.str->chars) == 0);
}

static int list_method(Value self, const char* name, Value* out) {
    NativeFn fn = NULL;
    if (!strcmp(name, "append")) fn = nat_list_append;
    else if (!strcmp(name, "pop")) fn = nat_list_pop;
    else if (!strcmp(name, "length") || !strcmp(name, "len")) fn = nat_list_length;
    else if (!strcmp(name, "get")) fn = nat_list_get;
    else if (!strcmp(name, "set")) fn = nat_list_set;
    else if (!strcmp(name, "insert")) fn = nat_list_insert;
    else if (!strcmp(name, "remove_at")) fn = nat_list_remove_at;
    else if (!strcmp(name, "index_of")) fn = nat_list_index_of;
    else if (!strcmp(name, "contains")) fn = nat_list_contains;
    else if (!strcmp(name, "reverse")) fn = nat_list_reverse;
    else if (!strcmp(name, "sort")) fn = nat_list_sort;
    else if (!strcmp(name, "slice")) fn = nat_list_slice;
    else if (!strcmp(name, "join")) fn = nat_list_join;
    if (!fn) return 0;
    *out = make_native(fn, self, name);
    return 1;
}
static int dict_method(Value self, const char* name, Value* out) {
    NativeFn fn = NULL;
    if (!strcmp(name, "keys")) fn = nat_dict_keys;
    else if (!strcmp(name, "has")) fn = nat_dict_has;
    else if (!strcmp(name, "get")) fn = nat_dict_get;
    else if (!strcmp(name, "set")) fn = nat_dict_set;
    else if (!strcmp(name, "remove")) fn = nat_dict_remove;
    else if (!strcmp(name, "length") || !strcmp(name, "len")) fn = nat_dict_length;
    else if (!strcmp(name, "values")) fn = nat_dict_values;
    else if (!strcmp(name, "items")) fn = nat_dict_items;
    else if (!strcmp(name, "merge")) fn = nat_dict_merge;
    else if (!strcmp(name, "clear")) fn = nat_dict_clear;
    if (!fn) return 0;
    *out = make_native(fn, self, name);
    return 1;
}
static int str_method(Value self, const char* name, Value* out) {
    NativeFn fn = NULL;
    if (!strcmp(name, "length") || !strcmp(name, "len")) fn = nat_str_length;
    else if (!strcmp(name, "upper")) fn = nat_str_upper;
    else if (!strcmp(name, "lower")) fn = nat_str_lower;
    else if (!strcmp(name, "contains")) fn = nat_str_contains;
    else if (!strcmp(name, "split")) fn = nat_str_split;
    else if (!strcmp(name, "replace")) fn = nat_str_replace;
    else if (!strcmp(name, "trim")) fn = nat_str_trim;
    else if (!strcmp(name, "find")) fn = nat_str_find;
    else if (!strcmp(name, "substr")) fn = nat_str_substr;
    else if (!strcmp(name, "repeat")) fn = nat_str_repeat;
    else if (!strcmp(name, "starts_with")) fn = nat_str_starts_with;
    else if (!strcmp(name, "ends_with")) fn = nat_str_ends_with;
    if (!fn) return 0;
    *out = make_native(fn, self, name);
    return 1;
}

Value interp_call_value(Interp* I, Value callee, int line, int argc, Value* argv) {
    switch (callee.type) {
        case VAL_NATIVE:
            return callee.as.nat->fn(I, callee.as.nat->self, argc, argv);
        case VAL_FUNCTION:
            return call_function(I, callee.as.fn, NULL_VAL(), argc, argv);
        case VAL_BOUND:
            return call_function(I, callee.as.bound->method, callee.as.bound->receiver, argc, argv);
        case VAL_CLASS:
            return instantiate(I, callee.as.klass, line, argc, argv);
        default:
            interp_error(I, line, "Value is not callable");
            return NULL_VAL();
    }
}

static Value eval_call(Interp* I, Node* n) {
    Value callee = interp_eval(I, n->a);
    int argc = n->item_count;
    Value* argv = argc ? (Value*)malloc(sizeof(Value) * argc) : NULL;
    for (int i = 0; i < argc; i++) argv[i] = interp_eval(I, n->items[i]);

    if (callee.type != VAL_NATIVE && callee.type != VAL_FUNCTION &&
        callee.type != VAL_BOUND && callee.type != VAL_CLASS) {
        for (int i = 0; i < argc; i++) value_release(argv[i]);
        if (argv) free(argv);
        value_release(callee);
        interp_error(I, n->line, "Value is not callable");
        return NULL_VAL();
    }

    Value result = interp_call_value(I, callee, n->line, argc, argv);
    for (int i = 0; i < argc; i++) value_release(argv[i]);
    if (argv) free(argv);
    value_release(callee);
    return result;
}

static char* index_key(Value idx, int* needs_free) {
    if (idx.type == VAL_STRING) {
        *needs_free = 0;
        return idx.as.str->chars;
    }
    *needs_free = 1;
    return value_to_string(idx, 0);
}

static Value apply_binary(Interp* I, int line, const char* op, Value a, Value b) {
    if (strcmp(op, "==") == 0) return BOOL_VAL(value_equals(a, b));
    if (strcmp(op, "!=") == 0) return BOOL_VAL(!value_equals(a, b));
    if (strcmp(op, "in") == 0) {
        if (b.type == VAL_LIST) {
            for (int i = 0; i < b.as.list->count; i++) if (value_equals(b.as.list->items[i], a)) return BOOL_VAL(1);
            return BOOL_VAL(0);
        }
        if (b.type == VAL_DICT) {
            if (a.type != VAL_STRING) return BOOL_VAL(0);
            Value tmp;
            int f = dict_get(b.as.dict, a.as.str->chars, &tmp);
            if (f) value_release(tmp);
            return BOOL_VAL(f);
        }
        if (b.type == VAL_STRING && a.type == VAL_STRING) return BOOL_VAL(strstr(b.as.str->chars, a.as.str->chars) != NULL);
        return BOOL_VAL(0);
    }
    if (strcmp(op, "+") == 0) {
        if (a.type == VAL_STRING || b.type == VAL_STRING) {
            char* sa = value_to_string(a, 0);
            char* sb = value_to_string(b, 0);
            int la = (int)strlen(sa), lb = (int)strlen(sb);
            char* buf = (char*)malloc(la + lb + 1);
            memcpy(buf, sa, la);
            memcpy(buf + la, sb, lb);
            buf[la + lb] = 0;
            Value r = make_string(buf, la + lb);
            free(sa); free(sb); free(buf);
            return r;
        }
        if (a.type == VAL_LIST && b.type == VAL_LIST) {
            Value r = make_list();
            for (int i = 0; i < a.as.list->count; i++) list_append_take(r.as.list, value_retain(a.as.list->items[i]));
            for (int i = 0; i < b.as.list->count; i++) list_append_take(r.as.list, value_retain(b.as.list->items[i]));
            return r;
        }
        if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) return NUMBER_VAL(a.as.number + b.as.number);
        interp_error(I, line, "Cannot add these values");
    }
    if ((strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0)
        && a.type == VAL_STRING && b.type == VAL_STRING) {
        int c = strcmp(a.as.str->chars, b.as.str->chars);
        if (strcmp(op, "<") == 0) return BOOL_VAL(c < 0);
        if (strcmp(op, ">") == 0) return BOOL_VAL(c > 0);
        if (strcmp(op, "<=") == 0) return BOOL_VAL(c <= 0);
        return BOOL_VAL(c >= 0);
    }
    if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) interp_error(I, line, "Operator '%s' requires numbers", op);
    double x = a.as.number, y = b.as.number;
    if (strcmp(op, "-") == 0) return NUMBER_VAL(x - y);
    if (strcmp(op, "*") == 0) return NUMBER_VAL(x * y);
    if (strcmp(op, "/") == 0) { if (y == 0) interp_error(I, line, "Division by zero"); return NUMBER_VAL(x / y); }
    if (strcmp(op, "%") == 0) { if (y == 0) interp_error(I, line, "Modulo by zero"); return NUMBER_VAL(fmod(x, y)); }
    if (strcmp(op, "<") == 0) return BOOL_VAL(x < y);
    if (strcmp(op, ">") == 0) return BOOL_VAL(x > y);
    if (strcmp(op, "<=") == 0) return BOOL_VAL(x <= y);
    if (strcmp(op, ">=") == 0) return BOOL_VAL(x >= y);
    interp_error(I, line, "Unknown operator '%s'", op);
    return NULL_VAL();
}

Value interp_eval(Interp* I, Node* n) {
    switch (n->type) {
        case N_NUMBER: return NUMBER_VAL(n->number);
        case N_STRING: return make_string_cstr(n->str);
        case N_BOOL: return BOOL_VAL(n->boolean);
        case N_NULL: return NULL_VAL();
        case N_IDENT: {
            Value out;
            if (env_get(I->env, n->str, &out)) return out;
            interp_error(I, n->line, "Undefined variable '%s'", n->str);
            return NULL_VAL();
        }
        case N_LIST: {
            Value list = make_list();
            for (int i = 0; i < n->item_count; i++)
                list_append_take(list.as.list, interp_eval(I, n->items[i]));
            return list;
        }
        case N_DICT: {
            Value dict = make_dict();
            for (int i = 0; i + 1 < n->item_count; i += 2) {
                Value k = interp_eval(I, n->items[i]);
                Value v = interp_eval(I, n->items[i + 1]);
                int nf;
                char* key = index_key(k, &nf);
                dict_set_take(dict.as.dict, key, v);
                if (nf) free(key);
                value_release(k);
            }
            return dict;
        }
        case N_MEMBER: {
            Value obj = interp_eval(I, n->a);
            Value out;
            if (obj.type == VAL_OBJECT) {
                if (dict_get(obj.as.inst->fields, n->str, &out)) { value_release(obj); return out; }
                Value m;
                if (dict_get(obj.as.inst->klass->methods, n->str, &m)) {
                    Value b = make_bound(obj, m.as.fn);
                    value_release(m);
                    value_release(obj);
                    return b;
                }
                value_release(obj);
                interp_error(I, n->line, "Undefined member '%s'", n->str);
            } else if (obj.type == VAL_DICT) {
                if (dict_get(obj.as.dict, n->str, &out)) { value_release(obj); return out; }
                if (dict_method(obj, n->str, &out)) { value_release(obj); return out; }
                value_release(obj);
                interp_error(I, n->line, "Undefined key '%s'", n->str);
            } else if (obj.type == VAL_LIST) {
                if (list_method(obj, n->str, &out)) { value_release(obj); return out; }
                value_release(obj);
                interp_error(I, n->line, "Unknown list operation '%s'", n->str);
            } else if (obj.type == VAL_STRING) {
                if (str_method(obj, n->str, &out)) { value_release(obj); return out; }
                value_release(obj);
                interp_error(I, n->line, "Unknown string operation '%s'", n->str);
            } else {
                value_release(obj);
                interp_error(I, n->line, "Cannot access member of this value");
            }
            return NULL_VAL();
        }
        case N_INDEX: {
            Value obj = interp_eval(I, n->a);
            Value idx = interp_eval(I, n->b);
            Value result = NULL_VAL();
            if (obj.type == VAL_LIST) {
                if (idx.type != VAL_NUMBER) { value_release(obj); value_release(idx); interp_error(I, n->line, "List index must be a number"); }
                int i = (int)idx.as.number;
                if (i < 0) i += obj.as.list->count;
                if (i < 0 || i >= obj.as.list->count) { value_release(obj); value_release(idx); interp_error(I, n->line, "List index out of range"); }
                result = value_retain(obj.as.list->items[i]);
            } else if (obj.type == VAL_DICT) {
                int nf;
                char* key = index_key(idx, &nf);
                if (!dict_get(obj.as.dict, key, &result)) result = NULL_VAL();
                if (nf) free(key);
            } else if (obj.type == VAL_STRING) {
                if (idx.type != VAL_NUMBER) { value_release(obj); value_release(idx); interp_error(I, n->line, "String index must be a number"); }
                int i = (int)idx.as.number;
                if (i < 0) i += obj.as.str->length;
                if (i < 0 || i >= obj.as.str->length) { value_release(obj); value_release(idx); interp_error(I, n->line, "String index out of range"); }
                result = make_string(&obj.as.str->chars[i], 1);
            } else {
                value_release(obj); value_release(idx);
                interp_error(I, n->line, "Cannot index this value");
            }
            value_release(obj);
            value_release(idx);
            return result;
        }
        case N_UNARY: {
            Value a = interp_eval(I, n->a);
            Value r;
            if (strcmp(n->str, "not") == 0) {
                r = BOOL_VAL(!value_truthy(a));
            } else {
                if (a.type != VAL_NUMBER) { value_release(a); interp_error(I, n->line, "Unary '-' requires a number"); }
                r = NUMBER_VAL(-a.as.number);
            }
            value_release(a);
            return r;
        }
        case N_BINARY: {
            const char* op = n->str;
            if (strcmp(op, "and") == 0) {
                Value a = interp_eval(I, n->a);
                if (!value_truthy(a)) { value_release(a); return BOOL_VAL(0); }
                value_release(a);
                Value b = interp_eval(I, n->b);
                int t = value_truthy(b);
                value_release(b);
                return BOOL_VAL(t);
            }
            if (strcmp(op, "or") == 0) {
                Value a = interp_eval(I, n->a);
                if (value_truthy(a)) { value_release(a); return BOOL_VAL(1); }
                value_release(a);
                Value b = interp_eval(I, n->b);
                int t = value_truthy(b);
                value_release(b);
                return BOOL_VAL(t);
            }
            Value a = interp_eval(I, n->a);
            Value b = interp_eval(I, n->b);
            Value r = apply_binary(I, n->line, op, a, b);
            value_release(a);
            value_release(b);
            return r;
        }
        case N_FUNC: return make_function(n, I->globals);
        case N_CALL: return eval_call(I, n);
        default:
            interp_error(I, n->line, "Cannot evaluate this expression");
            return NULL_VAL();
    }
}

static void assign_to(Interp* I, Node* target, Value value) {
    if (target->type == N_IDENT) {
        if (!env_set(I->env, target->str, value)) env_define(I->env, target->str, value);
        return;
    }
    if (target->type == N_MEMBER) {
        Value obj = interp_eval(I, target->a);
        if (obj.type == VAL_OBJECT) {
            dict_set_take(obj.as.inst->fields, target->str, value_retain(value));
        } else if (obj.type == VAL_DICT) {
            dict_set_take(obj.as.dict, target->str, value_retain(value));
        } else {
            value_release(obj);
            interp_error(I, target->line, "Cannot assign member on this value");
        }
        value_release(obj);
        return;
    }
    if (target->type == N_INDEX) {
        Value obj = interp_eval(I, target->a);
        Value idx = interp_eval(I, target->b);
        if (obj.type == VAL_LIST) {
            if (idx.type != VAL_NUMBER) { value_release(obj); value_release(idx); interp_error(I, target->line, "List index must be a number"); }
            int i = (int)idx.as.number;
            ObjList* l = obj.as.list;
            if (i < 0) i += l->count;
            if (i == l->count) {
                list_append_take(l, value_retain(value));
            } else if (i >= 0 && i < l->count) {
                value_release(l->items[i]);
                l->items[i] = value_retain(value);
            } else {
                value_release(obj); value_release(idx);
                interp_error(I, target->line, "List index out of range");
            }
        } else if (obj.type == VAL_DICT) {
            int nf;
            char* key = index_key(idx, &nf);
            dict_set_take(obj.as.dict, key, value_retain(value));
            if (nf) free(key);
        } else {
            value_release(obj); value_release(idx);
            interp_error(I, target->line, "Cannot index-assign this value");
        }
        value_release(obj);
        value_release(idx);
        return;
    }
    interp_error(I, target->line, "Invalid assignment target");
}

void interp_exec(Interp* I, Node* n) {
    switch (n->type) {
        case N_BLOCK:
            for (int i = 0; i < n->item_count; i++) {
                interp_exec(I, n->items[i]);
                if (I->flow != FLOW_NORMAL) return;
            }
            return;
        case N_VARDECL: {
            Value v = interp_eval(I, n->a);
            env_define(I->env, n->str, v);
            value_release(v);
            return;
        }
        case N_ASSIGN: {
            Value v = interp_eval(I, n->b);
            assign_to(I, n->a, v);
            value_release(v);
            return;
        }
        case N_COMPOUND: {
            Value cur = interp_eval(I, n->a);
            Value rhs = interp_eval(I, n->b);
            Value res = apply_binary(I, n->line, n->str, cur, rhs);
            value_release(cur);
            value_release(rhs);
            assign_to(I, n->a, res);
            value_release(res);
            return;
        }
        case N_OUT: {
            Value v = interp_eval(I, n->a);
            char* s = value_to_string(v, 0);
            printf("%s\n", s);
            free(s);
            value_release(v);
            return;
        }
        case N_INPUT: {
            char buf[1024];
            if (fgets(buf, sizeof(buf), stdin)) {
                size_t len = strlen(buf);
                while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = 0;
                Value s = make_string(buf, (int)len);
                env_define(I->env, n->str, s);
                value_release(s);
            } else {
                Value s = make_string_cstr("");
                env_define(I->env, n->str, s);
                value_release(s);
            }
            return;
        }
        case N_RETURN: {
            Value v = n->a ? interp_eval(I, n->a) : NULL_VAL();
            I->return_value = v;
            I->flow = FLOW_RETURN;
            return;
        }
        case N_FUNC: {
            Value f = make_function(n, I->globals);
            env_define(I->env, n->str, f);
            value_release(f);
            return;
        }
        case N_CLASS: {
            ObjClass* k = (ObjClass*)malloc(sizeof(ObjClass));
            k->refcount = 1;
            k->name = strdup(n->str);
            Value md = make_dict();
            k->methods = md.as.dict;
            for (int i = 0; i < n->item_count; i++) {
                Node* m = n->items[i];
                Value f = make_function(m, I->globals);
                dict_set_take(k->methods, m->str, f);
            }
            Value kv;
            kv.type = VAL_CLASS;
            kv.as.klass = k;
            env_define(I->env, n->str, kv);
            value_release(kv);
            return;
        }
        case N_IF: {
            Value cond = interp_eval(I, n->a);
            int t = value_truthy(cond);
            value_release(cond);
            if (t) interp_exec(I, n->b);
            else if (n->c) interp_exec(I, n->c);
            return;
        }
        case N_LOOP: {
            Value cv = interp_eval(I, n->a);
            if (cv.type != VAL_NUMBER) { value_release(cv); interp_error(I, n->line, "Loop count must be a number"); }
            int count = (int)cv.as.number;
            value_release(cv);
            for (int i = 0; i < count; i++) {
                interp_exec(I, n->b);
                if (I->flow == FLOW_CONTINUE) { I->flow = FLOW_NORMAL; continue; }
                if (I->flow == FLOW_BREAK) { I->flow = FLOW_NORMAL; break; }
                if (I->flow != FLOW_NORMAL) return;
            }
            return;
        }
        case N_WHILE: {
            for (;;) {
                Value cond = interp_eval(I, n->a);
                int t = value_truthy(cond);
                value_release(cond);
                if (!t) break;
                interp_exec(I, n->b);
                if (I->flow == FLOW_CONTINUE) { I->flow = FLOW_NORMAL; continue; }
                if (I->flow == FLOW_BREAK) { I->flow = FLOW_NORMAL; break; }
                if (I->flow != FLOW_NORMAL) return;
            }
            return;
        }
        case N_FOR: {
            Value it = interp_eval(I, n->a);
            int count = 0;
            if (it.type == VAL_LIST) count = it.as.list->count;
            else if (it.type == VAL_STRING) count = it.as.str->length;
            else if (it.type == VAL_DICT) count = it.as.dict->count;
            else { value_release(it); interp_error(I, n->line, "Can only iterate over a list, string, or dict"); }
            for (int i = 0; i < count; i++) {
                Value item;
                if (it.type == VAL_LIST) item = value_retain(it.as.list->items[i]);
                else if (it.type == VAL_STRING) item = make_string(&it.as.str->chars[i], 1);
                else item = make_string_cstr(it.as.dict->entries[i].key);
                env_define(I->env, n->str, item);
                value_release(item);
                interp_exec(I, n->b);
                if (I->flow == FLOW_CONTINUE) { I->flow = FLOW_NORMAL; continue; }
                if (I->flow == FLOW_BREAK) { I->flow = FLOW_NORMAL; break; }
                if (I->flow != FLOW_NORMAL) { value_release(it); return; }
            }
            value_release(it);
            return;
        }
        case N_BREAK:
            I->flow = FLOW_BREAK;
            return;
        case N_CONTINUE:
            I->flow = FLOW_CONTINUE;
            return;
        case N_TRY: {
            jmp_buf jb;
            if (I->handler_count >= 64) interp_error(I, n->line, "Exception handler overflow");
            I->handlers[I->handler_count++] = &jb;
            if (setjmp(jb) == 0) {
                interp_exec(I, n->a);
                I->handler_count--;
            } else {
                I->handler_count--;
                I->flow = FLOW_NORMAL;
                Value msg = make_string_cstr(I->error_msg);
                env_define(I->env, n->str, msg);
                value_release(msg);
                interp_exec(I, n->b);
            }
            return;
        }
        case N_THREAD: {
            Value v = interp_eval(I, n->a);
            value_release(v);
            return;
        }
        case N_IMPORT:
            do_import(I, n->str, n->line);
            return;
        case N_EXPRSTMT: {
            Value v = interp_eval(I, n->a);
            value_release(v);
            return;
        }
        default:
            interp_error(I, n->line, "Cannot execute this statement");
            return;
    }
}

void interp_run(Interp* I, Node* program) {
    jmp_buf base;
    I->handlers[I->handler_count++] = &base;
    if (setjmp(base) == 0) {
        I->env = I->globals;
        I->flow = FLOW_NORMAL;
        for (int i = 0; i < program->item_count; i++) {
            interp_exec(I, program->items[i]);
            if (I->flow != FLOW_NORMAL) break;
        }
    } else {
        fprintf(stderr, "FC Runtime Error on Line %d: %s\n", I->error_line, I->error_msg);
        I->had_error = 1;
    }
    I->handler_count--;
}
