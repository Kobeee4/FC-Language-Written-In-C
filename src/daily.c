#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>
#include "interpreter.h"

#if defined(_WIN32)
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

static double dn(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_NUMBER) interp_error(I, 0, "%s expects a number argument", fn);
    return a[i].as.number;
}
static const char* ds(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_STRING) interp_error(I, 0, "%s expects a string argument", fn);
    return a[i].as.str->chars;
}
static void af(ObjDict* d, const char* name, NativeFn fn) {
    dict_set_take(d, name, make_native(fn, NULL_VAL(), name));
}
static void mod(Interp* I, const char* name, Value d) {
    env_define(I->globals, name, d);
    value_release(d);
}

static int hv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static Value en_hex(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "encode.hex");
    int n = (int)strlen(t);
    static const char* hx = "0123456789abcdef";
    char* b = (char*)malloc(n * 2 + 1);
    for (int i = 0; i < n; i++) { unsigned char c = (unsigned char)t[i]; b[i * 2] = hx[c >> 4]; b[i * 2 + 1] = hx[c & 15]; }
    b[n * 2] = 0;
    Value v = make_string(b, n * 2);
    free(b);
    return v;
}
static Value en_unhex(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "encode.unhex");
    int n = (int)strlen(t);
    char* b = (char*)malloc(n / 2 + 1);
    int j = 0;
    for (int i = 0; i + 1 < n; i += 2) { int hi = hv(t[i]), lo = hv(t[i + 1]); if (hi < 0 || lo < 0) break; b[j++] = (char)((hi << 4) | lo); }
    b[j] = 0;
    Value v = make_string(b, j);
    free(b);
    return v;
}
static Value en_url(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = ds(I, argc, a, 0, "encode.url");
    static const char* hx = "0123456789ABCDEF";
    int n = (int)strlen(in);
    char* b = (char*)malloc(n * 3 + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)in[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') b[j++] = c;
        else { b[j++] = '%'; b[j++] = hx[c >> 4]; b[j++] = hx[c & 15]; }
    }
    b[j] = 0;
    Value v = make_string(b, j);
    free(b);
    return v;
}
static Value en_unurl(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = ds(I, argc, a, 0, "encode.url_decode");
    int n = (int)strlen(in);
    char* b = (char*)malloc(n + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (in[i] == '%' && i + 2 < n) { int hi = hv(in[i + 1]), lo = hv(in[i + 2]); if (hi >= 0 && lo >= 0) { b[j++] = (char)((hi << 4) | lo); i += 2; continue; } }
        b[j++] = (in[i] == '+') ? ' ' : in[i];
    }
    b[j] = 0;
    Value v = make_string(b, j);
    free(b);
    return v;
}
static Value en_rot13(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "encode.rot13");
    int n = (int)strlen(t);
    char* b = (char*)malloc(n + 1);
    for (int i = 0; i < n; i++) {
        char c = t[i];
        if (c >= 'a' && c <= 'z') b[i] = 'a' + (c - 'a' + 13) % 26;
        else if (c >= 'A' && c <= 'Z') b[i] = 'A' + (c - 'A' + 13) % 26;
        else b[i] = c;
    }
    b[n] = 0;
    Value v = make_string(b, n);
    free(b);
    return v;
}

static uint32_t crc32_of(const char* s) {
    uint32_t crc = 0xFFFFFFFF;
    for (const char* p = s; *p; p++) {
        crc ^= (unsigned char)*p;
        for (int k = 0; k < 8; k++) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
    return crc ^ 0xFFFFFFFF;
}
static Value h_md5(Interp* I, Value s, int argc, Value* a) { (void)s; const char* t = ds(I, argc, a, 0, "hash.md5"); char o[33]; fc_md5((const unsigned char*)t, strlen(t), o); return make_string_cstr(o); }
static Value h_sha1(Interp* I, Value s, int argc, Value* a) { (void)s; const char* t = ds(I, argc, a, 0, "hash.sha1"); char o[41]; fc_sha1((const unsigned char*)t, strlen(t), o); return make_string_cstr(o); }
static Value h_sha256(Interp* I, Value s, int argc, Value* a) { (void)s; const char* t = ds(I, argc, a, 0, "hash.sha256"); char o[65]; fc_sha256((const unsigned char*)t, strlen(t), o); return make_string_cstr(o); }
static Value h_crc32(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)crc32_of(ds(I, argc, a, 0, "hash.crc32"))); }
static Value h_fnv1a(Interp* I, Value s, int argc, Value* a) { (void)s; const char* t = ds(I, argc, a, 0, "hash.fnv1a"); unsigned int h = 2166136261u; for (const char* p = t; *p; p++) { h ^= (unsigned char)*p; h *= 16777619u; } return NUMBER_VAL((double)h); }

