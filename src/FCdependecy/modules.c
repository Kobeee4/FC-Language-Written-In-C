#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "interpreter.h"

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#include <conio.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

static double arg_num(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_NUMBER) interp_error(I, 0, "%s expects a number argument", fn);
    return a[i].as.number;
}
static const char* arg_str(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_STRING) interp_error(I, 0, "%s expects a string argument", fn);
    return a[i].as.str->chars;
}

static Value m_cbrt(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(cbrt(arg_num(I, argc, a, 0, "math.cbrt"))); }
static Value m_log2(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(log2(arg_num(I, argc, a, 0, "math.log2"))); }
static Value m_trunc(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(trunc(arg_num(I, argc, a, 0, "math.trunc"))); }
static Value c_fnv1a(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "crypto.fnv1a");
    unsigned int h = 2166136261u;
    for (const char* p = str; *p; p++) { h ^= (unsigned char)*p; h *= 16777619u; }
    return NUMBER_VAL((double)h);
}
static Value rnd_bool(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; return BOOL_VAL(rand() & 1); }
static Value rnd_shuffle(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "random.shuffle expects a list");
    ObjList* src = a[0].as.list;
    Value out = make_list();
    for (int i = 0; i < src->count; i++) list_append_take(out.as.list, value_retain(src->items[i]));
    ObjList* l = out.as.list;
    for (int i = l->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Value t = l->items[i];
        l->items[i] = l->items[j];
        l->items[j] = t;
    }
    return out;
}
static Value rnd_sample(Interp* I, Value s, int argc, Value* a) {
    if (argc < 2 || a[0].type != VAL_LIST) interp_error(I, 0, "random.sample expects (list, n)");
    int n = (int)arg_num(I, argc, a, 1, "random.sample");
    Value sh = rnd_shuffle(I, s, 1, a);
    ObjList* l = sh.as.list;
    if (n > l->count) n = l->count;
    if (n < 0) n = 0;
    Value out = make_list();
    for (int i = 0; i < n; i++) list_append_take(out.as.list, value_retain(l->items[i]));
    value_release(sh);
    return out;
}

static Value m_sqrt(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(sqrt(arg_num(I, argc, a, 0, "math.sqrt"))); }
static Value m_pow(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(pow(arg_num(I, argc, a, 0, "math.pow"), arg_num(I, argc, a, 1, "math.pow"))); }
static Value m_abs(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(fabs(arg_num(I, argc, a, 0, "math.abs"))); }
static Value m_floor(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(floor(arg_num(I, argc, a, 0, "math.floor"))); }
static Value m_ceil(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(ceil(arg_num(I, argc, a, 0, "math.ceil"))); }
static Value m_sin(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(sin(arg_num(I, argc, a, 0, "math.sin"))); }
static Value m_cos(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(cos(arg_num(I, argc, a, 0, "math.cos"))); }
static Value m_random(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; return NUMBER_VAL((double)rand() / ((double)RAND_MAX + 1.0)); }
static Value m_tan(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(tan(arg_num(I, argc, a, 0, "math.tan"))); }
static Value m_log(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(log(arg_num(I, argc, a, 0, "math.log"))); }
static Value m_log10(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(log10(arg_num(I, argc, a, 0, "math.log10"))); }
static Value m_exp(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(exp(arg_num(I, argc, a, 0, "math.exp"))); }
static Value m_round(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(round(arg_num(I, argc, a, 0, "math.round"))); }
static Value m_hypot(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(hypot(arg_num(I, argc, a, 0, "math.hypot"), arg_num(I, argc, a, 1, "math.hypot"))); }
static Value m_atan(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(atan(arg_num(I, argc, a, 0, "math.atan"))); }
static Value m_atan2(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(atan2(arg_num(I, argc, a, 0, "math.atan2"), arg_num(I, argc, a, 1, "math.atan2"))); }
static Value m_min(Interp* I, Value s, int argc, Value* a) { (void)s; double x = arg_num(I, argc, a, 0, "math.min"), y = arg_num(I, argc, a, 1, "math.min"); return NUMBER_VAL(x < y ? x : y); }
static Value m_max(Interp* I, Value s, int argc, Value* a) { (void)s; double x = arg_num(I, argc, a, 0, "math.max"), y = arg_num(I, argc, a, 1, "math.max"); return NUMBER_VAL(x > y ? x : y); }

static Value c_hash(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* algo = arg_str(I, argc, a, 0, "crypto.hash");
    const char* data = arg_str(I, argc, a, 1, "crypto.hash");
    if (strcmp(algo, "sha256") != 0) interp_error(I, 0, "crypto.hash only supports 'sha256'");
    char hex[65];
    fc_sha256((const unsigned char*)data, strlen(data), hex);
    return make_string_cstr(hex);
}

static Value t_now(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; return NUMBER_VAL((double)time(NULL)); }
static Value t_sleep(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double sec = arg_num(I, argc, a, 0, "time.sleep");
#if defined(_WIN32)
    Sleep((DWORD)(sec * 1000));
#else
    struct timespec ts;
    ts.tv_sec = (time_t)sec;
    ts.tv_nsec = (long)((sec - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif
    return NULL_VAL();
}
static Value t_format(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    time_t t = (time_t)arg_num(I, argc, a, 0, "time.format");
    char buf[64];
    struct tm* tm = localtime(&t);
    if (!tm) return make_string_cstr("");
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return make_string_cstr(buf);
}

static int is_number_list(Value v) {
    if (v.type != VAL_LIST) return 0;
    for (int i = 0; i < v.as.list->count; i++)
        if (v.as.list->items[i].type != VAL_NUMBER) return 0;
    return 1;
}

static Value ai_matrix(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int rows = (int)arg_num(I, argc, a, 0, "ai.matrix");
    int cols = (int)arg_num(I, argc, a, 1, "ai.matrix");
    Value m = make_list();
    for (int r = 0; r < rows; r++) {
        Value row = make_list();
        for (int c = 0; c < cols; c++) list_append_take(row.as.list, NUMBER_VAL(0));
        list_append_take(m.as.list, row);
    }
    return m;
}

static Value ai_dot(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || a[0].type != VAL_LIST || a[1].type != VAL_LIST)
        interp_error(I, 0, "ai.dot expects two matrices");
    ObjList* m1 = a[0].as.list;
    ObjList* m2 = a[1].as.list;
    if (m1->count == 0 || m2->count == 0) interp_error(I, 0, "ai.dot: empty matrix");
    if (m1->items[0].type != VAL_LIST || m2->items[0].type != VAL_LIST)
        interp_error(I, 0, "ai.dot expects nested lists");
    int n = m1->items[0].as.list->count;
    int p = m2->items[0].as.list->count;
    if (m2->count != n) interp_error(I, 0, "ai.dot: dimension mismatch");
    Value out = make_list();
    for (int i = 0; i < m1->count; i++) {
        ObjList* ri = m1->items[i].as.list;
        Value row = make_list();
        for (int j = 0; j < p; j++) {
            double sum = 0;
            for (int kk = 0; kk < n; kk++) {
                double x = ri->items[kk].as.number;
                double y = m2->items[kk].as.list->items[j].as.number;
                sum += x * y;
            }
            list_append_take(row.as.list, NUMBER_VAL(sum));
        }
        list_append_take(out.as.list, row);
    }
    return out;
}

static Value ai_train_linear(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || !is_number_list(a[0]) || !is_number_list(a[1]))
        interp_error(I, 0, "ai.train_linear expects two number lists");
    ObjList* xs = a[0].as.list;
    ObjList* ys = a[1].as.list;
    int n = xs->count < ys->count ? xs->count : ys->count;
    if (n < 2) interp_error(I, 0, "ai.train_linear needs at least 2 points");
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; i++) {
        double x = xs->items[i].as.number;
        double y = ys->items[i].as.number;
        sx += x; sy += y; sxx += x * x; sxy += x * y;
    }
    double denom = (n * sxx - sx * sx);
    if (denom == 0) interp_error(I, 0, "ai.train_linear: cannot fit");
    double m = (n * sxy - sx * sy) / denom;
    double b = (sy - m * sx) / n;
    Value model = make_dict();
    dict_set_take(model.as.dict, "m", NUMBER_VAL(m));
    dict_set_take(model.as.dict, "b", NUMBER_VAL(b));
    return model;
}

static Value ai_predict(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || a[0].type != VAL_DICT) interp_error(I, 0, "ai.predict expects (model, input)");
    Value mv, bv;
    if (!dict_get(a[0].as.dict, "m", &mv) || !dict_get(a[0].as.dict, "b", &bv))
        interp_error(I, 0, "ai.predict: invalid model");
    double m = mv.as.number, b = bv.as.number;
    value_release(mv);
    value_release(bv);
    if (a[1].type == VAL_NUMBER) return NUMBER_VAL(m * a[1].as.number + b);
    if (a[1].type == VAL_LIST) {
        Value out = make_list();
        for (int i = 0; i < a[1].as.list->count; i++) {
            double x = a[1].as.list->items[i].as.number;
            list_append_take(out.as.list, NUMBER_VAL(m * x + b));
        }
        return out;
    }
    interp_error(I, 0, "ai.predict: input must be number or list");
    return NULL_VAL();
}

static Value f_read(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.read");
    char* data = read_file(path);
    if (!data) interp_error(I, 0, "file.read: cannot open '%s'", path);
    Value v = make_string_cstr(data);
    free(data);
    return v;
}
static Value f_write(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.write");
    const char* data = arg_str(I, argc, a, 1, "file.write");
    FILE* f = fopen(path, "wb");
    if (!f) interp_error(I, 0, "file.write: cannot open '%s'", path);
    size_t n = fwrite(data, 1, strlen(data), f);
    fclose(f);
    return NUMBER_VAL((double)n);
}

static Value db_query(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* sql = arg_str(I, argc, a, 0, "db.query");
    Value rows = make_list();
    Value r1 = make_dict();
    dict_set_take(r1.as.dict, "id", NUMBER_VAL(1));
    dict_set_take(r1.as.dict, "name", make_string_cstr("alpha"));
    dict_set_take(r1.as.dict, "query", make_string_cstr(sql));
    Value r2 = make_dict();
    dict_set_take(r2.as.dict, "id", NUMBER_VAL(2));
    dict_set_take(r2.as.dict, "name", make_string_cstr("beta"));
    dict_set_take(r2.as.dict, "query", make_string_cstr(sql));
    list_append_take(rows.as.list, r1);
    list_append_take(rows.as.list, r2);
    return rows;
}

static unsigned char* g_canvas = NULL;
static int g_w = 0, g_h = 0;

