#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "interpreter.h"

static double xn(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_NUMBER) interp_error(I, 0, "%s expects a number argument", fn);
    return a[i].as.number;
}
static const char* xs(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_STRING) interp_error(I, 0, "%s expects a string argument", fn);
    return a[i].as.str->chars;
}
static void af(ObjDict* d, const char* name, NativeFn fn) {
    dict_set_take(d, name, make_native(fn, NULL_VAL(), name));
}

static Value cv_to_base(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    long n = (long)xn(I, argc, a, 0, "convert.to_base");
    int base = (int)xn(I, argc, a, 1, "convert.to_base");
    if (base < 2 || base > 36) interp_error(I, 0, "convert.to_base: base must be 2-36");
    char tmp[80];
    int i = 0, neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    if (n == 0) tmp[i++] = '0';
    while (n > 0) {
        int d = (int)(n % base);
        tmp[i++] = d < 10 ? (char)('0' + d) : (char)('a' + d - 10);
        n /= base;
    }
    char out[82];
    int j = 0;
    if (neg) out[j++] = '-';
    while (i > 0) out[j++] = tmp[--i];
    out[j] = 0;
    return make_string(out, j);
}
static Value cv_from_base(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = xs(I, argc, a, 0, "convert.from_base");
    int base = (int)xn(I, argc, a, 1, "convert.from_base");
    return NUMBER_VAL((double)strtol(str, NULL, base));
}
static Value cv_bin(Interp* I, Value s, int argc, Value* a) {
    (void)argc;
    Value b = NUMBER_VAL(2);
    Value args[2];
    args[0] = a[0];
    args[1] = b;
    return cv_to_base(I, s, 2, args);
}
static Value cv_hex(Interp* I, Value s, int argc, Value* a) {
    (void)argc;
    Value b = NUMBER_VAL(16);
    Value args[2];
    args[0] = a[0];
    args[1] = b;
    return cv_to_base(I, s, 2, args);
}
static Value cv_oct(Interp* I, Value s, int argc, Value* a) {
    (void)argc;
    Value b = NUMBER_VAL(8);
    Value args[2];
    args[0] = a[0];
    args[1] = b;
    return cv_to_base(I, s, 2, args);
}

static Value io_print(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    for (int i = 0; i < argc; i++) {
        if (i) printf(" ");
        char* str = value_to_string(a[i], 0);
        printf("%s", str);
        free(str);
    }
    printf("\n");
    return NULL_VAL();
}
static Value io_write(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    for (int i = 0; i < argc; i++) {
        char* str = value_to_string(a[i], 0);
        printf("%s", str);
        free(str);
    }
    return NULL_VAL();
}
static Value io_eprint(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    for (int i = 0; i < argc; i++) {
        if (i) fprintf(stderr, " ");
        char* str = value_to_string(a[i], 0);
        fprintf(stderr, "%s", str);
        free(str);
    }
    fprintf(stderr, "\n");
    return NULL_VAL();
}
static Value io_read_line(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return NULL_VAL();
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = 0;
    return make_string(buf, (int)len);
}
static Value io_read_all(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    int cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = (char)c;
    }
    buf[len] = 0;
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value io_flush(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    fflush(stdout);
    return NULL_VAL();
}

static Value sys_platform(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
#if defined(_WIN32)
    return make_string_cstr("windows");
#elif defined(__ANDROID__)
    return make_string_cstr("android");
#elif defined(__APPLE__)
    return make_string_cstr("darwin");
#else
    return make_string_cstr("linux");
#endif
}
static Value sys_exit(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    exit(argc >= 1 && a[0].type == VAL_NUMBER ? (int)a[0].as.number : 0);
    return NULL_VAL();
}
static Value sys_args(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc; (void)a;
    Value l = make_list();
    for (int i = 0; i < I->argc; i++) list_append_take(l.as.list, make_string_cstr(I->argv[i]));
    return l;
}

static Value uuid_v4(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    static const char* hx = "0123456789abcdef";
    char u[37];
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) u[i] = '-';
        else if (i == 14) u[i] = '4';
        else {
            int r = rand() & 15;
            if (i == 19) r = (r & 3) | 8;
            u[i] = hx[r];
        }
    }
    u[36] = 0;
    return make_string(u, 36);
}

