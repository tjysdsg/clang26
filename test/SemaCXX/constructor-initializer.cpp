// RUN: clang-cc -Wreorder -fsyntax-only -verify %s
class A { 
  int m;
   A() : A::m(17) { } // expected-error {{member initializer 'm' does not name a non-static data member or base class}}
   A(int);
};

class B : public A { 
public:
  B() : A(), m(1), n(3.14) { }

private:
  int m;
  float n;  
};


class C : public virtual B { 
public:
  C() : B() { }
};

class D : public C { 
public:
  D() : B(), C() { }
};

class E : public D, public B { 
public:
  E() : B(), D() { } // expected-error{{base class initializer 'class B' names both a direct base class and an inherited virtual base class}}
};


typedef int INT;

class F : public B { 
public:
  int B;

  F() : B(17),
        m(17), // expected-error{{member initializer 'm' does not name a non-static data member or base class}}
        INT(17) // expected-error{{constructor initializer 'INT' (aka 'int') does not name a class}}
  { 
  }
};

class G : A {
  G() : A(10); // expected-error{{expected '{'}}
};

void f() : a(242) { } // expected-error{{only constructors take base initializers}}

class H : A {
  H();
};

H::H() : A(10) { }


class  X {};
class Y {};

struct S : Y, virtual X {
  S (); 
};

struct Z : S { 
  Z() : X(), S(), E()  {} // expected-error {{type 'class E' is not a direct or virtual base of 'Z'}}
};

class U { 
  union { int a; char* p; };
  union { int b; double d; };

  U() :  a(1), p(0), d(1.0)  {} // expected-error {{multiple initializations given for non-static member 'p'}} \
                        // expected-note {{previous initialization is here}}
};

struct V {};
struct Base {};
struct Base1 {};

struct Derived : Base, Base1, virtual V {
  Derived ();
};

struct Current : Derived {
  int Derived;
  Current() : Derived(1), ::Derived(), // expected-warning {{member 'Derived' will be initialized after}} \
                                       // expected-note {{base '::Derived'}} \
                                       // expected-warning {{base class '::Derived' will be initialized after}}
                          ::Derived::Base(), // expected-error {{type '::Derived::Base' is not a direct or virtual base of 'Current'}}
                           Derived::Base1(), // expected-error {{type 'Derived::Base1' is not a direct or virtual base of 'Current'}}
                           Derived::V(), // expected-note {{base 'Derived::V'}}
                           ::NonExisting(), // expected-error {{member initializer 'NonExisting' does not name a non-static data member or}}
                           INT::NonExisting()  {} // expected-error {{expected a class or namespace}} \
						  // expected-error {{member initializer 'NonExisting' does not name a non-static data member or}}
};

                        // FIXME. This is bad message!
struct M { 		// expected-note {{candidate function}}	\
			 // expected-note {{candidate function}}
  M(int i, int j);	// expected-note {{candidate function}} \
			// // expected-note {{candidate function}}
};

struct N : M  {
  N() : M(1), 	// expected-error {{no matching constructor for initialization of 'M'}}
        m1(100) {  } // expected-error {{no matching constructor for initialization of 'm1'}}
  M m1;
};

struct P : M  { // expected-error {{default constructor for 'struct M' is missing in initialization of base class}}
  P()  {  }
  M m; // expected-error {{default constructor for 'struct M' is missing in initialization of member}}
};

struct Q {
  Q() : f1(1,2),	// expected-error {{Too many arguments for member initializer 'f1'}}
        pf(0.0)  { }   // expected-error {{incompatible type passing 'double', expected 'float *'}}
  float f1;

  float *pf;
};