static Value gui_window(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* title = arg_str(I, argc, a, 0, "gui.window");
    int w = (int)arg_num(I, argc, a, 1, "gui.window");
    int h = (int)arg_num(I, argc, a, 2, "gui.window");
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) interp_error(I, 0, "gui.window: invalid size");
    if (g_canvas) free(g_canvas);
    g_w = w; g_h = h;
    g_canvas = (unsigned char*)calloc((size_t)w * h * 3, 1);
    printf("[gui] window '%s' %dx%d created\n", title, w, h);
    return NULL_VAL();
}
static Value gui_draw_pixel(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (!g_canvas) interp_error(I, 0, "gui.draw_pixel: no window");
    int x = (int)arg_num(I, argc, a, 0, "gui.draw_pixel");
    int y = (int)arg_num(I, argc, a, 1, "gui.draw_pixel");
    int color = (int)arg_num(I, argc, a, 2, "gui.draw_pixel");
    if (x < 0 || y < 0 || x >= g_w || y >= g_h) return NULL_VAL();
    size_t off = ((size_t)y * g_w + x) * 3;
    g_canvas[off] = (color >> 16) & 0xff;
    g_canvas[off + 1] = (color >> 8) & 0xff;
    g_canvas[off + 2] = color & 0xff;
    return NULL_VAL();
}
static Value gui_refresh(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    if (!g_canvas) return NULL_VAL();
    FILE* f = fopen("fc_canvas.ppm", "wb");
    if (!f) return NULL_VAL();
    fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
    fwrite(g_canvas, 1, (size_t)g_w * g_h * 3, f);
    fclose(f);
    printf("[gui] refreshed -> fc_canvas.ppm\n");
    return NULL_VAL();
}

#if !defined(_WIN32)
static Value net_fetch_url(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* url = arg_str(I, argc, a, 0, "net.fetch_url");
    const char* p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;
    char host[256];
    int port = 80;
    char path[1024] = "/";
    int i = 0;
    while (*p && *p != '/' && *p != ':' && i < 255) host[i++] = *p++;
    host[i] = 0;
    if (*p == ':') {
        p++;
        port = atoi(p);
        while (*p && *p != '/') p++;
    }
    if (*p == '/') { strncpy(path, p, sizeof(path) - 1); path[sizeof(path) - 1] = 0; }

    struct hostent* he = gethostbyname(host);
    if (!he) interp_error(I, 0, "net.fetch_url: cannot resolve host '%s'", host);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "net.fetch_url: socket failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        interp_error(I, 0, "net.fetch_url: connection failed");
    }
    char req[1400];
    snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    send(sock, req, strlen(req), 0);

    int cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    int r;
    char tmp[2048];
    while ((r = recv(sock, tmp, sizeof(tmp), 0)) > 0) {
        if (len + r + 1 > cap) { while (len + r + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf + len, tmp, r);
        len += r;
    }
    close(sock);
    buf[len] = 0;
    char* body = strstr(buf, "\r\n\r\n");
    Value v = make_string_cstr(body ? body + 4 : buf);
    free(buf);
    return v;
}
static Value net_send(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* ip = arg_str(I, argc, a, 0, "net.send");
    int port = (int)arg_num(I, argc, a, 1, "net.send");
    const char* data = arg_str(I, argc, a, 2, "net.send");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "net.send: socket failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        interp_error(I, 0, "net.send: connection failed");
    }
    ssize_t n = send(sock, data, strlen(data), 0);
    close(sock);
    return NUMBER_VAL((double)n);
}
static Value net_listen(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int port = (int)arg_num(I, argc, a, 0, "net.listen");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "net.listen: socket failed");
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); interp_error(I, 0, "net.listen: bind failed"); }
    if (listen(sock, 1) < 0) { close(sock); interp_error(I, 0, "net.listen: listen failed"); }
    int client = accept(sock, NULL, NULL);
    if (client < 0) { close(sock); interp_error(I, 0, "net.listen: accept failed"); }
    char buf[4096];
    ssize_t n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n < 0) n = 0;
    buf[n] = 0;
    const char* resp = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nFC server ok\n";
    send(client, resp, strlen(resp), 0);
    close(client);
    close(sock);
    return make_string_cstr(buf);
}
#else
static Value net_fetch_url(Interp* I, Value s, int argc, Value* a) { (void)s; (void)argc; (void)a; (void)I; return make_string_cstr("[net disabled on Windows build]"); }
static Value net_send(Interp* I, Value s, int argc, Value* a) { (void)s; (void)argc; (void)a; (void)I; return NUMBER_VAL(0); }
static Value net_listen(Interp* I, Value s, int argc, Value* a) { (void)s; (void)argc; (void)a; (void)I; return make_string_cstr("[net disabled on Windows build]"); }
#endif

static void json_skip(const char** p) {
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
}
static Value json_value(Interp* I, const char** p);

static Value json_string_token(Interp* I, const char** p) {
    (*p)++;
    int cap = 16, len = 0;
    char* buf = (char*)malloc(cap);
    while (**p && **p != '"') {
        char c = **p;
        if (c == '\\') {
            (*p)++;
            char e = **p;
            if (e == 'n') c = '\n';
            else if (e == 't') c = '\t';
            else if (e == 'r') c = '\r';
            else if (e == '"') c = '"';
            else if (e == '\\') c = '\\';
            else if (e == '/') c = '/';
            else c = e;
        }
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = c;
        (*p)++;
    }
    if (**p != '"') { free(buf); interp_error(I, 0, "json.parse: unterminated string"); }
    (*p)++;
    buf[len] = 0;
    Value v = make_string(buf, len);
    free(buf);
    return v;
}

static Value json_value(Interp* I, const char** p) {
    json_skip(p);
    char c = **p;
    if (c == '"') return json_string_token(I, p);
    if (c == '{') {
        (*p)++;
        Value d = make_dict();
        json_skip(p);
        if (**p == '}') { (*p)++; return d; }
        for (;;) {
            json_skip(p);
            if (**p != '"') interp_error(I, 0, "json.parse: expected key string");
            Value key = json_string_token(I, p);
            json_skip(p);
            if (**p != ':') interp_error(I, 0, "json.parse: expected ':'");
            (*p)++;
            Value val = json_value(I, p);
            dict_set_take(d.as.dict, key.as.str->chars, val);
            value_release(key);
            json_skip(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == '}') { (*p)++; break; }
            interp_error(I, 0, "json.parse: expected ',' or '}'");
        }
        return d;
    }
    if (c == '[') {
        (*p)++;
        Value l = make_list();
        json_skip(p);
        if (**p == ']') { (*p)++; return l; }
        for (;;) {
            Value val = json_value(I, p);
            list_append_take(l.as.list, val);
            json_skip(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == ']') { (*p)++; break; }
            interp_error(I, 0, "json.parse: expected ',' or ']'");
        }
        return l;
    }
    if (strncmp(*p, "true", 4) == 0) { *p += 4; return BOOL_VAL(1); }
    if (strncmp(*p, "false", 5) == 0) { *p += 5; return BOOL_VAL(0); }
    if (strncmp(*p, "null", 4) == 0) { *p += 4; return NULL_VAL(); }
    if (c == '-' || (c >= '0' && c <= '9')) {
        char* end;
        double d = strtod(*p, &end);
        *p = end;
        return NUMBER_VAL(d);
    }
    interp_error(I, 0, "json.parse: unexpected character");
    return NULL_VAL();
}

static Value json_parse(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "json.parse");
    const char* p = str;
    return json_value(I, &p);
}
static Value json_stringify(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    if (argc < 1) return make_string_cstr("null");
    char* str = value_to_string(a[0], 1);
    Value v = make_string_cstr(str);
    free(str);
    return v;
}

static int is_callable(Value v) {
    return v.type == VAL_FUNCTION || v.type == VAL_NATIVE || v.type == VAL_BOUND || v.type == VAL_CLASS;
}

static Value fn_map(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || !is_callable(a[0]) || a[1].type != VAL_LIST) interp_error(I, 0, "fn.map expects (function, list)");
    Value out = make_list();
    ObjList* l = a[1].as.list;
    for (int i = 0; i < l->count; i++) {
        Value arg = value_retain(l->items[i]);
        Value r = interp_call_value(I, a[0], 0, 1, &arg);
        value_release(arg);
        list_append_take(out.as.list, r);
    }
    return out;
}
static Value fn_filter(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || !is_callable(a[0]) || a[1].type != VAL_LIST) interp_error(I, 0, "fn.filter expects (function, list)");
    Value out = make_list();
    ObjList* l = a[1].as.list;
    for (int i = 0; i < l->count; i++) {
        Value arg = value_retain(l->items[i]);
        Value r = interp_call_value(I, a[0], 0, 1, &arg);
        int keep = value_truthy(r);
        value_release(r);
        if (keep) list_append_take(out.as.list, arg);
        else value_release(arg);
    }
    return out;
}
static Value fn_reduce(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 3 || !is_callable(a[0]) || a[1].type != VAL_LIST) interp_error(I, 0, "fn.reduce expects (function, list, initial)");
    ObjList* l = a[1].as.list;
    Value acc = value_retain(a[2]);
    for (int i = 0; i < l->count; i++) {
        Value args[2];
        args[0] = acc;
        args[1] = value_retain(l->items[i]);
        Value r = interp_call_value(I, a[0], 0, 2, args);
        value_release(args[0]);
        value_release(args[1]);
        acc = r;
    }
    return acc;
}
static Value fn_each(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || !is_callable(a[0]) || a[1].type != VAL_LIST) interp_error(I, 0, "fn.each expects (function, list)");
    ObjList* l = a[1].as.list;
    for (int i = 0; i < l->count; i++) {
        Value arg = value_retain(l->items[i]);
        Value r = interp_call_value(I, a[0], 0, 1, &arg);
        value_release(arg);
        value_release(r);
    }
    return NULL_VAL();
}

static Value t_clock(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    return NUMBER_VAL((double)clock() / (double)CLOCKS_PER_SEC);
}
static Value t_parts(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    time_t t = (time_t)arg_num(I, argc, a, 0, "time.parts");
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

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static Value c_b64encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "crypto.base64_encode");
    size_t len = strlen(str);
    size_t outlen = 4 * ((len + 2) / 3);
    char* out = (char*)malloc(outlen + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = (unsigned char)str[i] << 16;
        if (i + 1 < len) n |= (unsigned char)str[i + 1] << 8;
        if (i + 2 < len) n |= (unsigned char)str[i + 2];
        out[j++] = B64[(n >> 18) & 63];
        out[j++] = B64[(n >> 12) & 63];
        out[j++] = (i + 1 < len) ? B64[(n >> 6) & 63] : '=';
        out[j++] = (i + 2 < len) ? B64[n & 63] : '=';
    }
    out[j] = 0;
    Value v = make_string(out, (int)j);
    free(out);
    return v;
}
static int b64val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}
static Value c_b64decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "crypto.base64_decode");
    size_t len = strlen(str);
    char* out = (char*)malloc(len + 1);
    size_t j = 0;
    int buf = 0, bits = 0;
    for (size_t i = 0; i < len; i++) {
        int v = b64val(str[i]);
        if (v < 0) continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (char)((buf >> bits) & 0xff);
        }
    }
    out[j] = 0;
    Value r = make_string(out, (int)j);
    free(out);
    return r;
}

