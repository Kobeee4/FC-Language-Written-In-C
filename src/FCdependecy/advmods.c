#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "interpreter.h"

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

static double numarg(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_NUMBER) interp_error(I, 0, "%s expects a number argument", fn);
    return a[i].as.number;
}
static const char* strarg(Interp* I, int argc, Value* a, int i, const char* fn) {
    if (i >= argc || a[i].type != VAL_STRING) interp_error(I, 0, "%s expects a string argument", fn);
    return a[i].as.str->chars;
}
static void addfn(ObjDict* d, const char* name, NativeFn fn) {
    dict_set_take(d, name, make_native(fn, NULL_VAL(), name));
}
static int hx_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static double* read_matrix(Interp* I, Value v, int* rows, int* cols, const char* fn) {
    if (v.type != VAL_LIST || v.as.list->count == 0) interp_error(I, 0, "%s expects a non-empty matrix", fn);
    ObjList* m = v.as.list;
    if (m->items[0].type != VAL_LIST) interp_error(I, 0, "%s expects a list of rows", fn);
    int r = m->count;
    int c = m->items[0].as.list->count;
    if (c == 0) interp_error(I, 0, "%s: empty row", fn);
    double* out = (double*)malloc(sizeof(double) * r * c);
    for (int i = 0; i < r; i++) {
        if (m->items[i].type != VAL_LIST || m->items[i].as.list->count != c) { free(out); interp_error(I, 0, "%s: ragged matrix", fn); }
        ObjList* row = m->items[i].as.list;
        for (int j = 0; j < c; j++) {
            if (row->items[j].type != VAL_NUMBER) { free(out); interp_error(I, 0, "%s: matrix must contain numbers", fn); }
            out[i * c + j] = row->items[j].as.number;
        }
    }
    *rows = r; *cols = c;
    return out;
}
static Value matrix_value(const double* m, int r, int c) {
    Value out = make_list();
    for (int i = 0; i < r; i++) {
        Value row = make_list();
        for (int j = 0; j < c; j++) list_append_take(row.as.list, NUMBER_VAL(m[i * c + j]));
        list_append_take(out.as.list, row);
    }
    return out;
}

