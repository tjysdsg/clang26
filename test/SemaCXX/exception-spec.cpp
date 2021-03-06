// RUN: clang-cc -fsyntax-only -verify -fms-extensions %s

// Straight from the standard:
// Plain function with spec
void f() throw(int);
// Pointer to function with spec
void (*fp)() throw (int);
// Function taking reference to function with spec
void g(void pfa() throw(int));
// Typedef for pointer to function with spec
typedef int (*pf)() throw(int); // expected-error {{specifications are not allowed in typedefs}}

// Some more:
// Function returning function with spec
void (*h())() throw(int);
// Ultimate parser thrill: function with spec returning function with spec and
// taking pointer to function with spec.
// The actual function throws int, the return type double, the argument float.
void (*i() throw(int))(void (*)() throw(float)) throw(double);
// Pointer to pointer to function taking function with spec
void (**k)(void pfa() throw(int)); // no-error
// Pointer to pointer to function with spec
void (**j)() throw(int); // expected-error {{not allowed beyond a single}}
// Pointer to function returning pointer to pointer to function with spec
void (**(*h())())() throw(int); // expected-error {{not allowed beyond a single}}

struct Incomplete;

// Exception spec must not have incomplete types, or pointers to them, except
// void.
void ic1() throw(void); // expected-error {{incomplete type 'void' is not allowed in exception specification}}
void ic2() throw(Incomplete); // expected-error {{incomplete type 'struct Incomplete' is not allowed in exception specification}}
void ic3() throw(void*);
void ic4() throw(Incomplete*); // expected-error {{pointer to incomplete type 'struct Incomplete' is not allowed in exception specification}}
void ic5() throw(Incomplete&); // expected-error {{reference to incomplete type 'struct Incomplete' is not allowed in exception specification}}

// Redeclarations
typedef int INT;
void r1() throw(int);
void r1() throw(int);

void r2() throw(int);
void r2() throw(INT);

// throw-any spec and no spec at all are semantically equivalent
void r3();
void r3() throw(...);

void r4() throw(int, float);
void r4() throw(float, int);

void r5() throw(int); // expected-note {{previous declaration}}
void r5(); // expected-error {{exception specification in declaration does not match}}

void r6() throw(...); // expected-note {{previous declaration}}
void r6() throw(int); // expected-error {{exception specification in declaration does not match}}

void r7() throw(int); // expected-note {{previous declaration}}
void r7() throw(float); // expected-error {{exception specification in declaration does not match}}

// Top-level const doesn't matter.
void r8() throw(int);
void r8() throw(const int);

struct A
{
};

struct B1 : A
{
};

struct B2 : A
{
};

struct D : B1, B2
{
};

struct P : private A
{
};

struct Base
{
  virtual void f1() throw();
  virtual void f2();
  virtual void f3() throw(...);
  virtual void f4() throw(int, float);

  virtual void f5() throw(int, float);
  virtual void f6() throw(A);
  virtual void f7() throw(A, int, float);
  virtual void f8();

  virtual void g1() throw(); // expected-note {{overridden virtual function is here}}
  virtual void g2() throw(int); // expected-note {{overridden virtual function is here}}
  virtual void g3() throw(A); // expected-note {{overridden virtual function is here}}
  virtual void g4() throw(B1); // expected-note {{overridden virtual function is here}}
  virtual void g5() throw(A); // expected-note {{overridden virtual function is here}}
};
struct Derived : Base
{
  virtual void f1() throw();
  virtual void f2() throw(...);
  virtual void f3();
  virtual void f4() throw(float, int);

  virtual void f5() throw(float);
  virtual void f6() throw(B1);
  virtual void f7() throw(B1, B2, int);
  virtual void f8() throw(B2, B2, int, float, char, double, bool);

  virtual void g1() throw(int); // expected-error {{exception specification of overriding function is more lax}}
  virtual void g2(); // expected-error {{exception specification of overriding function is more lax}}
  virtual void g3() throw(D); // expected-error {{exception specification of overriding function is more lax}}
  virtual void g4() throw(A); // expected-error {{exception specification of overriding function is more lax}}
  virtual void g5() throw(P); // expected-error {{exception specification of overriding function is more lax}}
};