static int rx_classmatch(const char* cls, int len, char c) {
    int neg = 0, i = 0, res = 0;
    if (i < len && cls[i] == '^') { neg = 1; i++; }
    while (i < len) {
        if (cls[i] == '\\' && i + 1 < len) {
            char e = cls[i + 1];
            if (e == 'd') { if (c >= '0' && c <= '9') res = 1; }
            else if (e == 'w') { if (isalnum((unsigned char)c) || c == '_') res = 1; }
            else if (e == 's') { if (c == ' ' || c == '\t' || c == '\n' || c == '\r') res = 1; }
            else if (e == c) res = 1;
            i += 2;
            continue;
        }
        if (i + 2 < len && cls[i + 1] == '-') { if (c >= cls[i] && c <= cls[i + 2]) res = 1; i += 3; continue; }
        if (cls[i] == c) res = 1;
        i++;
    }
    return neg ? !res : res;
}
static int rx_elemlen(const char* p) {
    if (*p == '\\' && p[1]) return 2;
    if (*p == '[') {
        const char* q = p + 1;
        if (*q == '^') q++;
        if (*q == ']') q++;
        while (*q && *q != ']') { if (*q == '\\' && q[1]) q++; q++; }
        if (*q == ']') q++;
        return (int)(q - p);
    }
    return 1;
}
static int rx_one(const char* el, int ellen, char c) {
    if (c == 0) return 0;
    if (el[0] == '\\') {
        char e = el[1];
        if (e == 'd') return c >= '0' && c <= '9';
        if (e == 'w') return isalnum((unsigned char)c) || c == '_';
        if (e == 's') return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        if (e == 'D') return !(c >= '0' && c <= '9');
        if (e == 'W') return !(isalnum((unsigned char)c) || c == '_');
        if (e == 'S') return !(c == ' ' || c == '\t' || c == '\n' || c == '\r');
        return e == c;
    }
    if (el[0] == '[') return rx_classmatch(el + 1, ellen - 2, c);
    if (el[0] == '.') return 1;
    return el[0] == c;
}
static int rx_here(const char* pat, const char* text) {
    if (*pat == 0) return 0;
    if (pat[0] == '$' && pat[1] == 0) return *text == 0 ? 0 : -1;
    int el = rx_elemlen(pat);
    char q = pat[el];
    if (q == '*' || q == '+' || q == '?') {
        const char* rest = pat + el + 1;
        if (q == '?') {
            if (*text && rx_one(pat, el, *text)) { int r = rx_here(rest, text + 1); if (r >= 0) return 1 + r; }
            int r = rx_here(rest, text);
            return r < 0 ? -1 : r;
        }
        int minc = (q == '+') ? 1 : 0;
        int count = 0;
        const char* t = text;
        while (*t && rx_one(pat, el, *t)) { t++; count++; }
        for (int k = count; k >= minc; k--) { int r = rx_here(rest, text + k); if (r >= 0) return k + r; }
        return -1;
    }
    if (*text && rx_one(pat, el, *text)) { int r = rx_here(pat + el, text + 1); if (r >= 0) return 1 + r; }
    return -1;
}
static int rx_match(const char* pat, const char* text, int* start, int* len) {
    if (pat[0] == '^') { int r = rx_here(pat + 1, text); if (r >= 0) { *start = 0; *len = r; return 1; } return 0; }
    int i = 0;
    for (const char* t = text;; t++, i++) {
        int r = rx_here(pat, t);
        if (r >= 0) { *start = i; *len = r; return 1; }
        if (*t == 0) break;
    }
    return 0;
}
static Value re_test(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* pat = ds(I, argc, a, 0, "re.test");
    const char* txt = ds(I, argc, a, 1, "re.test");
    int st, ln;
    return BOOL_VAL(rx_match(pat, txt, &st, &ln));
}
static Value re_find(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* pat = ds(I, argc, a, 0, "re.find");
    const char* txt = ds(I, argc, a, 1, "re.find");
    int st, ln;
    if (rx_match(pat, txt, &st, &ln)) return make_string(txt + st, ln);
    return NULL_VAL();
}
static Value re_find_all(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* pat = ds(I, argc, a, 0, "re.find_all");
    const char* txt = ds(I, argc, a, 1, "re.find_all");
    Value out = make_list();
    const char* p = txt;
    while (*p) {
        int st, ln;
        if (!rx_match(pat, p, &st, &ln)) break;
        list_append_take(out.as.list, make_string(p + st, ln));
        p += st + (ln > 0 ? ln : 1);
    }
    return out;
}