static Value str_upper(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "str.upper");
    Value v = make_string_cstr(str);
    for (int i = 0; i < v.as.str->length; i++) { char c = v.as.str->chars[i]; if (c >= 'a' && c <= 'z') v.as.str->chars[i] = c - 32; }
    return v;
}
static Value str_lower(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "str.lower");
    Value v = make_string_cstr(str);
    for (int i = 0; i < v.as.str->length; i++) { char c = v.as.str->chars[i]; if (c >= 'A' && c <= 'Z') v.as.str->chars[i] = c + 32; }
    return v;
}
static Value str_trim(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "str.trim");
    int n = (int)strlen(str), start = 0, end = n;
    while (start < end && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r')) start++;
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\n' || str[end - 1] == '\r')) end--;
    return make_string(&str[start], end - start);
}
static Value str_split(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "str.split");
    const char* sep = (argc >= 2 && a[1].type == VAL_STRING) ? a[1].as.str->chars : " ";
    Value list = make_list();
    int seplen = (int)strlen(sep);
    if (seplen == 0) { list_append_take(list.as.list, make_string_cstr(str)); return list; }
    const char* start = str;
    const char* found;
    while ((found = strstr(start, sep)) != NULL) {
        list_append_take(list.as.list, make_string(start, (int)(found - start)));
        start = found + seplen;
    }
    list_append_take(list.as.list, make_string_cstr(start));
    return list;
}
static Value str_join(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "str.join expects (list, sep)");
    const char* sep = (argc >= 2 && a[1].type == VAL_STRING) ? a[1].as.str->chars : "";
    ObjList* l = a[0].as.list;
    int cap = 32, len = 0, seplen = (int)strlen(sep);
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    for (int i = 0; i < l->count; i++) {
        char* item = value_to_string(l->items[i], 0);
        int il = (int)strlen(item);
        int need = len + il + (i ? seplen : 0) + 1;
        if (need > cap) { while (need > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        if (i) { memcpy(buf + len, sep, seplen); len += seplen; }
        memcpy(buf + len, item, il); len += il;
        buf[len] = 0;
        free(item);
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value str_replace(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "str.replace");
    const char* old = arg_str(I, argc, a, 1, "str.replace");
    const char* neu = arg_str(I, argc, a, 2, "str.replace");
    int oldlen = (int)strlen(old), newlen = (int)strlen(neu);
    if (oldlen == 0) return make_string_cstr(str);
    int cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    const char* p = str;
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
static Value str_format(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* tpl = arg_str(I, argc, a, 0, "str.format");
    int cap = 32, len = 0, next = 1;
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    for (const char* p = tpl; *p; p++) {
        if (p[0] == '{' && p[1] == '}') {
            const char* ins = "";
            char* tmp = NULL;
            if (next < argc) { tmp = value_to_string(a[next++], 0); ins = tmp; }
            int il = (int)strlen(ins);
            if (len + il + 1 > cap) { while (len + il + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
            memcpy(buf + len, ins, il); len += il;
            if (tmp) free(tmp);
            p++;
        } else {
            if (len + 2 > cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = *p;
        }
        buf[len] = 0;
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}

static Value os_getenv(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* name = arg_str(I, argc, a, 0, "os.getenv");
    const char* val = getenv(name);
    return val ? make_string_cstr(val) : NULL_VAL();
}
static Value os_system(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* cmd = arg_str(I, argc, a, 0, "os.system");
    int code = system(cmd);
    return NUMBER_VAL((double)code);
}
static Value os_exit(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    int code = (argc >= 1 && a[0].type == VAL_NUMBER) ? (int)a[0].as.number : 0;
    exit(code);
    return NULL_VAL();
}
static Value os_platform(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
#if defined(_WIN32)
    return make_string_cstr("windows");
#else
    return make_string_cstr("posix");
#endif
}
static Value os_cwd(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    char buf[4096];
#if defined(_WIN32)
    if (_getcwd(buf, sizeof(buf))) return make_string_cstr(buf);
#else
    if (getcwd(buf, sizeof(buf))) return make_string_cstr(buf);
#endif
    return make_string_cstr("");
}
static Value os_args(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc; (void)a;
    Value list = make_list();
    for (int i = 0; i < I->argc; i++) list_append_take(list.as.list, make_string_cstr(I->argv[i]));
    return list;
}

static Value rnd_seed(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    srand((unsigned int)arg_num(I, argc, a, 0, "random.seed"));
    return NULL_VAL();
}
static Value rnd_float(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    return NUMBER_VAL((double)rand() / ((double)RAND_MAX + 1.0));
}
static Value rnd_int(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int lo = (int)arg_num(I, argc, a, 0, "random.int");
    int hi = (int)arg_num(I, argc, a, 1, "random.int");
    if (hi < lo) { int t = lo; lo = hi; hi = t; }
    int span = hi - lo + 1;
    return NUMBER_VAL(lo + rand() % span);
}
static Value rnd_choice(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST || a[0].as.list->count == 0) interp_error(I, 0, "random.choice expects a non-empty list");
    ObjList* l = a[0].as.list;
    return value_retain(l->items[rand() % l->count]);
}

static double f_sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
static double f_relu(double x) { return x > 0 ? x : 0; }
static double f_leaky(double x) { return x > 0 ? x : 0.01 * x; }
static double f_dsigmoid(double x) { double s = f_sigmoid(x); return s * (1 - s); }
static double f_drelu(double x) { return x > 0 ? 1 : 0; }
static double f_dtanh(double x) { double t = tanh(x); return 1 - t * t; }

static Value nn_unary(Interp* I, int argc, Value* a, double (*f)(double), const char* name) {
    if (argc < 1) interp_error(I, 0, "%s expects an argument", name);
    if (a[0].type == VAL_NUMBER) return NUMBER_VAL(f(a[0].as.number));
    if (a[0].type == VAL_LIST) {
        Value out = make_list();
        ObjList* l = a[0].as.list;
        for (int i = 0; i < l->count; i++) {
            if (l->items[i].type != VAL_NUMBER) interp_error(I, 0, "%s: list must contain numbers", name);
            list_append_take(out.as.list, NUMBER_VAL(f(l->items[i].as.number)));
        }
        return out;
    }
    interp_error(I, 0, "%s expects a number or list", name);
    return NULL_VAL();
}
static Value nn_sigmoid(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_sigmoid, "nn.sigmoid"); }
static Value nn_relu(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_relu, "nn.relu"); }
static Value nn_tanh(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, tanh, "nn.tanh"); }
static Value nn_leaky_relu(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_leaky, "nn.leaky_relu"); }
static Value nn_dsigmoid(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_dsigmoid, "nn.dsigmoid"); }
static Value nn_drelu(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_drelu, "nn.drelu"); }
static Value nn_dtanh(Interp* I, Value s, int argc, Value* a) { (void)s; return nn_unary(I, argc, a, f_dtanh, "nn.dtanh"); }
static Value nn_softmax(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "nn.softmax expects a list");
    ObjList* l = a[0].as.list;
    double m = -1e308, sum = 0;
    for (int i = 0; i < l->count; i++) { if (l->items[i].type != VAL_NUMBER) interp_error(I, 0, "nn.softmax: numbers only"); if (l->items[i].as.number > m) m = l->items[i].as.number; }
    Value out = make_list();
    for (int i = 0; i < l->count; i++) sum += exp(l->items[i].as.number - m);
    for (int i = 0; i < l->count; i++) list_append_take(out.as.list, NUMBER_VAL(exp(l->items[i].as.number - m) / sum));
    return out;
}

static void mat_dims(Interp* I, Value m, int* rows, int* cols, const char* name) {
    if (m.type != VAL_LIST || m.as.list->count == 0 || m.as.list->items[0].type != VAL_LIST)
        interp_error(I, 0, "%s expects a matrix (list of lists)", name);
    *rows = m.as.list->count;
    *cols = m.as.list->items[0].as.list->count;
}
static double mat_at(Value m, int r, int c) { return m.as.list->items[r].as.list->items[c].as.number; }

static Value ai_elementwise(Interp* I, int argc, Value* a, int sub, const char* name) {
    (void)argc;
    int r1, c1, r2, c2;
    mat_dims(I, a[0], &r1, &c1, name);
    mat_dims(I, a[1], &r2, &c2, name);
    if (r1 != r2 || c1 != c2) interp_error(I, 0, "%s: shape mismatch", name);
    Value out = make_list();
    for (int r = 0; r < r1; r++) {
        Value row = make_list();
        for (int c = 0; c < c1; c++) {
            double v = sub ? mat_at(a[0], r, c) - mat_at(a[1], r, c) : mat_at(a[0], r, c) + mat_at(a[1], r, c);
            list_append_take(row.as.list, NUMBER_VAL(v));
        }
        list_append_take(out.as.list, row);
    }
    return out;
}
static Value ai_add(Interp* I, Value s, int argc, Value* a) { (void)s; if (argc < 2) interp_error(I, 0, "ai.add expects 2 matrices"); return ai_elementwise(I, argc, a, 0, "ai.add"); }
static Value ai_sub(Interp* I, Value s, int argc, Value* a) { (void)s; if (argc < 2) interp_error(I, 0, "ai.sub expects 2 matrices"); return ai_elementwise(I, argc, a, 1, "ai.sub"); }
static Value ai_scale(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int r, c;
    mat_dims(I, a[0], &r, &c, "ai.scale");
    double k = arg_num(I, argc, a, 1, "ai.scale");
    Value out = make_list();
    for (int i = 0; i < r; i++) {
        Value row = make_list();
        for (int j = 0; j < c; j++) list_append_take(row.as.list, NUMBER_VAL(mat_at(a[0], i, j) * k));
        list_append_take(out.as.list, row);
    }
    return out;
}
static Value ai_transpose(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    int r, c;
    mat_dims(I, a[0], &r, &c, "ai.transpose");
    Value out = make_list();
    for (int j = 0; j < c; j++) {
        Value row = make_list();
        for (int i = 0; i < r; i++) list_append_take(row.as.list, NUMBER_VAL(mat_at(a[0], i, j)));
        list_append_take(out.as.list, row);
    }
    return out;
}
static Value ai_apply(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || !is_callable(a[0])) interp_error(I, 0, "ai.apply expects (function, matrix)");
    int r, c;
    mat_dims(I, a[1], &r, &c, "ai.apply");
    Value out = make_list();
    for (int i = 0; i < r; i++) {
        Value row = make_list();
        for (int j = 0; j < c; j++) {
            Value arg = NUMBER_VAL(mat_at(a[1], i, j));
            Value rv = interp_call_value(I, a[0], 0, 1, &arg);
            list_append_take(row.as.list, rv);
        }
        list_append_take(out.as.list, row);
    }
    return out;
}
static Value ai_randn(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int rows = (int)arg_num(I, argc, a, 0, "ai.randn");
    int cols = (int)arg_num(I, argc, a, 1, "ai.randn");
    Value out = make_list();
    for (int i = 0; i < rows; i++) {
        Value row = make_list();
        for (int j = 0; j < cols; j++) list_append_take(row.as.list, NUMBER_VAL((double)rand() / RAND_MAX * 2.0 - 1.0));
        list_append_take(out.as.list, row);
    }
    return out;
}
static Value ai_shape(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    int r, c;
    mat_dims(I, a[0], &r, &c, "ai.shape");
    Value out = make_list();
    list_append_take(out.as.list, NUMBER_VAL(r));
    list_append_take(out.as.list, NUMBER_VAL(c));
    return out;
}

