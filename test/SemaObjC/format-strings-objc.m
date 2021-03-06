// RUN: clang-cc -fsyntax-only -verify %s

//===----------------------------------------------------------------------===//
// The following code is reduced using delta-debugging from
// Foundation.h (Mac OS X).
//
// It includes the basic definitions for the test cases below.
// Not including Foundation.h directly makes this test case both svelt and
// portable to non-Mac platforms.
//===----------------------------------------------------------------------===//

typedef signed char BOOL;
typedef unsigned int NSUInteger;
@class NSString, Protocol;
extern void NSLog(NSString *format, ...) __attribute__((format(__NSString__, 1, 2)));
typedef struct _NSZone NSZone;
@class NSInvocation, NSMethodSignature, NSCoder, NSString, NSEnumerator;
@protocol NSObject  - (BOOL)isEqual:(id)object; @end
@protocol NSCopying  - (id)copyWithZone:(NSZone *)zone; @end
@protocol NSMutableCopying  - (id)mutableCopyWithZone:(NSZone *)zone; @end
@protocol NSCoding  - (void)encodeWithCoder:(NSCoder *)aCoder; @end
@interface NSObject <NSObject> {} @end
typedef float CGFloat;
@interface NSString : NSObject <NSCopying, NSMutableCopying, NSCoding>    - (NSUInteger)length; @end
@interface NSSimpleCString : NSString {} @end
@interface NSConstantString : NSSimpleCString @end
extern void *_NSConstantStringClassReference;

typedef const struct __CFString * CFStringRef;
extern void CFStringCreateWithFormat(CFStringRef format, ...) __attribute__((format(CFString, 1, 2)));

//===----------------------------------------------------------------------===//
// Test cases.
//===----------------------------------------------------------------------===//

void check_nslog(unsigned k) {
  NSLog(@"%d%%", k); // no-warning
  NSLog(@"%s%lb%d", "unix", 10,20); // expected-warning {{lid conversion '%lb'}}
}

// Check type validation
extern void NSLog2(int format, ...) __attribute__((format(__NSString__, 1, 2))); // expected-error {{format argument not an NSString}}
extern void CFStringCreateWithFormat2(int *format, ...) __attribute__((format(CFString, 1, 2))); // expected-error {{format argument not a CFString}}