static Value tm_parts_dict(time_t t) {
    struct tm* tm = localtime(&t);
    Value d = make_dict();
    if (tm) {
        dict_set_take(d.as.dict, "year", NUMBER_VAL(tm->tm_year + 1900));
        dict_set_take(d.as.dict, "month", NUMBER_VAL(tm->tm_mon + 1));
        dict_set_take(d.as.dict, "day", NUMBER_VAL(tm->tm_mday));
        dict_set_take(d.as.dict, "hour", NUMBER_VAL(tm->tm_hour));
        dict_set_take(d.as.dict, "minute", NUMBER_VAL(tm->tm_min));
        dict_set_take(d.as.dict, "second", NUMBER_VAL(tm->tm_sec));
        dict_set_take(d.as.dict, "weekday", NUMBER_VAL(tm->tm_wday));
    }
    return d;
}
static Value dt_now(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; return tm_parts_dict(time(NULL)); }
static Value dt_format(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    time_t t = (time_t)dn(I, argc, a, 0, "datetime.format");
    const char* fmt = (argc >= 2 && a[1].type == VAL_STRING) ? a[1].as.str->chars : "%Y-%m-%d %H:%M:%S";
    char buf[128];
    struct tm* tm = localtime(&t);
    if (!tm) return make_string_cstr("");
    strftime(buf, sizeof(buf), fmt, tm);
    return make_string_cstr(buf);
}
static Value dt_iso(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    time_t t = (time_t)dn(I, argc, a, 0, "datetime.iso");
    char buf[64];
    struct tm* tm = localtime(&t);
    if (!tm) return make_string_cstr("");
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);
    return make_string_cstr(buf);
}
static Value dt_add_days(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double t = dn(I, argc, a, 0, "datetime.add_days");
    double n = dn(I, argc, a, 1, "datetime.add_days");
    return NUMBER_VAL(t + n * 86400.0);
}
static Value dt_weekday(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    static const char* names[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    time_t t = (time_t)dn(I, argc, a, 0, "datetime.weekday");
    struct tm* tm = localtime(&t);
    if (!tm) return make_string_cstr("");
    return make_string_cstr(names[tm->tm_wday % 7]);
}
static Value dt_diff(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return NUMBER_VAL(dn(I, argc, a, 0, "datetime.diff") - dn(I, argc, a, 1, "datetime.diff"));
}

static Value fs_list(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = ds(I, argc, a, 0, "fs.list");
    Value out = make_list();
#if !defined(_WIN32)
    DIR* d = opendir(path);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
            list_append_take(out.as.list, make_string_cstr(e->d_name));
        }
        closedir(d);
    }
#else
    (void)path;