static Value la_identity(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int n = (int)numarg(I, argc, a, 0, "linalg.identity");
    if (n <= 0 || n > 4096) interp_error(I, 0, "linalg.identity: bad size");
    double* m = (double*)calloc((size_t)n * n, sizeof(double));
    for (int i = 0; i < n; i++) m[i * n + i] = 1;
    Value v = matrix_value(m, n, n);
    free(m);
    return v;
}
static Value la_zeros(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    int r = (int)numarg(I, argc, a, 0, "linalg.zeros");
    int c = (int)numarg(I, argc, a, 1, "linalg.zeros");
    if (r <= 0 || c <= 0 || r > 4096 || c > 4096) interp_error(I, 0, "linalg.zeros: bad size");
    double* m = (double*)calloc((size_t)r * c, sizeof(double));
    Value v = matrix_value(m, r, c);
    free(m);
    return v;
}
static Value la_transpose(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    int r, c;
    double* m = read_matrix(I, a[0], &r, &c, "linalg.transpose");
    double* t = (double*)malloc(sizeof(double) * r * c);
    for (int i = 0; i < r; i++) for (int j = 0; j < c; j++) t[j * r + i] = m[i * c + j];
    Value v = matrix_value(t, c, r);
    free(m); free(t);
    return v;
}
static Value la_matmul(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "linalg.matmul expects two matrices");
    int ra, ca, rb, cb;
    double* A = read_matrix(I, a[0], &ra, &ca, "linalg.matmul");
    double* B = read_matrix(I, a[1], &rb, &cb, "linalg.matmul");
    if (ca != rb) { free(A); free(B); interp_error(I, 0, "linalg.matmul: dimension mismatch"); }
    double* C = (double*)calloc((size_t)ra * cb, sizeof(double));
    for (int i = 0; i < ra; i++)
        for (int k = 0; k < ca; k++) {
            double v = A[i * ca + k];
            for (int j = 0; j < cb; j++) C[i * cb + j] += v * B[k * cb + j];
        }
    Value v = matrix_value(C, ra, cb);
    free(A); free(B); free(C);
    return v;
}
static double det_inplace(double* a, int n) {
    double det = 1;
    for (int col = 0; col < n; col++) {
        int piv = col;
        double best = fabs(a[col * n + col]);
        for (int row = col + 1; row < n; row++) { double v = fabs(a[row * n + col]); if (v > best) { best = v; piv = row; } }
        if (best < 1e-12) return 0;
        if (piv != col) { for (int j = 0; j < n; j++) { double t = a[col * n + j]; a[col * n + j] = a[piv * n + j]; a[piv * n + j] = t; } det = -det; }
        det *= a[col * n + col];
        for (int row = col + 1; row < n; row++) {
            double f = a[row * n + col] / a[col * n + col];
            for (int j = col; j < n; j++) a[row * n + j] -= f * a[col * n + j];
        }
    }
    return det;
}
static Value la_det(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    int r, c;
    double* m = read_matrix(I, a[0], &r, &c, "linalg.det");
    if (r != c) { free(m); interp_error(I, 0, "linalg.det: matrix must be square"); }
    double d = det_inplace(m, r);
    free(m);
    return NUMBER_VAL(d);
}
static Value la_inverse(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    int r, c;
    double* m = read_matrix(I, a[0], &r, &c, "linalg.inverse");
    if (r != c) { free(m); interp_error(I, 0, "linalg.inverse: matrix must be square"); }
    int n = r;
    double* aug = (double*)calloc((size_t)n * 2 * n, sizeof(double));
    for (int i = 0; i < n; i++) { for (int j = 0; j < n; j++) aug[i * 2 * n + j] = m[i * n + j]; aug[i * 2 * n + n + i] = 1; }
    free(m);
    for (int col = 0; col < n; col++) {
        int piv = col;
        double best = fabs(aug[col * 2 * n + col]);
        for (int row = col + 1; row < n; row++) { double v = fabs(aug[row * 2 * n + col]); if (v > best) { best = v; piv = row; } }
        if (best < 1e-12) { free(aug); interp_error(I, 0, "linalg.inverse: matrix is singular"); }
        if (piv != col) for (int j = 0; j < 2 * n; j++) { double t = aug[col * 2 * n + j]; aug[col * 2 * n + j] = aug[piv * 2 * n + j]; aug[piv * 2 * n + j] = t; }
        double d = aug[col * 2 * n + col];
        for (int j = 0; j < 2 * n; j++) aug[col * 2 * n + j] /= d;
        for (int row = 0; row < n; row++) {
            if (row == col) continue;
            double f = aug[row * 2 * n + col];
            for (int j = 0; j < 2 * n; j++) aug[row * 2 * n + j] -= f * aug[col * 2 * n + j];
        }
    }
    double* inv = (double*)malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) inv[i * n + j] = aug[i * 2 * n + n + j];
    free(aug);
    Value v = matrix_value(inv, n, n);
    free(inv);
    return v;
}
static Value la_solve(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2 || a[1].type != VAL_LIST) interp_error(I, 0, "linalg.solve expects (matrix, vector)");
    int r, c;
    double* m = read_matrix(I, a[0], &r, &c, "linalg.solve");
    if (r != c) { free(m); interp_error(I, 0, "linalg.solve: matrix must be square"); }
    ObjList* bl = a[1].as.list;
    if (bl->count != r) { free(m); interp_error(I, 0, "linalg.solve: vector length mismatch"); }
    int n = r;
    double* aug = (double*)malloc(sizeof(double) * n * (n + 1));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) aug[i * (n + 1) + j] = m[i * n + j];
        if (bl->items[i].type != VAL_NUMBER) { free(m); free(aug); interp_error(I, 0, "linalg.solve: vector must be numbers"); }
        aug[i * (n + 1) + n] = bl->items[i].as.number;
    }
    free(m);
    for (int col = 0; col < n; col++) {
        int piv = col;
        double best = fabs(aug[col * (n + 1) + col]);
        for (int row = col + 1; row < n; row++) { double v = fabs(aug[row * (n + 1) + col]); if (v > best) { best = v; piv = row; } }
        if (best < 1e-12) { free(aug); interp_error(I, 0, "linalg.solve: no unique solution"); }
        if (piv != col) for (int j = 0; j <= n; j++) { double t = aug[col * (n + 1) + j]; aug[col * (n + 1) + j] = aug[piv * (n + 1) + j]; aug[piv * (n + 1) + j] = t; }
        for (int row = 0; row < n; row++) {
            if (row == col) continue;
            double f = aug[row * (n + 1) + col] / aug[col * (n + 1) + col];
            for (int j = col; j <= n; j++) aug[row * (n + 1) + j] -= f * aug[col * (n + 1) + j];
        }
    }
    Value out = make_list();
    for (int i = 0; i < n; i++) list_append_take(out.as.list, NUMBER_VAL(aug[i * (n + 1) + n] / aug[i * (n + 1) + i]));
    free(aug);
    return out;
}