static Value sec_xor(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = arg_str(I, argc, a, 0, "sec.xor");
    const char* key = arg_str(I, argc, a, 1, "sec.xor");
    int tl = (int)strlen(text), kl = (int)strlen(key);
    if (kl == 0) interp_error(I, 0, "sec.xor: empty key");
    char* buf = (char*)malloc(tl + 1);
    for (int i = 0; i < tl; i++) buf[i] = text[i] ^ key[i % kl];
    buf[tl] = 0;
    Value v = make_string(buf, tl);
    free(buf);
    return v;
}
static Value sec_caesar(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = arg_str(I, argc, a, 0, "sec.caesar");
    int shift = ((int)arg_num(I, argc, a, 1, "sec.caesar") % 26 + 26) % 26;
    int n = (int)strlen(text);
    char* buf = (char*)malloc(n + 1);
    for (int i = 0; i < n; i++) {
        char ch = text[i];
        if (ch >= 'a' && ch <= 'z') buf[i] = 'a' + (ch - 'a' + shift) % 26;
        else if (ch >= 'A' && ch <= 'Z') buf[i] = 'A' + (ch - 'A' + shift) % 26;
        else buf[i] = ch;
    }
    buf[n] = 0;
    Value v = make_string(buf, n);
    free(buf);
    return v;
}
static Value sec_rot13(Interp* I, Value s, int argc, Value* a) {
    (void)argc;
    Value shift = NUMBER_VAL(13);
    Value args[2];
    args[0] = a[0];
    args[1] = shift;
    return sec_caesar(I, s, 2, args);
}
static Value sec_hex_encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = arg_str(I, argc, a, 0, "sec.hex_encode");
    int n = (int)strlen(text);
    static const char* hx = "0123456789abcdef";
    char* buf = (char*)malloc(n * 2 + 1);
    for (int i = 0; i < n; i++) {
        unsigned char b = (unsigned char)text[i];
        buf[i * 2] = hx[b >> 4];
        buf[i * 2 + 1] = hx[b & 15];
    }
    buf[n * 2] = 0;
    Value v = make_string(buf, n * 2);
    free(buf);
    return v;
}
static int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static Value sec_hex_decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* hex = arg_str(I, argc, a, 0, "sec.hex_decode");
    int n = (int)strlen(hex);
    char* buf = (char*)malloc(n / 2 + 1);
    int j = 0;
    for (int i = 0; i + 1 < n; i += 2) {
        int hi = hexv(hex[i]), lo = hexv(hex[i + 1]);
        if (hi < 0 || lo < 0) break;
        buf[j++] = (char)((hi << 4) | lo);
    }
    buf[j] = 0;
    Value v = make_string(buf, j);
    free(buf);
    return v;
}
static Value sec_crc32(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = arg_str(I, argc, a, 0, "sec.crc32");
    uint32_t crc = 0xFFFFFFFF;
    for (const char* p = text; *p; p++) {
        crc ^= (unsigned char)*p;
        for (int k = 0; k < 8; k++) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
    return NUMBER_VAL((double)(crc ^ 0xFFFFFFFF));
}
static Value sec_rand_bytes(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int n = (int)arg_num(I, argc, a, 0, "sec.rand_bytes");
    if (n < 0) n = 0;
    if (n > 1000000) interp_error(I, 0, "sec.rand_bytes: too many");
    static const char* hx = "0123456789abcdef";
    char* buf = (char*)malloc(n * 2 + 1);
    for (int i = 0; i < n; i++) {
        int b = rand() & 0xff;
        buf[i * 2] = hx[b >> 4];
        buf[i * 2 + 1] = hx[b & 15];
    }
    buf[n * 2] = 0;
    Value v = make_string(buf, n * 2);
    free(buf);
    return v;
}
static Value sec_strength(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* pw = arg_str(I, argc, a, 0, "sec.strength");
    int lower = 0, upper = 0, digit = 0, sym = 0, len = (int)strlen(pw);
    for (const char* p = pw; *p; p++) {
        if (*p >= 'a' && *p <= 'z') lower = 1;
        else if (*p >= 'A' && *p <= 'Z') upper = 1;
        else if (*p >= '0' && *p <= '9') digit = 1;
        else sym = 1;
    }
    int score = len * 4 + (lower + upper + digit + sym) * 10;
    if (score > 100) score = 100;
    return NUMBER_VAL(score);
}

static Value web_url_encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = arg_str(I, argc, a, 0, "web.url_encode");
    static const char* hx = "0123456789ABCDEF";
    int n = (int)strlen(in);
    char* buf = (char*)malloc(n * 3 + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)in[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            buf[j++] = c;
        } else {
            buf[j++] = '%';
            buf[j++] = hx[c >> 4];
            buf[j++] = hx[c & 15];
        }
    }
    buf[j] = 0;
    Value v = make_string(buf, j);
    free(buf);
    return v;
}
static Value web_url_decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = arg_str(I, argc, a, 0, "web.url_decode");
    int n = (int)strlen(in);
    char* buf = (char*)malloc(n + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (in[i] == '%' && i + 2 < n) {
            int hi = hexv(in[i + 1]), lo = hexv(in[i + 2]);
            if (hi >= 0 && lo >= 0) { buf[j++] = (char)((hi << 4) | lo); i += 2; continue; }
        }
        buf[j++] = (in[i] == '+') ? ' ' : in[i];
    }
    buf[j] = 0;
    Value v = make_string(buf, j);
    free(buf);
    return v;
}
static Value web_html_escape(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = arg_str(I, argc, a, 0, "web.html_escape");
    int cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    for (const char* p = in; *p; p++) {
        const char* rep = NULL;
        if (*p == '<') rep = "&lt;";
        else if (*p == '>') rep = "&gt;";
        else if (*p == '&') rep = "&amp;";
        else if (*p == '"') rep = "&quot;";
        int rl = rep ? (int)strlen(rep) : 1;
        if (len + rl + 1 > cap) { while (len + rl + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        if (rep) { memcpy(buf + len, rep, rl); len += rl; }
        else buf[len++] = *p;
    }
    buf[len] = 0;
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value web_get(Interp* I, Value s, int argc, Value* a) {
    return net_fetch_url(I, s, argc, a);
}
static Value web_serve(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int port = (int)arg_num(I, argc, a, 0, "web.serve");
    if (argc < 2 || !is_callable(a[1])) interp_error(I, 0, "web.serve expects (port, handler)");
#if !defined(_WIN32)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "web.serve: socket failed");
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); interp_error(I, 0, "web.serve: bind failed (port in use?)"); }
    if (listen(sock, 16) < 0) { close(sock); interp_error(I, 0, "web.serve: listen failed"); }
    printf("[web] serving on http://0.0.0.0:%d (Ctrl-C to stop)\n", port);
    for (;;) {
        int client = accept(sock, NULL, NULL);
        if (client < 0) continue;
        char req[8192];
        ssize_t n = recv(client, req, sizeof(req) - 1, 0);
        if (n < 0) n = 0;
        req[n] = 0;
        Value reqv = make_string_cstr(req);
        Value resp = interp_call_value(I, a[1], 0, 1, &reqv);
        value_release(reqv);
        int status = 200;
        char type[128];
        strcpy(type, "text/html");
        char* body;
        if (resp.type == VAL_DICT) {
            Value bv, sv, tv;
            if (dict_get(resp.as.dict, "body", &bv)) { body = value_to_string(bv, 0); value_release(bv); }
            else body = strdup("");
            if (dict_get(resp.as.dict, "status", &sv)) { if (sv.type == VAL_NUMBER) status = (int)sv.as.number; value_release(sv); }
            if (dict_get(resp.as.dict, "type", &tv)) { if (tv.type == VAL_STRING) { strncpy(type, tv.as.str->chars, 127); type[127] = 0; } value_release(tv); }
        } else {
            body = value_to_string(resp, 0);
        }
        value_release(resp);
        int blen = (int)strlen(body);
        const char* stxt = status == 404 ? "404 Not Found" : status == 500 ? "500 Internal Server Error" : status == 302 ? "302 Found" : "200 OK";
        char head[512];
        int hl = snprintf(head, sizeof(head), "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", stxt, type, blen);
        send(client, head, hl, 0);
        send(client, body, blen, 0);
        free(body);
        close(client);
    }
    close(sock);
    return NULL_VAL();
#else
    (void)port;
    interp_error(I, 0, "web.serve is not available on the Windows build");
    return NULL_VAL();
#endif
}

static Value term_clear(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; printf("\033[2J\033[H"); fflush(stdout); return NULL_VAL(); }
static Value term_at(Interp* I, Value s, int argc, Value* a) { (void)s; printf("\033[%d;%dH", (int)arg_num(I, argc, a, 1, "term.at"), (int)arg_num(I, argc, a, 0, "term.at")); fflush(stdout); return NULL_VAL(); }
static Value term_color(Interp* I, Value s, int argc, Value* a) { (void)s; printf("\033[%dm", (int)arg_num(I, argc, a, 0, "term.color")); fflush(stdout); return NULL_VAL(); }
static Value term_bg(Interp* I, Value s, int argc, Value* a) { (void)s; printf("\033[%dm", (int)arg_num(I, argc, a, 0, "term.bg") + 10); fflush(stdout); return NULL_VAL(); }
static Value term_reset(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; printf("\033[0m"); fflush(stdout); return NULL_VAL(); }
static Value term_hide_cursor(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; printf("\033[?25l"); fflush(stdout); return NULL_VAL(); }
static Value term_show_cursor(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; printf("\033[?25h"); fflush(stdout); return NULL_VAL(); }
static Value term_write(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    if (argc >= 1) { char* str = value_to_string(a[0], 0); printf("%s", str); free(str); fflush(stdout); }
    return NULL_VAL();
}

static Value file_exists(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.exists");
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return BOOL_VAL(1); }
    return BOOL_VAL(0);
}
static Value file_delete(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.delete");
    return BOOL_VAL(remove(path) == 0);
}
static Value file_size(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.size");
    FILE* f = fopen(path, "rb");
    if (!f) return NUMBER_VAL(-1);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return NUMBER_VAL((double)sz);
}
static Value file_append(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.append");
    const char* data = arg_str(I, argc, a, 1, "file.append");
    FILE* f = fopen(path, "ab");
    if (!f) interp_error(I, 0, "file.append: cannot open '%s'", path);
    size_t n = fwrite(data, 1, strlen(data), f);
    fclose(f);
    return NUMBER_VAL((double)n);
}
static Value file_read_bytes(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.read_bytes");
    FILE* f = fopen(path, "rb");
    if (!f) interp_error(I, 0, "file.read_bytes: cannot open '%s'", path);
    Value list = make_list();
    int c;
    while ((c = fgetc(f)) != EOF) list_append_take(list.as.list, NUMBER_VAL(c));
    fclose(f);
    return list;
}
static Value file_write_bytes(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.write_bytes");
    if (argc < 2 || a[1].type != VAL_LIST) interp_error(I, 0, "file.write_bytes expects (path, list)");
    FILE* f = fopen(path, "wb");
    if (!f) interp_error(I, 0, "file.write_bytes: cannot open '%s'", path);
    ObjList* l = a[1].as.list;
    for (int i = 0; i < l->count; i++) {
        if (l->items[i].type != VAL_NUMBER) { fclose(f); interp_error(I, 0, "file.write_bytes: list must be numbers"); }
        fputc(((int)l->items[i].as.number) & 0xff, f);
    }
    fclose(f);
    return NUMBER_VAL((double)l->count);
}
static Value file_lines(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "file.lines");
    char* data = read_file(path);
    if (!data) interp_error(I, 0, "file.lines: cannot open '%s'", path);
    Value list = make_list();
    char* start = data;
    char* p = data;
    for (;;) {
        if (*p == '\n' || *p == 0) {
            int len = (int)(p - start);
            if (len > 0 && start[len - 1] == '\r') len--;
            list_append_take(list.as.list, make_string(start, len));
            if (*p == 0) break;
            start = p + 1;
        }
        p++;
    }
    free(data);
    return list;
}