#endif
    return out;
}
static int path_present(const char* p) { FILE* f = fopen(p, "rb"); if (f) { fclose(f); return 1; } return 0; }
static Value fs_exists(Interp* I, Value s, int argc, Value* a) { (void)s; return BOOL_VAL(path_present(ds(I, argc, a, 0, "fs.exists"))); }
static Value fs_is_dir(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = ds(I, argc, a, 0, "fs.is_dir");
#if !defined(_WIN32)
    struct stat st;
    if (stat(p, &st) == 0) return BOOL_VAL((st.st_mode & S_IFMT) == S_IFDIR);
#else
    (void)p;
#endif
    return BOOL_VAL(0);
}
static Value fs_is_file(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = ds(I, argc, a, 0, "fs.is_file");
#if !defined(_WIN32)
    struct stat st;
    if (stat(p, &st) == 0) return BOOL_VAL((st.st_mode & S_IFMT) == S_IFREG);
#else
    return BOOL_VAL(path_present(p));
#endif
    return BOOL_VAL(0);
}
static Value fs_mkdir(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = ds(I, argc, a, 0, "fs.mkdir");
#if !defined(_WIN32)
    return BOOL_VAL(mkdir(p, 0755) == 0);
#else
    return BOOL_VAL(_mkdir(p) == 0);
#endif
}
static Value fs_remove(Interp* I, Value s, int argc, Value* a) { (void)s; return BOOL_VAL(remove(ds(I, argc, a, 0, "fs.remove")) == 0); }
static Value fs_rename(Interp* I, Value s, int argc, Value* a) { (void)s; const char* x = ds(I, argc, a, 0, "fs.rename"); const char* y = ds(I, argc, a, 1, "fs.rename"); return BOOL_VAL(rename(x, y) == 0); }
static Value fs_cwd(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    char buf[4096];
#if !defined(_WIN32)
    if (getcwd(buf, sizeof(buf))) return make_string_cstr(buf);
#else
    if (_getcwd(buf, sizeof(buf))) return make_string_cstr(buf);
#endif
    return make_string_cstr("");
}
static Value fs_size(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    FILE* f = fopen(ds(I, argc, a, 0, "fs.size"), "rb");
    if (!f) return NUMBER_VAL(-1);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return NUMBER_VAL((double)sz);
}

static Value va_email(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* e = ds(I, argc, a, 0, "validate.email");
    const char* at = strchr(e, '@');
    if (!at || at == e) return BOOL_VAL(0);
    const char* dot = strchr(at, '.');
    if (!dot || dot == at + 1 || dot[1] == 0) return BOOL_VAL(0);
    return BOOL_VAL(1);
}
static Value va_url(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* u = ds(I, argc, a, 0, "validate.url");
    return BOOL_VAL(strncmp(u, "http://", 7) == 0 || strncmp(u, "https://", 8) == 0);
}
static Value va_ipv4(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* ip = ds(I, argc, a, 0, "validate.ipv4");
    int parts = 0, val = 0, digits = 0;
    for (const char* p = ip;; p++) {
        if (*p >= '0' && *p <= '9') { val = val * 10 + (*p - '0'); digits++; if (val > 255 || digits > 3) return BOOL_VAL(0); }
        else if (*p == '.' || *p == 0) {
            if (digits == 0) return BOOL_VAL(0);
            parts++;
            val = 0;
            digits = 0;
            if (*p == 0) break;
        } else return BOOL_VAL(0);
    }
    return BOOL_VAL(parts == 4);
}
static Value va_integer(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "validate.integer");
    if (*t == '-' || *t == '+') t++;
    if (*t == 0) return BOOL_VAL(0);
    for (; *t; t++) if (*t < '0' || *t > '9') return BOOL_VAL(0);
    return BOOL_VAL(1);
}
static Value va_alpha(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "validate.alpha");
    if (*t == 0) return BOOL_VAL(0);
    for (; *t; t++) if (!isalpha((unsigned char)*t)) return BOOL_VAL(0);
    return BOOL_VAL(1);
}
static Value va_alnum(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* t = ds(I, argc, a, 0, "validate.alnum");
    if (*t == 0) return BOOL_VAL(0);
    for (; *t; t++) if (!isalnum((unsigned char)*t)) return BOOL_VAL(0);
    return BOOL_VAL(1);
}