static void read_complex(Interp* I, Value v, double* re, double* im, const char* fn) {
    if (v.type == VAL_NUMBER) { *re = v.as.number; *im = 0; return; }
    if (v.type == VAL_DICT) {
        Value rv, iv;
        *re = 0; *im = 0;
        if (dict_get(v.as.dict, "re", &rv)) { if (rv.type == VAL_NUMBER) *re = rv.as.number; value_release(rv); }
        if (dict_get(v.as.dict, "im", &iv)) { if (iv.type == VAL_NUMBER) *im = iv.as.number; value_release(iv); }
        return;
    }
    interp_error(I, 0, "%s expects a complex number or a number", fn);
}
static Value make_complex(double re, double im) {
    Value d = make_dict();
    dict_set_take(d.as.dict, "re", NUMBER_VAL(re));
    dict_set_take(d.as.dict, "im", NUMBER_VAL(im));
    return d;
}
static Value cx_new(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double re = numarg(I, argc, a, 0, "cplx.new");
    double im = argc >= 2 ? numarg(I, argc, a, 1, "cplx.new") : 0;
    return make_complex(re, im);
}
static Value cx_add(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "cplx.add expects two arguments");
    double r1 = 0, i1 = 0, r2 = 0, i2 = 0;
    read_complex(I, a[0], &r1, &i1, "cplx.add");
    read_complex(I, a[1], &r2, &i2, "cplx.add");
    return make_complex(r1 + r2, i1 + i2);
}
static Value cx_sub(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "cplx.sub expects two arguments");
    double r1 = 0, i1 = 0, r2 = 0, i2 = 0;
    read_complex(I, a[0], &r1, &i1, "cplx.sub");
    read_complex(I, a[1], &r2, &i2, "cplx.sub");
    return make_complex(r1 - r2, i1 - i2);
}
static Value cx_mul(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "cplx.mul expects two arguments");
    double r1 = 0, i1 = 0, r2 = 0, i2 = 0;
    read_complex(I, a[0], &r1, &i1, "cplx.mul");
    read_complex(I, a[1], &r2, &i2, "cplx.mul");
    return make_complex(r1 * r2 - i1 * i2, r1 * i2 + i1 * r2);
}
static Value cx_div(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 2) interp_error(I, 0, "cplx.div expects two arguments");
    double r1 = 0, i1 = 0, r2 = 0, i2 = 0;
    read_complex(I, a[0], &r1, &i1, "cplx.div");
    read_complex(I, a[1], &r2, &i2, "cplx.div");
    double d = r2 * r2 + i2 * i2;
    if (d == 0) interp_error(I, 0, "cplx.div: division by zero");
    return make_complex((r1 * r2 + i1 * i2) / d, (i1 * r2 - r1 * i2) / d);
}
static Value cx_abs(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    double re = 0, im = 0;
    read_complex(I, a[0], &re, &im, "cplx.abs");
    return NUMBER_VAL(hypot(re, im));
}
static Value cx_arg(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    double re = 0, im = 0;
    read_complex(I, a[0], &re, &im, "cplx.arg");
    return NUMBER_VAL(atan2(im, re));
}
static Value cx_conj(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    double re = 0, im = 0;
    read_complex(I, a[0], &re, &im, "cplx.conj");
    return make_complex(re, -im);
}
static Value cx_polar(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double r = numarg(I, argc, a, 0, "cplx.polar");
    double t = numarg(I, argc, a, 1, "cplx.polar");
    return make_complex(r * cos(t), r * sin(t));
}
static Value cx_str(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc;
    double re = 0, im = 0;
    read_complex(I, a[0], &re, &im, "cplx.str");
    char buf[160];
    char num1[48], num2[48];
    snprintf(num1, sizeof(num1), "%g", re);
    snprintf(num2, sizeof(num2), "%g", fabs(im));
    snprintf(buf, sizeof(buf), "%s %c %si", num1, im < 0 ? '-' : '+', num2);
    return make_string_cstr(buf);
}

