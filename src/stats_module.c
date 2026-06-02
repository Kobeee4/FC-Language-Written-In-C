#include <stdlib.h>
#include <math.h>
#include "interpreter.h"

static ObjList* number_list(Interp* I, int argc, Value* argv, const char* fn) {
    if (argc < 1 || argv[0].type != VAL_LIST)
        interp_error(I, 0, "%s expects a list of numbers", fn);
    ObjList* l = argv[0].as.list;
    for (int i = 0; i < l->count; i++)
        if (l->items[i].type != VAL_NUMBER)
            interp_error(I, 0, "%s: every element must be a number", fn);
    return l;
}

static double list_sum(ObjList* l) {
    double s = 0;
    for (int i = 0; i < l->count; i++) s += l->items[i].as.number;
    return s;
}

static Value stats_sum(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    return NUMBER_VAL(list_sum(number_list(I, argc, argv, "stats.sum")));
}

static Value stats_mean(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.mean");
    if (l->count == 0) interp_error(I, 0, "stats.mean: empty list");
    return NUMBER_VAL(list_sum(l) / l->count);
}

static Value stats_min(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.min");
    if (l->count == 0) interp_error(I, 0, "stats.min: empty list");
    double m = l->items[0].as.number;
    for (int i = 1; i < l->count; i++)
        if (l->items[i].as.number < m) m = l->items[i].as.number;
    return NUMBER_VAL(m);
}

static Value stats_max(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.max");
    if (l->count == 0) interp_error(I, 0, "stats.max: empty list");
    double m = l->items[0].as.number;
    for (int i = 1; i < l->count; i++)
        if (l->items[i].as.number > m) m = l->items[i].as.number;
    return NUMBER_VAL(m);
}

static double population_variance(ObjList* l) {
    double mean = list_sum(l) / l->count;
    double acc = 0;
    for (int i = 0; i < l->count; i++) {
        double d = l->items[i].as.number - mean;
        acc += d * d;
    }
    return acc / l->count;
}

static Value stats_variance(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.variance");
    if (l->count == 0) interp_error(I, 0, "stats.variance: empty list");
    return NUMBER_VAL(population_variance(l));
}

static Value stats_stdev(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.stdev");
    if (l->count == 0) interp_error(I, 0, "stats.stdev: empty list");
    return NUMBER_VAL(sqrt(population_variance(l)));
}

static int cmp_double(const void* a, const void* b) {
    double x = *(const double*)a, y = *(const double*)b;
    return (x > y) - (x < y);
}

static Value stats_median(Interp* I, Value self, int argc, Value* argv) {
    (void)self;
    ObjList* l = number_list(I, argc, argv, "stats.median");
    int n = l->count;
    if (n == 0) interp_error(I, 0, "stats.median: empty list");
    double* a = (double*)malloc(sizeof(double) * n);
    for (int i = 0; i < n; i++) a[i] = l->items[i].as.number;
    qsort(a, n, sizeof(double), cmp_double);
    double med = (n % 2) ? a[n / 2] : (a[n / 2 - 1] + a[n / 2]) / 2.0;
    free(a);
    return NUMBER_VAL(med);
}

static Value stats_describe(Interp* I, Value self, int argc, Value* argv) {
    ObjList* l = number_list(I, argc, argv, "stats.describe");
    if (l->count == 0) interp_error(I, 0, "stats.describe: empty list");
    Value d = make_dict();
    dict_set_take(d.as.dict, "count", NUMBER_VAL(l->count));
    dict_set_take(d.as.dict, "sum", stats_sum(I, self, argc, argv));
    dict_set_take(d.as.dict, "mean", stats_mean(I, self, argc, argv));
    dict_set_take(d.as.dict, "min", stats_min(I, self, argc, argv));
    dict_set_take(d.as.dict, "max", stats_max(I, self, argc, argv));
    dict_set_take(d.as.dict, "median", stats_median(I, self, argc, argv));
    dict_set_take(d.as.dict, "stdev", stats_stdev(I, self, argc, argv));
    return d;
}

static void add_fn(ObjDict* mod, const char* name, NativeFn fn) {
    dict_set_take(mod, name, make_native(fn, NULL_VAL(), name));
}

void register_stats(Interp* I) {
    Value mod = make_dict();
    add_fn(mod.as.dict, "sum", stats_sum);
    add_fn(mod.as.dict, "mean", stats_mean);
    add_fn(mod.as.dict, "min", stats_min);
    add_fn(mod.as.dict, "max", stats_max);
    add_fn(mod.as.dict, "median", stats_median);
    add_fn(mod.as.dict, "variance", stats_variance);
    add_fn(mod.as.dict, "stdev", stats_stdev);
    add_fn(mod.as.dict, "describe", stats_describe);
    env_define(I->globals, "stats", mod);
    value_release(mod);
}