static Value cli_parse(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "cli.parse expects a list of arguments");
    ObjList* args = a[0].as.list;
    Value flags = make_dict();
    Value positional = make_list();
    for (int i = 0; i < args->count; i++) {
        if (args->items[i].type != VAL_STRING) continue;
        const char* arg = args->items[i].as.str->chars;
        if (arg[0] == '-' && arg[1] == '-') {
            const char* key = arg + 2;
            const char* eq = strchr(key, '=');
            if (eq) {
                char* k = (char*)malloc(eq - key + 1);
                memcpy(k, key, eq - key);
                k[eq - key] = 0;
                dict_set_take(flags.as.dict, k, make_string_cstr(eq + 1));
                free(k);
            } else if (i + 1 < args->count && args->items[i + 1].type == VAL_STRING && args->items[i + 1].as.str->chars[0] != '-') {
                dict_set_take(flags.as.dict, key, value_retain(args->items[i + 1]));
                i++;
            } else {
                dict_set_take(flags.as.dict, key, BOOL_VAL(1));
            }
        } else if (arg[0] == '-' && arg[1]) {
            char k[2] = { arg[1], 0 };
            dict_set_take(flags.as.dict, k, BOOL_VAL(1));
        } else {
            list_append_take(positional.as.list, value_retain(args->items[i]));
        }
    }
    Value out = make_dict();
    dict_set_take(out.as.dict, "flags", flags);
    dict_set_take(out.as.dict, "args", positional);
    return out;
}

static void log_at(const char* level, Value v) {
    char ts[32];
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    if (tm) strftime(ts, sizeof(ts), "%H:%M:%S", tm);
    else strcpy(ts, "??:??:??");
    char* msg = value_to_string(v, 0);
    fprintf(stderr, "[%s] %s %s\n", level, ts, msg);
    free(msg);
}
static Value lg_info(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; if (argc) log_at("INFO", a[0]); return NULL_VAL(); }
static Value lg_warn(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; if (argc) log_at("WARN", a[0]); return NULL_VAL(); }
static Value lg_error(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; if (argc) log_at("ERROR", a[0]); return NULL_VAL(); }
static Value lg_debug(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; if (argc) log_at("DEBUG", a[0]); return NULL_VAL(); }

static Value sh_run(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* cmd = ds(I, argc, a, 0, "shell.run");
#if defined(_WIN32)
    FILE* p = _popen(cmd, "r");
#else
    FILE* p = popen(cmd, "r");
#endif
    if (!p) interp_error(I, 0, "shell.run: cannot start command");
    int cap = 256, len = 0;
    char* buf = (char*)malloc(cap);
    int c;
    while ((c = fgetc(p)) != EOF) { if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); } buf[len++] = (char)c; }
    buf[len] = 0;
#if defined(_WIN32)
    _pclose(p);
#else
    pclose(p);
#endif
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value sh_code(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return NUMBER_VAL((double)system(ds(I, argc, a, 0, "shell.code")));
}

static Value ht_name(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    char buf[256];
#if !defined(_WIN32)
    if (gethostname(buf, sizeof(buf)) == 0) return make_string_cstr(buf);
#endif
    return make_string_cstr("unknown");
}
static Value ht_resolve(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* host = ds(I, argc, a, 0, "host.resolve");
#if !defined(_WIN32)
    struct hostent* he = gethostbyname(host);
    if (!he) return NULL_VAL();
    struct in_addr addr;
    memcpy(&addr, he->h_addr_list[0], sizeof(addr));
    return make_string_cstr(inet_ntoa(addr));
#else
    (void)host;
    return NULL_VAL();
#endif
}
static Value ht_scan(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* host = ds(I, argc, a, 0, "host.scan");
    int p0 = (int)dn(I, argc, a, 1, "host.scan");
    int p1 = (int)dn(I, argc, a, 2, "host.scan");
    Value open = make_list();
#if !defined(_WIN32)
    struct hostent* he = gethostbyname(host);
    if (!he) return open;
    struct in_addr ip;
    memcpy(&ip, he->h_addr_list[0], sizeof(ip));
    for (int port = p0; port <= p1; port++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons((unsigned short)port);
        sa.sin_addr = ip;
        int r = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
        if (r == 0) {
            list_append_take(open.as.list, NUMBER_VAL(port));
        } else {
            fd_set wf;
            FD_ZERO(&wf);
            FD_SET(sock, &wf);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 300000;
            if (select(sock + 1, NULL, &wf, NULL, &tv) > 0) {
                int err = 0;
                socklen_t l = sizeof(err);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &l);
                if (err == 0) list_append_take(open.as.list, NUMBER_VAL(port));
            }
        }
        close(sock);
    }