static const char B32A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static int b32_rev(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '2' && c <= '7') return c - '2' + 26;
    return -1;
}
static Value cd_b32_encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "codec.base32_encode");
    int n = (int)strlen(in);
    int cap = ((n + 4) / 5) * 8 + 1;
    char* out = (char*)malloc(cap);
    int j = 0, buffer = 0, bits = 0;
    for (int i = 0; i < n; i++) {
        buffer = (buffer << 8) | (unsigned char)in[i];
        bits += 8;
        while (bits >= 5) { bits -= 5; out[j++] = B32A[(buffer >> bits) & 31]; }
    }
    if (bits > 0) out[j++] = B32A[(buffer << (5 - bits)) & 31];
    while (j % 8 != 0) out[j++] = '=';
    out[j] = 0;
    Value v = make_string(out, j);
    free(out);
    return v;
}
static Value cd_b32_decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "codec.base32_decode");
    int n = (int)strlen(in);
    char* out = (char*)malloc(n + 1);
    int j = 0, buffer = 0, bits = 0;
    for (int i = 0; i < n; i++) {
        int v = b32_rev(in[i]);
        if (v < 0) continue;
        buffer = (buffer << 5) | v;
        bits += 5;
        if (bits >= 8) { bits -= 8; out[j++] = (char)((buffer >> bits) & 0xff); }
    }
    out[j] = 0;
    Value r = make_string(out, j);
    free(out);
    return r;
}
static const char B64U[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static int b64u_rev(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}
static Value cd_b64u_encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "codec.base64url_encode");
    size_t n = strlen(in);
    char* out = (char*)malloc(4 * ((n + 2) / 3) + 1);
    size_t j = 0;
    for (size_t i = 0; i < n; i += 3) {
        unsigned int x = (unsigned char)in[i] << 16;
        if (i + 1 < n) x |= (unsigned char)in[i + 1] << 8;
        if (i + 2 < n) x |= (unsigned char)in[i + 2];
        out[j++] = B64U[(x >> 18) & 63];
        out[j++] = B64U[(x >> 12) & 63];
        if (i + 1 < n) out[j++] = B64U[(x >> 6) & 63];
        if (i + 2 < n) out[j++] = B64U[x & 63];
    }
    out[j] = 0;
    Value v = make_string(out, (int)j);
    free(out);
    return v;
}
static Value cd_b64u_decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "codec.base64url_decode");
    size_t n = strlen(in);
    char* out = (char*)malloc(n + 1);
    size_t j = 0;
    int buffer = 0, bits = 0;
    for (size_t i = 0; i < n; i++) {
        int v = b64u_rev(in[i]);
        if (v < 0) continue;
        buffer = (buffer << 6) | v;
        bits += 6;
        if (bits >= 8) { bits -= 8; out[j++] = (char)((buffer >> bits) & 0xff); }
    }
    out[j] = 0;
    Value r = make_string(out, (int)j);
    free(out);
    return r;
}
static Value cd_rot47(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "codec.rot47");
    int n = (int)strlen(in);
    char* out = (char*)malloc(n + 1);
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c >= 33 && c <= 126) out[i] = (char)(33 + (c - 33 + 47) % 94);
        else out[i] = (char)c;
    }
    out[n] = 0;
    Value v = make_string(out, n);
    free(out);
    return v;
}

