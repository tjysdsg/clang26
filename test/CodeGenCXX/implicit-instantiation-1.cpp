// RUN: clang-cc -emit-llvm %s -o %t &&

template<typename T>
struct X {
  void f(T) { }
  void f(char) { }
  
  void g(T) { }
  
  void h(T) { }
};

void foo(X<int> &xi, X<float> *xfp, int i, float f) {
  // RUN: grep "linkonce_odr.*_ZN1XIiE1fEi" %t | count 1 &&
  xi.f(i);
  
  // RUN: grep "linkonce_odr.*_ZN1XIiE1gEi" %t | count 1 &&
  xi.g(f);
  
  // RUN: grep "linkonce_odr.*_ZN1XIfE1fEf" %t | count 1 &&
  xfp->f(f);
  
  // RUN: grep "linkonce_odr.*_ZN1XIfE1hEf" %t | count 0 &&
  
  // RUN: true
}