static char* url_decode_n(const char* in, int n) {
    char* buf = (char*)malloc(n + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (in[i] == '%' && i + 2 < n) {
            int hi = hexv(in[i + 1]), lo = hexv(in[i + 2]);
            if (hi >= 0 && lo >= 0) { buf[j++] = (char)((hi << 4) | lo); i += 2; continue; }
        }
        buf[j++] = (in[i] == '+') ? ' ' : in[i];
    }
    buf[j] = 0;
    return buf;
}
static Value web_parse_request(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* req = arg_str(I, argc, a, 0, "web.parse_request");
    Value d = make_dict();
    const char* p = req;
    const char* eol = strstr(p, "\r\n");
    int firstlen = eol ? (int)(eol - p) : (int)strlen(p);
    const char* sp1 = memchr(p, ' ', firstlen);
    if (sp1) {
        dict_set_take(d.as.dict, "method", make_string(p, (int)(sp1 - p)));
        const char* t = sp1 + 1;
        const char* sp2 = memchr(t, ' ', firstlen - (int)(t - p));
        int tlen = sp2 ? (int)(sp2 - t) : firstlen - (int)(t - p);
        const char* q = memchr(t, '?', tlen);
        int pathlen = q ? (int)(q - t) : tlen;
        char* dpath = url_decode_n(t, pathlen);
        dict_set_take(d.as.dict, "path", make_string_cstr(dpath));
        free(dpath);
        Value query = make_dict();
        if (q) {
            const char* qs = q + 1;
            int qlen = tlen - pathlen - 1;
            int start = 0;
            for (int i = 0; i <= qlen; i++) {
                if (i == qlen || qs[i] == '&') {
                    const char* pair = qs + start;
                    int plen = i - start;
                    const char* eq = memchr(pair, '=', plen);
                    if (eq) {
                        char* k = url_decode_n(pair, (int)(eq - pair));
                        char* v = url_decode_n(eq + 1, plen - (int)(eq - pair) - 1);
                        dict_set_take(query.as.dict, k, make_string_cstr(v));
                        free(k); free(v);
                    } else if (plen > 0) {
                        char* k = url_decode_n(pair, plen);
                        dict_set_take(query.as.dict, k, make_string_cstr(""));
                        free(k);
                    }
                    start = i + 1;
                }
            }
        }
        dict_set_take(d.as.dict, "query", query);
    } else {
        dict_set_take(d.as.dict, "method", make_string_cstr(""));
        dict_set_take(d.as.dict, "path", make_string_cstr("/"));
        dict_set_take(d.as.dict, "query", make_dict());
    }
    Value headers = make_dict();
    const char* body = strstr(req, "\r\n\r\n");
    const char* line = eol ? eol + 2 : NULL;
    while (line && (!body || line < body)) {
        const char* le = strstr(line, "\r\n");
        if (!le) break;
        const char* colon = memchr(line, ':', le - line);
        if (colon) {
            char* hk = (char*)malloc(colon - line + 1);
            memcpy(hk, line, colon - line);
            hk[colon - line] = 0;
            const char* vstart = colon + 1;
            while (*vstart == ' ') vstart++;
            dict_set_take(headers.as.dict, hk, make_string(vstart, (int)(le - vstart)));
            free(hk);
        }
        line = le + 2;
    }
    dict_set_take(d.as.dict, "headers", headers);
    dict_set_take(d.as.dict, "body", make_string_cstr(body ? body + 4 : ""));
    return d;
}
static const char* mime_of(const char* path) {
    const char* dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot, ".html") || !strcmp(dot, ".htm")) return "text/html";
    if (!strcmp(dot, ".css")) return "text/css";
    if (!strcmp(dot, ".js")) return "application/javascript";
    if (!strcmp(dot, ".json")) return "application/json";
    if (!strcmp(dot, ".png")) return "image/png";
    if (!strcmp(dot, ".jpg") || !strcmp(dot, ".jpeg")) return "image/jpeg";
    if (!strcmp(dot, ".gif")) return "image/gif";
    if (!strcmp(dot, ".svg")) return "image/svg+xml";
    if (!strcmp(dot, ".txt")) return "text/plain";
    if (!strcmp(dot, ".ppm")) return "image/x-portable-pixmap";
    return "application/octet-stream";
}
static Value web_mime(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return make_string_cstr(mime_of(arg_str(I, argc, a, 0, "web.mime")));
}
static Value web_serve_dir(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int port = (int)arg_num(I, argc, a, 0, "web.serve_dir");
    const char* dir = arg_str(I, argc, a, 1, "web.serve_dir");
#if !defined(_WIN32)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "web.serve_dir: socket failed");
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); interp_error(I, 0, "web.serve_dir: bind failed"); }
    if (listen(sock, 16) < 0) { close(sock); interp_error(I, 0, "web.serve_dir: listen failed"); }
    printf("[web] serving '%s' on http://0.0.0.0:%d (Ctrl-C to stop)\n", dir, port);
    for (;;) {
        int client = accept(sock, NULL, NULL);
        if (client < 0) continue;
        char reqbuf[8192];
        ssize_t rn = recv(client, reqbuf, sizeof(reqbuf) - 1, 0);
        if (rn < 0) rn = 0;
        reqbuf[rn] = 0;
        char path[1024] = "/";
        const char* sp1 = strchr(reqbuf, ' ');
        if (sp1) {
            const char* t = sp1 + 1;
            const char* sp2 = strchr(t, ' ');
            int tlen = sp2 ? (int)(sp2 - t) : 0;
            const char* q = memchr(t, '?', tlen);
            int plen = q ? (int)(q - t) : tlen;
            if (plen > 1000) plen = 1000;
            char* dec = url_decode_n(t, plen);
            strncpy(path, dec, sizeof(path) - 1);
            path[sizeof(path) - 1] = 0;
            free(dec);
        }
        if (strstr(path, "..")) strcpy(path, "/");
        char full[2048];
        if (strcmp(path, "/") == 0) snprintf(full, sizeof(full), "%s/index.html", dir);
        else snprintf(full, sizeof(full), "%s%s", dir, path);
        FILE* f = fopen(full, "rb");
        if (!f) {
            const char* nf = "<h1>404 Not Found</h1>";
            char head[256];
            int hl = snprintf(head, sizeof(head), "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(nf));
            send(client, head, hl, 0);
            send(client, nf, strlen(nf), 0);
        } else {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* data = (char*)malloc(sz);
            size_t got = fread(data, 1, sz, f);
            fclose(f);
            char head[256];
            int hl = snprintf(head, sizeof(head), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", mime_of(full), (long)got);
            send(client, head, hl, 0);
            send(client, data, got, 0);
            free(data);
        }
        close(client);
    }
    close(sock);
    return NULL_VAL();
#else
    (void)port; (void)dir;
    interp_error(I, 0, "web.serve_dir is not available on the Windows build");
    return NULL_VAL();
#endif
}

static void gui_putpixel(int x, int y, int color) {
    if (!g_canvas || x < 0 || y < 0 || x >= g_w || y >= g_h) return;
    size_t off = ((size_t)y * g_w + x) * 3;
    g_canvas[off] = (color >> 16) & 0xff;
    g_canvas[off + 1] = (color >> 8) & 0xff;
    g_canvas[off + 2] = color & 0xff;
}
static Value gui_fill(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (!g_canvas) interp_error(I, 0, "gui.fill: no window");
    int color = (int)arg_num(I, argc, a, 0, "gui.fill");
    for (int y = 0; y < g_h; y++) for (int x = 0; x < g_w; x++) gui_putpixel(x, y, color);
    return NULL_VAL();
}
static Value gui_rect(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int x = (int)arg_num(I, argc, a, 0, "gui.rect");
    int y = (int)arg_num(I, argc, a, 1, "gui.rect");
    int w = (int)arg_num(I, argc, a, 2, "gui.rect");
    int h = (int)arg_num(I, argc, a, 3, "gui.rect");
    int color = (int)arg_num(I, argc, a, 4, "gui.rect");
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) gui_putpixel(x + i, y + j, color);
    return NULL_VAL();
}
static Value gui_line(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int x0 = (int)arg_num(I, argc, a, 0, "gui.line");
    int y0 = (int)arg_num(I, argc, a, 1, "gui.line");
    int x1 = (int)arg_num(I, argc, a, 2, "gui.line");
    int y1 = (int)arg_num(I, argc, a, 3, "gui.line");
    int color = (int)arg_num(I, argc, a, 4, "gui.line");
    int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gui_putpixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return NULL_VAL();
}
static Value gui_save(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* path = arg_str(I, argc, a, 0, "gui.save");
    if (!g_canvas) interp_error(I, 0, "gui.save: no window");
    FILE* f = fopen(path, "wb");
    if (!f) interp_error(I, 0, "gui.save: cannot open '%s'", path);
    fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
    fwrite(g_canvas, 1, (size_t)g_w * g_h * 3, f);
    fclose(f);
    return NULL_VAL();
}

