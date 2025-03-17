#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

struct lval {
  int type;
  long num;
  char* err;
  char* sym;
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  int count;
  lval** cell;
};

struct lenv {
  int count;
  char** syms;
  lval** vals;
};

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(const char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval* lval_sym(const char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  int i;
  switch(v->type) {
    case LVAL_NUM: break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_FUN:
      if(!v->builtin) {
        lval_del(v->formals);
        lval_del(v->body);
        lenv* e = v->env;
        if(e) {
          for(i = 0; i < e->count; i++) {
            free(e->syms[i]);
            lval_del(e->vals[i]);
          }
          free(e->syms);
          free(e->vals);
          free(e);
        }
      }
      break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      for(i = 0; i < v->count; i++) { lval_del(v->cell[i]); }
      free(v->cell);
      break;
  }
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv* e) {
  for(int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lval* lenv_get(lenv* e, lval* k) {
  for(int i = 0; i < e->count; i++) {
    if(strcmp(e->syms[i], k->sym) == 0) {
      return e->vals[i];
    }
  }
  return lval_err("Unbound Symbol");
}

void lenv_put(lenv* e, lval* k, lval* v) {
  for(int i = 0; i < e->count; i++) {
    if(strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = v;
      return;
    }
  }
  e->count++;
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
  e->vals[e->count - 1] = v;
}

lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);
  for(int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = e->vals[i];
  }
  return n;
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;
  switch(v->type) {
    case LVAL_NUM: x->num = v->num; break;
    case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
    case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
    case LVAL_FUN:
      if(v->builtin) { x->builtin = v->builtin; }
      else {
        x->builtin = NULL;
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for(int i = 0; i < x->count; i++) { x->cell[i] = lval_copy(v->cell[i]); }
      break;
  }
  return x;
}

void lval_print(lval *v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for(int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if(i != (v->count - 1)) { putchar(' '); }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch(v->type) {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_FUN:
      if(v->builtin) { printf("<builtin>"); }
      else { printf("(\\ "); lval_print(v->formals); putchar(' '); lval_print(v->body); putchar(')'); }
      break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lenv* e, lval* a, const char* op) {
  for(int i = 0; i < a->count; i++) {
    if(a->cell[i]->type != LVAL_NUM) { lval_del(a); return lval_err("Cannot operate on non-number!"); }
  }
  lval* x = lval_pop(a, 0);
  if((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }
  while(a->count > 0) {
    lval* y = lval_pop(a, 0);
    if(strcmp(op, "+") == 0) { x->num += y->num; }
    if(strcmp(op, "-") == 0) { x->num -= y->num; }
    if(strcmp(op, "*") == 0) { x->num *= y->num; }
    if(strcmp(op, "/") == 0) {
      if(y->num == 0) { lval_del(x); lval_del(y); x = lval_err("Division By Zero!"); break; }
      x->num /= y->num;
    }
    lval_del(y);
  }
  lval_del(a);
  return x;
}

lval* builtin_head(lenv* e, lval* a) {
  if(a->count != 1) { lval_del(a); return lval_err("Function 'head' passed too many arguments!"); }
  if(a->cell[0]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function 'head' passed incorrect type!"); }
  if(a->cell[0]->count == 0) { lval_del(a); return lval_err("Function 'head' passed {}!"); }
  lval* v = lval_take(a, 0);
  while(v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  if(a->count != 1) { lval_del(a); return lval_err("Function 'tail' passed too many arguments!"); }
  if(a->cell[0]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function 'tail' passed incorrect type!"); }
  if(a->cell[0]->count == 0) { lval_del(a); return lval_err("Function 'tail' passed {}!"); }
  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  if(a->count != 1) { lval_del(a); return lval_err("Function 'eval' passed too many arguments!"); }
  if(a->cell[0]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function 'eval' passed incorrect type!"); }
  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return x;
}

lval* builtin_join(lenv* e, lval* a) {
  for(int i = 0; i < a->count; i++) {
    if(a->cell[i]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function 'join' passed incorrect type."); }
  }
  lval* x = lval_pop(a, 0);
  while(a->count) {
    lval* y = lval_pop(a, 0);
    int oldCount = x->count;
    x->count += y->count;
    x->cell = realloc(x->cell, sizeof(lval*) * x->count);
    for(int i = 0; i < y->count; i++) { x->cell[oldCount + i] = y->cell[i]; }
    free(y->cell);
    free(y);
  }
  lval_del(a);
  return x;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  if(f->builtin) { return f->builtin(e, a); }
  while(a->count) {
    if(f->formals->count == 0) { lval_del(a); return lval_err("Function passed too many arguments."); }
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_pop(a, 0);
    lenv_put(f->env, sym, val);
    lval_del(sym);
  }
  if(f->formals->count == 0) {
    f->env = lenv_copy(f->env);
    lval* expr = lval_add(lval_sexpr(), lval_copy(f->body));
    return builtin_eval(e, expr);
  }
  return lval_copy(f);
}

lval* builtin_lambda(lenv* e, lval* a) {
  if(a->count != 2) { lval_del(a); return lval_err("Function '\\' passed incorrect number of arguments."); }
  if(a->cell[0]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function '\\' passed incorrect type."); }
  for(int i = 0; i < a->cell[0]->count; i++) {
    if(a->cell[0]->cell[i]->type != LVAL_SYM) { lval_del(a); return lval_err("Cannot define non-symbol."); }
  }
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval* f = lval_fun(NULL);
  f->builtin = NULL;
  f->env = lenv_new();
  f->formals = formals;
  f->body = body;
  lval_del(a);
  return f;
}

lval* builtin_def(lenv* e, lval* a) {
  if(a->cell[0]->type != LVAL_QEXPR) { lval_del(a); return lval_err("Function 'def' passed incorrect type!"); }
  lval* syms = a->cell[0];
  for(int i = 0; i < syms->count; i++) {
    if(syms->cell[i]->type != LVAL_SYM) { lval_del(a); return lval_err("Function 'def' cannot define non-symbol"); }
  }
  if(syms->count != a->count - 1) { lval_del(a); return lval_err("Function 'def' passed incorrect number of values to symbols"); }
  for(int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i + 1]);
  }
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

void lenv_add_builtin(lenv* e, const char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "\\", builtin_lambda);
  lenv_add_builtin(e, "def", builtin_def);
}

lval* lval_read_num(const char** s) {
  char* end;
  long x = strtol(*s, &end, 10);
  *s = end;
  return lval_num(x);
}

lval* lval_read(const char** s);
lval* lval_read_expr(const char** s, char end) {
  lval* x = (end == ')') ? lval_sexpr() : lval_qexpr();
  (*s)++;
  while(**s && **s != end) {
    if(isspace(**s)) { (*s)++; continue; }
    x = lval_add(x, lval_read(s));
  }
  if(**s == end) { (*s)++; }
  return x;
}

lval* lval_read(const char** s) {
  while(isspace(**s)) { (*s)++; }
  if(**s == '\0') return NULL;
  if(isdigit(**s) || ((**s == '-') && isdigit((*s)[1]))) return lval_read_num(s);
  if(isalpha(**s) || strchr("+-/*\\", **s)) {
    const char* start = *s;
    while(isalnum(**s) || strchr("+-/*\\", **s)) { (*s)++; }
    int len = *s - start;
    char* token = malloc(len + 1);
    strncpy(token, start, len);
    token[len] = '\0';
    lval* x = lval_sym(token);
    free(token);
    return x;
  }
  if(**s == '(') return lval_read_expr(s, ')');
  if(**s == '{') return lval_read_expr(s, '}');
  (*s)++;
  return lval_err("Unexpected character");
}

lval* lval_eval(lenv* e, lval* v) {
  if(v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if(v->type == LVAL_SEXPR) {
    for(int i = 0; i < v->count; i++) {
      v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for(int i = 0; i < v->count; i++) {
      if(v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
    }
    if(v->count == 0) return v;
    if(v->count == 1) return lval_take(v, 0);
    lval* f = lval_pop(v, 0);
    if(f->type != LVAL_FUN) {
      lval_del(f); lval_del(v);
      return lval_err("First element is not a function");
    }
    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
  }
  return v;
}

int main() {
  lenv* e = lenv_new();
  lenv_add_builtins(e);
  char buf[2048];
  while(1) {
    printf("lisp> ");
    if(!fgets(buf, 2048, stdin)) break;
    const char* input = buf;
    lval* expr = lval_read(&input);
    if(!expr) continue;
    lval* result = lval_eval(e, expr);
    lval_println(result);
    lval_del(result);
  }
  lenv_del(e);
  return 0;
}