static Value ansi_wrap(const char* code, const char* text) {
    int len = (int)strlen(text);
    int codelen = (int)strlen(code);
    char* buf = (char*)malloc(len + codelen + 16);
    int n = sprintf(buf, "\033[%sm%s\033[0m", code, text);
    Value v = make_string(buf, n);
    free(buf);
    return v;
}
static Value ansi_bold(Interp* I, Value s, int argc, Value* a) { (void)s; return ansi_wrap("1", xs(I, argc, a, 0, "ansi.bold")); }
static Value ansi_dim(Interp* I, Value s, int argc, Value* a) { (void)s; return ansi_wrap("2", xs(I, argc, a, 0, "ansi.dim")); }
static Value ansi_italic(Interp* I, Value s, int argc, Value* a) { (void)s; return ansi_wrap("3", xs(I, argc, a, 0, "ansi.italic")); }
static Value ansi_under(Interp* I, Value s, int argc, Value* a) { (void)s; return ansi_wrap("4", xs(I, argc, a, 0, "ansi.under")); }
static Value ansi_color(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = xs(I, argc, a, 0, "ansi.color");
    int code = (int)xn(I, argc, a, 1, "ansi.color");
    char c[16];
    sprintf(c, "%d", code);
    return ansi_wrap(c, text);
}
static Value ansi_bg(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = xs(I, argc, a, 0, "ansi.bg");
    int code = (int)xn(I, argc, a, 1, "ansi.bg");
    char c[16];
    sprintf(c, "%d", code + 10);
    return ansi_wrap(c, text);
}
static Value ansi_strip(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = xs(I, argc, a, 0, "ansi.strip");
    int n = (int)strlen(in);
    char* buf = (char*)malloc(n + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (in[i] == '\033' && i + 1 < n && in[i + 1] == '[') {
            i += 2;
            while (i < n && in[i] != 'm') i++;
        } else {
            buf[j++] = in[i];
        }
    }
    buf[j] = 0;
    Value v = make_string(buf, j);
    free(buf);
    return v;
}

static ObjList* need_vec(Interp* I, Value v, const char* fn) {
    if (v.type != VAL_LIST) interp_error(I, 0, "%s expects a vector (list of numbers)", fn);
    for (int i = 0; i < v.as.list->count; i++)
        if (v.as.list->items[i].type != VAL_NUMBER) interp_error(I, 0, "%s: vector must contain numbers", fn);
    return v.as.list;
}
static Value vec_add(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "vec.add expects 2 vectors");
    ObjList* x = need_vec(I, a[0], "vec.add");
    ObjList* y = need_vec(I, a[1], "vec.add");
    if (x->count != y->count) interp_error(I, 0, "vec.add: length mismatch");
    Value out = make_list();
    for (int i = 0; i < x->count; i++) list_append_take(out.as.list, NUMBER_VAL(x->items[i].as.number + y->items[i].as.number));
    return out;
}
static Value vec_sub(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "vec.sub expects 2 vectors");
    ObjList* x = need_vec(I, a[0], "vec.sub");
    ObjList* y = need_vec(I, a[1], "vec.sub");
    if (x->count != y->count) interp_error(I, 0, "vec.sub: length mismatch");
    Value out = make_list();
    for (int i = 0; i < x->count; i++) list_append_take(out.as.list, NUMBER_VAL(x->items[i].as.number - y->items[i].as.number));
    return out;
}
static Value vec_scale(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    ObjList* x = need_vec(I, a[0], "vec.scale");
    double k = xn(I, argc, a, 1, "vec.scale");
    Value out = make_list();
    for (int i = 0; i < x->count; i++) list_append_take(out.as.list, NUMBER_VAL(x->items[i].as.number * k));
    return out;
}
static Value vec_dot(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "vec.dot expects 2 vectors");
    ObjList* x = need_vec(I, a[0], "vec.dot");
    ObjList* y = need_vec(I, a[1], "vec.dot");
    if (x->count != y->count) interp_error(I, 0, "vec.dot: length mismatch");
    double d = 0;
    for (int i = 0; i < x->count; i++) d += x->items[i].as.number * y->items[i].as.number;
    return NUMBER_VAL(d);
}
static Value vec_length(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    ObjList* x = need_vec(I, a[0], "vec.length");
    double d = 0;
    for (int i = 0; i < x->count; i++) d += x->items[i].as.number * x->items[i].as.number;
    return NUMBER_VAL(sqrt(d));
}
static Value vec_normalize(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    ObjList* x = need_vec(I, a[0], "vec.normalize");
    double d = 0;
    for (int i = 0; i < x->count; i++) d += x->items[i].as.number * x->items[i].as.number;
    d = sqrt(d);
    Value out = make_list();
    for (int i = 0; i < x->count; i++) list_append_take(out.as.list, NUMBER_VAL(d == 0 ? 0 : x->items[i].as.number / d));
    return out;
}
static Value vec_distance(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "vec.distance expects 2 vectors");
    ObjList* x = need_vec(I, a[0], "vec.distance");
    ObjList* y = need_vec(I, a[1], "vec.distance");
    if (x->count != y->count) interp_error(I, 0, "vec.distance: length mismatch");
    double d = 0;
    for (int i = 0; i < x->count; i++) {
        double t = x->items[i].as.number - y->items[i].as.number;
        d += t * t;
    }
    return NUMBER_VAL(sqrt(d));
}
static Value vec_lerp(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 3) interp_error(I, 0, "vec.lerp expects (a, b, t)");
    ObjList* x = need_vec(I, a[0], "vec.lerp");
    ObjList* y = need_vec(I, a[1], "vec.lerp");
    double t = xn(I, argc, a, 2, "vec.lerp");
    if (x->count != y->count) interp_error(I, 0, "vec.lerp: length mismatch");
    Value out = make_list();
    for (int i = 0; i < x->count; i++) {
        double av = x->items[i].as.number;
        double bv = y->items[i].as.number;
        list_append_take(out.as.list, NUMBER_VAL(av + (bv - av) * t));
    }
    return out;
}