static void sha256_raw(const unsigned char* d, size_t n, unsigned char out[32]) {
    char hex[65];
    fc_sha256(d, n, hex);
    for (int i = 0; i < 32; i++) out[i] = (unsigned char)((hx_val(hex[i * 2]) << 4) | hx_val(hex[i * 2 + 1]));
}
static Value hm_sha256(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* key = strarg(I, argc, a, 0, "hmac.sha256");
    const char* msg = strarg(I, argc, a, 1, "hmac.sha256");
    size_t klen = strlen(key), mlen = strlen(msg);
    unsigned char k[64];
    memset(k, 0, 64);
    if (klen > 64) sha256_raw((const unsigned char*)key, klen, k);
    else memcpy(k, key, klen);
    unsigned char ipad[64], opad[64];
    for (int i = 0; i < 64; i++) { ipad[i] = k[i] ^ 0x36; opad[i] = k[i] ^ 0x5c; }
    unsigned char* inner = (unsigned char*)malloc(64 + mlen);
    memcpy(inner, ipad, 64);
    memcpy(inner + 64, msg, mlen);
    unsigned char ihash[32];
    sha256_raw(inner, 64 + mlen, ihash);
    free(inner);
    unsigned char outer[96];
    memcpy(outer, opad, 64);
    memcpy(outer + 64, ihash, 32);
    char hex[65];
    fc_sha256(outer, 96, hex);
    return make_string_cstr(hex);
}

static int is_unreserved(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}
static Value url_encode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "url.encode");
    static const char* hx = "0123456789ABCDEF";
    int n = (int)strlen(in);
    char* b = (char*)malloc(n * 3 + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)in[i];
        if (is_unreserved(c)) b[j++] = c;
        else { b[j++] = '%'; b[j++] = hx[c >> 4]; b[j++] = hx[c & 15]; }
    }
    b[j] = 0;
    Value v = make_string(b, j);
    free(b);
    return v;
}
static char* percent_decode(const char* in, int n) {
    char* b = (char*)malloc(n + 1);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (in[i] == '%' && i + 2 < n) { int hi = hx_val(in[i + 1]), lo = hx_val(in[i + 2]); if (hi >= 0 && lo >= 0) { b[j++] = (char)((hi << 4) | lo); i += 2; continue; } }
        b[j++] = (in[i] == '+') ? ' ' : in[i];
    }
    b[j] = 0;
    return b;
}
static Value url_decode(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* in = strarg(I, argc, a, 0, "url.decode");
    char* d = percent_decode(in, (int)strlen(in));
    Value v = make_string_cstr(d);
    free(d);
    return v;
}
static Value parse_query_string(const char* qs, int qlen) {
    Value query = make_dict();
    int start = 0;
    for (int i = 0; i <= qlen; i++) {
        if (i == qlen || qs[i] == '&') {
            const char* pair = qs + start;
            int plen = i - start;
            if (plen > 0) {
                const char* eq = memchr(pair, '=', plen);
                if (eq) {
                    char* k = percent_decode(pair, (int)(eq - pair));
                    char* v = percent_decode(eq + 1, plen - (int)(eq - pair) - 1);
                    dict_set_take(query.as.dict, k, make_string_cstr(v));
                    free(k); free(v);
                } else {
                    char* k = percent_decode(pair, plen);
                    dict_set_take(query.as.dict, k, make_string_cstr(""));
                    free(k);
                }
            }
            start = i + 1;
        }
    }
    return query;
}
static Value url_query(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* qs = strarg(I, argc, a, 0, "url.query");
    return parse_query_string(qs, (int)strlen(qs));
}
static Value url_parse(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* url = strarg(I, argc, a, 0, "url.parse");
    Value d = make_dict();
    const char* p = url;
    const char* scheme_end = strstr(p, "://");
    char scheme[32] = "";
    if (scheme_end && scheme_end - p < 31) {
        int sl = (int)(scheme_end - p);
        memcpy(scheme, p, sl);
        scheme[sl] = 0;
        p = scheme_end + 3;
    }
    dict_set_take(d.as.dict, "scheme", make_string_cstr(scheme));
    const char* host_start = p;
    while (*p && *p != '/' && *p != ':' && *p != '?' && *p != '#') p++;
    dict_set_take(d.as.dict, "host", make_string(host_start, (int)(p - host_start)));
    int port = (strcmp(scheme, "https") == 0) ? 443 : 80;
    if (*p == ':') {
        p++;
        port = atoi(p);
        while (*p && *p != '/' && *p != '?' && *p != '#') p++;
    }
    dict_set_take(d.as.dict, "port", NUMBER_VAL(port));
    const char* path_start = p;
    while (*p && *p != '?' && *p != '#') p++;
    int plen = (int)(p - path_start);
    dict_set_take(d.as.dict, "path", plen > 0 ? make_string(path_start, plen) : make_string_cstr("/"));
    if (*p == '?') {
        p++;
        const char* q = p;
        while (*p && *p != '#') p++;
        int qlen = (int)(p - q);
        dict_set_take(d.as.dict, "query", make_string(q, qlen));
        dict_set_take(d.as.dict, "params", parse_query_string(q, qlen));
    } else {
        dict_set_take(d.as.dict, "query", make_string_cstr(""));
        dict_set_take(d.as.dict, "params", make_dict());
    }
    dict_set_take(d.as.dict, "fragment", make_string_cstr(*p == '#' ? p + 1 : ""));
    return d;
}
static Value url_build(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    if (argc < 1 || a[0].type != VAL_DICT) interp_error(I, 0, "url.build expects a dict");
    ObjDict* d = a[0].as.dict;
    char buf[2048];
    int len = 0;
    Value v;
    const char* scheme = "http";
    if (dict_get(d, "scheme", &v)) { if (v.type == VAL_STRING && v.as.str->length) scheme = v.as.str->chars; }
    len += snprintf(buf + len, sizeof(buf) - len, "%s://", scheme);
    if (dict_get(d, "host", &v) && v.type == VAL_STRING) { len += snprintf(buf + len, sizeof(buf) - len, "%s", v.as.str->chars); value_release(v); }
    if (dict_get(d, "port", &v) && v.type == VAL_NUMBER) {
        int port = (int)v.as.number;
        if (!((strcmp(scheme, "http") == 0 && port == 80) || (strcmp(scheme, "https") == 0 && port == 443)))
            len += snprintf(buf + len, sizeof(buf) - len, ":%d", port);
        value_release(v);
    }
    if (dict_get(d, "path", &v) && v.type == VAL_STRING) { len += snprintf(buf + len, sizeof(buf) - len, "%s", v.as.str->chars); value_release(v); }
    if (dict_get(d, "query", &v) && v.type == VAL_STRING && v.as.str->length) { len += snprintf(buf + len, sizeof(buf) - len, "?%s", v.as.str->chars); value_release(v); }
    return make_string(buf, len);
}

