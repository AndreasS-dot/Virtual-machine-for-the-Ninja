/*
 * bigint.h -- big integer library
 */


#ifndef _BIGINT_H_
#define _BIGINT_H_


/* object representation */
typedef void* BigObjRef;


#include <stdio.h>


typedef struct {
  int nd;     /* number of digits; array may be bigger */
        /* nd = 0 exactly when number = 0 */
  unsigned char sign;   /* one of BIG_NEGATIVE or BIG_POSITIVE */
        /* zero always has BIG_POSITIVE here */
  unsigned char digits[1];  /* the digits proper; number base is 256 */
        /* LS digit first; MS digit is not zero */
} Big;

#include "support.h"


/* big integer processor registers */

typedef struct {
  BigObjRef op1;      /* first (or single) operand */
  BigObjRef op2;      /* second operand (if present) */
  BigObjRef res;      /* result of operation */
  BigObjRef rem;      /* remainder in case of division */
} BIP;

extern BIP bip;     /* registers of the processor */


/* big integer processor functions */

int bigSgn(void);     /* sign */
int bigCmp(void);     /* comparison */
void bigNeg(void);      /* negation */
void bigAdd(void);      /* addition */
void bigSub(void);      /* subtraction */
void bigMul(void);      /* multiplication */
void bigDiv(void);      /* division */

void bigFromInt(int n);     /* conversion int --> big */
int bigToInt(void);     /* conversion big --> int */

void bigRead(FILE *in);     /* read a big integer */
void bigPrint(FILE *out);   /* print a big integer */

void bigDump(FILE *out, BigObjRef bigObjRef); /* dump a big integer object */


#endif /* _BIGINT_H_ */
#define halt 0
#define pushc 1
#define add 2
#define sub 3
#define mul 4
#define div 5
#define mod 6
#define rdint 7
#define wrint 8
#define rdchr 9
#define wrchr 10
#define pushg 11
#define popg 12
#define asf 13
#define rsf 14
#define pushl 15
#define popl 16
#define eq 17
#define ne 18
#define lt 19
#define le 20
#define gt 21
#define ge 22
#define jmp 23
#define brt 24
#define brf 25
#define call 26
#define ret 27
#define drop 28
#define pushr 29 
#define popr 30
#define dup 31
#define new 32
#define getf 33
#define putf 34
#define newa 35
#define getfa 36
#define putfa 37
#define getsz 38
#define pushn 39
#define refeq 40
#define refne 41