static Value tpl_render(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* tpl = xs(I, argc, a, 0, "template.render");
    if (argc < 2 || a[1].type != VAL_DICT) interp_error(I, 0, "template.render expects (string, dict)");
    ObjDict* d = a[1].as.dict;
    int cap = 64, len = 0;
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    const char* p = tpl;
    while (*p) {
        if (p[0] == '{' && p[1] == '{') {
            const char* end = strstr(p + 2, "}}");
            if (!end) break;
            char key[128];
            int klen = (int)(end - (p + 2));
            int ks = 0;
            for (int i = 0; i < klen && ks < 127; i++) {
                char c = p[2 + i];
                if (c != ' ' && c != '\t') key[ks++] = c;
            }
            key[ks] = 0;
            Value val;
            char* ins = NULL;
            const char* show = "";
            if (dict_get(d, key, &val)) { ins = value_to_string(val, 0); show = ins; value_release(val); }
            int il = (int)strlen(show);
            if (len + il + 1 > cap) { while (len + il + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
            memcpy(buf + len, show, il);
            len += il;
            buf[len] = 0;
            if (ins) free(ins);
            p = end + 2;
        } else {
            if (len + 2 > cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = *p++;
            buf[len] = 0;
        }
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}

void register_extras(Interp* I) {
    Value convert = make_dict();
    af(convert.as.dict, "to_base", cv_to_base);
    af(convert.as.dict, "from_base", cv_from_base);
    af(convert.as.dict, "bin", cv_bin);
    af(convert.as.dict, "hex", cv_hex);
    af(convert.as.dict, "oct", cv_oct);
    env_define(I->globals, "convert", convert);
    value_release(convert);

    Value io = make_dict();
    af(io.as.dict, "print", io_print);
    af(io.as.dict, "write", io_write);
    af(io.as.dict, "eprint", io_eprint);
    af(io.as.dict, "read_line", io_read_line);
    af(io.as.dict, "read_all", io_read_all);
    af(io.as.dict, "flush", io_flush);
    env_define(I->globals, "io", io);
    value_release(io);

    Value sys = make_dict();
    af(sys.as.dict, "platform", sys_platform);
    af(sys.as.dict, "exit", sys_exit);
    af(sys.as.dict, "args", sys_args);
    dict_set_take(sys.as.dict, "version", make_string_cstr("1.0"));
    dict_set_take(sys.as.dict, "name", make_string_cstr("FC"));
    env_define(I->globals, "sys", sys);
    value_release(sys);

    Value uuid = make_dict();
    af(uuid.as.dict, "v4", uuid_v4);
    env_define(I->globals, "uuid", uuid);
    value_release(uuid);

    Value ansi = make_dict();
    af(ansi.as.dict, "bold", ansi_bold);
    af(ansi.as.dict, "dim", ansi_dim);
    af(ansi.as.dict, "italic", ansi_italic);
    af(ansi.as.dict, "under", ansi_under);
    af(ansi.as.dict, "color", ansi_color);
    af(ansi.as.dict, "bg", ansi_bg);
    af(ansi.as.dict, "strip", ansi_strip);
    env_define(I->globals, "ansi", ansi);
    value_release(ansi);

    Value vec = make_dict();
    af(vec.as.dict, "add", vec_add);
    af(vec.as.dict, "sub", vec_sub);
    af(vec.as.dict, "scale", vec_scale);
    af(vec.as.dict, "dot", vec_dot);
    af(vec.as.dict, "length", vec_length);
    af(vec.as.dict, "normalize", vec_normalize);
    af(vec.as.dict, "distance", vec_distance);
    af(vec.as.dict, "lerp", vec_lerp);
    env_define(I->globals, "vec", vec);
    value_release(vec);

    Value tpl = make_dict();
    af(tpl.as.dict, "render", tpl_render);
    env_define(I->globals, "template", tpl);
    value_release(tpl);
}