static const char* status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 418: return "I'm a teapot";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}
static Value http_status_text(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    return make_string_cstr(status_text((int)numarg(I, argc, a, 0, "http.status_text")));
}
#if !defined(_WIN32)
static Value http_get(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    const char* url = strarg(I, argc, a, 0, "http.get");
    const char* p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;
    else if (strncmp(p, "https://", 8) == 0) interp_error(I, 0, "http.get: https is not supported (plain http only)");
    char host[256];
    int port = 80, i = 0;
    char path[1024] = "/";
    while (*p && *p != '/' && *p != ':' && i < 255) host[i++] = *p++;
    host[i] = 0;
    if (*p == ':') { p++; port = atoi(p); while (*p && *p != '/') p++; }
    if (*p == '/') { strncpy(path, p, sizeof(path) - 1); path[sizeof(path) - 1] = 0; }
    struct hostent* he = gethostbyname(host);
    if (!he) interp_error(I, 0, "http.get: cannot resolve '%s'", host);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) interp_error(I, 0, "http.get: socket failed");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); interp_error(I, 0, "http.get: connection failed"); }
    char req[1600];
    snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: FC\r\nConnection: close\r\n\r\n", path, host);
    send(sock, req, strlen(req), 0);
    int cap = 8192, len = 0;
    char* buf = (char*)malloc(cap);
    char tmp[4096];
    int r;
    while ((r = recv(sock, tmp, sizeof(tmp), 0)) > 0) {
        if (len + r + 1 > cap) { while (len + r + 1 > cap) cap *= 2; buf = (char*)realloc(buf, cap); }
        memcpy(buf + len, tmp, r);
        len += r;
    }
    close(sock);
    buf[len] = 0;
    Value out = make_dict();
    int status = 0;
    const char* sp = strchr(buf, ' ');
    if (sp) status = atoi(sp + 1);
    dict_set_take(out.as.dict, "status", NUMBER_VAL(status));
    Value headers = make_dict();
    const char* body = strstr(buf, "\r\n\r\n");
    const char* line = strstr(buf, "\r\n");
    if (line) line += 2;
    while (line && body && line < body) {
        const char* le = strstr(line, "\r\n");
        if (!le || le > body) break;
        const char* colon = memchr(line, ':', le - line);
        if (colon) {
            char* hk = (char*)malloc(colon - line + 1);
            memcpy(hk, line, colon - line);
            hk[colon - line] = 0;
            const char* vs = colon + 1;
            while (*vs == ' ') vs++;
            dict_set_take(headers.as.dict, hk, make_string(vs, (int)(le - vs)));
            free(hk);
        }
        line = le + 2;
    }
    dict_set_take(out.as.dict, "headers", headers);
    dict_set_take(out.as.dict, "body", make_string_cstr(body ? body + 4 : ""));
    free(buf);
    return out;
}
#else
static Value http_get(Interp* I, Value s, int argc, Value* a) {
    (void)s; (void)argc; (void)a;
    interp_error(I, 0, "http.get is not available on this build");
    return NULL_VAL();
}
#endif

