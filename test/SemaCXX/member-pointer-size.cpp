// RUN: clang-cc -triple x86_64-unknown-unknown %s -fsyntax-only -verify &&
// RUN: clang-cc -triple i686-unknown-unknown %s -fsyntax-only -verify
#include <stddef.h>

struct A;

void f() {
  int A::*dataMember;
  
  int (A::*memberFunction)();
  
  typedef int assert1[sizeof(dataMember) == sizeof(ptrdiff_t) ? 1 : -1];
  typedef int assert2[sizeof(memberFunction) == sizeof(ptrdiff_t) * 2 ? 1 : -1];
}