#if !defined(_WIN32)
static struct termios g_orig_term;
static int g_raw = 0;
static Value term_raw(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    int on = (argc >= 1) && value_truthy(a[0]);
    if (on && !g_raw) {
        tcgetattr(0, &g_orig_term);
        struct termios t = g_orig_term;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &t);
        g_raw = 1;
    } else if (!on && g_raw) {
        tcsetattr(0, TCSANOW, &g_orig_term);
        g_raw = 0;
    }
    return NULL_VAL();
}
static Value term_key(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    unsigned char c;
    ssize_t n = read(0, &c, 1);
    if (n == 1) return make_string((char*)&c, 1);
    return make_string_cstr("");
}
static Value term_getch(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    struct termios old, t;
    tcgetattr(0, &old);
    t = old;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
    unsigned char c = 0;
    ssize_t n = read(0, &c, 1);
    tcsetattr(0, TCSANOW, &old);
    if (n == 1) return make_string((char*)&c, 1);
    return make_string_cstr("");
}
static Value term_size(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    struct winsize ws;
    Value out = make_list();
    if (ioctl(1, TIOCGWINSZ, &ws) == 0) {
        list_append_take(out.as.list, NUMBER_VAL(ws.ws_col));
        list_append_take(out.as.list, NUMBER_VAL(ws.ws_row));
    } else {
        list_append_take(out.as.list, NUMBER_VAL(80));
        list_append_take(out.as.list, NUMBER_VAL(24));
    }
    return out;
}
#else
static Value term_raw(Interp* I, Value s, int argc, Value* a) { (void)I; (void)s; (void)argc; (void)a; return NULL_VAL(); }
static Value term_key(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    if (_kbhit()) { char c = (char)_getch(); return make_string(&c, 1); }
    return make_string_cstr("");
}
static Value term_getch(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    char c = (char)_getch();
    return make_string(&c, 1);
}
static Value term_size(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    Value out = make_list();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        list_append_take(out.as.list, NUMBER_VAL(csbi.srWindow.Right - csbi.srWindow.Left + 1));
        list_append_take(out.as.list, NUMBER_VAL(csbi.srWindow.Bottom - csbi.srWindow.Top + 1));
    } else {
        list_append_take(out.as.list, NUMBER_VAL(80));
        list_append_take(out.as.list, NUMBER_VAL(24));
    }
    return out;
}
#endif

static double dgcd(double a, double b) {
    long long x = (long long)a, y = (long long)b;
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    while (y) { long long t = x % y; x = y; y = t; }
    return (double)x;
}
static Value m_gcd(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(dgcd(arg_num(I, argc, a, 0, "math.gcd"), arg_num(I, argc, a, 1, "math.gcd"))); }
static Value m_lcm(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double x = arg_num(I, argc, a, 0, "math.lcm"), y = arg_num(I, argc, a, 1, "math.lcm");
    double g = dgcd(x, y);
    return NUMBER_VAL(g == 0 ? 0 : fabs(x * y) / g);
}
static Value m_factorial(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int n = (int)arg_num(I, argc, a, 0, "math.factorial");
    if (n < 0) interp_error(I, 0, "math.factorial: negative");
    double r = 1;
    for (int i = 2; i <= n; i++) r *= i;
    return NUMBER_VAL(r);
}
static Value m_sign(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double x = arg_num(I, argc, a, 0, "math.sign");
    return NUMBER_VAL(x > 0 ? 1 : (x < 0 ? -1 : 0));
}
static Value m_clamp(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double x = arg_num(I, argc, a, 0, "math.clamp"), lo = arg_num(I, argc, a, 1, "math.clamp"), hi = arg_num(I, argc, a, 2, "math.clamp");
    return NUMBER_VAL(x < lo ? lo : (x > hi ? hi : x));
}
static Value m_deg(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(arg_num(I, argc, a, 0, "math.deg") * 180.0 / 3.14159265358979323846); }
static Value m_rad(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(arg_num(I, argc, a, 0, "math.rad") * 3.14159265358979323846 / 180.0); }

static uint32_t tou(double d) { return (uint32_t)(long long)d; }
static Value bit_and(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(tou(arg_num(I, argc, a, 0, "bit.and")) & tou(arg_num(I, argc, a, 1, "bit.and")))); }
static Value bit_or(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(tou(arg_num(I, argc, a, 0, "bit.or")) | tou(arg_num(I, argc, a, 1, "bit.or")))); }
static Value bit_xor(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(tou(arg_num(I, argc, a, 0, "bit.xor")) ^ tou(arg_num(I, argc, a, 1, "bit.xor")))); }
static Value bit_not(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(~tou(arg_num(I, argc, a, 0, "bit.not")))); }
static Value bit_shl(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(tou(arg_num(I, argc, a, 0, "bit.shl")) << (tou(arg_num(I, argc, a, 1, "bit.shl")) & 31))); }
static Value bit_shr(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL((double)(tou(arg_num(I, argc, a, 0, "bit.shr")) >> (tou(arg_num(I, argc, a, 1, "bit.shr")) & 31))); }

static Value csv_parse(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* text = arg_str(I, argc, a, 0, "csv.parse");
    Value rows = make_list();
    const char* line = text;
    const char* p = text;
    for (;;) {
        if (*p == '\n' || *p == 0) {
            int linelen = (int)(p - line);
            if (linelen > 0 && line[linelen - 1] == '\r') linelen--;
            Value fields = make_list();
            int start = 0;
            for (int i = 0; i <= linelen; i++) {
                if (i == linelen || line[i] == ',') {
                    list_append_take(fields.as.list, make_string(line + start, i - start));
                    start = i + 1;
                }
            }
            list_append_take(rows.as.list, fields);
            if (*p == 0) break;
            line = p + 1;
        }
        p++;
    }
    return rows;
}
static Value csv_stringify(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "csv.stringify expects a list of rows");
    int cap = 64, len = 0;
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    ObjList* rows = a[0].as.list;
    for (int r = 0; r < rows->count; r++) {
        if (rows->items[r].type != VAL_LIST) interp_error(I, 0, "csv.stringify: each row must be a list");
        ObjList* row = rows->items[r].as.list;
        for (int c = 0; c < row->count; c++) {
            char* field = value_to_string(row->items[c], 0);
            int fl = (int)strlen(field);
            int need = len + fl + 2;
            if (need > cap) { while (need > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
            if (c) buf[len++] = ',';
            memcpy(buf + len, field, fl); len += fl;
            free(field);
        }
        buf[len++] = '\n';
        if (len + 1 > cap) { cap = len + 1; buf = (char*)realloc(buf, cap); }
        buf[len] = 0;
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}

static Value bi_sum(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "sum expects a list");
    double total = 0;
    ObjList* l = a[0].as.list;
    for (int i = 0; i < l->count; i++) {
        if (l->items[i].type != VAL_NUMBER) interp_error(I, 0, "sum: list must contain numbers");
        total += l->items[i].as.number;
    }
    return NUMBER_VAL(total);
}
static Value bi_sorted(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "sorted expects a list");
    ObjList* src = a[0].as.list;
    Value out = make_list();
    for (int i = 0; i < src->count; i++) list_append_take(out.as.list, value_retain(src->items[i]));
    ObjList* l = out.as.list;
    for (int i = 1; i < l->count; i++) {
        Value key = l->items[i];
        int j = i - 1;
        while (j >= 0) {
            Value b = l->items[j];
            int gt = 0;
            if (key.type == VAL_NUMBER && b.type == VAL_NUMBER) gt = b.as.number > key.as.number;
            else if (key.type == VAL_STRING && b.type == VAL_STRING) gt = strcmp(b.as.str->chars, key.as.str->chars) > 0;
            else { value_release(out); interp_error(I, 0, "sorted requires all numbers or all strings"); }
            if (!gt) break;
            l->items[j + 1] = l->items[j];
            j--;
        }
        l->items[j + 1] = key;
    }
    return out;
}
static Value bi_reversed(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1) interp_error(I, 0, "reversed expects an argument");
    if (a[0].type == VAL_LIST) {
        ObjList* l = a[0].as.list;
        Value out = make_list();
        for (int i = l->count - 1; i >= 0; i--) list_append_take(out.as.list, value_retain(l->items[i]));
        return out;
    }
    if (a[0].type == VAL_STRING) {
        ObjString* str = a[0].as.str;
        char* buf = (char*)malloc(str->length + 1);
        for (int i = 0; i < str->length; i++) buf[i] = str->chars[str->length - 1 - i];
        buf[str->length] = 0;
        Value v = make_string(buf, str->length);
        free(buf);
        return v;
    }
    interp_error(I, 0, "reversed expects a list or string");
    return NULL_VAL();
}
static Value bi_enumerate(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_LIST) interp_error(I, 0, "enumerate expects a list");
    Value out = make_list();
    ObjList* l = a[0].as.list;
    for (int i = 0; i < l->count; i++) {
        Value pair = make_list();
        list_append_take(pair.as.list, NUMBER_VAL(i));
        list_append_take(pair.as.list, value_retain(l->items[i]));
        list_append_take(out.as.list, pair);
    }
    return out;
}
static Value bi_zip(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || a[0].type != VAL_LIST || a[1].type != VAL_LIST) interp_error(I, 0, "zip expects two lists");
    ObjList* la = a[0].as.list;
    ObjList* lb = a[1].as.list;
    int n = la->count < lb->count ? la->count : lb->count;
    Value out = make_list();
    for (int i = 0; i < n; i++) {
        Value pair = make_list();
        list_append_take(pair.as.list, value_retain(la->items[i]));
        list_append_take(pair.as.list, value_retain(lb->items[i]));
        list_append_take(out.as.list, pair);
    }
    return out;
}