#else
    (void)host; (void)p0; (void)p1;
#endif
    return open;
}

void register_daily(Interp* I) {
    Value enc = make_dict();
    af(enc.as.dict, "hex", en_hex);
    af(enc.as.dict, "unhex", en_unhex);
    af(enc.as.dict, "url", en_url);
    af(enc.as.dict, "url_decode", en_unurl);
    af(enc.as.dict, "rot13", en_rot13);
    mod(I, "encode", enc);

    Value hash = make_dict();
    af(hash.as.dict, "md5", h_md5);
    af(hash.as.dict, "sha1", h_sha1);
    af(hash.as.dict, "sha256", h_sha256);
    af(hash.as.dict, "crc32", h_crc32);
    af(hash.as.dict, "fnv1a", h_fnv1a);
    mod(I, "hash", hash);

    Value re = make_dict();
    af(re.as.dict, "test", re_test);
    af(re.as.dict, "find", re_find);
    af(re.as.dict, "find_all", re_find_all);
    mod(I, "re", re);

    Value dt = make_dict();
    af(dt.as.dict, "now", dt_now);
    af(dt.as.dict, "format", dt_format);
    af(dt.as.dict, "iso", dt_iso);
    af(dt.as.dict, "add_days", dt_add_days);
    af(dt.as.dict, "weekday", dt_weekday);
    af(dt.as.dict, "diff", dt_diff);
    mod(I, "datetime", dt);

    Value fs = make_dict();
    af(fs.as.dict, "list", fs_list);
    af(fs.as.dict, "exists", fs_exists);
    af(fs.as.dict, "is_dir", fs_is_dir);
    af(fs.as.dict, "is_file", fs_is_file);
    af(fs.as.dict, "mkdir", fs_mkdir);
    af(fs.as.dict, "remove", fs_remove);
    af(fs.as.dict, "rename", fs_rename);
    af(fs.as.dict, "cwd", fs_cwd);
    af(fs.as.dict, "size", fs_size);
    mod(I, "fs", fs);

    Value va = make_dict();
    af(va.as.dict, "email", va_email);
    af(va.as.dict, "url", va_url);
    af(va.as.dict, "ipv4", va_ipv4);
    af(va.as.dict, "integer", va_integer);
    af(va.as.dict, "alpha", va_alpha);
    af(va.as.dict, "alnum", va_alnum);
    mod(I, "validate", va);

    Value cli = make_dict();
    af(cli.as.dict, "parse", cli_parse);
    mod(I, "cli", cli);

    Value lg = make_dict();
    af(lg.as.dict, "info", lg_info);
    af(lg.as.dict, "warn", lg_warn);
    af(lg.as.dict, "error", lg_error);
    af(lg.as.dict, "debug", lg_debug);
    mod(I, "log", lg);

    Value sh = make_dict();
    af(sh.as.dict, "run", sh_run);
    af(sh.as.dict, "code", sh_code);
    mod(I, "shell", sh);

    Value ht = make_dict();
    af(ht.as.dict, "name", ht_name);
    af(ht.as.dict, "resolve", ht_resolve);
    af(ht.as.dict, "scan", ht_scan);
    mod(I, "host", ht);
}
