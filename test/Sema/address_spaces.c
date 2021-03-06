// RUN: clang-cc %s -fsyntax-only -verify

#define _AS1 __attribute__((address_space(1)))
#define _AS2 __attribute__((address_space(2)))
#define _AS3 __attribute__((address_space(3)))

void foo(_AS3 float *a) {
  _AS2 *x;// expected-warning {{type specifier missing, defaults to 'int'}}
  _AS1 float * _AS2 *B;

  int _AS1 _AS2 *Y;   // expected-error {{multiple address spaces specified for type}}
  int *_AS1 _AS2 *Z;  // expected-error {{multiple address spaces specified for type}}

  _AS1 int local;     // expected-error {{automatic variable qualified with an address space}}
  _AS1 int array[5];  // expected-error {{automatic variable qualified with an address space}}
  _AS1 int arrarr[5][5]; // expected-error {{automatic variable qualified with an address space}}

  __attribute__((address_space(-1))) int *_boundsA; // expected-error {{address space is negative}}
  __attribute__((address_space(0xFFFFFF))) int *_boundsB;
  __attribute__((address_space(0x1000000))) int *_boundsC; // expected-error {{address space is larger than the maximum supported}}
  // chosen specifically to overflow 32 bits and come out reasonable
  __attribute__((address_space(4294967500))) int *_boundsD; // expected-error {{address space is larger than the maximum supported}}

  *a = 5.0f;
}

struct _st {
 int x, y;
} s __attribute ((address_space(1))) = {1, 1};


// rdar://6774906
__attribute__((address_space(256))) void * * const base = 0;
void * get_0(void) {
  return base[0];  // expected-error {{illegal implicit cast between two pointers with different address spaces}} \
                      expected-warning {{returning 'void __attribute__((address_space(256)))*' discards qualifiers, expected 'void *'}}
}