static int clamp255(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}
static int rgb_int(int r, int g, int b) {
    return (clamp255(r) << 16) | (clamp255(g) << 8) | clamp255(b);
}
static Value color_rgb(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return NUMBER_VAL(rgb_int((int)arg_num(I, argc, a, 0, "color.rgb"), (int)arg_num(I, argc, a, 1, "color.rgb"), (int)arg_num(I, argc, a, 2, "color.rgb")));
}
static Value color_hex(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "color.hex");
    if (*str == '#') str++;
    long v = strtol(str, NULL, 16);
    return NUMBER_VAL((double)(v & 0xFFFFFF));
}
static Value color_hsv(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double h = arg_num(I, argc, a, 0, "color.hsv");
    double sat = arg_num(I, argc, a, 1, "color.hsv");
    double val = arg_num(I, argc, a, 2, "color.hsv");
    double c = val * sat;
    double x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    double m = val - c;
    double r = 0, g = 0, b = 0;
    if (h < 60) { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else { r = c; b = x; }
    return NUMBER_VAL(rgb_int((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255)));
}

static Value path_join(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    int cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    buf[0] = 0;
    for (int i = 0; i < argc; i++) {
        if (a[i].type != VAL_STRING) continue;
        const char* part = a[i].as.str->chars;
        int pl = a[i].as.str->length;
        int need = len + pl + 2;
        if (need > cap) { while (need > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        if (len > 0 && buf[len - 1] != '/' && pl > 0 && part[0] != '/') buf[len++] = '/';
        memcpy(buf + len, part, pl);
        len += pl;
        buf[len] = 0;
    }
    Value v = make_string(buf, len);
    free(buf);
    return v;
}
static Value path_basename(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = arg_str(I, argc, a, 0, "path.basename");
    const char* slash = strrchr(p, '/');
    return make_string_cstr(slash ? slash + 1 : p);
}
static Value path_dirname(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = arg_str(I, argc, a, 0, "path.dirname");
    const char* slash = strrchr(p, '/');
    if (!slash) return make_string_cstr(".");
    return make_string(p, (int)(slash - p));
}
static Value path_ext(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* p = arg_str(I, argc, a, 0, "path.ext");
    const char* slash = strrchr(p, '/');
    const char* base = slash ? slash + 1 : p;
    const char* dot = strrchr(base, '.');
    return make_string_cstr(dot ? dot : "");
}

static int file_present(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}
static Value pkg_install(Interp* I, Value s, int argc, Value* a) {
    const char* name = arg_str(I, argc, a, 0, "pkg.install");
    if (argc < 2) {
        printf("[pkg] '%s' is built-in or already present\n", name);
        return BOOL_VAL(file_present(name));
    }
    Value body = net_fetch_url(I, s, 1, &a[1]);
    if (body.type != VAL_STRING) { value_release(body); interp_error(I, 0, "pkg.install: download failed"); }
#if !defined(_WIN32)
    mkdir("packages", 0755);
#else
    _mkdir("packages");
#endif
    char pathbuf[512];
    snprintf(pathbuf, sizeof(pathbuf), "packages/%s.fc", name);
    FILE* f = fopen(pathbuf, "wb");
    if (!f) { value_release(body); interp_error(I, 0, "pkg.install: cannot write '%s'", pathbuf); }
    fwrite(body.as.str->chars, 1, body.as.str->length, f);
    fclose(f);
    value_release(body);
    printf("[pkg] installed '%s' -> %s\n", name, pathbuf);
    return BOOL_VAL(1);
}
static Value pkg_installed(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* name = arg_str(I, argc, a, 0, "pkg.installed");
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s.fc", name);
    if (file_present(buf)) return BOOL_VAL(1);
    snprintf(buf, sizeof(buf), "packages/%s.fc", name);
    if (file_present(buf)) return BOOL_VAL(1);
    const char* home = getenv("HOME");
    if (home) {
        snprintf(buf, sizeof(buf), "%s/.fc/packages/%s.fc", home, name);
        if (file_present(buf)) return BOOL_VAL(1);
    }
    return BOOL_VAL(0);
}
static Value pkg_path(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    Value l = make_list();
    list_append_take(l.as.list, make_string_cstr("."));
    list_append_take(l.as.list, make_string_cstr("packages"));
    const char* home = getenv("HOME");
    if (home) {
        char b[1024];
        snprintf(b, sizeof(b), "%s/.fc/packages", home);
        list_append_take(l.as.list, make_string_cstr(b));
    }
    const char* fp = getenv("FCPATH");
    if (fp) list_append_take(l.as.list, make_string_cstr(fp));
    return l;
}
static Value pkg_list(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s; (void)argc; (void)a;
    Value list = make_list();
#if !defined(_WIN32)
    DIR* d = opendir("packages");
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* nm = e->d_name;
            size_t l = strlen(nm);
            if (l > 3 && strcmp(nm + l - 3, ".fc") == 0) {
                list_append_take(list.as.list, make_string(nm, (int)(l - 3)));
            }
        }
        closedir(d);
    }
#endif
    return list;
}

static const char* type_name(Value v) {
    switch (v.type) {
        case VAL_NULL: return "null";
        case VAL_NUMBER: return "number";
        case VAL_BOOL: return "bool";
        case VAL_STRING: return "string";
        case VAL_LIST: return "list";
        case VAL_DICT: return "dict";
        case VAL_FUNCTION: case VAL_NATIVE: case VAL_BOUND: return "function";
        case VAL_CLASS: return "class";
        case VAL_OBJECT: return "object";
        default: return "unknown";
    }
}

static Value bi_print(Interp* I, Value s, int argc, Value* a) {
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
static Value bi_len(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1) interp_error(I, 0, "len expects 1 argument");
    if (a[0].type == VAL_STRING) return NUMBER_VAL(a[0].as.str->length);
    if (a[0].type == VAL_LIST) return NUMBER_VAL(a[0].as.list->count);
    if (a[0].type == VAL_DICT) return NUMBER_VAL(a[0].as.dict->count);
    interp_error(I, 0, "len expects a string, list, or dict");
    return NULL_VAL();
}
static Value bi_str(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    if (argc < 1) return make_string_cstr("");
    char* str = value_to_string(a[0], 0);
    Value v = make_string_cstr(str);
    free(str);
    return v;
}
static Value bi_num(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1) interp_error(I, 0, "num expects 1 argument");
    if (a[0].type == VAL_NUMBER) return a[0];
    if (a[0].type == VAL_BOOL) return NUMBER_VAL(a[0].as.boolean ? 1 : 0);
    if (a[0].type == VAL_STRING) {
        char* end;
        double d = strtod(a[0].as.str->chars, &end);
        if (end == a[0].as.str->chars) interp_error(I, 0, "num: cannot convert '%s'", a[0].as.str->chars);
        return NUMBER_VAL(d);
    }
    interp_error(I, 0, "num: cannot convert this value");
    return NULL_VAL();
}
static Value bi_int(Interp* I, Value s, int argc, Value* a) {
    Value n = bi_num(I, s, argc, a);
    return NUMBER_VAL((double)(long long)n.as.number);
}
static Value bi_bool(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    if (argc < 1) return BOOL_VAL(0);
    return BOOL_VAL(value_truthy(a[0]));
}
static Value bi_type(Interp* I, Value s, int argc, Value* a) {
    (void)I; (void)s;
    if (argc < 1) return make_string_cstr("null");
    return make_string_cstr(type_name(a[0]));
}
static Value bi_range(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double start = 0, stop = 0, step = 1;
    if (argc == 1) { stop = arg_num(I, argc, a, 0, "range"); }
    else if (argc == 2) { start = arg_num(I, argc, a, 0, "range"); stop = arg_num(I, argc, a, 1, "range"); }
    else if (argc >= 3) { start = arg_num(I, argc, a, 0, "range"); stop = arg_num(I, argc, a, 1, "range"); step = arg_num(I, argc, a, 2, "range"); }
    else interp_error(I, 0, "range expects 1 to 3 numbers");
    if (step == 0) interp_error(I, 0, "range step cannot be 0");
    Value list = make_list();
    if (step > 0) for (double v = start; v < stop; v += step) list_append_take(list.as.list, NUMBER_VAL(v));
    else for (double v = start; v > stop; v += step) list_append_take(list.as.list, NUMBER_VAL(v));
    return list;
}
static Value bi_abs(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return NUMBER_VAL(fabs(arg_num(I, argc, a, 0, "abs")));
}
static Value bi_min(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc == 1 && a[0].type == VAL_LIST) {
        ObjList* l = a[0].as.list;
        if (l->count == 0) interp_error(I, 0, "min of empty list");
        double m = l->items[0].as.number;
        for (int i = 1; i < l->count; i++) if (l->items[i].as.number < m) m = l->items[i].as.number;
        return NUMBER_VAL(m);
    }
    if (argc == 0) interp_error(I, 0, "min expects arguments");
    double m = arg_num(I, argc, a, 0, "min");
    for (int i = 1; i < argc; i++) { double x = arg_num(I, argc, a, i, "min"); if (x < m) m = x; }
    return NUMBER_VAL(m);
}
static Value bi_max(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc == 1 && a[0].type == VAL_LIST) {
        ObjList* l = a[0].as.list;
        if (l->count == 0) interp_error(I, 0, "max of empty list");
        double m = l->items[0].as.number;
        for (int i = 1; i < l->count; i++) if (l->items[i].as.number > m) m = l->items[i].as.number;
        return NUMBER_VAL(m);
    }
    if (argc == 0) interp_error(I, 0, "max expects arguments");
    double m = arg_num(I, argc, a, 0, "max");
    for (int i = 1; i < argc; i++) { double x = arg_num(I, argc, a, i, "max"); if (x > m) m = x; }
    return NUMBER_VAL(m);
}
static Value bi_assert(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1) interp_error(I, 0, "assert expects a condition");
    if (!value_truthy(a[0])) {
        const char* msg = (argc >= 2 && a[1].type == VAL_STRING) ? a[1].as.str->chars : "assertion failed";
        interp_error(I, 0, "%s", msg);
    }
    return NULL_VAL();
}
static Value bi_chr(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int c = (int)arg_num(I, argc, a, 0, "chr");
    char ch = (char)c;
    return make_string(&ch, 1);
}
static Value bi_ord(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* str = arg_str(I, argc, a, 0, "ord");
    return NUMBER_VAL((double)(unsigned char)str[0]);
}

static void defn(ObjDict* d, const char* name, NativeFn fn) {
    dict_set_take(d, name, make_native(fn, NULL_VAL(), name));
}

static void def_global(Interp* I, const char* name, NativeFn fn) {
    Value v = make_native(fn, NULL_VAL(), name);
    env_define(I->globals, name, v);
    value_release(v);
}

static void define_module(Interp* I, const char* name, Value dict) {
    env_define(I->globals, name, dict);
    value_release(dict);
}

void register_stats(Interp* I);
void register_extras(Interp* I);
void register_daily(Interp* I);
void register_advanced(Interp* I);
#ifdef FC_USE_SDL
void register_win(Interp* I);
#endif