static Value mx_asin(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(asin(numarg(I, argc, a, 0, "mathx.asin"))); }
static Value mx_acos(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(acos(numarg(I, argc, a, 0, "mathx.acos"))); }
static Value mx_sinh(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(sinh(numarg(I, argc, a, 0, "mathx.sinh"))); }
static Value mx_cosh(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(cosh(numarg(I, argc, a, 0, "mathx.cosh"))); }
static Value mx_asinh(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(asinh(numarg(I, argc, a, 0, "mathx.asinh"))); }
static Value mx_acosh(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(acosh(numarg(I, argc, a, 0, "mathx.acosh"))); }
static Value mx_atanh(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(atanh(numarg(I, argc, a, 0, "mathx.atanh"))); }
static Value mx_expm1(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(expm1(numarg(I, argc, a, 0, "mathx.expm1"))); }
static Value mx_log1p(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(log1p(numarg(I, argc, a, 0, "mathx.log1p"))); }
static Value mx_copysign(Interp* I, Value s, int argc, Value* a) { (void)s; return NUMBER_VAL(copysign(numarg(I, argc, a, 0, "mathx.copysign"), numarg(I, argc, a, 1, "mathx.copysign"))); }
static Value mx_hypot3(Interp* I, Value s, int argc, Value* a) { (void)s; double x = numarg(I, argc, a, 0, "mathx.hypot3"), y = numarg(I, argc, a, 1, "mathx.hypot3"), z = numarg(I, argc, a, 2, "mathx.hypot3"); return NUMBER_VAL(sqrt(x * x + y * y + z * z)); }
static Value mx_lerp(Interp* I, Value s, int argc, Value* a) { (void)s; double x = numarg(I, argc, a, 0, "mathx.lerp"), y = numarg(I, argc, a, 1, "mathx.lerp"), t = numarg(I, argc, a, 2, "mathx.lerp"); return NUMBER_VAL(x + (y - x) * t); }
static Value mx_map_range(Interp* I, Value s, int argc, Value* a) {
    (void)s;
    double x = numarg(I, argc, a, 0, "mathx.map_range");
    double a0 = numarg(I, argc, a, 1, "mathx.map_range"), b0 = numarg(I, argc, a, 2, "mathx.map_range");
    double a1 = numarg(I, argc, a, 3, "mathx.map_range"), b1 = numarg(I, argc, a, 4, "mathx.map_range");
    if (b0 == a0) interp_error(I, 0, "mathx.map_range: empty source range");
    return NUMBER_VAL(a1 + (x - a0) * (b1 - a1) / (b0 - a0));
}
static Value mx_is_nan(Interp* I, Value s, int argc, Value* a) { (void)s; return BOOL_VAL(isnan(numarg(I, argc, a, 0, "mathx.is_nan"))); }
static Value mx_is_inf(Interp* I, Value s, int argc, Value* a) { (void)s; return BOOL_VAL(isinf(numarg(I, argc, a, 0, "mathx.is_inf"))); }