void register_modules(Interp* I) {
    def_global(I, "print", bi_print);
    def_global(I, "len", bi_len);
    def_global(I, "str", bi_str);
    def_global(I, "num", bi_num);
    def_global(I, "int", bi_int);
    def_global(I, "bool", bi_bool);
    def_global(I, "type", bi_type);
    def_global(I, "range", bi_range);
    def_global(I, "abs", bi_abs);
    def_global(I, "min", bi_min);
    def_global(I, "max", bi_max);
    def_global(I, "assert", bi_assert);
    def_global(I, "chr", bi_chr);
    def_global(I, "ord", bi_ord);
    def_global(I, "sum", bi_sum);
    def_global(I, "sorted", bi_sorted);
    def_global(I, "reversed", bi_reversed);
    def_global(I, "enumerate", bi_enumerate);
    def_global(I, "zip", bi_zip);

    Value math = make_dict();
    defn(math.as.dict, "sqrt", m_sqrt);
    defn(math.as.dict, "pow", m_pow);
    defn(math.as.dict, "abs", m_abs);
    defn(math.as.dict, "floor", m_floor);
    defn(math.as.dict, "ceil", m_ceil);
    defn(math.as.dict, "sin", m_sin);
    defn(math.as.dict, "cos", m_cos);
    defn(math.as.dict, "random", m_random);
    defn(math.as.dict, "tan", m_tan);
    defn(math.as.dict, "log", m_log);
    defn(math.as.dict, "log10", m_log10);
    defn(math.as.dict, "exp", m_exp);
    defn(math.as.dict, "round", m_round);
    defn(math.as.dict, "hypot", m_hypot);
    defn(math.as.dict, "atan", m_atan);
    defn(math.as.dict, "atan2", m_atan2);
    defn(math.as.dict, "min", m_min);
    defn(math.as.dict, "max", m_max);
    defn(math.as.dict, "gcd", m_gcd);
    defn(math.as.dict, "lcm", m_lcm);
    defn(math.as.dict, "factorial", m_factorial);
    defn(math.as.dict, "sign", m_sign);
    defn(math.as.dict, "clamp", m_clamp);
    defn(math.as.dict, "deg", m_deg);
    defn(math.as.dict, "rad", m_rad);
    defn(math.as.dict, "cbrt", m_cbrt);
    defn(math.as.dict, "log2", m_log2);
    defn(math.as.dict, "trunc", m_trunc);
    dict_set_take(math.as.dict, "pi", NUMBER_VAL(3.14159265358979323846));
    dict_set_take(math.as.dict, "e", NUMBER_VAL(2.71828182845904523536));
    dict_set_take(math.as.dict, "tau", NUMBER_VAL(6.28318530717958647692));
    define_module(I, "math", math);

    Value bit = make_dict();
    defn(bit.as.dict, "and", bit_and);
    defn(bit.as.dict, "or", bit_or);
    defn(bit.as.dict, "xor", bit_xor);
    defn(bit.as.dict, "not", bit_not);
    defn(bit.as.dict, "shl", bit_shl);
    defn(bit.as.dict, "shr", bit_shr);
    define_module(I, "bit", bit);

    Value csv = make_dict();
    defn(csv.as.dict, "parse", csv_parse);
    defn(csv.as.dict, "stringify", csv_stringify);
    define_module(I, "csv", csv);

    Value crypto = make_dict();
    defn(crypto.as.dict, "hash", c_hash);
    defn(crypto.as.dict, "base64_encode", c_b64encode);
    defn(crypto.as.dict, "base64_decode", c_b64decode);
    defn(crypto.as.dict, "fnv1a", c_fnv1a);
    define_module(I, "crypto", crypto);

    Value tm = make_dict();
    defn(tm.as.dict, "now", t_now);
    defn(tm.as.dict, "sleep", t_sleep);
    defn(tm.as.dict, "format", t_format);
    defn(tm.as.dict, "clock", t_clock);
    defn(tm.as.dict, "parts", t_parts);
    define_module(I, "time", tm);

    Value strm = make_dict();
    defn(strm.as.dict, "upper", str_upper);
    defn(strm.as.dict, "lower", str_lower);
    defn(strm.as.dict, "trim", str_trim);
    defn(strm.as.dict, "split", str_split);
    defn(strm.as.dict, "join", str_join);
    defn(strm.as.dict, "replace", str_replace);
    defn(strm.as.dict, "format", str_format);
    define_module(I, "text", strm);

    Value os = make_dict();
    defn(os.as.dict, "getenv", os_getenv);
    defn(os.as.dict, "system", os_system);
    defn(os.as.dict, "exit", os_exit);
    defn(os.as.dict, "platform", os_platform);
    defn(os.as.dict, "cwd", os_cwd);
    defn(os.as.dict, "args", os_args);
    define_module(I, "os", os);

    Value rnd = make_dict();
    defn(rnd.as.dict, "seed", rnd_seed);
    defn(rnd.as.dict, "float", rnd_float);
    defn(rnd.as.dict, "int", rnd_int);
    defn(rnd.as.dict, "choice", rnd_choice);
    defn(rnd.as.dict, "bool", rnd_bool);
    defn(rnd.as.dict, "shuffle", rnd_shuffle);
    defn(rnd.as.dict, "sample", rnd_sample);
    define_module(I, "random", rnd);

    Value ai = make_dict();
    defn(ai.as.dict, "matrix", ai_matrix);
    defn(ai.as.dict, "dot", ai_dot);
    defn(ai.as.dict, "train_linear", ai_train_linear);
    defn(ai.as.dict, "predict", ai_predict);
    defn(ai.as.dict, "add", ai_add);
    defn(ai.as.dict, "sub", ai_sub);
    defn(ai.as.dict, "scale", ai_scale);
    defn(ai.as.dict, "transpose", ai_transpose);
    defn(ai.as.dict, "apply", ai_apply);
    defn(ai.as.dict, "randn", ai_randn);
    defn(ai.as.dict, "shape", ai_shape);
    define_module(I, "ai", ai);

    Value nn = make_dict();
    defn(nn.as.dict, "sigmoid", nn_sigmoid);
    defn(nn.as.dict, "relu", nn_relu);
    defn(nn.as.dict, "tanh", nn_tanh);
    defn(nn.as.dict, "leaky_relu", nn_leaky_relu);
    defn(nn.as.dict, "softmax", nn_softmax);
    defn(nn.as.dict, "dsigmoid", nn_dsigmoid);
    defn(nn.as.dict, "drelu", nn_drelu);
    defn(nn.as.dict, "dtanh", nn_dtanh);
    define_module(I, "nn", nn);

    Value sec = make_dict();
    defn(sec.as.dict, "xor", sec_xor);
    defn(sec.as.dict, "caesar", sec_caesar);
    defn(sec.as.dict, "rot13", sec_rot13);
    defn(sec.as.dict, "hex_encode", sec_hex_encode);
    defn(sec.as.dict, "hex_decode", sec_hex_decode);
    defn(sec.as.dict, "crc32", sec_crc32);
    defn(sec.as.dict, "rand_bytes", sec_rand_bytes);
    defn(sec.as.dict, "strength", sec_strength);
    define_module(I, "sec", sec);

    Value web = make_dict();
    defn(web.as.dict, "get", web_get);
    defn(web.as.dict, "serve", web_serve);
    defn(web.as.dict, "serve_dir", web_serve_dir);
    defn(web.as.dict, "parse_request", web_parse_request);
    defn(web.as.dict, "mime", web_mime);
    defn(web.as.dict, "url_encode", web_url_encode);
    defn(web.as.dict, "url_decode", web_url_decode);
    defn(web.as.dict, "html_escape", web_html_escape);
    define_module(I, "web", web);

    Value term = make_dict();
    defn(term.as.dict, "clear", term_clear);
    defn(term.as.dict, "at", term_at);
    defn(term.as.dict, "color", term_color);
    defn(term.as.dict, "bg", term_bg);
    defn(term.as.dict, "reset", term_reset);
    defn(term.as.dict, "hide_cursor", term_hide_cursor);
    defn(term.as.dict, "show_cursor", term_show_cursor);
    defn(term.as.dict, "write", term_write);
    defn(term.as.dict, "raw", term_raw);
    defn(term.as.dict, "key", term_key);
    defn(term.as.dict, "getch", term_getch);
    defn(term.as.dict, "size", term_size);
    define_module(I, "term", term);

    Value file = make_dict();
    defn(file.as.dict, "read", f_read);
    defn(file.as.dict, "write", f_write);
    defn(file.as.dict, "append", file_append);
    defn(file.as.dict, "exists", file_exists);
    defn(file.as.dict, "delete", file_delete);
    defn(file.as.dict, "size", file_size);
    defn(file.as.dict, "read_bytes", file_read_bytes);
    defn(file.as.dict, "write_bytes", file_write_bytes);
    defn(file.as.dict, "lines", file_lines);
    define_module(I, "file", file);

    Value db = make_dict();
    defn(db.as.dict, "query", db_query);
    define_module(I, "db", db);

    Value net = make_dict();
    defn(net.as.dict, "fetch_url", net_fetch_url);
    defn(net.as.dict, "send", net_send);
    defn(net.as.dict, "listen", net_listen);
    define_module(I, "net", net);

    Value gui = make_dict();
    defn(gui.as.dict, "window", gui_window);
    defn(gui.as.dict, "draw_pixel", gui_draw_pixel);
    defn(gui.as.dict, "refresh", gui_refresh);
    defn(gui.as.dict, "fill", gui_fill);
    defn(gui.as.dict, "rect", gui_rect);
    defn(gui.as.dict, "line", gui_line);
    defn(gui.as.dict, "save", gui_save);
    define_module(I, "gui", gui);

    Value json = make_dict();
    defn(json.as.dict, "parse", json_parse);
    defn(json.as.dict, "stringify", json_stringify);
    define_module(I, "json", json);

    Value fnmod = make_dict();
    defn(fnmod.as.dict, "map", fn_map);
    defn(fnmod.as.dict, "filter", fn_filter);
    defn(fnmod.as.dict, "reduce", fn_reduce);
    defn(fnmod.as.dict, "each", fn_each);
    define_module(I, "fn", fnmod);

    Value color = make_dict();
    defn(color.as.dict, "rgb", color_rgb);
    defn(color.as.dict, "hex", color_hex);
    defn(color.as.dict, "hsv", color_hsv);
    dict_set_take(color.as.dict, "black", NUMBER_VAL(0));
    dict_set_take(color.as.dict, "white", NUMBER_VAL(16777215));
    dict_set_take(color.as.dict, "red", NUMBER_VAL(16711680));
    dict_set_take(color.as.dict, "green", NUMBER_VAL(65280));
    dict_set_take(color.as.dict, "blue", NUMBER_VAL(255));
    dict_set_take(color.as.dict, "yellow", NUMBER_VAL(16776960));
    dict_set_take(color.as.dict, "cyan", NUMBER_VAL(65535));
    dict_set_take(color.as.dict, "magenta", NUMBER_VAL(16711935));
    dict_set_take(color.as.dict, "gray", NUMBER_VAL(8421504));
    dict_set_take(color.as.dict, "orange", NUMBER_VAL(16753920));
    define_module(I, "color", color);

    Value path = make_dict();
    defn(path.as.dict, "join", path_join);
    defn(path.as.dict, "basename", path_basename);
    defn(path.as.dict, "dirname", path_dirname);
    defn(path.as.dict, "ext", path_ext);
    define_module(I, "path", path);

    Value pkg = make_dict();
    defn(pkg.as.dict, "install", pkg_install);
    defn(pkg.as.dict, "installed", pkg_installed);
    defn(pkg.as.dict, "list", pkg_list);
    defn(pkg.as.dict, "path", pkg_path);
    define_module(I, "pkg", pkg);

    register_stats(I);
    register_extras(I);
    register_daily(I);
    register_advanced(I);
#ifdef FC_USE_SDL
    register_win(I);
#endif
}