void register_advanced(Interp* I) {
    Value linalg = make_dict();
    addfn(linalg.as.dict, "identity", la_identity);
    addfn(linalg.as.dict, "zeros", la_zeros);
    addfn(linalg.as.dict, "transpose", la_transpose);
    addfn(linalg.as.dict, "matmul", la_matmul);
    addfn(linalg.as.dict, "det", la_det);
    addfn(linalg.as.dict, "inverse", la_inverse);
    addfn(linalg.as.dict, "solve", la_solve);
    env_define(I->globals, "linalg", linalg);
    value_release(linalg);

    Value cplx = make_dict();
    addfn(cplx.as.dict, "new", cx_new);
    addfn(cplx.as.dict, "add", cx_add);
    addfn(cplx.as.dict, "sub", cx_sub);
    addfn(cplx.as.dict, "mul", cx_mul);
    addfn(cplx.as.dict, "div", cx_div);
    addfn(cplx.as.dict, "abs", cx_abs);
    addfn(cplx.as.dict, "arg", cx_arg);
    addfn(cplx.as.dict, "conj", cx_conj);
    addfn(cplx.as.dict, "polar", cx_polar);
    addfn(cplx.as.dict, "str", cx_str);
    env_define(I->globals, "cplx", cplx);
    value_release(cplx);

    Value codec = make_dict();
    addfn(codec.as.dict, "base32_encode", cd_b32_encode);
    addfn(codec.as.dict, "base32_decode", cd_b32_decode);
    addfn(codec.as.dict, "base64url_encode", cd_b64u_encode);
    addfn(codec.as.dict, "base64url_decode", cd_b64u_decode);
    addfn(codec.as.dict, "rot47", cd_rot47);
    env_define(I->globals, "codec", codec);
    value_release(codec);

    Value hmac = make_dict();
    addfn(hmac.as.dict, "sha256", hm_sha256);
    env_define(I->globals, "hmac", hmac);
    value_release(hmac);

    Value url = make_dict();
    addfn(url.as.dict, "parse", url_parse);
    addfn(url.as.dict, "build", url_build);
    addfn(url.as.dict, "query", url_query);
    addfn(url.as.dict, "encode", url_encode);
    addfn(url.as.dict, "decode", url_decode);
    env_define(I->globals, "url", url);
    value_release(url);

    Value http = make_dict();
    addfn(http.as.dict, "get", http_get);
    addfn(http.as.dict, "status_text", http_status_text);
    env_define(I->globals, "http", http);
    value_release(http);

    Value mathx = make_dict();
    addfn(mathx.as.dict, "asin", mx_asin);
    addfn(mathx.as.dict, "acos", mx_acos);
    addfn(mathx.as.dict, "sinh", mx_sinh);
    addfn(mathx.as.dict, "cosh", mx_cosh);
    addfn(mathx.as.dict, "asinh", mx_asinh);
    addfn(mathx.as.dict, "acosh", mx_acosh);
    addfn(mathx.as.dict, "atanh", mx_atanh);
    addfn(mathx.as.dict, "expm1", mx_expm1);
    addfn(mathx.as.dict, "log1p", mx_log1p);
    addfn(mathx.as.dict, "copysign", mx_copysign);
    addfn(mathx.as.dict, "hypot3", mx_hypot3);
    addfn(mathx.as.dict, "lerp", mx_lerp);
    addfn(mathx.as.dict, "map_range", mx_map_range);
    addfn(mathx.as.dict, "is_nan", mx_is_nan);
    addfn(mathx.as.dict, "is_inf", mx_is_inf);
    env_define(I->globals, "mathx", mathx);
    value_release(mathx);
}
