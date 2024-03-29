
#include <stdio.h>
#include <assert.h>
#include <malloc.h>  // memalign
#include <string.h>  // memset
#include <math.h>    // isnormal

typedef  unsigned char           UChar;
typedef  unsigned short int      UShort;
typedef  unsigned int            UInt;
typedef  signed int              Int;
typedef  unsigned char           UChar;
typedef  unsigned long long int  ULong;

typedef  unsigned char           Bool;
#define False ((Bool)0)
#define True  ((Bool)1)


#define ITERS 1

typedef
  enum { TySF=1234, TyDF, TyB, TyH, TyS, TyD, TyNONE }
  LaneTy;

union _V128 {
   UChar  u8[16];
   UShort u16[8];
   UInt   u32[4];
   ULong  u64[2];
   float  f32[4];
   double f64[2];
};
typedef  union _V128   V128;

static inline UChar randUChar ( void )
{
   static UInt seed = 80021;
   seed = 1103515245 * seed + 12345;
   return (seed >> 17) & 0xFF;
}

static ULong randULong ( LaneTy ty )
{
   Int i;
   ULong r = 0;
   for (i = 0; i < 8; i++) {
      r = (r << 8) | (ULong)(0xFF & randUChar());
   }
   return r;
}

/* Generates a random V128.  Ensures that that it contains normalised
   FP numbers when viewed as either F32x4 or F64x2, so that it is
   reasonable to use in FP test cases. */
static void randV128 ( /*OUT*/V128* v, LaneTy ty )
{
   static UInt nCalls = 0, nIters = 0;
   Int i;
   nCalls++;
   while (1) {
      nIters++;
      for (i = 0; i < 16; i++) {
         v->u8[i] = randUChar();
      }
      if (isnormal(v->f32[0]) && isnormal(v->f32[1]) && isnormal(v->f32[2])
          && isnormal(v->f32[3]) && isnormal(v->f64[0]) && isnormal(v->f64[1]))
        break;
   }
   if (0 == (nCalls & 0xFF))
      printf("randV128: %u calls, %u iters\n", nCalls, nIters);
}

static void showV128 ( V128* v )
{
   Int i;
   for (i = 15; i >= 0; i--)
      printf("%02x", (Int)v->u8[i]);
}

__attribute__((unused))
static void* memalign16(size_t szB)
{
   void* x;
   x = memalign(16, szB);
   assert(x);
   assert(0 == ((16-1) & (unsigned long)x));
   return x;
}


/* ---------------------------------------------------------------- */
/* -- Test functions                                             -- */
/* ---------------------------------------------------------------- */

/* Note this also sets the destination register to a known value (0x55..55)
   since it can sometimes be an input to the instruction too. */
#define GEN_UNARY_TEST(INSN,SUFFIXD,SUFFIXN) \
  __attribute__((noinline)) \
  static void test_##INSN##_##SUFFIXD##_##SUFFIXN ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[2]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        __asm__ __volatile__( \
           "ldr   q7, [%0, #0]   ; " \
           "ldr   q8, [%0, #16]   ; " \
           #INSN " v8." #SUFFIXD ", v7." #SUFFIXN " ; " \
           "str   q8, [%0, #16] " \
           : : "r"(&block[0]) : "memory", "v7", "v8" \
        ); \
        printf(#INSN   " v8." #SUFFIXD ", v7." #SUFFIXN); \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("\n"); \
     } \
  }


/* Note this also sets the destination register to a known value (0x55..55)
   since it can sometimes be an input to the instruction too. */
#define GEN_BINARY_TEST(INSN,SUFFIXD,SUFFIXN,SUFFIXM)  \
  __attribute__((noinline)) \
  static void test_##INSN##_##SUFFIXD##_##SUFFIXN##_##SUFFIXM ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[3]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        randV128(&block[2], ty); \
        __asm__ __volatile__( \
           "ldr   q7, [%0, #0]   ; " \
           "ldr   q8, [%0, #16]   ; " \
           "ldr   q9, [%0, #32]   ; " \
           #INSN " v9." #SUFFIXD ", v7." #SUFFIXN ", v8." #SUFFIXM " ; " \
           "str   q9, [%0, #32] " \
           : : "r"(&block[0]) : "memory", "v7", "v8", "v9" \
        ); \
        printf(#INSN   " v9." #SUFFIXD \
               ", v7." #SUFFIXN ", v8." #SUFFIXM "  ");   \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("  "); \
        showV128(&block[2]); printf("\n"); \
     } \
  }


/* Note this also sets the destination register to a known value (0x55..55)
   since it can sometimes be an input to the instruction too. */
#define GEN_SHIFT_TEST(INSN,SUFFIXD,SUFFIXN,AMOUNT) \
  __attribute__((noinline)) \
  static void test_##INSN##_##SUFFIXD##_##SUFFIXN##_##AMOUNT ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[2]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        __asm__ __volatile__( \
           "ldr   q7, [%0, #0]   ; " \
           "ldr   q8, [%0, #16]   ; " \
           #INSN " v8." #SUFFIXD ", v7." #SUFFIXN ", #" #AMOUNT " ; " \
           "str   q8, [%0, #16] " \
           : : "r"(&block[0]) : "memory", "v7", "v8" \
        ); \
        printf(#INSN   " v8." #SUFFIXD ", v7." #SUFFIXN ", #" #AMOUNT "  "); \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("\n"); \
     } \
  }


/* Generate a test that involves one integer reg and one vector reg,
   with no bias as towards which is input or output. */
#define GEN_ONEINT_ONEVEC_TEST(TESTNAME,INSN,INTREGNO,VECREGNO) \
  __attribute__((noinline)) \
  static void test_##TESTNAME ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[4]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        randV128(&block[2], ty); \
        randV128(&block[3], ty); \
        __asm__ __volatile__( \
           "ldr   q"#VECREGNO", [%0, #0]  ; " \
           "ldr   x"#INTREGNO", [%0, #16] ; " \
           INSN " ; " \
           "str   q"#VECREGNO", [%0, #32] ; " \
           "str   x"#INTREGNO", [%0, #48] ; " \
           : : "r"(&block[0]) : "memory", "v"#VECREGNO, "x"#INTREGNO \
        ); \
        printf(INSN   "   "); \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("  "); \
        showV128(&block[2]); printf("  "); \
        showV128(&block[3]); printf("\n"); \
     } \
  }


/* Generate a test that involves two vector regs,
   with no bias as towards which is input or output. 
   It's OK to use x10 as scratch.*/
#define GEN_TWOVEC_TEST(TESTNAME,INSN,VECREG1NO,VECREG2NO) \
  __attribute__((noinline)) \
  static void test_##TESTNAME ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[4]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        randV128(&block[2], ty); \
        randV128(&block[3], ty); \
        __asm__ __volatile__( \
           "ldr   q"#VECREG1NO", [%0, #0]  ; " \
           "ldr   q"#VECREG2NO", [%0, #16] ; " \
           INSN " ; " \
           "str   q"#VECREG1NO", [%0, #32] ; " \
           "str   q"#VECREG2NO", [%0, #48] ; " \
           : : "r"(&block[0]) \
             : "memory", "v"#VECREG1NO, "v"#VECREG2NO, "x10" \
        ); \
        printf(INSN   "   "); \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("  "); \
        showV128(&block[2]); printf("  "); \
        showV128(&block[3]); printf("\n"); \
     } \
  }


/* Generate a test that involves three vector regs,
   with no bias as towards which is input or output.  It's also OK
   to use v16, v17, v18 as scratch. */
#define GEN_THREEVEC_TEST(TESTNAME,INSN,VECREG1NO,VECREG2NO,VECREG3NO)  \
  __attribute__((noinline)) \
  static void test_##TESTNAME ( LaneTy ty ) { \
     Int i; \
     for (i = 0; i < ITERS; i++) { \
        V128 block[6]; \
        memset(block, 0x55, sizeof(block)); \
        randV128(&block[0], ty); \
        randV128(&block[1], ty); \
        randV128(&block[2], ty); \
        randV128(&block[3], ty); \
        randV128(&block[4], ty); \
        randV128(&block[5], ty); \
        __asm__ __volatile__( \
           "ldr   q"#VECREG1NO", [%0, #0]  ; " \
           "ldr   q"#VECREG2NO", [%0, #16] ; " \
           "ldr   q"#VECREG3NO", [%0, #32] ; " \
           INSN " ; " \
           "str   q"#VECREG1NO", [%0, #48] ; " \
           "str   q"#VECREG2NO", [%0, #64] ; " \
           "str   q"#VECREG3NO", [%0, #80] ; " \
           : : "r"(&block[0]) \
           : "memory", "v"#VECREG1NO, "v"#VECREG2NO, "v"#VECREG3NO, \
             "v16", "v17", "v18" \
        ); \
        printf(INSN   "   "); \
        showV128(&block[0]); printf("  "); \
        showV128(&block[1]); printf("  "); \
        showV128(&block[2]); printf("  "); \
        showV128(&block[3]); printf("  "); \
        showV128(&block[4]); printf("  "); \
        showV128(&block[5]); printf("\n"); \
     } \
  }


void test_UMINV ( void )
{
  int i;
  V128 block[2];

  /* -- 4s -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyS);
    randV128(&block[1], TyS);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "uminv s8, v7.4s   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMINV v8, v7.4s  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "uminv h8, v7.8h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMINV h8, v7.8h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 4h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "uminv h8, v7.4h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMINV h8, v7.4h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 16b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "uminv b8, v7.16b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMINV b8, v7.16b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "uminv b8, v7.8b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMINV b8, v7.8b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

}


void test_UMAXV ( void )
{
  int i;
  V128 block[2];

  /* -- 4s -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyS);
    randV128(&block[1], TyS);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "umaxv s8, v7.4s   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMAXV v8, v7.4s  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "umaxv h8, v7.8h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMAXV h8, v7.8h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 4h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "umaxv h8, v7.4h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMAXV h8, v7.4h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 16b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "umaxv b8, v7.16b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMAXV b8, v7.16b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "umaxv b8, v7.8b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("UMAXV b8, v7.8b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

}


void test_INS_general ( void )
{
  V128 block[3];

  /* -- D[0..1] -- */

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyD);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.d[0], x19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.u64[0],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyD);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.d[1], x19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.d[1],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  /* -- S[0..3] -- */

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyS);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.s[0], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.s[0],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyS);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.s[1], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.s[1],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyS);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.s[2], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.s[2],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyS);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.s[3], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.s[3],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  /* -- H[0..7] -- */

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[0], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[0],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[1], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[1],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[2], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[2],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[3], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[3],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[4], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[4],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[5], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[5],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[6], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[6],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyH);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.h[7], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.h[7],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  /* -- B[0,15] -- */

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyB);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.b[0], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.b[0],x19  ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");

  memset(&block, 0x55, sizeof(block));
  block[1].u64[0] = randULong(TyB);
  __asm__ __volatile__(
     "ldr q7, [%0, #0]   ; "
     "ldr x19, [%0, #16] ; "
     "ins v7.b[15], w19   ; "
     "str q7, [%0, #32] "
     : : "r"(&block[0]) : "memory", "x19", "v7"
  );
  printf("INS v7.b[15],x19 ");
  showV128(&block[0]); printf("  %016llx  ", block[1].u64[0]);
  showV128(&block[2]); printf("\n");
}



void test_SMINV ( void )
{
  int i;
  V128 block[2];

  /* -- 4s -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyS);
    randV128(&block[1], TyS);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "sminv s8, v7.4s   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMINV v8, v7.4s  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "sminv h8, v7.8h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMINV h8, v7.8h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 4h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "sminv h8, v7.4h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMINV h8, v7.4h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 16b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "sminv b8, v7.16b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMINV b8, v7.16b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "sminv b8, v7.8b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMINV b8, v7.8b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

}


void test_SMAXV ( void )
{
  int i;
  V128 block[2];

  /* -- 4s -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyS);
    randV128(&block[1], TyS);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "smaxv s8, v7.4s   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMAXV v8, v7.4s  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "smaxv h8, v7.8h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMAXV h8, v7.8h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 4h -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyH);
    randV128(&block[1], TyH);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "smaxv h8, v7.4h   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMAXV h8, v7.4h  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 16b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "smaxv b8, v7.16b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMAXV b8, v7.16b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

  /* -- 8b -- */

  for (i = 0; i < 10; i++) {
    memset(&block, 0x55, sizeof(block));
    randV128(&block[0], TyB);
    randV128(&block[1], TyB);
    __asm__ __volatile__(
       "ldr   q7, [%0, #0]   ; "
       "smaxv b8, v7.8b   ; "
       "str   q8, [%0, #16] "
       : : "r"(&block[0]) : "memory", "v7", "v8"
                         );
    printf("SMAXV b8, v7.8b  ");
    showV128(&block[0]); printf("  ");
    showV128(&block[1]); printf("\n");
  }

}


GEN_BINARY_TEST(umax, 4s, 4s, 4s)
GEN_BINARY_TEST(umax, 2s, 2s, 2s)
GEN_BINARY_TEST(umax, 8h, 8h, 8h)
GEN_BINARY_TEST(umax, 4h, 4h, 4h)
GEN_BINARY_TEST(umax, 16b, 16b, 16b)
GEN_BINARY_TEST(umax, 8b, 8b, 8b)

GEN_BINARY_TEST(umin, 4s, 4s, 4s)
GEN_BINARY_TEST(umin, 2s, 2s, 2s)
GEN_BINARY_TEST(umin, 8h, 8h, 8h)
GEN_BINARY_TEST(umin, 4h, 4h, 4h)
GEN_BINARY_TEST(umin, 16b, 16b, 16b)
GEN_BINARY_TEST(umin, 8b, 8b, 8b)

GEN_BINARY_TEST(smax, 4s, 4s, 4s)
GEN_BINARY_TEST(smax, 2s, 2s, 2s)
GEN_BINARY_TEST(smax, 8h, 8h, 8h)
GEN_BINARY_TEST(smax, 4h, 4h, 4h)
GEN_BINARY_TEST(smax, 16b, 16b, 16b)
GEN_BINARY_TEST(smax, 8b, 8b, 8b)

GEN_BINARY_TEST(smin, 4s, 4s, 4s)
GEN_BINARY_TEST(smin, 2s, 2s, 2s)
GEN_BINARY_TEST(smin, 8h, 8h, 8h)
GEN_BINARY_TEST(smin, 4h, 4h, 4h)
GEN_BINARY_TEST(smin, 16b, 16b, 16b)
GEN_BINARY_TEST(smin, 8b, 8b, 8b)

GEN_BINARY_TEST(add, 2d, 2d, 2d)
GEN_BINARY_TEST(add, 4s, 4s, 4s)
GEN_BINARY_TEST(add, 2s, 2s, 2s)
GEN_BINARY_TEST(add, 8h, 8h, 8h)
GEN_BINARY_TEST(add, 4h, 4h, 4h)
GEN_BINARY_TEST(add, 16b, 16b, 16b)
GEN_BINARY_TEST(add, 8b, 8b, 8b)

GEN_BINARY_TEST(sub, 2d, 2d, 2d)
GEN_BINARY_TEST(sub, 4s, 4s, 4s)
GEN_BINARY_TEST(sub, 2s, 2s, 2s)
GEN_BINARY_TEST(sub, 8h, 8h, 8h)
GEN_BINARY_TEST(sub, 4h, 4h, 4h)
GEN_BINARY_TEST(sub, 16b, 16b, 16b)
GEN_BINARY_TEST(sub, 8b, 8b, 8b)

GEN_BINARY_TEST(mul, 4s, 4s, 4s)
GEN_BINARY_TEST(mul, 2s, 2s, 2s)
GEN_BINARY_TEST(mul, 8h, 8h, 8h)
GEN_BINARY_TEST(mul, 4h, 4h, 4h)
GEN_BINARY_TEST(mul, 16b, 16b, 16b)
GEN_BINARY_TEST(mul, 8b, 8b, 8b)

GEN_BINARY_TEST(mla, 4s, 4s, 4s)
GEN_BINARY_TEST(mla, 2s, 2s, 2s)
GEN_BINARY_TEST(mla, 8h, 8h, 8h)
GEN_BINARY_TEST(mla, 4h, 4h, 4h)
GEN_BINARY_TEST(mla, 16b, 16b, 16b)
GEN_BINARY_TEST(mla, 8b, 8b, 8b)

GEN_BINARY_TEST(mls, 4s, 4s, 4s)
GEN_BINARY_TEST(mls, 2s, 2s, 2s)
GEN_BINARY_TEST(mls, 8h, 8h, 8h)
GEN_BINARY_TEST(mls, 4h, 4h, 4h)
GEN_BINARY_TEST(mls, 16b, 16b, 16b)
GEN_BINARY_TEST(mls, 8b, 8b, 8b)

GEN_BINARY_TEST(and, 16b, 16b, 16b)
GEN_BINARY_TEST(and, 8b, 8b, 8b)

GEN_BINARY_TEST(bic, 16b, 16b, 16b)
GEN_BINARY_TEST(bic, 8b, 8b, 8b)

GEN_BINARY_TEST(orr, 16b, 16b, 16b)
GEN_BINARY_TEST(orr, 8b, 8b, 8b)

GEN_BINARY_TEST(orn, 16b, 16b, 16b)
GEN_BINARY_TEST(orn, 8b, 8b, 8b)

GEN_BINARY_TEST(eor, 16b, 16b, 16b)
GEN_BINARY_TEST(eor, 8b, 8b, 8b)

GEN_BINARY_TEST(bsl, 16b, 16b, 16b)
GEN_BINARY_TEST(bsl, 8b, 8b, 8b)

GEN_BINARY_TEST(bit, 16b, 16b, 16b)
GEN_BINARY_TEST(bit, 8b, 8b, 8b)

GEN_BINARY_TEST(bif, 16b, 16b, 16b)
GEN_BINARY_TEST(bif, 8b, 8b, 8b)

GEN_BINARY_TEST(cmeq, 2d, 2d, 2d)
GEN_BINARY_TEST(cmeq, 4s, 4s, 4s)
GEN_BINARY_TEST(cmeq, 2s, 2s, 2s)
GEN_BINARY_TEST(cmeq, 8h, 8h, 8h)
GEN_BINARY_TEST(cmeq, 4h, 4h, 4h)
GEN_BINARY_TEST(cmeq, 16b, 16b, 16b)
GEN_BINARY_TEST(cmeq, 8b, 8b, 8b)

GEN_BINARY_TEST(cmtst, 2d, 2d, 2d)
GEN_BINARY_TEST(cmtst, 4s, 4s, 4s)
GEN_BINARY_TEST(cmtst, 2s, 2s, 2s)
GEN_BINARY_TEST(cmtst, 8h, 8h, 8h)
GEN_BINARY_TEST(cmtst, 4h, 4h, 4h)
GEN_BINARY_TEST(cmtst, 16b, 16b, 16b)
GEN_BINARY_TEST(cmtst, 8b, 8b, 8b)

GEN_BINARY_TEST(cmhi, 2d, 2d, 2d)
GEN_BINARY_TEST(cmhi, 4s, 4s, 4s)
GEN_BINARY_TEST(cmhi, 2s, 2s, 2s)
GEN_BINARY_TEST(cmhi, 8h, 8h, 8h)
GEN_BINARY_TEST(cmhi, 4h, 4h, 4h)
GEN_BINARY_TEST(cmhi, 16b, 16b, 16b)
GEN_BINARY_TEST(cmhi, 8b, 8b, 8b)

GEN_BINARY_TEST(cmgt, 2d, 2d, 2d)
GEN_BINARY_TEST(cmgt, 4s, 4s, 4s)
GEN_BINARY_TEST(cmgt, 2s, 2s, 2s)
GEN_BINARY_TEST(cmgt, 8h, 8h, 8h)
GEN_BINARY_TEST(cmgt, 4h, 4h, 4h)
GEN_BINARY_TEST(cmgt, 16b, 16b, 16b)
GEN_BINARY_TEST(cmgt, 8b, 8b, 8b)

GEN_BINARY_TEST(cmhs, 2d, 2d, 2d)
GEN_BINARY_TEST(cmhs, 4s, 4s, 4s)
GEN_BINARY_TEST(cmhs, 2s, 2s, 2s)
GEN_BINARY_TEST(cmhs, 8h, 8h, 8h)
GEN_BINARY_TEST(cmhs, 4h, 4h, 4h)
GEN_BINARY_TEST(cmhs, 16b, 16b, 16b)
GEN_BINARY_TEST(cmhs, 8b, 8b, 8b)

GEN_BINARY_TEST(cmge, 2d, 2d, 2d)
GEN_BINARY_TEST(cmge, 4s, 4s, 4s)
GEN_BINARY_TEST(cmge, 2s, 2s, 2s)
GEN_BINARY_TEST(cmge, 8h, 8h, 8h)
GEN_BINARY_TEST(cmge, 4h, 4h, 4h)
GEN_BINARY_TEST(cmge, 16b, 16b, 16b)
GEN_BINARY_TEST(cmge, 8b, 8b, 8b)

GEN_SHIFT_TEST(ushr, 2d, 2d, 1)
GEN_SHIFT_TEST(ushr, 2d, 2d, 13)
GEN_SHIFT_TEST(ushr, 2d, 2d, 64)
GEN_SHIFT_TEST(sshr, 2d, 2d, 1)
GEN_SHIFT_TEST(sshr, 2d, 2d, 13)
GEN_SHIFT_TEST(sshr, 2d, 2d, 64)
GEN_SHIFT_TEST(shl,  2d, 2d, 0)
GEN_SHIFT_TEST(shl,  2d, 2d, 13)
GEN_SHIFT_TEST(shl,  2d, 2d, 63)

GEN_SHIFT_TEST(ushr, 4s, 4s, 1)
GEN_SHIFT_TEST(ushr, 4s, 4s, 13)
GEN_SHIFT_TEST(ushr, 4s, 4s, 32)
GEN_SHIFT_TEST(sshr, 4s, 4s, 1)
GEN_SHIFT_TEST(sshr, 4s, 4s, 13)
GEN_SHIFT_TEST(sshr, 4s, 4s, 32)
GEN_SHIFT_TEST(shl,  4s, 4s, 0)
GEN_SHIFT_TEST(shl,  4s, 4s, 13)
GEN_SHIFT_TEST(shl,  4s, 4s, 31)

GEN_SHIFT_TEST(ushr, 2s, 2s, 1)
GEN_SHIFT_TEST(ushr, 2s, 2s, 13)
GEN_SHIFT_TEST(ushr, 2s, 2s, 32)
GEN_SHIFT_TEST(sshr, 2s, 2s, 1)
GEN_SHIFT_TEST(sshr, 2s, 2s, 13)
GEN_SHIFT_TEST(sshr, 2s, 2s, 32)
GEN_SHIFT_TEST(shl,  2s, 2s, 0)
GEN_SHIFT_TEST(shl,  2s, 2s, 13)
GEN_SHIFT_TEST(shl,  2s, 2s, 31)

GEN_SHIFT_TEST(ushr, 8h, 8h, 1)
GEN_SHIFT_TEST(ushr, 8h, 8h, 13)
GEN_SHIFT_TEST(ushr, 8h, 8h, 16)
GEN_SHIFT_TEST(sshr, 8h, 8h, 1)
GEN_SHIFT_TEST(sshr, 8h, 8h, 13)
GEN_SHIFT_TEST(sshr, 8h, 8h, 16)
GEN_SHIFT_TEST(shl,  8h, 8h, 0)
GEN_SHIFT_TEST(shl,  8h, 8h, 13)
GEN_SHIFT_TEST(shl,  8h, 8h, 15)

GEN_SHIFT_TEST(ushr, 4h, 4h, 1)
GEN_SHIFT_TEST(ushr, 4h, 4h, 13)
GEN_SHIFT_TEST(ushr, 4h, 4h, 16)
GEN_SHIFT_TEST(sshr, 4h, 4h, 1)
GEN_SHIFT_TEST(sshr, 4h, 4h, 13)
GEN_SHIFT_TEST(sshr, 4h, 4h, 16)
GEN_SHIFT_TEST(shl,  4h, 4h, 0)
GEN_SHIFT_TEST(shl,  4h, 4h, 13)
GEN_SHIFT_TEST(shl,  4h, 4h, 15)

GEN_SHIFT_TEST(ushr, 16b, 16b, 1)
GEN_SHIFT_TEST(ushr, 16b, 16b, 8)
GEN_SHIFT_TEST(sshr, 16b, 16b, 1)
GEN_SHIFT_TEST(sshr, 16b, 16b, 8)
GEN_SHIFT_TEST(shl,  16b, 16b, 0)
GEN_SHIFT_TEST(shl,  16b, 16b, 7)

GEN_SHIFT_TEST(ushr, 8b, 8b, 1)
GEN_SHIFT_TEST(ushr, 8b, 8b, 8)
GEN_SHIFT_TEST(sshr, 8b, 8b, 1)
GEN_SHIFT_TEST(sshr, 8b, 8b, 8)
GEN_SHIFT_TEST(shl,  8b, 8b, 0)
GEN_SHIFT_TEST(shl,  8b, 8b, 7)

GEN_SHIFT_TEST(ushll,  2d, 2s, 0)
GEN_SHIFT_TEST(ushll,  2d, 2s, 15)
GEN_SHIFT_TEST(ushll,  2d, 2s, 31)
GEN_SHIFT_TEST(ushll2, 2d, 4s, 0)
GEN_SHIFT_TEST(ushll2, 2d, 4s, 15)
GEN_SHIFT_TEST(ushll2, 2d, 4s, 31)
GEN_SHIFT_TEST(ushll,  4s, 4h,  0)
GEN_SHIFT_TEST(ushll,  4s, 4h,  7)
GEN_SHIFT_TEST(ushll,  4s, 4h,  15)
GEN_SHIFT_TEST(ushll2, 4s, 8h,  0)
GEN_SHIFT_TEST(ushll2, 4s, 8h,  7)
GEN_SHIFT_TEST(ushll2, 4s, 8h,  15)
GEN_SHIFT_TEST(ushll,  8h, 8b,  0)
GEN_SHIFT_TEST(ushll,  8h, 8b,  3)
GEN_SHIFT_TEST(ushll,  8h, 8b,  7)
GEN_SHIFT_TEST(ushll2, 8h, 16b, 0)
GEN_SHIFT_TEST(ushll2, 8h, 16b, 3)
GEN_SHIFT_TEST(ushll2, 8h, 16b, 7)

GEN_SHIFT_TEST(sshll,  2d, 2s,  0)
GEN_SHIFT_TEST(sshll,  2d, 2s,  15)
GEN_SHIFT_TEST(sshll,  2d, 2s,  31)
GEN_SHIFT_TEST(sshll2, 2d, 4s,  0)
GEN_SHIFT_TEST(sshll2, 2d, 4s,  15)
GEN_SHIFT_TEST(sshll2, 2d, 4s,  31)
GEN_SHIFT_TEST(sshll,  4s, 4h,  0)
GEN_SHIFT_TEST(sshll,  4s, 4h,  7)
GEN_SHIFT_TEST(sshll,  4s, 4h,  15)
GEN_SHIFT_TEST(sshll2, 4s, 8h,  0)
GEN_SHIFT_TEST(sshll2, 4s, 8h,  7)
GEN_SHIFT_TEST(sshll2, 4s, 8h,  15)
GEN_SHIFT_TEST(sshll,  8h, 8b,  0)
GEN_SHIFT_TEST(sshll,  8h, 8b,  3)
GEN_SHIFT_TEST(sshll,  8h, 8b,  7)
GEN_SHIFT_TEST(sshll2, 8h, 16b, 0)
GEN_SHIFT_TEST(sshll2, 8h, 16b, 3)
GEN_SHIFT_TEST(sshll2, 8h, 16b, 7)


GEN_UNARY_TEST(xtn,  2s, 2d)
GEN_UNARY_TEST(xtn2, 4s, 2d)
GEN_UNARY_TEST(xtn,  4h, 4s)
GEN_UNARY_TEST(xtn2, 8h, 4s)
GEN_UNARY_TEST(xtn,  8b, 8h)
GEN_UNARY_TEST(xtn2, 16b, 8h)

GEN_ONEINT_ONEVEC_TEST(umov_x_d0,  "umov x9, v10.d[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_x_d1,  "umov x9, v10.d[1]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_s0,  "umov w9, v10.s[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_s3,  "umov w9, v10.s[3]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_h0,  "umov w9, v10.h[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_h7,  "umov w9, v10.h[7]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_b0,  "umov w9, v10.b[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(umov_w_b15, "umov w9, v10.b[15]", 9, 10)

GEN_ONEINT_ONEVEC_TEST(smov_x_s0,  "smov x9, v10.s[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_x_s3,  "smov x9, v10.s[3]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_x_h0,  "smov x9, v10.h[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_x_h7,  "smov x9, v10.h[7]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_w_h0,  "smov w9, v10.h[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_w_h7,  "smov w9, v10.h[7]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_x_b0,  "smov x9, v10.b[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_x_b15, "smov x9, v10.b[15]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_w_b0,  "smov w9, v10.b[0]", 9, 10)
GEN_ONEINT_ONEVEC_TEST(smov_w_b15, "smov w9, v10.b[15]", 9, 10)

GEN_TWOVEC_TEST(fcvtn_2s_2d, "fcvtn  v22.2s, v23.2d", 22, 23)
GEN_TWOVEC_TEST(fcvtn_4s_2d, "fcvtn2 v22.4s, v23.2d", 22, 23)

GEN_UNARY_TEST(neg, 2d, 2d)
GEN_UNARY_TEST(neg, 4s, 4s)
GEN_UNARY_TEST(neg, 2s, 2s)
GEN_UNARY_TEST(neg, 8h, 8h)
GEN_UNARY_TEST(neg, 4h, 4h)
GEN_UNARY_TEST(neg, 16b, 16b)
GEN_UNARY_TEST(neg, 8b,  8b)

GEN_BINARY_TEST(fadd, 2d, 2d, 2d)
GEN_BINARY_TEST(fadd, 4s, 4s, 4s)
GEN_BINARY_TEST(fadd, 2s, 2s, 2s)
GEN_BINARY_TEST(fsub, 2d, 2d, 2d)
GEN_BINARY_TEST(fsub, 4s, 4s, 4s)
GEN_BINARY_TEST(fsub, 2s, 2s, 2s)
GEN_BINARY_TEST(fmul, 2d, 2d, 2d)
GEN_BINARY_TEST(fmul, 4s, 4s, 4s)
GEN_BINARY_TEST(fmul, 2s, 2s, 2s)
GEN_BINARY_TEST(fdiv, 2d, 2d, 2d)
GEN_BINARY_TEST(fdiv, 4s, 4s, 4s)
GEN_BINARY_TEST(fdiv, 2s, 2s, 2s)
GEN_BINARY_TEST(fmla, 2d, 2d, 2d)
GEN_BINARY_TEST(fmla, 4s, 4s, 4s)
GEN_BINARY_TEST(fmla, 2s, 2s, 2s)
GEN_BINARY_TEST(fmls, 2d, 2d, 2d)
GEN_BINARY_TEST(fmls, 4s, 4s, 4s)
GEN_BINARY_TEST(fmls, 2s, 2s, 2s)
GEN_BINARY_TEST(fabd, 2d, 2d, 2d)
GEN_BINARY_TEST(fabd, 4s, 4s, 4s)
GEN_BINARY_TEST(fabd, 2s, 2s, 2s)

GEN_THREEVEC_TEST(add_d_d_d, "add d21, d22, d23", 21, 22, 23)
GEN_THREEVEC_TEST(sub_d_d_d, "sub d21, d22, d23", 21, 22, 23)

/* overkill -- don't need two vecs, only one */
GEN_TWOVEC_TEST(fmov_d_imm_01, "fmov d22, #0.125", 22, 23)
GEN_TWOVEC_TEST(fmov_d_imm_02, "fmov d22, #-4.0",  22, 23)
GEN_TWOVEC_TEST(fmov_d_imm_03, "fmov d22, #1.0",   22, 23)
GEN_TWOVEC_TEST(fmov_s_imm_01, "fmov s22, #0.125", 22, 23)
GEN_TWOVEC_TEST(fmov_s_imm_02, "fmov s22, #-4.0",  22, 23)
GEN_TWOVEC_TEST(fmov_s_imm_03, "fmov s22, #-1.0",   22, 23)

GEN_ONEINT_ONEVEC_TEST(fmov_s_w,  "fmov s7,      w15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(fmov_d_x,  "fmov d7,      x15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(fmov_d1_x, "fmov v7.d[1], x15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(fmov_w_s,  "fmov w15,      s7", 15, 7)
GEN_ONEINT_ONEVEC_TEST(fmov_x_d,  "fmov x15,      d7", 15, 7)
GEN_ONEINT_ONEVEC_TEST(fmov_x_d1, "fmov x15, v7.d[1]", 15, 7)

GEN_TWOVEC_TEST(fmov_2d_imm_01, "fmov v22.2d, #0.125", 22, 23)
GEN_TWOVEC_TEST(fmov_2d_imm_02, "fmov v22.2d, #-4.0",  22, 23)
GEN_TWOVEC_TEST(fmov_2d_imm_03, "fmov v22.2d, #1.0",   22, 23)
GEN_TWOVEC_TEST(fmov_4s_imm_01, "fmov v22.4s, #0.125", 22, 23)
GEN_TWOVEC_TEST(fmov_4s_imm_02, "fmov v22.4s, #-4.0",  22, 23)
GEN_TWOVEC_TEST(fmov_4s_imm_03, "fmov v22.4s, #1.0",   22, 23)
GEN_TWOVEC_TEST(fmov_2s_imm_01, "fmov v22.2s, #0.125", 22, 23)
GEN_TWOVEC_TEST(fmov_2s_imm_02, "fmov v22.2s, #-4.0",  22, 23)
GEN_TWOVEC_TEST(fmov_2s_imm_03, "fmov v22.2s, #1.0",   22, 23)

GEN_ONEINT_ONEVEC_TEST(scvtf_s_w, "scvtf s7, w15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(scvtf_d_w, "scvtf d7, w15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(scvtf_s_x, "scvtf s7, x15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(scvtf_d_x, "scvtf d7, x15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(ucvtf_s_w, "ucvtf s7, w15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(ucvtf_d_w, "ucvtf d7, w15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(ucvtf_s_x, "ucvtf s7, x15", 15, 7)
GEN_ONEINT_ONEVEC_TEST(ucvtf_d_x, "ucvtf d7, x15", 15, 7)

GEN_THREEVEC_TEST(fadd_d_d_d,  "fadd d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fadd_s_s_s,  "fadd s2, s11, s29", 2, 11, 29)
GEN_THREEVEC_TEST(fsub_d_d_d,  "fsub d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fsub_s_s_s,  "fsub s2, s11, s29", 2, 11, 29)
GEN_THREEVEC_TEST(fmul_d_d_d,  "fmul d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fmul_s_s_s,  "fmul s2, s11, s29", 2, 11, 29)
GEN_THREEVEC_TEST(fdiv_d_d_d,  "fdiv d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fdiv_s_s_s,  "fdiv s2, s11, s29", 2, 11, 29)
GEN_THREEVEC_TEST(fnmul_d_d_d, "fnmul d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fnmul_s_s_s, "fnmul s2, s11, s29", 2, 11, 29)

GEN_THREEVEC_TEST(fabd_d_d_d,  "fabd d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(fabd_s_s_s,  "fabd s2, s11, s29", 2, 11, 29)

GEN_TWOVEC_TEST(fmov_d_d,  "fmov d22, d23",   22, 23)
GEN_TWOVEC_TEST(fmov_s_s,  "fmov s22, s23",   22, 23)
GEN_TWOVEC_TEST(fabs_d_d,  "fabs d22, d23",   22, 23)
GEN_TWOVEC_TEST(fabs_s_s,  "fabs s22, s23",   22, 23)
GEN_TWOVEC_TEST(fneg_d_d,  "fneg d22, d23",   22, 23)
GEN_TWOVEC_TEST(fneg_s_s,  "fneg s22, s23",   22, 23)
GEN_TWOVEC_TEST(fsqrt_d_d, "fsqrt d22, d23",   22, 23)
GEN_TWOVEC_TEST(fsqrt_s_s, "fsqrt s22, s23",   22, 23)

GEN_UNARY_TEST(fneg, 2d, 2d)
GEN_UNARY_TEST(fneg, 4s, 4s)
GEN_UNARY_TEST(fneg, 2s, 2s)
GEN_UNARY_TEST(fabs, 2d, 2d)
GEN_UNARY_TEST(fabs, 4s, 4s)
GEN_UNARY_TEST(fabs, 2s, 2s)

GEN_BINARY_TEST(fcmeq, 2d, 2d, 2d)
GEN_BINARY_TEST(fcmeq, 4s, 4s, 4s)
GEN_BINARY_TEST(fcmeq, 2s, 2s, 2s)
GEN_BINARY_TEST(fcmge, 2d, 2d, 2d)
GEN_BINARY_TEST(fcmge, 4s, 4s, 4s)
GEN_BINARY_TEST(fcmge, 2s, 2s, 2s)
GEN_BINARY_TEST(fcmgt, 2d, 2d, 2d)
GEN_BINARY_TEST(fcmgt, 4s, 4s, 4s)
GEN_BINARY_TEST(fcmgt, 2s, 2s, 2s)
GEN_BINARY_TEST(facge, 2d, 2d, 2d)
GEN_BINARY_TEST(facge, 4s, 4s, 4s)
GEN_BINARY_TEST(facge, 2s, 2s, 2s)
GEN_BINARY_TEST(facgt, 2d, 2d, 2d)
GEN_BINARY_TEST(facgt, 4s, 4s, 4s)
GEN_BINARY_TEST(facgt, 2s, 2s, 2s)

// Uses v15 as the first table entry
GEN_THREEVEC_TEST(
   tbl_16b_1reg, "tbl v21.16b, {v15.16b}, v23.16b", 21, 15, 23)
// and v15 ^ v21 as the second table entry
GEN_THREEVEC_TEST(
   tbl_16b_2reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "tbl v21.16b, {v15.16b, v16.16b}, v23.16b", 21, 15, 23)
// and v15 ^ v23 as the third table entry
GEN_THREEVEC_TEST(
   tbl_16b_3reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "eor v17.16b, v15.16b, v23.16b ; "
                 "tbl v21.16b, {v15.16b, v16.16b, v17.16b}, v23.16b",
                 21, 15, 23)
// and v21 ^ v23 as the fourth table entry
GEN_THREEVEC_TEST(
   tbl_16b_4reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "eor v17.16b, v15.16b, v23.16b ; "
                 "eor v18.16b, v21.16b, v23.16b ; "
                 "tbl v21.16b, {v15.16b, v16.16b, v17.16b, v18.16b}, v23.16b",
                 21, 15, 23)

// Same register scheme for tbl .8b, tbx .16b, tbx.8b
GEN_THREEVEC_TEST(
   tbl_8b_1reg, "tbl v21.8b, {v15.16b}, v23.8b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbl_8b_2reg, "eor v16.16b, v15.16b, v21.16b ; "
                "tbl v21.8b, {v15.16b, v16.16b}, v23.8b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbl_8b_3reg, "eor v16.16b, v15.16b, v21.16b ; "
                "eor v17.16b, v15.16b, v23.16b ; "
                "tbl v21.8b, {v15.16b, v16.16b, v17.16b}, v23.8b",
                21, 15, 23)
GEN_THREEVEC_TEST(
   tbl_8b_4reg, "eor v16.16b, v15.16b, v21.16b ; "
                "eor v17.16b, v15.16b, v23.16b ; "
                "eor v18.16b, v21.16b, v23.16b ; "
                "tbl v21.8b, {v15.16b, v16.16b, v17.16b, v18.16b}, v23.8b",
                21, 15, 23)

GEN_THREEVEC_TEST(
   tbx_16b_1reg, "tbx v21.16b, {v15.16b}, v23.16b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_16b_2reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "tbx v21.16b, {v15.16b, v16.16b}, v23.16b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_16b_3reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "eor v17.16b, v15.16b, v23.16b ; "
                 "tbx v21.16b, {v15.16b, v16.16b, v17.16b}, v23.16b",
                 21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_16b_4reg, "eor v16.16b, v15.16b, v21.16b ; "
                 "eor v17.16b, v15.16b, v23.16b ; "
                 "eor v18.16b, v21.16b, v23.16b ; "
                 "tbx v21.16b, {v15.16b, v16.16b, v17.16b, v18.16b}, v23.16b",
                 21, 15, 23)

// Same register scheme for tbx .8b, tbx .16b, tbx.8b
GEN_THREEVEC_TEST(
   tbx_8b_1reg, "tbx v21.8b, {v15.16b}, v23.8b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_8b_2reg, "eor v16.16b, v15.16b, v21.16b ; "
                "tbx v21.8b, {v15.16b, v16.16b}, v23.8b", 21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_8b_3reg, "eor v16.16b, v15.16b, v21.16b ; "
                "eor v17.16b, v15.16b, v23.16b ; "
                "tbx v21.8b, {v15.16b, v16.16b, v17.16b}, v23.8b",
                21, 15, 23)
GEN_THREEVEC_TEST(
   tbx_8b_4reg, "eor v16.16b, v15.16b, v21.16b ; "
                "eor v17.16b, v15.16b, v23.16b ; "
                "eor v18.16b, v21.16b, v23.16b ; "
                "tbx v21.8b, {v15.16b, v16.16b, v17.16b, v18.16b}, v23.8b",
                21, 15, 23)

GEN_TWOVEC_TEST(cmge_zero_2d_2d,   "cmge v5.2d,  v22.2d,  #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_4s_4s,   "cmge v5.4s,  v22.4s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_2s_2s,   "cmge v5.2s,  v22.2s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_8h_8h,   "cmge v5.8h,  v22.8h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_4h_4h,   "cmge v5.4h,  v22.4h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_16b_16b, "cmge v5.16b, v22.16b, #0", 5, 22)
GEN_TWOVEC_TEST(cmge_zero_8b_8b,   "cmge v5.8b,  v22.8b,  #0", 5, 22)

GEN_TWOVEC_TEST(cmgt_zero_2d_2d,   "cmgt v5.2d,  v22.2d,  #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_4s_4s,   "cmgt v5.4s,  v22.4s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_2s_2s,   "cmgt v5.2s,  v22.2s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_8h_8h,   "cmgt v5.8h,  v22.8h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_4h_4h,   "cmgt v5.4h,  v22.4h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_16b_16b, "cmgt v5.16b, v22.16b, #0", 5, 22)
GEN_TWOVEC_TEST(cmgt_zero_8b_8b,   "cmgt v5.8b,  v22.8b,  #0", 5, 22)

GEN_TWOVEC_TEST(cmle_zero_2d_2d,   "cmle v5.2d,  v22.2d,  #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_4s_4s,   "cmle v5.4s,  v22.4s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_2s_2s,   "cmle v5.2s,  v22.2s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_8h_8h,   "cmle v5.8h,  v22.8h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_4h_4h,   "cmle v5.4h,  v22.4h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_16b_16b, "cmle v5.16b, v22.16b, #0", 5, 22)
GEN_TWOVEC_TEST(cmle_zero_8b_8b,   "cmle v5.8b,  v22.8b,  #0", 5, 22)

GEN_TWOVEC_TEST(cmeq_zero_2d_2d,   "cmeq v5.2d,  v22.2d,  #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_4s_4s,   "cmeq v5.4s,  v22.4s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_2s_2s,   "cmeq v5.2s,  v22.2s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_8h_8h,   "cmeq v5.8h,  v22.8h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_4h_4h,   "cmeq v5.4h,  v22.4h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_16b_16b, "cmeq v5.16b, v22.16b, #0", 5, 22)
GEN_TWOVEC_TEST(cmeq_zero_8b_8b,   "cmeq v5.8b,  v22.8b,  #0", 5, 22)

GEN_TWOVEC_TEST(cmlt_zero_2d_2d,   "cmlt v5.2d,  v22.2d,  #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_4s_4s,   "cmlt v5.4s,  v22.4s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_2s_2s,   "cmlt v5.2s,  v22.2s,  #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_8h_8h,   "cmlt v5.8h,  v22.8h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_4h_4h,   "cmlt v5.4h,  v22.4h,  #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_16b_16b, "cmlt v5.16b, v22.16b, #0", 5, 22)
GEN_TWOVEC_TEST(cmlt_zero_8b_8b,   "cmlt v5.8b,  v22.8b,  #0", 5, 22)

GEN_TWOVEC_TEST(abs_d_d,  "abs d22, d23",   22, 23)
GEN_TWOVEC_TEST(neg_d_d,  "neg d22, d23",   22, 23)

GEN_UNARY_TEST(abs, 2d, 2d)
GEN_UNARY_TEST(abs, 4s, 4s)
GEN_UNARY_TEST(abs, 2s, 2s)
GEN_UNARY_TEST(abs, 8h, 8h)
GEN_UNARY_TEST(abs, 4h, 4h)
GEN_UNARY_TEST(abs, 16b, 16b)
GEN_UNARY_TEST(abs, 8b, 8b)

GEN_BINARY_TEST(addhn,   2s, 2d, 2d)
GEN_BINARY_TEST(addhn2,  4s, 2d, 2d)
GEN_BINARY_TEST(addhn,   4h, 4s, 4s)
GEN_BINARY_TEST(addhn2,  8h, 4s, 4s)
GEN_BINARY_TEST(addhn,   8b, 8h, 8h)
GEN_BINARY_TEST(addhn2,  16b, 8h, 8h)
GEN_BINARY_TEST(subhn,   2s, 2d, 2d)
GEN_BINARY_TEST(subhn2,  4s, 2d, 2d)
GEN_BINARY_TEST(subhn,   4h, 4s, 4s)
GEN_BINARY_TEST(subhn2,  8h, 4s, 4s)
GEN_BINARY_TEST(subhn,   8b, 8h, 8h)
GEN_BINARY_TEST(subhn2,  16b, 8h, 8h)
GEN_BINARY_TEST(raddhn,  2s, 2d, 2d)
GEN_BINARY_TEST(raddhn2, 4s, 2d, 2d)
GEN_BINARY_TEST(raddhn,  4h, 4s, 4s)
GEN_BINARY_TEST(raddhn2, 8h, 4s, 4s)
GEN_BINARY_TEST(raddhn,  8b, 8h, 8h)
GEN_BINARY_TEST(raddhn2, 16b, 8h, 8h)
GEN_BINARY_TEST(rsubhn,  2s, 2d, 2d)
GEN_BINARY_TEST(rsubhn2, 4s, 2d, 2d)
GEN_BINARY_TEST(rsubhn,  4h, 4s, 4s)
GEN_BINARY_TEST(rsubhn2, 8h, 4s, 4s)
GEN_BINARY_TEST(rsubhn,  8b, 8h, 8h)
GEN_BINARY_TEST(rsubhn2, 16b, 8h, 8h)

GEN_TWOVEC_TEST(addp_d_2d,  "addp d22, v23.2d",   22, 23)

GEN_BINARY_TEST(addp, 2d, 2d, 2d)
GEN_BINARY_TEST(addp, 4s, 4s, 4s)
GEN_BINARY_TEST(addp, 2s, 2s, 2s)
GEN_BINARY_TEST(addp, 8h, 8h, 8h)
GEN_BINARY_TEST(addp, 4h, 4h, 4h)
GEN_BINARY_TEST(addp, 16b, 16b, 16b)
GEN_BINARY_TEST(addp, 8b, 8b, 8b)

GEN_TWOVEC_TEST(addv_s_4s,  "addv s22, v23.4s",  22, 23)
GEN_TWOVEC_TEST(addv_h_8h,  "addv h22, v23.8h",  22, 23)
GEN_TWOVEC_TEST(addv_h_4h,  "addv h22, v23.4h",  22, 23)
GEN_TWOVEC_TEST(addv_b_16b, "addv b22, v23.16b", 22, 23)
GEN_TWOVEC_TEST(addv_b_8b,  "addv b22, v23.8b",  22, 23)

/* overkill -- don't need two vecs, only one */
GEN_TWOVEC_TEST(orr_8h_0x5A_lsl0, "orr v22.8h, #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(orr_8h_0xA5_lsl8, "orr v22.8h, #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(orr_4h_0x5A_lsl0, "orr v22.4h, #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(orr_4h_0xA5_lsl8, "orr v22.4h, #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(orr_4s_0x5A_lsl0,  "orr v22.4s, #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(orr_4s_0x6B_lsl8,  "orr v22.4s, #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(orr_4s_0x49_lsl16, "orr v22.4s, #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(orr_4s_0x3D_lsl24, "orr v22.4s, #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(orr_2s_0x5A_lsl0,  "orr v22.2s, #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(orr_2s_0x6B_lsl8,  "orr v22.2s, #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(orr_2s_0x49_lsl16, "orr v22.2s, #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(orr_2s_0x3D_lsl24, "orr v22.2s, #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(bic_8h_0x5A_lsl0, "bic v22.8h, #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(bic_8h_0xA5_lsl8, "bic v22.8h, #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(bic_4h_0x5A_lsl0, "bic v22.4h, #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(bic_4h_0xA5_lsl8, "bic v22.4h, #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(bic_4s_0x5A_lsl0,  "bic v22.4s, #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(bic_4s_0x6B_lsl8,  "bic v22.4s, #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(bic_4s_0x49_lsl16, "bic v22.4s, #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(bic_4s_0x3D_lsl24, "bic v22.4s, #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(bic_2s_0x5A_lsl0,  "bic v22.2s, #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(bic_2s_0x6B_lsl8,  "bic v22.2s, #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(bic_2s_0x49_lsl16, "bic v22.2s, #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(bic_2s_0x3D_lsl24, "bic v22.2s, #0x3D, LSL #24", 22, 23)

GEN_UNARY_TEST(cls, 4s, 4s)
GEN_UNARY_TEST(cls, 2s, 2s)
GEN_UNARY_TEST(cls, 8h, 8h)
GEN_UNARY_TEST(cls, 4h, 4h)
GEN_UNARY_TEST(cls, 16b, 16b)
GEN_UNARY_TEST(cls, 8b, 8b)

GEN_UNARY_TEST(clz, 4s, 4s)
GEN_UNARY_TEST(clz, 2s, 2s)
GEN_UNARY_TEST(clz, 8h, 8h)
GEN_UNARY_TEST(clz, 4h, 4h)
GEN_UNARY_TEST(clz, 16b, 16b)
GEN_UNARY_TEST(clz, 8b, 8b)

GEN_THREEVEC_TEST(cmeq_d_d_d,  "cmeq  d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(cmge_d_d_d,  "cmge  d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(cmgt_d_d_d,  "cmgt  d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(cmhi_d_d_d,  "cmhi  d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(cmhs_d_d_d,  "cmhs  d2, d11, d29", 2, 11, 29)
GEN_THREEVEC_TEST(cmtst_d_d_d, "cmtst d2, d11, d29", 2, 11, 29)

GEN_TWOVEC_TEST(cmeq_zero_d_d,  "cmeq  d2, d11, #0", 2, 11)
GEN_TWOVEC_TEST(cmge_zero_d_d,  "cmge  d2, d11, #0", 2, 11)
GEN_TWOVEC_TEST(cmgt_zero_d_d,  "cmgt  d2, d11, #0", 2, 11)
GEN_TWOVEC_TEST(cmle_zero_d_d,  "cmle  d2, d11, #0", 2, 11)
GEN_TWOVEC_TEST(cmlt_zero_d_d,  "cmlt  d2, d11, #0", 2, 11)

GEN_UNARY_TEST(cnt, 16b, 16b)
GEN_UNARY_TEST(cnt, 8b, 8b)

GEN_TWOVEC_TEST(dup_d_d0,  "dup d22, v23.d[0]", 22, 23)
GEN_TWOVEC_TEST(dup_d_d1,  "dup d22, v23.d[1]", 22, 23)
GEN_TWOVEC_TEST(dup_s_s0,  "dup s22, v23.s[0]", 22, 23)
GEN_TWOVEC_TEST(dup_s_s3,  "dup s22, v23.s[3]", 22, 23)
GEN_TWOVEC_TEST(dup_h_h0,  "dup h22, v23.h[0]", 22, 23)
GEN_TWOVEC_TEST(dup_h_h6,  "dup h22, v23.h[6]", 22, 23)
GEN_TWOVEC_TEST(dup_b_b0,  "dup b0,  v23.b[0]",  22, 23)
GEN_TWOVEC_TEST(dup_b_b13, "dup b13, v23.b[13]", 22, 23)

GEN_TWOVEC_TEST(dup_2d_d0,  "dup v9.2d, v17.d[0]", 9, 17)
GEN_TWOVEC_TEST(dup_2d_d1,  "dup v9.2d, v17.d[1]", 9, 17)
GEN_TWOVEC_TEST(dup_4s_s0,  "dup v9.4s, v17.s[0]", 9, 17)
GEN_TWOVEC_TEST(dup_4s_s3,  "dup v9.4s, v17.s[3]", 9, 17)
GEN_TWOVEC_TEST(dup_2s_s0,  "dup v9.2s, v17.s[0]", 9, 17)
GEN_TWOVEC_TEST(dup_2s_s2,  "dup v9.2s, v17.s[2]", 9, 17)
GEN_TWOVEC_TEST(dup_8h_h0,  "dup v9.8h, v17.h[0]", 9, 17)
GEN_TWOVEC_TEST(dup_8h_h6,  "dup v9.8h, v17.h[6]", 9, 17)
GEN_TWOVEC_TEST(dup_4h_h1,  "dup v9.4h, v17.h[1]", 9, 17)
GEN_TWOVEC_TEST(dup_4h_h5,  "dup v9.4h, v17.h[5]", 9, 17)
GEN_TWOVEC_TEST(dup_16b_b2,  "dup v9.16b, v17.b[2]", 9, 17)
GEN_TWOVEC_TEST(dup_16b_b12, "dup v9.16b, v17.b[12]", 9, 17)
GEN_TWOVEC_TEST(dup_8b_b3,  "dup v9.8b, v17.b[3]", 9, 17)
GEN_TWOVEC_TEST(dup_8b_b13, "dup v9.8b, v17.b[13]", 9, 17)

GEN_TWOVEC_TEST(dup_2d_x,  "mov x10, v17.d[0];  dup v9.2d,  x10", 9, 17)
GEN_TWOVEC_TEST(dup_4s_w,  "mov x10, v17.d[0];  dup v9.4s,  w10", 9, 17)
GEN_TWOVEC_TEST(dup_2s_w,  "mov x10, v17.d[0];  dup v9.2s,  w10", 9, 17)
GEN_TWOVEC_TEST(dup_8h_w,  "mov x10, v17.d[0];  dup v9.8h,  w10",  9, 17)
GEN_TWOVEC_TEST(dup_4h_w,  "mov x10, v17.d[0];  dup v9.4h,  w10",  9, 17)
GEN_TWOVEC_TEST(dup_16b_w, "mov x10, v17.d[0];  dup v9.16b, w10", 9, 17)
GEN_TWOVEC_TEST(dup_8b_w,  "mov x10, v17.d[0];  dup v9.8b,  w10",  9, 17)

GEN_THREEVEC_TEST(ext_16b_16b_16b_0x0,
                  "ext  v2.16b, v11.16b, v29.16b, #0", 2, 11, 29)
GEN_THREEVEC_TEST(ext_16b_16b_16b_0x7,
                  "ext  v2.16b, v11.16b, v29.16b, #7", 2, 11, 29)
GEN_THREEVEC_TEST(ext_16b_16b_16b_0x8,
                  "ext  v2.16b, v11.16b, v29.16b, #8", 2, 11, 29)
GEN_THREEVEC_TEST(ext_16b_16b_16b_0x9,
                  "ext  v2.16b, v11.16b, v29.16b, #9", 2, 11, 29)
GEN_THREEVEC_TEST(ext_16b_16b_16b_0xF,
                  "ext  v2.16b, v11.16b, v29.16b, #15", 2, 11, 29)

GEN_THREEVEC_TEST(ext_8b_8b_8b_0x0,
                  "ext  v2.8b, v11.8b, v29.8b, #0", 2, 11, 29)
GEN_THREEVEC_TEST(ext_8b_8b_8b_0x1,
                  "ext  v2.8b, v11.8b, v29.8b, #1", 2, 11, 29)
GEN_THREEVEC_TEST(ext_8b_8b_8b_0x6,
                  "ext  v2.8b, v11.8b, v29.8b, #6", 2, 11, 29)
GEN_THREEVEC_TEST(ext_8b_8b_8b_0x7,
                  "ext  v2.8b, v11.8b, v29.8b, #7", 2, 11, 29)


GEN_TWOVEC_TEST(ins_d0_d0, "ins v3.d[0], v24.d[0]", 3, 24)
GEN_TWOVEC_TEST(ins_d0_d1, "ins v3.d[0], v24.d[1]", 3, 24)
GEN_TWOVEC_TEST(ins_d1_d0, "ins v3.d[1], v24.d[0]", 3, 24)
GEN_TWOVEC_TEST(ins_d1_d1, "ins v3.d[1], v24.d[1]", 3, 24)

GEN_TWOVEC_TEST(ins_s0_s2, "ins v3.s[0], v24.s[2]", 3, 24)
GEN_TWOVEC_TEST(ins_s3_s0, "ins v3.s[3], v24.s[0]", 3, 24)
GEN_TWOVEC_TEST(ins_s2_s1, "ins v3.s[2], v24.s[1]", 3, 24)
GEN_TWOVEC_TEST(ins_s1_s3, "ins v3.s[1], v24.s[3]", 3, 24)

GEN_TWOVEC_TEST(ins_h0_h6, "ins v3.h[0], v24.h[6]", 3, 24)
GEN_TWOVEC_TEST(ins_h7_h0, "ins v3.h[7], v24.h[0]", 3, 24)
GEN_TWOVEC_TEST(ins_h6_h1, "ins v3.h[6], v24.h[1]", 3, 24)
GEN_TWOVEC_TEST(ins_h1_h7, "ins v3.h[1], v24.h[7]", 3, 24)

GEN_TWOVEC_TEST(ins_b0_b14, "ins v3.b[0],  v24.b[14]", 3, 24)
GEN_TWOVEC_TEST(ins_b15_b8, "ins v3.b[15], v24.b[8]",  3, 24)
GEN_TWOVEC_TEST(ins_b13_b9, "ins v3.b[13], v24.b[9]",  3, 24)
GEN_TWOVEC_TEST(ins_b5_b12, "ins v3.b[5],  v24.b[12]", 3, 24)

GEN_THREEVEC_TEST(mla_4s_4s_s0, "mla v2.4s, v11.4s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mla_4s_4s_s3, "mla v2.4s, v11.4s, v29.s[3]", 2, 11, 29)
GEN_THREEVEC_TEST(mla_2s_2s_s0, "mla v2.2s, v11.2s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mla_2s_2s_s3, "mla v2.2s, v11.2s, v29.s[3]", 2, 11, 29)
// For the 'h' version of these, Rm can only be <= 15 (!)
GEN_THREEVEC_TEST(mla_8h_8h_h1, "mla v2.8h, v11.8h, v2.h[1]", 2, 11, 9)
GEN_THREEVEC_TEST(mla_8h_8h_h5, "mla v2.8h, v11.8h, v2.h[5]", 2, 11, 9)
GEN_THREEVEC_TEST(mla_4h_4h_h2, "mla v2.4h, v11.4h, v2.h[2]", 2, 11, 9)
GEN_THREEVEC_TEST(mla_4h_4h_h7, "mla v2.4h, v11.4h, v2.h[7]", 2, 11, 9)

GEN_THREEVEC_TEST(mls_4s_4s_s0, "mls v2.4s, v11.4s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mls_4s_4s_s3, "mls v2.4s, v11.4s, v29.s[3]", 2, 11, 29)
GEN_THREEVEC_TEST(mls_2s_2s_s0, "mls v2.2s, v11.2s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mls_2s_2s_s3, "mls v2.2s, v11.2s, v29.s[3]", 2, 11, 29)
// For the 'h' version of these, Rm can only be <= 15 (!)
GEN_THREEVEC_TEST(mls_8h_8h_h1, "mls v2.8h, v11.8h, v2.h[1]", 2, 11, 9)
GEN_THREEVEC_TEST(mls_8h_8h_h5, "mls v2.8h, v11.8h, v2.h[5]", 2, 11, 9)
GEN_THREEVEC_TEST(mls_4h_4h_h2, "mls v2.4h, v11.4h, v2.h[2]", 2, 11, 9)
GEN_THREEVEC_TEST(mls_4h_4h_h7, "mls v2.4h, v11.4h, v2.h[7]", 2, 11, 9)

GEN_THREEVEC_TEST(mul_4s_4s_s0, "mul v2.4s, v11.4s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mul_4s_4s_s3, "mul v2.4s, v11.4s, v29.s[3]", 2, 11, 29)
GEN_THREEVEC_TEST(mul_2s_2s_s0, "mul v2.2s, v11.2s, v29.s[0]", 2, 11, 29)
GEN_THREEVEC_TEST(mul_2s_2s_s3, "mul v2.2s, v11.2s, v29.s[3]", 2, 11, 29)
// For the 'h' version of these, Rm can only be <= 15 (!)
GEN_THREEVEC_TEST(mul_8h_8h_h1, "mul v2.8h, v11.8h, v2.h[1]", 2, 11, 9)
GEN_THREEVEC_TEST(mul_8h_8h_h5, "mul v2.8h, v11.8h, v2.h[5]", 2, 11, 9)
GEN_THREEVEC_TEST(mul_4h_4h_h2, "mul v2.4h, v11.4h, v2.h[2]", 2, 11, 9)
GEN_THREEVEC_TEST(mul_4h_4h_h7, "mul v2.4h, v11.4h, v2.h[7]", 2, 11, 9)

/* overkill -- don't need two vecs, only one */
GEN_TWOVEC_TEST(movi_16b_0x9C_lsl0, "movi v22.16b, #0x9C, LSL #0", 22, 23)
GEN_TWOVEC_TEST(movi_8b_0x8B_lsl0,  "movi v22.8b,  #0x8B, LSL #0", 22, 23)
GEN_TWOVEC_TEST(movi_8h_0x5A_lsl0,  "movi v22.8h,  #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(movi_8h_0xA5_lsl8,  "movi v22.8h,  #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(movi_4h_0x5A_lsl0,  "movi v22.4h,  #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(movi_4h_0xA5_lsl8,  "movi v22.4h,  #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(movi_4s_0x5A_lsl0,  "movi v22.4s,  #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(movi_4s_0x6B_lsl8,  "movi v22.4s,  #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(movi_4s_0x49_lsl16, "movi v22.4s,  #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(movi_4s_0x3D_lsl24, "movi v22.4s,  #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(movi_2s_0x5A_lsl0,  "movi v22.2s,  #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(movi_2s_0x6B_lsl8,  "movi v22.2s,  #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(movi_2s_0x49_lsl16, "movi v22.2s,  #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(movi_2s_0x3D_lsl24, "movi v22.2s,  #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(movi_4s_0x6B_msl8,  "movi v22.4s,  #0x6B, MSL #8", 22, 23)
GEN_TWOVEC_TEST(movi_4s_0x94_msl16, "movi v22.4s,  #0x94, MSL #16", 22, 23)
GEN_TWOVEC_TEST(movi_2s_0x7A_msl8,  "movi v22.2s,  #0x7A, MSL #8", 22, 23)
GEN_TWOVEC_TEST(movi_2s_0xA5_msl16, "movi v22.2s,  #0xA5, MSL #16", 22, 23)

GEN_TWOVEC_TEST(movi_d_0xA5,  "movi d22,    #0xFF00FF0000FF00FF", 22, 23)
GEN_TWOVEC_TEST(movi_2d_0xB4, "movi v22.2d, #0xFF00FFFF00FF0000", 22, 23)

/* overkill -- don't need two vecs, only one */
GEN_TWOVEC_TEST(mvni_8h_0x5A_lsl0,  "mvni v22.8h,  #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(mvni_8h_0xA5_lsl8,  "mvni v22.8h,  #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(mvni_4h_0x5A_lsl0,  "mvni v22.4h,  #0x5A, LSL #0", 22, 23)
GEN_TWOVEC_TEST(mvni_4h_0xA5_lsl8,  "mvni v22.4h,  #0xA5, LSL #8", 22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x5A_lsl0,  "mvni v22.4s,  #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x6B_lsl8,  "mvni v22.4s,  #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x49_lsl16, "mvni v22.4s,  #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x3D_lsl24, "mvni v22.4s,  #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(mvni_2s_0x5A_lsl0,  "mvni v22.2s,  #0x5A, LSL #0",  22, 23)
GEN_TWOVEC_TEST(mvni_2s_0x6B_lsl8,  "mvni v22.2s,  #0x6B, LSL #8",  22, 23)
GEN_TWOVEC_TEST(mvni_2s_0x49_lsl16, "mvni v22.2s,  #0x49, LSL #16", 22, 23)
GEN_TWOVEC_TEST(mvni_2s_0x3D_lsl24, "mvni v22.2s,  #0x3D, LSL #24", 22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x6B_msl8,  "mvni v22.4s,  #0x6B, MSL #8", 22, 23)
GEN_TWOVEC_TEST(mvni_4s_0x94_msl16, "mvni v22.4s,  #0x94, MSL #16", 22, 23)
GEN_TWOVEC_TEST(mvni_2s_0x7A_msl8,  "mvni v22.2s,  #0x7A, MSL #8", 22, 23)
GEN_TWOVEC_TEST(mvni_2s_0xA5_msl16, "mvni v22.2s,  #0xA5, MSL #16", 22, 23)

GEN_UNARY_TEST(not, 16b, 16b)
GEN_UNARY_TEST(not, 8b,  8b)

GEN_BINARY_TEST(pmul, 16b, 16b, 16b)
GEN_BINARY_TEST(pmul, 8b, 8b, 8b)

GEN_BINARY_TEST(pmull,  8h, 8b,  8b)
GEN_BINARY_TEST(pmull2, 8h, 16b, 16b)
//GEN_BINARY_TEST(pmull,  1q, 1d,  1d)
//GEN_BINARY_TEST(pmull,  1q, 2d,  2d)

GEN_UNARY_TEST(rbit, 16b, 16b)
GEN_UNARY_TEST(rbit, 8b, 8b)
GEN_UNARY_TEST(rev16, 16b, 16b)
GEN_UNARY_TEST(rev16, 8b, 8b)
GEN_UNARY_TEST(rev32, 16b, 16b)
GEN_UNARY_TEST(rev32, 8b, 8b)
GEN_UNARY_TEST(rev32, 8h, 8h)
GEN_UNARY_TEST(rev32, 4h, 4h)
GEN_UNARY_TEST(rev64, 16b, 16b)
GEN_UNARY_TEST(rev64, 8b, 8b)
GEN_UNARY_TEST(rev64, 8h, 8h)
GEN_UNARY_TEST(rev64, 4h, 4h)
GEN_UNARY_TEST(rev64, 4s, 4s)
GEN_UNARY_TEST(rev64, 2s, 2s)

GEN_BINARY_TEST(saba, 4s, 4s, 4s)
GEN_BINARY_TEST(saba, 2s, 2s, 2s)
GEN_BINARY_TEST(saba, 8h, 8h, 8h)
GEN_BINARY_TEST(saba, 4h, 4h, 4h)
GEN_BINARY_TEST(saba, 16b, 16b, 16b)
GEN_BINARY_TEST(saba, 8b, 8b, 8b)

GEN_BINARY_TEST(uaba, 4s, 4s, 4s)
GEN_BINARY_TEST(uaba, 2s, 2s, 2s)
GEN_BINARY_TEST(uaba, 8h, 8h, 8h)
GEN_BINARY_TEST(uaba, 4h, 4h, 4h)
GEN_BINARY_TEST(uaba, 16b, 16b, 16b)
GEN_BINARY_TEST(uaba, 8b, 8b, 8b)

GEN_THREEVEC_TEST(sabal_2d_2s_2s,  "sabal  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(sabal2_2d_4s_4s, "sabal2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(sabal_4s_4h_4h,  "sabal  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(sabal2_4s_8h_8h, "sabal2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(sabal_8h_8b_8b,  "sabal  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(sabal2_8h_16b_16b, 
                                   "sabal2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(uabal_2d_2s_2s,  "uabal  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uabal2_2d_4s_4s, "uabal2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(uabal_4s_4h_4h,  "uabal  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uabal2_4s_8h_8h, "uabal2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(uabal_8h_8b_8b,  "uabal  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(uabal2_8h_16b_16b, 
                                   "uabal2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(sabd_4s_4s_4s,    "sabd v2.4s, v11.4s, v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(sabd_2s_2s_2s,    "sabd v2.2s, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(sabd_8h_8h_8h,    "sabd v2.8h, v11.8h, v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(sabd_4h_4h_4h,    "sabd v2.4h, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(sabd_16b_16b_16b, "sabd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(sabd_8b_8b_8b,    "sabd v2.8b, v11.8b, v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(uabd_4s_4s_4s,    "uabd v2.4s, v11.4s, v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(uabd_2s_2s_2s,    "uabd v2.2s, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uabd_8h_8h_8h,    "uabd v2.8h, v11.8h, v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(uabd_4h_4h_4h,    "uabd v2.4h, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uabd_16b_16b_16b, "uabd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(uabd_8b_8b_8b,    "uabd v2.8b, v11.8b, v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(sabdl_2d_2s_2s,  "sabdl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(sabdl2_2d_4s_4s, "sabdl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(sabdl_4s_4h_4h,  "sabdl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(sabdl2_4s_8h_8h, "sabdl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(sabdl_8h_8b_8b,  "sabdl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(sabdl2_8h_16b_16b, 
                                   "sabdl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(uabdl_2d_2s_2s,  "uabdl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uabdl2_2d_4s_4s, "uabdl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(uabdl_4s_4h_4h,  "uabdl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uabdl2_4s_8h_8h, "uabdl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(uabdl_8h_8b_8b,  "uabdl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(uabdl2_8h_16b_16b, 
                                   "uabdl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_TWOVEC_TEST(sadalp_4h_8b,  "sadalp v3.4h, v19.8b",  3, 19)
GEN_TWOVEC_TEST(sadalp_8h_16b, "sadalp v3.8h, v19.16b", 3, 19)
GEN_TWOVEC_TEST(sadalp_2s_4h,  "sadalp v3.2s, v19.4h",  3, 19)
GEN_TWOVEC_TEST(sadalp_4s_8h,  "sadalp v3.4s, v19.8h",  3, 19)
GEN_TWOVEC_TEST(sadalp_1d_2s,  "sadalp v3.1d, v19.2s",  3, 19)
GEN_TWOVEC_TEST(sadalp_2d_4s,  "sadalp v3.2d, v19.4s",  3, 19)

GEN_TWOVEC_TEST(uadalp_4h_8b,  "uadalp v3.4h, v19.8b",  3, 19)
GEN_TWOVEC_TEST(uadalp_8h_16b, "uadalp v3.8h, v19.16b", 3, 19)
GEN_TWOVEC_TEST(uadalp_2s_4h,  "uadalp v3.2s, v19.4h",  3, 19)
GEN_TWOVEC_TEST(uadalp_4s_8h,  "uadalp v3.4s, v19.8h",  3, 19)
GEN_TWOVEC_TEST(uadalp_1d_2s,  "uadalp v3.1d, v19.2s",  3, 19)
GEN_TWOVEC_TEST(uadalp_2d_4s,  "uadalp v3.2d, v19.4s",  3, 19)

GEN_THREEVEC_TEST(saddl_2d_2s_2s,  "saddl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(saddl2_2d_4s_4s, "saddl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(saddl_4s_4h_4h,  "saddl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(saddl2_4s_8h_8h, "saddl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(saddl_8h_8b_8b,  "saddl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(saddl2_8h_16b_16b, 
                                   "saddl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(uaddl_2d_2s_2s,  "uaddl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uaddl2_2d_4s_4s, "uaddl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(uaddl_4s_4h_4h,  "uaddl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uaddl2_4s_8h_8h, "uaddl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(uaddl_8h_8b_8b,  "uaddl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(uaddl2_8h_16b_16b, 
                                   "uaddl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(ssubl_2d_2s_2s,  "ssubl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(ssubl2_2d_4s_4s, "ssubl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(ssubl_4s_4h_4h,  "ssubl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(ssubl2_4s_8h_8h, "ssubl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(ssubl_8h_8b_8b,  "ssubl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(ssubl2_8h_16b_16b, 
                                   "ssubl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(usubl_2d_2s_2s,  "usubl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(usubl2_2d_4s_4s, "usubl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(usubl_4s_4h_4h,  "usubl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(usubl2_4s_8h_8h, "usubl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(usubl_8h_8b_8b,  "usubl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(usubl2_8h_16b_16b, 
                                   "usubl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_TWOVEC_TEST(saddlp_4h_8b,  "saddlp v3.4h, v19.8b",  3, 19)
GEN_TWOVEC_TEST(saddlp_8h_16b, "saddlp v3.8h, v19.16b", 3, 19)
GEN_TWOVEC_TEST(saddlp_2s_4h,  "saddlp v3.2s, v19.4h",  3, 19)
GEN_TWOVEC_TEST(saddlp_4s_8h,  "saddlp v3.4s, v19.8h",  3, 19)
GEN_TWOVEC_TEST(saddlp_1d_2s,  "saddlp v3.1d, v19.2s",  3, 19)
GEN_TWOVEC_TEST(saddlp_2d_4s,  "saddlp v3.2d, v19.4s",  3, 19)

GEN_TWOVEC_TEST(uaddlp_4h_8b,  "uaddlp v3.4h, v19.8b",  3, 19)
GEN_TWOVEC_TEST(uaddlp_8h_16b, "uaddlp v3.8h, v19.16b", 3, 19)
GEN_TWOVEC_TEST(uaddlp_2s_4h,  "uaddlp v3.2s, v19.4h",  3, 19)
GEN_TWOVEC_TEST(uaddlp_4s_8h,  "uaddlp v3.4s, v19.8h",  3, 19)
GEN_TWOVEC_TEST(uaddlp_1d_2s,  "uaddlp v3.1d, v19.2s",  3, 19)
GEN_TWOVEC_TEST(uaddlp_2d_4s,  "uaddlp v3.2d, v19.4s",  3, 19)

GEN_TWOVEC_TEST(saddlv_h_16b, "saddlv h3, v19.16b",  3, 19)
GEN_TWOVEC_TEST(saddlv_h_8b,  "saddlv h3, v19.8b",   3, 19)
GEN_TWOVEC_TEST(saddlv_s_8h,  "saddlv s3, v19.8h",   3, 19)
GEN_TWOVEC_TEST(saddlv_s_4h,  "saddlv s3, v19.4h",   3, 19)
GEN_TWOVEC_TEST(saddlv_d_4s,  "saddlv d3, v19.4s",   3, 19)

GEN_TWOVEC_TEST(uaddlv_h_16b, "uaddlv h3, v19.16b",  3, 19)
GEN_TWOVEC_TEST(uaddlv_h_8b,  "uaddlv h3, v19.8b",   3, 19)
GEN_TWOVEC_TEST(uaddlv_s_8h,  "uaddlv s3, v19.8h",   3, 19)
GEN_TWOVEC_TEST(uaddlv_s_4h,  "uaddlv s3, v19.4h",   3, 19)
GEN_TWOVEC_TEST(uaddlv_d_4s,  "uaddlv d3, v19.4s",   3, 19)

GEN_THREEVEC_TEST(saddw2_8h_8h_16b, "saddw2 v5.8h, v13.8h, v31.16b", 5, 13, 31)
GEN_THREEVEC_TEST(saddw_8h_8h_8b,   "saddw  v5.8h, v13.8h, v31.8b",  5, 13, 31)
GEN_THREEVEC_TEST(saddw2_4s_4s_8h,  "saddw2 v5.4s, v13.4s, v31.8h",  5, 13, 31)
GEN_THREEVEC_TEST(saddw_4s_4s_4h,   "saddw  v5.4s, v13.4s, v31.4h",  5, 13, 31)
GEN_THREEVEC_TEST(saddw2_2d_2d_4s,  "saddw2 v5.2d, v13.2d, v31.4s",  5, 13, 31)
GEN_THREEVEC_TEST(saddw_2d_2d_2s,   "saddw  v5.2d, v13.2d, v31.2s",  5, 13, 31)

GEN_THREEVEC_TEST(uaddw2_8h_8h_16b, "uaddw2 v5.8h, v13.8h, v31.16b", 5, 13, 31)
GEN_THREEVEC_TEST(uaddw_8h_8h_8b,   "uaddw  v5.8h, v13.8h, v31.8b",  5, 13, 31)
GEN_THREEVEC_TEST(uaddw2_4s_4s_8h,  "uaddw2 v5.4s, v13.4s, v31.8h",  5, 13, 31)
GEN_THREEVEC_TEST(uaddw_4s_4s_4h,   "uaddw  v5.4s, v13.4s, v31.4h",  5, 13, 31)
GEN_THREEVEC_TEST(uaddw2_2d_2d_4s,  "uaddw2 v5.2d, v13.2d, v31.4s",  5, 13, 31)
GEN_THREEVEC_TEST(uaddw_2d_2d_2s,   "uaddw  v5.2d, v13.2d, v31.2s",  5, 13, 31)

GEN_THREEVEC_TEST(ssubw2_8h_8h_16b, "ssubw2 v5.8h, v13.8h, v31.16b", 5, 13, 31)
GEN_THREEVEC_TEST(ssubw_8h_8h_8b,   "ssubw  v5.8h, v13.8h, v31.8b",  5, 13, 31)
GEN_THREEVEC_TEST(ssubw2_4s_4s_8h,  "ssubw2 v5.4s, v13.4s, v31.8h",  5, 13, 31)
GEN_THREEVEC_TEST(ssubw_4s_4s_4h,   "ssubw  v5.4s, v13.4s, v31.4h",  5, 13, 31)
GEN_THREEVEC_TEST(ssubw2_2d_2d_4s,  "ssubw2 v5.2d, v13.2d, v31.4s",  5, 13, 31)
GEN_THREEVEC_TEST(ssubw_2d_2d_2s,   "ssubw  v5.2d, v13.2d, v31.2s",  5, 13, 31)

GEN_THREEVEC_TEST(usubw2_8h_8h_16b, "usubw2 v5.8h, v13.8h, v31.16b", 5, 13, 31)
GEN_THREEVEC_TEST(usubw_8h_8h_8b,   "usubw  v5.8h, v13.8h, v31.8b",  5, 13, 31)
GEN_THREEVEC_TEST(usubw2_4s_4s_8h,  "usubw2 v5.4s, v13.4s, v31.8h",  5, 13, 31)
GEN_THREEVEC_TEST(usubw_4s_4s_4h,   "usubw  v5.4s, v13.4s, v31.4h",  5, 13, 31)
GEN_THREEVEC_TEST(usubw2_2d_2d_4s,  "usubw2 v5.2d, v13.2d, v31.4s",  5, 13, 31)
GEN_THREEVEC_TEST(usubw_2d_2d_2s,   "usubw  v5.2d, v13.2d, v31.2s",  5, 13, 31)

GEN_THREEVEC_TEST(shadd_4s_4s_4s,   "shadd v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(shadd_2s_2s_2s,   "shadd v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(shadd_8h_8h_8h,   "shadd v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(shadd_4h_4h_4h,   "shadd v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(shadd_16b_16b_16b,"shadd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(shadd_8b_8b_8b,   "shadd v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(uhadd_4s_4s_4s,   "uhadd v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(uhadd_2s_2s_2s,   "uhadd v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uhadd_8h_8h_8h,   "uhadd v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(uhadd_4h_4h_4h,   "uhadd v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uhadd_16b_16b_16b,"uhadd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(uhadd_8b_8b_8b,   "uhadd v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(shsub_4s_4s_4s,   "shsub v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(shsub_2s_2s_2s,   "shsub v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(shsub_8h_8h_8h,   "shsub v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(shsub_4h_4h_4h,   "shsub v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(shsub_16b_16b_16b,"shsub v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(shsub_8b_8b_8b,   "shsub v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(uhsub_4s_4s_4s,   "uhsub v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(uhsub_2s_2s_2s,   "uhsub v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(uhsub_8h_8h_8h,   "uhsub v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(uhsub_4h_4h_4h,   "uhsub v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(uhsub_16b_16b_16b,"uhsub v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(uhsub_8b_8b_8b,   "uhsub v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_TWOVEC_TEST(shll_8h_8b_8,   "shll  v3.8h, v24.8b,  #8", 3, 24)
GEN_TWOVEC_TEST(shll2_8h_16b_8, "shll2 v3.8h, v24.16b, #8", 3, 24)
GEN_TWOVEC_TEST(shll_4s_4h_16,  "shll  v3.4s, v24.4h, #16", 3, 24)
GEN_TWOVEC_TEST(shll2_4s_8h_16, "shll2 v3.4s, v24.8h, #16", 3, 24)
GEN_TWOVEC_TEST(shll_2d_2s_32,  "shll  v3.2d, v24.2s, #32", 3, 24)
GEN_TWOVEC_TEST(shll2_2d_4s_32, "shll2 v3.2d, v24.4s, #32", 3, 24)

GEN_TWOVEC_TEST(shrn_2s_2d_1,   "shrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(shrn_2s_2d_32,  "shrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(shrn2_4s_2d_1,  "shrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(shrn2_4s_2d_32, "shrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(shrn_4h_4s_1,   "shrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(shrn_4h_4s_16,  "shrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(shrn2_8h_4s_1,  "shrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(shrn2_8h_4s_16, "shrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(shrn_8b_8h_1,   "shrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(shrn_8b_8h_8,   "shrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(shrn2_16b_8h_1, "shrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(shrn2_16b_8h_8, "shrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(rshrn_2s_2d_1,   "rshrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn_2s_2d_32,  "rshrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(rshrn2_4s_2d_1,  "rshrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn2_4s_2d_32, "rshrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(rshrn_4h_4s_1,   "rshrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn_4h_4s_16,  "rshrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(rshrn2_8h_4s_1,  "rshrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn2_8h_4s_16, "rshrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(rshrn_8b_8h_1,   "rshrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn_8b_8h_8,   "rshrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(rshrn2_16b_8h_1, "rshrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(rshrn2_16b_8h_8, "rshrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(sli_d_d_0,  "sli d5, d28, #0",  5, 28)
GEN_TWOVEC_TEST(sli_d_d_32, "sli d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(sli_d_d_63, "sli d5, d28, #63", 5, 28)
GEN_TWOVEC_TEST(sri_d_d_1,  "sri d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(sri_d_d_33, "sri d5, d28, #33", 5, 28)
GEN_TWOVEC_TEST(sri_d_d_64, "sri d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(sli_2d_2d_0,   "sli v6.2d,  v27.2d, #0",  6, 27)
GEN_TWOVEC_TEST(sli_2d_2d_32,  "sli v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(sli_2d_2d_63,  "sli v6.2d,  v27.2d, #63", 6, 27)
GEN_TWOVEC_TEST(sli_4s_4s_0,   "sli v6.4s,  v27.4s, #0",  6, 27)
GEN_TWOVEC_TEST(sli_4s_4s_16,  "sli v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(sli_4s_4s_31,  "sli v6.4s,  v27.4s, #31", 6, 27)
GEN_TWOVEC_TEST(sli_2s_2s_0,   "sli v6.2s,  v27.2s, #0",  6, 27)
GEN_TWOVEC_TEST(sli_2s_2s_16,  "sli v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(sli_2s_2s_31,  "sli v6.2s,  v27.2s, #31", 6, 27)
GEN_TWOVEC_TEST(sli_8h_8h_0,   "sli v6.8h,  v27.8h, #0",  6, 27)
GEN_TWOVEC_TEST(sli_8h_8h_8,   "sli v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(sli_8h_8h_15,  "sli v6.8h,  v27.8h, #15", 6, 27)
GEN_TWOVEC_TEST(sli_4h_4h_0,   "sli v6.4h,  v27.4h, #0",  6, 27)
GEN_TWOVEC_TEST(sli_4h_4h_8,   "sli v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(sli_4h_4h_15,  "sli v6.4h,  v27.4h, #15", 6, 27)
GEN_TWOVEC_TEST(sli_16b_16b_0, "sli v6.16b, v27.16b, #0", 6, 27)
GEN_TWOVEC_TEST(sli_16b_16b_3, "sli v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(sli_16b_16b_7, "sli v6.16b, v27.16b, #7", 6, 27)
GEN_TWOVEC_TEST(sli_8b_8b_0,   "sli v6.8b,  v27.8b, #0",  6, 27)
GEN_TWOVEC_TEST(sli_8b_8b_3,   "sli v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(sli_8b_8b_7,   "sli v6.8b,  v27.8b, #7",  6, 27)

GEN_TWOVEC_TEST(sri_2d_2d_1,   "sri v6.2d,  v27.2d,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_2d_2d_33,  "sri v6.2d,  v27.2d,  #33", 6, 27)
GEN_TWOVEC_TEST(sri_2d_2d_64,  "sri v6.2d,  v27.2d,  #64", 6, 27)
GEN_TWOVEC_TEST(sri_4s_4s_1,   "sri v6.4s,  v27.4s,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_4s_4s_17,  "sri v6.4s,  v27.4s,  #17", 6, 27)
GEN_TWOVEC_TEST(sri_4s_4s_32,  "sri v6.4s,  v27.4s,  #32", 6, 27)
GEN_TWOVEC_TEST(sri_2s_2s_1,   "sri v6.2s,  v27.2s,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_2s_2s_17,  "sri v6.2s,  v27.2s,  #17", 6, 27)
GEN_TWOVEC_TEST(sri_2s_2s_32,  "sri v6.2s,  v27.2s,  #32", 6, 27)
GEN_TWOVEC_TEST(sri_8h_8h_1,   "sri v6.8h,  v27.8h,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_8h_8h_8,   "sri v6.8h,  v27.8h,  #8",  6, 27)
GEN_TWOVEC_TEST(sri_8h_8h_16,  "sri v6.8h,  v27.8h,  #16", 6, 27)
GEN_TWOVEC_TEST(sri_4h_4h_1,   "sri v6.4h,  v27.4h,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_4h_4h_8,   "sri v6.4h,  v27.4h,  #8",  6, 27)
GEN_TWOVEC_TEST(sri_4h_4h_16,  "sri v6.4h,  v27.4h,  #16", 6, 27)
GEN_TWOVEC_TEST(sri_16b_16b_1, "sri v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(sri_16b_16b_4, "sri v6.16b, v27.16b, #4", 6, 27)
GEN_TWOVEC_TEST(sri_16b_16b_8, "sri v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(sri_8b_8b_1,   "sri v6.8b,  v27.8b,  #1",  6, 27)
GEN_TWOVEC_TEST(sri_8b_8b_4,   "sri v6.8b,  v27.8b,  #4",  6, 27)
GEN_TWOVEC_TEST(sri_8b_8b_8,   "sri v6.8b,  v27.8b,  #8",  6, 27)

GEN_BINARY_TEST(smaxp, 4s, 4s, 4s)
GEN_BINARY_TEST(smaxp, 2s, 2s, 2s)
GEN_BINARY_TEST(smaxp, 8h, 8h, 8h)
GEN_BINARY_TEST(smaxp, 4h, 4h, 4h)
GEN_BINARY_TEST(smaxp, 16b, 16b, 16b)
GEN_BINARY_TEST(smaxp, 8b, 8b, 8b)

GEN_BINARY_TEST(umaxp, 4s, 4s, 4s)
GEN_BINARY_TEST(umaxp, 2s, 2s, 2s)
GEN_BINARY_TEST(umaxp, 8h, 8h, 8h)
GEN_BINARY_TEST(umaxp, 4h, 4h, 4h)
GEN_BINARY_TEST(umaxp, 16b, 16b, 16b)
GEN_BINARY_TEST(umaxp, 8b, 8b, 8b)

GEN_BINARY_TEST(sminp, 4s, 4s, 4s)
GEN_BINARY_TEST(sminp, 2s, 2s, 2s)
GEN_BINARY_TEST(sminp, 8h, 8h, 8h)
GEN_BINARY_TEST(sminp, 4h, 4h, 4h)
GEN_BINARY_TEST(sminp, 16b, 16b, 16b)
GEN_BINARY_TEST(sminp, 8b, 8b, 8b)

GEN_BINARY_TEST(uminp, 4s, 4s, 4s)
GEN_BINARY_TEST(uminp, 2s, 2s, 2s)
GEN_BINARY_TEST(uminp, 8h, 8h, 8h)
GEN_BINARY_TEST(uminp, 4h, 4h, 4h)
GEN_BINARY_TEST(uminp, 16b, 16b, 16b)
GEN_BINARY_TEST(uminp, 8b, 8b, 8b)

GEN_THREEVEC_TEST(smlal_2d_2s_s0,  "smlal  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal_2d_2s_s3,  "smlal  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal2_2d_4s_s1, "smlal2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal2_2d_4s_s2, "smlal2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal_4s_4h_h0,  "smlal  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal_4s_4h_h7,  "smlal  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal2_4s_8h_h1, "smlal2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smlal2_4s_8h_h4, "smlal2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(umlal_2d_2s_s0,  "umlal  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal_2d_2s_s3,  "umlal  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal2_2d_4s_s1, "umlal2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal2_2d_4s_s2, "umlal2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal_4s_4h_h0,  "umlal  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal_4s_4h_h7,  "umlal  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal2_4s_8h_h1, "umlal2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umlal2_4s_8h_h4, "umlal2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(smlsl_2d_2s_s0,  "smlsl  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl_2d_2s_s3,  "smlsl  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl2_2d_4s_s1, "smlsl2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl2_2d_4s_s2, "smlsl2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl_4s_4h_h0,  "smlsl  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl_4s_4h_h7,  "smlsl  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl2_4s_8h_h1, "smlsl2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smlsl2_4s_8h_h4, "smlsl2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(umlsl_2d_2s_s0,  "umlsl  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl_2d_2s_s3,  "umlsl  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl2_2d_4s_s1, "umlsl2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl2_2d_4s_s2, "umlsl2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl_4s_4h_h0,  "umlsl  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl_4s_4h_h7,  "umlsl  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl2_4s_8h_h1, "umlsl2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umlsl2_4s_8h_h4, "umlsl2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(smull_2d_2s_s0,  "smull  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smull_2d_2s_s3,  "smull  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(smull2_2d_4s_s1, "smull2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smull2_2d_4s_s2, "smull2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(smull_4s_4h_h0,  "smull  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(smull_4s_4h_h7,  "smull  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(smull2_4s_8h_h1, "smull2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(smull2_4s_8h_h4, "smull2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(umull_2d_2s_s0,  "umull  v29.2d, v20.2s, v3.s[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umull_2d_2s_s3,  "umull  v29.2d, v20.2s, v3.s[3]", 29, 20, 3)
GEN_THREEVEC_TEST(umull2_2d_4s_s1, "umull2 v29.2d, v20.4s, v3.s[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umull2_2d_4s_s2, "umull2 v29.2d, v20.4s, v3.s[2]", 29, 20, 3)
GEN_THREEVEC_TEST(umull_4s_4h_h0,  "umull  v29.4s, v20.4h, v3.h[0]", 29, 20, 3)
GEN_THREEVEC_TEST(umull_4s_4h_h7,  "umull  v29.4s, v20.4h, v3.h[7]", 29, 20, 3)
GEN_THREEVEC_TEST(umull2_4s_8h_h1, "umull2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)
GEN_THREEVEC_TEST(umull2_4s_8h_h4, "umull2 v29.4s, v20.8h, v3.h[1]", 29, 20, 3)

GEN_THREEVEC_TEST(smlal_2d_2s_2s,  "smlal  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(smlal2_2d_4s_4s, "smlal2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(smlal_4s_4h_4h,  "smlal  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(smlal2_4s_8h_8h, "smlal2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(smlal_8h_8b_8b,  "smlal  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(smlal2_8h_16b_16b, 
                                   "smlal2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(umlal_2d_2s_2s,  "umlal  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(umlal2_2d_4s_4s, "umlal2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(umlal_4s_4h_4h,  "umlal  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(umlal2_4s_8h_8h, "umlal2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(umlal_8h_8b_8b,  "umlal  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(umlal2_8h_16b_16b, 
                                   "umlal2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(smlsl_2d_2s_2s,  "smlsl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(smlsl2_2d_4s_4s, "smlsl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(smlsl_4s_4h_4h,  "smlsl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(smlsl2_4s_8h_8h, "smlsl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(smlsl_8h_8b_8b,  "smlsl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(smlsl2_8h_16b_16b, 
                                   "smlsl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(umlsl_2d_2s_2s,  "umlsl  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(umlsl2_2d_4s_4s, "umlsl2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(umlsl_4s_4h_4h,  "umlsl  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(umlsl2_4s_8h_8h, "umlsl2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(umlsl_8h_8b_8b,  "umlsl  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(umlsl2_8h_16b_16b, 
                                   "umlsl2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(smull_2d_2s_2s,  "smull  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(smull2_2d_4s_4s, "smull2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(smull_4s_4h_4h,  "smull  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(smull2_4s_8h_8h, "smull2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(smull_8h_8b_8b,  "smull  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(smull2_8h_16b_16b, 
                                   "smull2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_THREEVEC_TEST(umull_2d_2s_2s,  "umull  v2.2d, v11.2s, v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(umull2_2d_4s_4s, "umull2 v2.2d, v11.4s, v29.4s", 2, 11, 29)

GEN_THREEVEC_TEST(umull_4s_4h_4h,  "umull  v2.4s, v11.4h, v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(umull2_4s_8h_8h, "umull2 v2.4s, v11.8h, v29.8h", 2, 11, 29)

GEN_THREEVEC_TEST(umull_8h_8b_8b,  "umull  v2.8h, v11.8b, v29.8b", 2, 11, 29)
GEN_THREEVEC_TEST(umull2_8h_16b_16b, 
                                   "umull2 v2.8h, v11.16b, v29.16b", 2, 11, 29)

GEN_TWOVEC_TEST(sqabs_d_d, "sqabs d7, d30", 7, 30)
GEN_TWOVEC_TEST(sqabs_s_s, "sqabs s7, s30", 7, 30)
GEN_TWOVEC_TEST(sqabs_h_h, "sqabs h7, h30", 7, 30)
GEN_TWOVEC_TEST(sqabs_b_b, "sqabs b7, b30", 7, 30)

GEN_TWOVEC_TEST(sqneg_d_d, "sqneg d7, d30", 7, 30)
GEN_TWOVEC_TEST(sqneg_s_s, "sqneg s7, s30", 7, 30)
GEN_TWOVEC_TEST(sqneg_h_h, "sqneg h7, h30", 7, 30)
GEN_TWOVEC_TEST(sqneg_b_b, "sqneg b7, b30", 7, 30)

GEN_UNARY_TEST(sqabs, 2d, 2d)
GEN_UNARY_TEST(sqabs, 4s, 4s)
GEN_UNARY_TEST(sqabs, 2s, 2s)
GEN_UNARY_TEST(sqabs, 8h, 8h)
GEN_UNARY_TEST(sqabs, 4h, 4h)
GEN_UNARY_TEST(sqabs, 16b, 16b)
GEN_UNARY_TEST(sqabs, 8b, 8b)

GEN_UNARY_TEST(sqneg, 2d, 2d)
GEN_UNARY_TEST(sqneg, 4s, 4s)
GEN_UNARY_TEST(sqneg, 2s, 2s)
GEN_UNARY_TEST(sqneg, 8h, 8h)
GEN_UNARY_TEST(sqneg, 4h, 4h)
GEN_UNARY_TEST(sqneg, 16b, 16b)
GEN_UNARY_TEST(sqneg, 8b, 8b)

GEN_THREEVEC_TEST(sqadd_d_d_d, "sqadd d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(sqadd_s_s_s, "sqadd s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqadd_h_h_h, "sqadd h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(sqadd_b_b_b, "sqadd b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(sqadd_2d_2d_2d,    "sqadd v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(sqadd_4s_4s_4s,    "sqadd v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqadd_2s_2s_2s,    "sqadd v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqadd_8h_8h_8h,    "sqadd v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqadd_4h_4h_4h,    "sqadd v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(sqadd_16b_16b_16b, "sqadd v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(sqadd_8b_8b_8b,    "sqadd v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uqadd_d_d_d, "uqadd d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(uqadd_s_s_s, "uqadd s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(uqadd_h_h_h, "uqadd h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(uqadd_b_b_b, "uqadd b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(uqadd_2d_2d_2d,    "uqadd v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uqadd_4s_4s_4s,    "uqadd v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uqadd_2s_2s_2s,    "uqadd v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uqadd_8h_8h_8h,    "uqadd v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uqadd_4h_4h_4h,    "uqadd v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uqadd_16b_16b_16b, "uqadd v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uqadd_8b_8b_8b,    "uqadd v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(sqsub_d_d_d, "sqsub d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(sqsub_s_s_s, "sqsub s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqsub_h_h_h, "sqsub h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(sqsub_b_b_b, "sqsub b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(sqsub_2d_2d_2d,    "sqsub v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(sqsub_4s_4s_4s,    "sqsub v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqsub_2s_2s_2s,    "sqsub v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqsub_8h_8h_8h,    "sqsub v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqsub_4h_4h_4h,    "sqsub v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(sqsub_16b_16b_16b, "sqsub v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(sqsub_8b_8b_8b,    "sqsub v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uqsub_d_d_d, "uqsub d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(uqsub_s_s_s, "uqsub s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(uqsub_h_h_h, "uqsub h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(uqsub_b_b_b, "uqsub b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(uqsub_2d_2d_2d,    "uqsub v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uqsub_4s_4s_4s,    "uqsub v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uqsub_2s_2s_2s,    "uqsub v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uqsub_8h_8h_8h,    "uqsub v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uqsub_4h_4h_4h,    "uqsub v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uqsub_16b_16b_16b, "uqsub v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uqsub_8b_8b_8b,    "uqsub v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(sqdmlal_d_s_s0, "sqdmlal d31, s30, v29.s[0]", 31,30,29)
GEN_THREEVEC_TEST(sqdmlal_d_s_s3, "sqdmlal d31, s30, v29.s[3]", 31,30,29)
GEN_THREEVEC_TEST(sqdmlal_s_h_h1, "sqdmlal s31, h30, v13.h[1]", 31,30,13)
GEN_THREEVEC_TEST(sqdmlal_s_h_h5, "sqdmlal s31, h30, v13.h[5]", 31,30,13)

GEN_THREEVEC_TEST(sqdmlsl_d_s_s0, "sqdmlsl d31, s30, v29.s[0]", 31,30,29)
GEN_THREEVEC_TEST(sqdmlsl_d_s_s3, "sqdmlsl d31, s30, v29.s[3]", 31,30,29)
GEN_THREEVEC_TEST(sqdmlsl_s_h_h1, "sqdmlsl s31, h30, v13.h[1]", 31,30,13)
GEN_THREEVEC_TEST(sqdmlsl_s_h_h5, "sqdmlsl s31, h30, v13.h[5]", 31,30,13)

GEN_THREEVEC_TEST(sqdmull_d_s_s0, "sqdmull d31, s30, v29.s[0]", 31,30,29)
GEN_THREEVEC_TEST(sqdmull_d_s_s3, "sqdmull d31, s30, v29.s[3]", 31,30,29)
GEN_THREEVEC_TEST(sqdmull_s_h_h1, "sqdmull s31, h30, v13.h[1]", 31,30,13)
GEN_THREEVEC_TEST(sqdmull_s_h_h5, "sqdmull s31, h30, v13.h[5]", 31,30,13)

GEN_THREEVEC_TEST(sqdmlal_2d_2s_s0, "sqdmlal  v29.2d, v20.2s, v3.s[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal_2d_2s_s3, "sqdmlal  v29.2d, v20.2s, v3.s[3]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal2_2d_4s_s1,"sqdmlal2 v29.2d, v20.4s, v3.s[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal2_2d_4s_s2,"sqdmlal2 v29.2d, v20.4s, v3.s[2]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal_4s_4h_h0, "sqdmlal  v29.4s, v20.4h, v3.h[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal_4s_4h_h7, "sqdmlal  v29.4s, v20.4h, v3.h[7]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal2_4s_8h_h1,"sqdmlal2 v29.4s, v20.8h, v3.h[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmlal2_4s_8h_h4,"sqdmlal2 v29.4s, v20.8h, v3.h[1]",29,20,3)

GEN_THREEVEC_TEST(sqdmlsl_2d_2s_s0, "sqdmlsl  v29.2d, v20.2s, v3.s[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl_2d_2s_s3, "sqdmlsl  v29.2d, v20.2s, v3.s[3]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl2_2d_4s_s1,"sqdmlsl2 v29.2d, v20.4s, v3.s[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl2_2d_4s_s2,"sqdmlsl2 v29.2d, v20.4s, v3.s[2]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl_4s_4h_h0, "sqdmlsl  v29.4s, v20.4h, v3.h[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl_4s_4h_h7, "sqdmlsl  v29.4s, v20.4h, v3.h[7]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl2_4s_8h_h1,"sqdmlsl2 v29.4s, v20.8h, v3.h[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmlsl2_4s_8h_h4,"sqdmlsl2 v29.4s, v20.8h, v3.h[1]",29,20,3)

GEN_THREEVEC_TEST(sqdmull_2d_2s_s0, "sqdmull  v29.2d, v20.2s, v3.s[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmull_2d_2s_s3, "sqdmull  v29.2d, v20.2s, v3.s[3]",29,20,3)
GEN_THREEVEC_TEST(sqdmull2_2d_4s_s1,"sqdmull2 v29.2d, v20.4s, v3.s[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmull2_2d_4s_s2,"sqdmull2 v29.2d, v20.4s, v3.s[2]",29,20,3)
GEN_THREEVEC_TEST(sqdmull_4s_4h_h0, "sqdmull  v29.4s, v20.4h, v3.h[0]",29,20,3)
GEN_THREEVEC_TEST(sqdmull_4s_4h_h7, "sqdmull  v29.4s, v20.4h, v3.h[7]",29,20,3)
GEN_THREEVEC_TEST(sqdmull2_4s_8h_h1,"sqdmull2 v29.4s, v20.8h, v3.h[1]",29,20,3)
GEN_THREEVEC_TEST(sqdmull2_4s_8h_h4,"sqdmull2 v29.4s, v20.8h, v3.h[1]",29,20,3)

GEN_THREEVEC_TEST(sqdmlal_d_s_s, "sqdmlal d0, s8, s16", 0, 8, 16)
GEN_THREEVEC_TEST(sqdmlal_s_h_h, "sqdmlal s0, h8, h16", 0, 8, 16)
GEN_THREEVEC_TEST(sqdmlsl_d_s_s, "sqdmlsl d0, s8, s16", 0, 8, 16)
GEN_THREEVEC_TEST(sqdmlsl_s_h_h, "sqdmlsl s0, h8, h16", 0, 8, 16)
GEN_THREEVEC_TEST(sqdmull_d_s_s, "sqdmull d0, s8, s16", 0, 8, 16)
GEN_THREEVEC_TEST(sqdmull_s_h_h, "sqdmull s0, h8, h16", 0, 8, 16)

GEN_THREEVEC_TEST(sqdmlal_2d_2s_2s,  "sqdmlal  v2.2d, v11.2s, v29.2s", 2,11,29)
GEN_THREEVEC_TEST(sqdmlal2_2d_4s_4s, "sqdmlal2 v2.2d, v11.4s, v29.4s", 2,11,29)
GEN_THREEVEC_TEST(sqdmlal_4s_4h_4h,  "sqdmlal  v2.4s, v11.4h, v29.4h", 2,11,29)
GEN_THREEVEC_TEST(sqdmlal2_4s_8h_8h, "sqdmlal2 v2.4s, v11.8h, v29.8h", 2,11,29)

GEN_THREEVEC_TEST(sqdmlsl_2d_2s_2s,  "sqdmlsl  v2.2d, v11.2s, v29.2s", 2,11,29)
GEN_THREEVEC_TEST(sqdmlsl2_2d_4s_4s, "sqdmlsl2 v2.2d, v11.4s, v29.4s", 2,11,29)
GEN_THREEVEC_TEST(sqdmlsl_4s_4h_4h,  "sqdmlsl  v2.4s, v11.4h, v29.4h", 2,11,29)
GEN_THREEVEC_TEST(sqdmlsl2_4s_8h_8h, "sqdmlsl2 v2.4s, v11.8h, v29.8h", 2,11,29)

GEN_THREEVEC_TEST(sqdmull_2d_2s_2s,  "sqdmull  v2.2d, v11.2s, v29.2s", 2,11,29)
GEN_THREEVEC_TEST(sqdmull2_2d_4s_4s, "sqdmull2 v2.2d, v11.4s, v29.4s", 2,11,29)
GEN_THREEVEC_TEST(sqdmull_4s_4h_4h,  "sqdmull  v2.4s, v11.4h, v29.4h", 2,11,29)
GEN_THREEVEC_TEST(sqdmull2_4s_8h_8h, "sqdmull2 v2.4s, v11.8h, v29.8h", 2,11,29)

GEN_THREEVEC_TEST(sqdmulh_s_s_s1, "sqdmulh s0, s1, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_s_s_s3, "sqdmulh s0, s1, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_h_h_h2, "sqdmulh h0, h1, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_h_h_h7, "sqdmulh h0, h1, v2.h[7]", 0,1,2)

GEN_THREEVEC_TEST(sqrdmulh_s_s_s1, "sqrdmulh s0, s1, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_s_s_s3, "sqrdmulh s0, s1, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_h_h_h2, "sqrdmulh h0, h1, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_h_h_h7, "sqrdmulh h0, h1, v2.h[7]", 0,1,2)

GEN_THREEVEC_TEST(sqdmulh_4s_4s_s1, "sqdmulh v0.4s, v1.4s, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_4s_4s_s3, "sqdmulh v0.4s, v1.4s, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_2s_2s_s1, "sqdmulh v0.2s, v1.2s, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_2s_2s_s3, "sqdmulh v0.2s, v1.2s, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_8h_8h_h2, "sqdmulh v0.8h, v1.8h, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_8h_8h_h7, "sqdmulh v0.8h, v1.8h, v2.h[7]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_4h_4h_h2, "sqdmulh v0.4h, v1.4h, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqdmulh_4h_4h_h7, "sqdmulh v0.4h, v1.4h, v2.h[7]", 0,1,2)

GEN_THREEVEC_TEST(sqrdmulh_4s_4s_s1, "sqrdmulh v0.4s, v1.4s, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_4s_4s_s3, "sqrdmulh v0.4s, v1.4s, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_2s_2s_s1, "sqrdmulh v0.2s, v1.2s, v2.s[1]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_2s_2s_s3, "sqrdmulh v0.2s, v1.2s, v2.s[3]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_8h_8h_h2, "sqrdmulh v0.8h, v1.8h, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_8h_8h_h7, "sqrdmulh v0.8h, v1.8h, v2.h[7]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_4h_4h_h2, "sqrdmulh v0.4h, v1.4h, v2.h[2]", 0,1,2)
GEN_THREEVEC_TEST(sqrdmulh_4h_4h_h7, "sqrdmulh v0.4h, v1.4h, v2.h[7]", 0,1,2)

GEN_THREEVEC_TEST(sqdmulh_s_s_s,  "sqdmulh  s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqdmulh_h_h_h,  "sqdmulh  h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_s_s_s, "sqrdmulh s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_h_h_h, "sqrdmulh h1, h2, h4", 1, 2, 4)

GEN_THREEVEC_TEST(sqdmulh_4s_4s_4s, "sqdmulh v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqdmulh_2s_2s_2s, "sqdmulh v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqdmulh_8h_8h_8h, "sqdmulh v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqdmulh_4h_4h_4h, "sqdmulh v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_4s_4s_4s, "sqrdmulh v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_2s_2s_2s, "sqrdmulh v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_8h_8h_8h, "sqrdmulh v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqrdmulh_4h_4h_4h, "sqrdmulh v1.4h,  v2.4h,  v4.4h",  1, 2, 4)

GEN_THREEVEC_TEST(sqshl_d_d_d, "sqshl d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(sqshl_s_s_s, "sqshl s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqshl_h_h_h, "sqshl h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(sqshl_b_b_b, "sqshl b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(sqshl_2d_2d_2d,    "sqshl v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(sqshl_4s_4s_4s,    "sqshl v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqshl_2s_2s_2s,    "sqshl v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqshl_8h_8h_8h,    "sqshl v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqshl_4h_4h_4h,    "sqshl v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(sqshl_16b_16b_16b, "sqshl v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(sqshl_8b_8b_8b,    "sqshl v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uqshl_d_d_d, "uqshl d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(uqshl_s_s_s, "uqshl s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(uqshl_h_h_h, "uqshl h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(uqshl_b_b_b, "uqshl b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(uqshl_2d_2d_2d,    "uqshl v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uqshl_4s_4s_4s,    "uqshl v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uqshl_2s_2s_2s,    "uqshl v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uqshl_8h_8h_8h,    "uqshl v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uqshl_4h_4h_4h,    "uqshl v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uqshl_16b_16b_16b, "uqshl v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uqshl_8b_8b_8b,    "uqshl v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(sqrshl_d_d_d, "sqrshl d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_s_s_s, "sqrshl s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_h_h_h, "sqrshl h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_b_b_b, "sqrshl b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_2d_2d_2d,    "sqrshl v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_4s_4s_4s,    "sqrshl v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_2s_2s_2s,    "sqrshl v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_8h_8h_8h,    "sqrshl v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_4h_4h_4h,    "sqrshl v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_16b_16b_16b, "sqrshl v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(sqrshl_8b_8b_8b,    "sqrshl v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uqrshl_d_d_d, "uqrshl d1, d2, d4", 1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_s_s_s, "uqrshl s1, s2, s4", 1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_h_h_h, "uqrshl h1, h2, h4", 1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_b_b_b, "uqrshl b1, b2, b4", 1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_2d_2d_2d,    "uqrshl v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_4s_4s_4s,    "uqrshl v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_2s_2s_2s,    "uqrshl v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_8h_8h_8h,    "uqrshl v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_4h_4h_4h,    "uqrshl v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_16b_16b_16b, "uqrshl v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uqrshl_8b_8b_8b,    "uqrshl v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_TWOVEC_TEST(sqrshrn_s_d_1,  "sqrshrn s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrn_s_d_17, "sqrshrn s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(sqrshrn_s_d_32, "sqrshrn s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(sqrshrn_h_s_1,  "sqrshrn h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrn_h_s_9,  "sqrshrn h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(sqrshrn_h_s_16, "sqrshrn h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(sqrshrn_b_h_1,  "sqrshrn b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrn_b_h_4,  "sqrshrn b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(sqrshrn_b_h_8,  "sqrshrn b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(uqrshrn_s_d_1,  "uqrshrn s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(uqrshrn_s_d_17, "uqrshrn s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(uqrshrn_s_d_32, "uqrshrn s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(uqrshrn_h_s_1,  "uqrshrn h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(uqrshrn_h_s_9,  "uqrshrn h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(uqrshrn_h_s_16, "uqrshrn h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(uqrshrn_b_h_1,  "uqrshrn b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(uqrshrn_b_h_4,  "uqrshrn b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(uqrshrn_b_h_8,  "uqrshrn b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(sqshrn_s_d_1,  "sqshrn s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrn_s_d_17, "sqshrn s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(sqshrn_s_d_32, "sqshrn s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(sqshrn_h_s_1,  "sqshrn h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrn_h_s_9,  "sqshrn h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(sqshrn_h_s_16, "sqshrn h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(sqshrn_b_h_1,  "sqshrn b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrn_b_h_4,  "sqshrn b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(sqshrn_b_h_8,  "sqshrn b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(uqshrn_s_d_1,  "uqshrn s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(uqshrn_s_d_17, "uqshrn s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(uqshrn_s_d_32, "uqshrn s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(uqshrn_h_s_1,  "uqshrn h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(uqshrn_h_s_9,  "uqshrn h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(uqshrn_h_s_16, "uqshrn h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(uqshrn_b_h_1,  "uqshrn b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(uqshrn_b_h_4,  "uqshrn b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(uqshrn_b_h_8,  "uqshrn b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(sqrshrun_s_d_1,  "sqrshrun s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrun_s_d_17, "sqrshrun s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(sqrshrun_s_d_32, "sqrshrun s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(sqrshrun_h_s_1,  "sqrshrun h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrun_h_s_9,  "sqrshrun h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(sqrshrun_h_s_16, "sqrshrun h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(sqrshrun_b_h_1,  "sqrshrun b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(sqrshrun_b_h_4,  "sqrshrun b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(sqrshrun_b_h_8,  "sqrshrun b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(sqshrun_s_d_1,  "sqshrun s2, d5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrun_s_d_17, "sqshrun s2, d5, #17", 2, 5)
GEN_TWOVEC_TEST(sqshrun_s_d_32, "sqshrun s2, d5, #32", 2, 5)
GEN_TWOVEC_TEST(sqshrun_h_s_1,  "sqshrun h2, s5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrun_h_s_9,  "sqshrun h2, s5, #9",  2, 5)
GEN_TWOVEC_TEST(sqshrun_h_s_16, "sqshrun h2, s5, #16", 2, 5)
GEN_TWOVEC_TEST(sqshrun_b_h_1,  "sqshrun b2, h5, #1",  2, 5)
GEN_TWOVEC_TEST(sqshrun_b_h_4,  "sqshrun b2, h5, #4",  2, 5)
GEN_TWOVEC_TEST(sqshrun_b_h_8,  "sqshrun b2, h5, #8",  2, 5)

GEN_TWOVEC_TEST(sqrshrn_2s_2d_1,   "sqrshrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn_2s_2d_17,  "sqrshrn  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqrshrn_2s_2d_32,  "sqrshrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqrshrn2_4s_2d_1,  "sqrshrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_4s_2d_17, "sqrshrn2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqrshrn2_4s_2d_32, "sqrshrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqrshrn_4h_4s_1,   "sqrshrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn_4h_4s_9,   "sqrshrn  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqrshrn_4h_4s_16,  "sqrshrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqrshrn2_8h_4s_1,  "sqrshrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_8h_4s_9,  "sqrshrn2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_8h_4s_16, "sqrshrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqrshrn_8b_8h_1,   "sqrshrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn_8b_8h_4,   "sqrshrn  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqrshrn_8b_8h_8,   "sqrshrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_16b_8h_1, "sqrshrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_16b_8h_4, "sqrshrn2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqrshrn2_16b_8h_8, "sqrshrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(uqrshrn_2s_2d_1,   "uqrshrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn_2s_2d_17,  "uqrshrn  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(uqrshrn_2s_2d_32,  "uqrshrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(uqrshrn2_4s_2d_1,  "uqrshrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_4s_2d_17, "uqrshrn2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(uqrshrn2_4s_2d_32, "uqrshrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(uqrshrn_4h_4s_1,   "uqrshrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn_4h_4s_9,   "uqrshrn  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(uqrshrn_4h_4s_16,  "uqrshrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(uqrshrn2_8h_4s_1,  "uqrshrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_8h_4s_9,  "uqrshrn2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_8h_4s_16, "uqrshrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(uqrshrn_8b_8h_1,   "uqrshrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn_8b_8h_4,   "uqrshrn  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(uqrshrn_8b_8h_8,   "uqrshrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_16b_8h_1, "uqrshrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_16b_8h_4, "uqrshrn2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(uqrshrn2_16b_8h_8, "uqrshrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(sqshrn_2s_2d_1,   "sqshrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn_2s_2d_17,  "sqshrn  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqshrn_2s_2d_32,  "sqshrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqshrn2_4s_2d_1,  "sqshrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_4s_2d_17, "sqshrn2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqshrn2_4s_2d_32, "sqshrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqshrn_4h_4s_1,   "sqshrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn_4h_4s_9,   "sqshrn  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqshrn_4h_4s_16,  "sqshrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqshrn2_8h_4s_1,  "sqshrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_8h_4s_9,  "sqshrn2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_8h_4s_16, "sqshrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqshrn_8b_8h_1,   "sqshrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn_8b_8h_4,   "sqshrn  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqshrn_8b_8h_8,   "sqshrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_16b_8h_1, "sqshrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_16b_8h_4, "sqshrn2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqshrn2_16b_8h_8, "sqshrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(uqshrn_2s_2d_1,   "uqshrn  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn_2s_2d_17,  "uqshrn  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(uqshrn_2s_2d_32,  "uqshrn  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(uqshrn2_4s_2d_1,  "uqshrn2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_4s_2d_17, "uqshrn2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(uqshrn2_4s_2d_32, "uqshrn2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(uqshrn_4h_4s_1,   "uqshrn  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn_4h_4s_9,   "uqshrn  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(uqshrn_4h_4s_16,  "uqshrn  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(uqshrn2_8h_4s_1,  "uqshrn2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_8h_4s_9,  "uqshrn2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_8h_4s_16, "uqshrn2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(uqshrn_8b_8h_1,   "uqshrn  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn_8b_8h_4,   "uqshrn  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(uqshrn_8b_8h_8,   "uqshrn  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_16b_8h_1, "uqshrn2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_16b_8h_4, "uqshrn2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(uqshrn2_16b_8h_8, "uqshrn2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(sqrshrun_2s_2d_1,   "sqrshrun  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun_2s_2d_17,  "sqrshrun  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqrshrun_2s_2d_32,  "sqrshrun  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqrshrun2_4s_2d_1,  "sqrshrun2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_4s_2d_17, "sqrshrun2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqrshrun2_4s_2d_32, "sqrshrun2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqrshrun_4h_4s_1,   "sqrshrun  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun_4h_4s_9,   "sqrshrun  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqrshrun_4h_4s_16,  "sqrshrun  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqrshrun2_8h_4s_1,  "sqrshrun2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_8h_4s_9,  "sqrshrun2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_8h_4s_16, "sqrshrun2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqrshrun_8b_8h_1,   "sqrshrun  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun_8b_8h_4,   "sqrshrun  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqrshrun_8b_8h_8,   "sqrshrun  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_16b_8h_1, "sqrshrun2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_16b_8h_4, "sqrshrun2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqrshrun2_16b_8h_8, "sqrshrun2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(sqshrun_2s_2d_1,   "sqshrun  v4.2s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun_2s_2d_17,  "sqshrun  v4.2s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqshrun_2s_2d_32,  "sqshrun  v4.2s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqshrun2_4s_2d_1,  "sqshrun2 v4.4s,  v29.2d, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_4s_2d_17, "sqshrun2 v4.4s,  v29.2d, #17", 4, 29)
GEN_TWOVEC_TEST(sqshrun2_4s_2d_32, "sqshrun2 v4.4s,  v29.2d, #32", 4, 29)
GEN_TWOVEC_TEST(sqshrun_4h_4s_1,   "sqshrun  v4.4h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun_4h_4s_9,   "sqshrun  v4.4h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqshrun_4h_4s_16,  "sqshrun  v4.4h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqshrun2_8h_4s_1,  "sqshrun2 v4.8h,  v29.4s, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_8h_4s_9,  "sqshrun2 v4.8h,  v29.4s, #9",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_8h_4s_16, "sqshrun2 v4.8h,  v29.4s, #16", 4, 29)
GEN_TWOVEC_TEST(sqshrun_8b_8h_1,   "sqshrun  v4.8b,  v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun_8b_8h_4,   "sqshrun  v4.8b,  v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqshrun_8b_8h_8,   "sqshrun  v4.8b,  v29.8h, #8",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_16b_8h_1, "sqshrun2 v4.16b, v29.8h, #1",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_16b_8h_4, "sqshrun2 v4.16b, v29.8h, #4",  4, 29)
GEN_TWOVEC_TEST(sqshrun2_16b_8h_8, "sqshrun2 v4.16b, v29.8h, #8",  4, 29)

GEN_TWOVEC_TEST(sqshl_d_d_0,  "sqshl d5, d28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshl_d_d_32, "sqshl d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(sqshl_d_d_63, "sqshl d5, d28, #63", 5, 28)
GEN_TWOVEC_TEST(sqshl_s_s_0,  "sqshl s5, s28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshl_s_s_16, "sqshl s5, s28, #16", 5, 28)
GEN_TWOVEC_TEST(sqshl_s_s_31, "sqshl s5, s28, #31", 5, 28)
GEN_TWOVEC_TEST(sqshl_h_h_0,  "sqshl h5, h28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshl_h_h_8,  "sqshl h5, h28, #8",  5, 28)
GEN_TWOVEC_TEST(sqshl_h_h_15, "sqshl h5, h28, #15", 5, 28)
GEN_TWOVEC_TEST(sqshl_b_b_0,  "sqshl b5, b28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshl_b_b_4,  "sqshl b5, b28, #4",  5, 28)
GEN_TWOVEC_TEST(sqshl_b_b_7,  "sqshl b5, b28, #7",  5, 28)

GEN_TWOVEC_TEST(uqshl_d_d_0,  "uqshl d5, d28, #0",  5, 28)
GEN_TWOVEC_TEST(uqshl_d_d_32, "uqshl d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(uqshl_d_d_63, "uqshl d5, d28, #63", 5, 28)
GEN_TWOVEC_TEST(uqshl_s_s_0,  "uqshl s5, s28, #0",  5, 28)
GEN_TWOVEC_TEST(uqshl_s_s_16, "uqshl s5, s28, #16", 5, 28)
GEN_TWOVEC_TEST(uqshl_s_s_31, "uqshl s5, s28, #31", 5, 28)
GEN_TWOVEC_TEST(uqshl_h_h_0,  "uqshl h5, h28, #0",  5, 28)
GEN_TWOVEC_TEST(uqshl_h_h_8,  "uqshl h5, h28, #8",  5, 28)
GEN_TWOVEC_TEST(uqshl_h_h_15, "uqshl h5, h28, #15", 5, 28)
GEN_TWOVEC_TEST(uqshl_b_b_0,  "uqshl b5, b28, #0",  5, 28)
GEN_TWOVEC_TEST(uqshl_b_b_4,  "uqshl b5, b28, #4",  5, 28)
GEN_TWOVEC_TEST(uqshl_b_b_7,  "uqshl b5, b28, #7",  5, 28)

GEN_TWOVEC_TEST(sqshlu_d_d_0,  "sqshlu d5, d28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshlu_d_d_32, "sqshlu d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(sqshlu_d_d_63, "sqshlu d5, d28, #63", 5, 28)
GEN_TWOVEC_TEST(sqshlu_s_s_0,  "sqshlu s5, s28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshlu_s_s_16, "sqshlu s5, s28, #16", 5, 28)
GEN_TWOVEC_TEST(sqshlu_s_s_31, "sqshlu s5, s28, #31", 5, 28)
GEN_TWOVEC_TEST(sqshlu_h_h_0,  "sqshlu h5, h28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshlu_h_h_8,  "sqshlu h5, h28, #8",  5, 28)
GEN_TWOVEC_TEST(sqshlu_h_h_15, "sqshlu h5, h28, #15", 5, 28)
GEN_TWOVEC_TEST(sqshlu_b_b_0,  "sqshlu b5, b28, #0",  5, 28)
GEN_TWOVEC_TEST(sqshlu_b_b_4,  "sqshlu b5, b28, #4",  5, 28)
GEN_TWOVEC_TEST(sqshlu_b_b_7,  "sqshlu b5, b28, #7",  5, 28)

GEN_TWOVEC_TEST(sqshl_2d_2d_0,   "sqshl v6.2d,  v27.2d, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_2d_2d_32,  "sqshl v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(sqshl_2d_2d_63,  "sqshl v6.2d,  v27.2d, #63", 6, 27)
GEN_TWOVEC_TEST(sqshl_4s_4s_0,   "sqshl v6.4s,  v27.4s, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_4s_4s_16,  "sqshl v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(sqshl_4s_4s_31,  "sqshl v6.4s,  v27.4s, #31", 6, 27)
GEN_TWOVEC_TEST(sqshl_2s_2s_0,   "sqshl v6.2s,  v27.2s, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_2s_2s_16,  "sqshl v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(sqshl_2s_2s_31,  "sqshl v6.2s,  v27.2s, #31", 6, 27)
GEN_TWOVEC_TEST(sqshl_8h_8h_0,   "sqshl v6.8h,  v27.8h, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_8h_8h_8,   "sqshl v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(sqshl_8h_8h_15,  "sqshl v6.8h,  v27.8h, #15", 6, 27)
GEN_TWOVEC_TEST(sqshl_4h_4h_0,   "sqshl v6.4h,  v27.4h, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_4h_4h_8,   "sqshl v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(sqshl_4h_4h_15,  "sqshl v6.4h,  v27.4h, #15", 6, 27)
GEN_TWOVEC_TEST(sqshl_16b_16b_0, "sqshl v6.16b, v27.16b, #0", 6, 27)
GEN_TWOVEC_TEST(sqshl_16b_16b_3, "sqshl v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(sqshl_16b_16b_7, "sqshl v6.16b, v27.16b, #7", 6, 27)
GEN_TWOVEC_TEST(sqshl_8b_8b_0,   "sqshl v6.8b,  v27.8b, #0",  6, 27)
GEN_TWOVEC_TEST(sqshl_8b_8b_3,   "sqshl v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(sqshl_8b_8b_7,   "sqshl v6.8b,  v27.8b, #7",  6, 27)

GEN_TWOVEC_TEST(uqshl_2d_2d_0,   "uqshl v6.2d,  v27.2d, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_2d_2d_32,  "uqshl v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(uqshl_2d_2d_63,  "uqshl v6.2d,  v27.2d, #63", 6, 27)
GEN_TWOVEC_TEST(uqshl_4s_4s_0,   "uqshl v6.4s,  v27.4s, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_4s_4s_16,  "uqshl v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(uqshl_4s_4s_31,  "uqshl v6.4s,  v27.4s, #31", 6, 27)
GEN_TWOVEC_TEST(uqshl_2s_2s_0,   "uqshl v6.2s,  v27.2s, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_2s_2s_16,  "uqshl v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(uqshl_2s_2s_31,  "uqshl v6.2s,  v27.2s, #31", 6, 27)
GEN_TWOVEC_TEST(uqshl_8h_8h_0,   "uqshl v6.8h,  v27.8h, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_8h_8h_8,   "uqshl v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(uqshl_8h_8h_15,  "uqshl v6.8h,  v27.8h, #15", 6, 27)
GEN_TWOVEC_TEST(uqshl_4h_4h_0,   "uqshl v6.4h,  v27.4h, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_4h_4h_8,   "uqshl v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(uqshl_4h_4h_15,  "uqshl v6.4h,  v27.4h, #15", 6, 27)
GEN_TWOVEC_TEST(uqshl_16b_16b_0, "uqshl v6.16b, v27.16b, #0", 6, 27)
GEN_TWOVEC_TEST(uqshl_16b_16b_3, "uqshl v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(uqshl_16b_16b_7, "uqshl v6.16b, v27.16b, #7", 6, 27)
GEN_TWOVEC_TEST(uqshl_8b_8b_0,   "uqshl v6.8b,  v27.8b, #0",  6, 27)
GEN_TWOVEC_TEST(uqshl_8b_8b_3,   "uqshl v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(uqshl_8b_8b_7,   "uqshl v6.8b,  v27.8b, #7",  6, 27)

GEN_TWOVEC_TEST(sqshlu_2d_2d_0,   "sqshlu v6.2d,  v27.2d, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_2d_2d_32,  "sqshlu v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(sqshlu_2d_2d_63,  "sqshlu v6.2d,  v27.2d, #63", 6, 27)
GEN_TWOVEC_TEST(sqshlu_4s_4s_0,   "sqshlu v6.4s,  v27.4s, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_4s_4s_16,  "sqshlu v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(sqshlu_4s_4s_31,  "sqshlu v6.4s,  v27.4s, #31", 6, 27)
GEN_TWOVEC_TEST(sqshlu_2s_2s_0,   "sqshlu v6.2s,  v27.2s, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_2s_2s_16,  "sqshlu v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(sqshlu_2s_2s_31,  "sqshlu v6.2s,  v27.2s, #31", 6, 27)
GEN_TWOVEC_TEST(sqshlu_8h_8h_0,   "sqshlu v6.8h,  v27.8h, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_8h_8h_8,   "sqshlu v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(sqshlu_8h_8h_15,  "sqshlu v6.8h,  v27.8h, #15", 6, 27)
GEN_TWOVEC_TEST(sqshlu_4h_4h_0,   "sqshlu v6.4h,  v27.4h, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_4h_4h_8,   "sqshlu v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(sqshlu_4h_4h_15,  "sqshlu v6.4h,  v27.4h, #15", 6, 27)
GEN_TWOVEC_TEST(sqshlu_16b_16b_0, "sqshlu v6.16b, v27.16b, #0", 6, 27)
GEN_TWOVEC_TEST(sqshlu_16b_16b_3, "sqshlu v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(sqshlu_16b_16b_7, "sqshlu v6.16b, v27.16b, #7", 6, 27)
GEN_TWOVEC_TEST(sqshlu_8b_8b_0,   "sqshlu v6.8b,  v27.8b, #0",  6, 27)
GEN_TWOVEC_TEST(sqshlu_8b_8b_3,   "sqshlu v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(sqshlu_8b_8b_7,   "sqshlu v6.8b,  v27.8b, #7",  6, 27)

GEN_TWOVEC_TEST(sqxtn_s_d,  "sqxtn s31,  d0", 31, 0)
GEN_TWOVEC_TEST(sqxtn_h_s,  "sqxtn h31,  s0", 31, 0)
GEN_TWOVEC_TEST(sqxtn_b_h,  "sqxtn b31,  h0", 31, 0)
GEN_TWOVEC_TEST(uqxtn_s_d,  "uqxtn s31,  d0", 31, 0)
GEN_TWOVEC_TEST(uqxtn_h_s,  "uqxtn h31,  s0", 31, 0)
GEN_TWOVEC_TEST(uqxtn_b_h,  "uqxtn b31,  h0", 31, 0)
GEN_TWOVEC_TEST(sqxtun_s_d, "sqxtun s31, d0", 31, 0)
GEN_TWOVEC_TEST(sqxtun_h_s, "sqxtun h31, s0", 31, 0)
GEN_TWOVEC_TEST(sqxtun_b_h, "sqxtun b31, h0", 31, 0)

GEN_UNARY_TEST(sqxtn,   2s, 2d)
GEN_UNARY_TEST(sqxtn2,  4s, 2d)
GEN_UNARY_TEST(sqxtn,   4h, 4s)
GEN_UNARY_TEST(sqxtn2,  8h, 4s)
GEN_UNARY_TEST(sqxtn,   8b, 8h)
GEN_UNARY_TEST(sqxtn2, 16b, 8h)

GEN_UNARY_TEST(uqxtn,   2s, 2d)
GEN_UNARY_TEST(uqxtn2,  4s, 2d)
GEN_UNARY_TEST(uqxtn,   4h, 4s)
GEN_UNARY_TEST(uqxtn2,  8h, 4s)
GEN_UNARY_TEST(uqxtn,   8b, 8h)
GEN_UNARY_TEST(uqxtn2, 16b, 8h)

GEN_UNARY_TEST(sqxtun,   2s, 2d)
GEN_UNARY_TEST(sqxtun2,  4s, 2d)
GEN_UNARY_TEST(sqxtun,   4h, 4s)
GEN_UNARY_TEST(sqxtun2,  8h, 4s)
GEN_UNARY_TEST(sqxtun,   8b, 8h)
GEN_UNARY_TEST(sqxtun2, 16b, 8h)

GEN_THREEVEC_TEST(srhadd_4s_4s_4s,"srhadd v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(srhadd_2s_2s_2s,"srhadd v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(srhadd_8h_8h_8h,"srhadd v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(srhadd_4h_4h_4h,"srhadd v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(srhadd_16b_16b_16b,
                                  "srhadd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(srhadd_8b_8b_8b,"srhadd v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(urhadd_4s_4s_4s,"urhadd v2.4s,  v11.4s,  v29.4s", 2, 11, 29)
GEN_THREEVEC_TEST(urhadd_2s_2s_2s,"urhadd v2.2s,  v11.2s,  v29.2s", 2, 11, 29)
GEN_THREEVEC_TEST(urhadd_8h_8h_8h,"urhadd v2.8h,  v11.8h,  v29.8h", 2, 11, 29)
GEN_THREEVEC_TEST(urhadd_4h_4h_4h,"urhadd v2.4h,  v11.4h,  v29.4h", 2, 11, 29)
GEN_THREEVEC_TEST(urhadd_16b_16b_16b,
                                  "urhadd v2.16b, v11.16b, v29.16b", 2, 11, 29)
GEN_THREEVEC_TEST(urhadd_8b_8b_8b,"urhadd v2.8b,  v11.8b,  v29.8b", 2, 11, 29)

GEN_THREEVEC_TEST(sshl_d_d_d, "sshl d29, d28, d27", 29, 28, 27)
GEN_THREEVEC_TEST(ushl_d_d_d, "ushl d29, d28, d27", 29, 28, 27)

GEN_THREEVEC_TEST(sshl_2d_2d_2d,    "sshl v29.2d, v28.2d, v27.2d", 29,28,27)
GEN_THREEVEC_TEST(sshl_4s_4s_4s,    "sshl v29.4s, v28.4s, v27.4s", 29,28,27)
GEN_THREEVEC_TEST(sshl_2s_2s_2s,    "sshl v29.2s, v28.2s, v27.2s", 29,28,27)
GEN_THREEVEC_TEST(sshl_8h_8h_8h,    "sshl v29.8h, v28.8h, v27.8h", 29,28,27)
GEN_THREEVEC_TEST(sshl_4h_4h_4h,    "sshl v29.4h, v28.4h, v27.4h", 29,28,27)
GEN_THREEVEC_TEST(sshl_16b_16b_16b, "sshl v29.16b, v28.16b, v27.16b", 29,28,27)
GEN_THREEVEC_TEST(sshl_8b_8b_8b,    "sshl v29.8b, v28.8b, v27.8b", 29,28,27)

GEN_THREEVEC_TEST(ushl_2d_2d_2d,    "ushl v29.2d, v28.2d, v27.2d", 29,28,27)
GEN_THREEVEC_TEST(ushl_4s_4s_4s,    "ushl v29.4s, v28.4s, v27.4s", 29,28,27)
GEN_THREEVEC_TEST(ushl_2s_2s_2s,    "ushl v29.2s, v28.2s, v27.2s", 29,28,27)
GEN_THREEVEC_TEST(ushl_8h_8h_8h,    "ushl v29.8h, v28.8h, v27.8h", 29,28,27)
GEN_THREEVEC_TEST(ushl_4h_4h_4h,    "ushl v29.4h, v28.4h, v27.4h", 29,28,27)
GEN_THREEVEC_TEST(ushl_16b_16b_16b, "ushl v29.16b, v28.16b, v27.16b", 29,28,27)
GEN_THREEVEC_TEST(ushl_8b_8b_8b,    "ushl v29.8b, v28.8b, v27.8b", 29,28,27)

GEN_TWOVEC_TEST(shl_d_d_0,  "shl d5, d28, #0",  5, 28)
GEN_TWOVEC_TEST(shl_d_d_32, "shl d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(shl_d_d_63, "shl d5, d28, #63", 5, 28)

GEN_TWOVEC_TEST(sshr_d_d_1,  "sshr d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(sshr_d_d_32, "sshr d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(sshr_d_d_64, "sshr d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(ushr_d_d_1,  "ushr d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(ushr_d_d_32, "ushr d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(ushr_d_d_64, "ushr d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(ssra_d_d_1,  "ssra d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(ssra_d_d_32, "ssra d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(ssra_d_d_64, "ssra d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(usra_d_d_1,  "usra d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(usra_d_d_32, "usra d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(usra_d_d_64, "usra d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(ssra_2d_2d_1,   "ssra v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_2d_2d_32,  "ssra v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(ssra_2d_2d_64,  "ssra v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(ssra_4s_4s_1,   "ssra v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_4s_4s_16,  "ssra v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(ssra_4s_4s_32,  "ssra v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(ssra_2s_2s_1,   "ssra v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_2s_2s_16,  "ssra v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(ssra_2s_2s_32,  "ssra v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(ssra_8h_8h_1,   "ssra v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_8h_8h_8,   "ssra v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(ssra_8h_8h_16,  "ssra v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(ssra_4h_4h_1,   "ssra v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_4h_4h_8,   "ssra v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(ssra_4h_4h_16,  "ssra v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(ssra_16b_16b_1, "ssra v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(ssra_16b_16b_3, "ssra v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(ssra_16b_16b_8, "ssra v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(ssra_8b_8b_1,   "ssra v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(ssra_8b_8b_3,   "ssra v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(ssra_8b_8b_8,   "ssra v6.8b,  v27.8b, #8",  6, 27)

GEN_TWOVEC_TEST(usra_2d_2d_1,   "usra v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(usra_2d_2d_32,  "usra v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(usra_2d_2d_64,  "usra v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(usra_4s_4s_1,   "usra v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(usra_4s_4s_16,  "usra v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(usra_4s_4s_32,  "usra v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(usra_2s_2s_1,   "usra v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(usra_2s_2s_16,  "usra v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(usra_2s_2s_32,  "usra v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(usra_8h_8h_1,   "usra v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(usra_8h_8h_8,   "usra v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(usra_8h_8h_16,  "usra v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(usra_4h_4h_1,   "usra v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(usra_4h_4h_8,   "usra v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(usra_4h_4h_16,  "usra v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(usra_16b_16b_1, "usra v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(usra_16b_16b_3, "usra v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(usra_16b_16b_8, "usra v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(usra_8b_8b_1,   "usra v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(usra_8b_8b_3,   "usra v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(usra_8b_8b_8,   "usra v6.8b,  v27.8b, #8",  6, 27)

GEN_THREEVEC_TEST(srshl_d_d_d, "srshl d29, d28, d27", 29, 28, 27)
GEN_THREEVEC_TEST(urshl_d_d_d, "urshl d29, d28, d27", 29, 28, 27)

GEN_THREEVEC_TEST(srshl_2d_2d_2d,   "srshl v29.2d, v28.2d, v27.2d", 29,28,27)
GEN_THREEVEC_TEST(srshl_4s_4s_4s,   "srshl v29.4s, v28.4s, v27.4s", 29,28,27)
GEN_THREEVEC_TEST(srshl_2s_2s_2s,   "srshl v29.2s, v28.2s, v27.2s", 29,28,27)
GEN_THREEVEC_TEST(srshl_8h_8h_8h,   "srshl v29.8h, v28.8h, v27.8h", 29,28,27)
GEN_THREEVEC_TEST(srshl_4h_4h_4h,   "srshl v29.4h, v28.4h, v27.4h", 29,28,27)
GEN_THREEVEC_TEST(srshl_16b_16b_16b,"srshl v29.16b, v28.16b, v27.16b", 29,28,27)
GEN_THREEVEC_TEST(srshl_8b_8b_8b,   "srshl v29.8b, v28.8b, v27.8b", 29,28,27)

GEN_THREEVEC_TEST(urshl_2d_2d_2d,   "urshl v29.2d, v28.2d, v27.2d", 29,28,27)
GEN_THREEVEC_TEST(urshl_4s_4s_4s,   "urshl v29.4s, v28.4s, v27.4s", 29,28,27)
GEN_THREEVEC_TEST(urshl_2s_2s_2s,   "urshl v29.2s, v28.2s, v27.2s", 29,28,27)
GEN_THREEVEC_TEST(urshl_8h_8h_8h,   "urshl v29.8h, v28.8h, v27.8h", 29,28,27)
GEN_THREEVEC_TEST(urshl_4h_4h_4h,   "urshl v29.4h, v28.4h, v27.4h", 29,28,27)
GEN_THREEVEC_TEST(urshl_16b_16b_16b,"urshl v29.16b, v28.16b, v27.16b", 29,28,27)
GEN_THREEVEC_TEST(urshl_8b_8b_8b,   "urshl v29.8b, v28.8b, v27.8b", 29,28,27)

GEN_TWOVEC_TEST(srshr_d_d_1,  "srshr d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(srshr_d_d_32, "srshr d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(srshr_d_d_64, "srshr d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(urshr_d_d_1,  "urshr d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(urshr_d_d_32, "urshr d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(urshr_d_d_64, "urshr d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(srshr_2d_2d_1,   "srshr v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_2d_2d_32,  "srshr v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(srshr_2d_2d_64,  "srshr v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(srshr_4s_4s_1,   "srshr v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_4s_4s_16,  "srshr v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(srshr_4s_4s_32,  "srshr v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(srshr_2s_2s_1,   "srshr v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_2s_2s_16,  "srshr v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(srshr_2s_2s_32,  "srshr v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(srshr_8h_8h_1,   "srshr v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_8h_8h_8,   "srshr v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(srshr_8h_8h_16,  "srshr v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(srshr_4h_4h_1,   "srshr v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_4h_4h_8,   "srshr v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(srshr_4h_4h_16,  "srshr v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(srshr_16b_16b_1, "srshr v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(srshr_16b_16b_3, "srshr v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(srshr_16b_16b_8, "srshr v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(srshr_8b_8b_1,   "srshr v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(srshr_8b_8b_3,   "srshr v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(srshr_8b_8b_8,   "srshr v6.8b,  v27.8b, #8",  6, 27)

GEN_TWOVEC_TEST(urshr_2d_2d_1,   "urshr v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_2d_2d_32,  "urshr v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(urshr_2d_2d_64,  "urshr v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(urshr_4s_4s_1,   "urshr v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_4s_4s_16,  "urshr v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(urshr_4s_4s_32,  "urshr v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(urshr_2s_2s_1,   "urshr v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_2s_2s_16,  "urshr v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(urshr_2s_2s_32,  "urshr v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(urshr_8h_8h_1,   "urshr v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_8h_8h_8,   "urshr v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(urshr_8h_8h_16,  "urshr v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(urshr_4h_4h_1,   "urshr v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_4h_4h_8,   "urshr v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(urshr_4h_4h_16,  "urshr v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(urshr_16b_16b_1, "urshr v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(urshr_16b_16b_3, "urshr v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(urshr_16b_16b_8, "urshr v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(urshr_8b_8b_1,   "urshr v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(urshr_8b_8b_3,   "urshr v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(urshr_8b_8b_8,   "urshr v6.8b,  v27.8b, #8",  6, 27)

GEN_TWOVEC_TEST(srsra_d_d_1,  "srsra d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(srsra_d_d_32, "srsra d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(srsra_d_d_64, "srsra d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(ursra_d_d_1,  "ursra d5, d28, #1",  5, 28)
GEN_TWOVEC_TEST(ursra_d_d_32, "ursra d5, d28, #32", 5, 28)
GEN_TWOVEC_TEST(ursra_d_d_64, "ursra d5, d28, #64", 5, 28)

GEN_TWOVEC_TEST(srsra_2d_2d_1,   "srsra v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_2d_2d_32,  "srsra v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(srsra_2d_2d_64,  "srsra v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(srsra_4s_4s_1,   "srsra v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_4s_4s_16,  "srsra v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(srsra_4s_4s_32,  "srsra v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(srsra_2s_2s_1,   "srsra v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_2s_2s_16,  "srsra v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(srsra_2s_2s_32,  "srsra v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(srsra_8h_8h_1,   "srsra v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_8h_8h_8,   "srsra v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(srsra_8h_8h_16,  "srsra v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(srsra_4h_4h_1,   "srsra v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_4h_4h_8,   "srsra v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(srsra_4h_4h_16,  "srsra v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(srsra_16b_16b_1, "srsra v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(srsra_16b_16b_3, "srsra v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(srsra_16b_16b_8, "srsra v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(srsra_8b_8b_1,   "srsra v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(srsra_8b_8b_3,   "srsra v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(srsra_8b_8b_8,   "srsra v6.8b,  v27.8b, #8",  6, 27)

GEN_TWOVEC_TEST(ursra_2d_2d_1,   "ursra v6.2d,  v27.2d, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_2d_2d_32,  "ursra v6.2d,  v27.2d, #32", 6, 27)
GEN_TWOVEC_TEST(ursra_2d_2d_64,  "ursra v6.2d,  v27.2d, #64", 6, 27)
GEN_TWOVEC_TEST(ursra_4s_4s_1,   "ursra v6.4s,  v27.4s, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_4s_4s_16,  "ursra v6.4s,  v27.4s, #16", 6, 27)
GEN_TWOVEC_TEST(ursra_4s_4s_32,  "ursra v6.4s,  v27.4s, #32", 6, 27)
GEN_TWOVEC_TEST(ursra_2s_2s_1,   "ursra v6.2s,  v27.2s, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_2s_2s_16,  "ursra v6.2s,  v27.2s, #16", 6, 27)
GEN_TWOVEC_TEST(ursra_2s_2s_32,  "ursra v6.2s,  v27.2s, #32", 6, 27)
GEN_TWOVEC_TEST(ursra_8h_8h_1,   "ursra v6.8h,  v27.8h, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_8h_8h_8,   "ursra v6.8h,  v27.8h, #8",  6, 27)
GEN_TWOVEC_TEST(ursra_8h_8h_16,  "ursra v6.8h,  v27.8h, #16", 6, 27)
GEN_TWOVEC_TEST(ursra_4h_4h_1,   "ursra v6.4h,  v27.4h, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_4h_4h_8,   "ursra v6.4h,  v27.4h, #8",  6, 27)
GEN_TWOVEC_TEST(ursra_4h_4h_16,  "ursra v6.4h,  v27.4h, #16", 6, 27)
GEN_TWOVEC_TEST(ursra_16b_16b_1, "ursra v6.16b, v27.16b, #1", 6, 27)
GEN_TWOVEC_TEST(ursra_16b_16b_3, "ursra v6.16b, v27.16b, #3", 6, 27)
GEN_TWOVEC_TEST(ursra_16b_16b_8, "ursra v6.16b, v27.16b, #8", 6, 27)
GEN_TWOVEC_TEST(ursra_8b_8b_1,   "ursra v6.8b,  v27.8b, #1",  6, 27)
GEN_TWOVEC_TEST(ursra_8b_8b_3,   "ursra v6.8b,  v27.8b, #3",  6, 27)
GEN_TWOVEC_TEST(ursra_8b_8b_8,   "ursra v6.8b,  v27.8b, #8",  6, 27)

GEN_TWOVEC_TEST(suqadd_d_d,  "suqadd d22, d23",   22, 23)
GEN_TWOVEC_TEST(suqadd_s_s,  "suqadd s22, s23",   22, 23)
GEN_TWOVEC_TEST(suqadd_h_h,  "suqadd h22, h23",   22, 23)
GEN_TWOVEC_TEST(suqadd_b_b,  "suqadd b22, b23",   22, 23)

GEN_TWOVEC_TEST(suqadd_2d_2d,   "suqadd v6.2d,  v27.2d",  6, 27)
GEN_TWOVEC_TEST(suqadd_4s_4s,   "suqadd v6.4s,  v27.4s",  6, 27)
GEN_TWOVEC_TEST(suqadd_2s_2s,   "suqadd v6.2s,  v27.2s",  6, 27)
GEN_TWOVEC_TEST(suqadd_8h_8h,   "suqadd v6.8h,  v27.8h",  6, 27)
GEN_TWOVEC_TEST(suqadd_4h_4h,   "suqadd v6.4h,  v27.4h",  6, 27)
GEN_TWOVEC_TEST(suqadd_16b_16b, "suqadd v6.16b, v27.16b", 6, 27)
GEN_TWOVEC_TEST(suqadd_8b_8b,   "suqadd v6.8b,  v27.8b",  6, 27)

GEN_TWOVEC_TEST(usqadd_d_d,  "usqadd d22, d23",   22, 23)
GEN_TWOVEC_TEST(usqadd_s_s,  "usqadd s22, s23",   22, 23)
GEN_TWOVEC_TEST(usqadd_h_h,  "usqadd h22, h23",   22, 23)
GEN_TWOVEC_TEST(usqadd_b_b,  "usqadd b22, b23",   22, 23)

GEN_TWOVEC_TEST(usqadd_2d_2d,   "usqadd v6.2d,  v27.2d",  6, 27)
GEN_TWOVEC_TEST(usqadd_4s_4s,   "usqadd v6.4s,  v27.4s",  6, 27)
GEN_TWOVEC_TEST(usqadd_2s_2s,   "usqadd v6.2s,  v27.2s",  6, 27)
GEN_TWOVEC_TEST(usqadd_8h_8h,   "usqadd v6.8h,  v27.8h",  6, 27)
GEN_TWOVEC_TEST(usqadd_4h_4h,   "usqadd v6.4h,  v27.4h",  6, 27)
GEN_TWOVEC_TEST(usqadd_16b_16b, "usqadd v6.16b, v27.16b", 6, 27)
GEN_TWOVEC_TEST(usqadd_8b_8b,   "usqadd v6.8b,  v27.8b",  6, 27)

GEN_THREEVEC_TEST(trn1_2d_2d_2d,    "trn1 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(trn1_4s_4s_4s,    "trn1 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(trn1_2s_2s_2s,    "trn1 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(trn1_8h_8h_8h,    "trn1 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(trn1_4h_4h_4h,    "trn1 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(trn1_16b_16b_16b, "trn1 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(trn1_8b_8b_8b,    "trn1 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(trn2_2d_2d_2d,    "trn2 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(trn2_4s_4s_4s,    "trn2 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(trn2_2s_2s_2s,    "trn2 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(trn2_8h_8h_8h,    "trn2 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(trn2_4h_4h_4h,    "trn2 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(trn2_16b_16b_16b, "trn2 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(trn2_8b_8b_8b,    "trn2 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uzp1_2d_2d_2d,    "uzp1 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uzp1_4s_4s_4s,    "uzp1 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uzp1_2s_2s_2s,    "uzp1 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uzp1_8h_8h_8h,    "uzp1 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uzp1_4h_4h_4h,    "uzp1 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uzp1_16b_16b_16b, "uzp1 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uzp1_8b_8b_8b,    "uzp1 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(uzp2_2d_2d_2d,    "uzp2 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(uzp2_4s_4s_4s,    "uzp2 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(uzp2_2s_2s_2s,    "uzp2 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(uzp2_8h_8h_8h,    "uzp2 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(uzp2_4h_4h_4h,    "uzp2 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(uzp2_16b_16b_16b, "uzp2 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(uzp2_8b_8b_8b,    "uzp2 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(zip1_2d_2d_2d,    "zip1 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(zip1_4s_4s_4s,    "zip1 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(zip1_2s_2s_2s,    "zip1 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(zip1_8h_8h_8h,    "zip1 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(zip1_4h_4h_4h,    "zip1 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(zip1_16b_16b_16b, "zip1 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(zip1_8b_8b_8b,    "zip1 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_THREEVEC_TEST(zip2_2d_2d_2d,    "zip2 v1.2d,  v2.2d,  v4.2d",  1, 2, 4)
GEN_THREEVEC_TEST(zip2_4s_4s_4s,    "zip2 v1.4s,  v2.4s,  v4.4s",  1, 2, 4)
GEN_THREEVEC_TEST(zip2_2s_2s_2s,    "zip2 v1.2s,  v2.2s,  v4.2s",  1, 2, 4)
GEN_THREEVEC_TEST(zip2_8h_8h_8h,    "zip2 v1.8h,  v2.8h,  v4.8h",  1, 2, 4)
GEN_THREEVEC_TEST(zip2_4h_4h_4h,    "zip2 v1.4h,  v2.4h,  v4.4h",  1, 2, 4)
GEN_THREEVEC_TEST(zip2_16b_16b_16b, "zip2 v1.16b, v2.16b, v4.16b", 1, 2, 4)
GEN_THREEVEC_TEST(zip2_8b_8b_8b,    "zip2 v1.8b,  v2.8b,  v4.8b",  1, 2, 4)

GEN_TWOVEC_TEST(urecpe_4s_4s,   "urecpe v6.4s,  v27.4s",  6, 27)
GEN_TWOVEC_TEST(urecpe_2s_2s,   "urecpe v6.2s,  v27.2s",  6, 27)

GEN_TWOVEC_TEST(ursqrte_4s_4s,   "ursqrte v6.4s,  v27.4s",  6, 27)
GEN_TWOVEC_TEST(ursqrte_2s_2s,   "ursqrte v6.2s,  v27.2s",  6, 27)


/* ---------------------------------------------------------------- */
/* -- main()                                                     -- */
/* ---------------------------------------------------------------- */

int main ( void )
{
   assert(sizeof(V128) == 16);

   // ======================== FP ========================

   // fabs      d,s
   // fabs      2d,4s,2s
   test_fabs_d_d(TyDF);
   test_fabs_s_s(TySF);
   test_fabs_2d_2d(TyDF);
   test_fabs_4s_4s(TySF);
   test_fabs_2s_2s(TyDF);
   test_fneg_2d_2d(TySF);
   test_fneg_4s_4s(TyDF);
   test_fneg_2s_2s(TySF);

   // fneg      d,s
   // fneg      2d,4s,2s
   test_fneg_d_d(TyDF);
   test_fneg_s_s(TySF);

   // fsqrt     d,s
   // fsqrt     2d,4s,2s
   test_fsqrt_d_d(TyDF);
   test_fsqrt_s_s(TySF);

   // fadd      d,s
   // fsub      d,s
   test_fadd_d_d_d(TyDF);
   test_fadd_s_s_s(TySF);
   test_fsub_d_d_d(TyDF);
   test_fsub_s_s_s(TySF);

   // fadd      2d,4s,2s
   // fsub      2d,4s,2s
   test_fadd_2d_2d_2d(TyDF);
   test_fadd_4s_4s_4s(TySF);
   test_fadd_2s_2s_2s(TySF);
   test_fsub_2d_2d_2d(TyDF);
   test_fsub_4s_4s_4s(TySF);
   test_fsub_2s_2s_2s(TySF);

   // fabd      d,s
   // fabd      2d,4s,2s
   test_fabd_d_d_d(TyDF);
   test_fabd_s_s_s(TySF);
   test_fabd_2d_2d_2d(TyDF);
   test_fabd_4s_4s_4s(TySF);
   test_fabd_2s_2s_2s(TySF);

   // faddp     d,s (floating add pair)
   // faddp     2d,4s,2s

   // fccmp     d,s (floating point conditional quiet compare)
   // fccmpe    d,s (floating point conditional signaling compare)

   // fcmeq     d,s
   // fcmge     d,s
   // fcmgt     d,s
   // facgt     d,s  (floating abs compare GE)
   // facge     d,s  (floating abs compare GE)

   // fcmeq     2d,4s,2s
   // fcmge     2d,4s,2s
   // fcmgt     2d,4s,2s
   // facge     2d,4s,2s
   // facgt     2d,4s,2s
   test_fcmeq_2d_2d_2d(TyDF);
   test_fcmeq_4s_4s_4s(TySF);
   test_fcmeq_2s_2s_2s(TySF);
   test_fcmge_2d_2d_2d(TyDF);
   test_fcmge_4s_4s_4s(TySF);
   test_fcmge_2s_2s_2s(TySF);
   test_fcmgt_2d_2d_2d(TyDF);
   test_fcmgt_4s_4s_4s(TySF);
   test_fcmgt_2s_2s_2s(TySF);
   test_facge_2d_2d_2d(TyDF);
   test_facge_4s_4s_4s(TySF);
   test_facge_2s_2s_2s(TySF);
   test_facgt_2d_2d_2d(TyDF);
   test_facgt_4s_4s_4s(TySF);
   test_facgt_2s_2s_2s(TySF);

   // fcmeq_z   d,s
   // fcmge_z   d,s
   // fcmgt_z   d,s
   // fcmle_z   d,s
   // fcmlt_z   d,s

   // fcmeq_z   2d,4s,2s
   // fcmge_z   2d,4s,2s
   // fcmgt_z   2d,4s,2s
   // fcmle_z   2d,4s,2s
   // fcmlt_z   2d,4s,2s

   // fcmp_z    d,s
   // fcmpe_z   d,s
   // fcmp      d,s (floating point quiet, set flags)
   // fcmpe     d,s (floating point signaling, set flags)

   // fcsel     d,s (fp cond select)

   // fdiv      d,s
   // fdiv      2d,4s,2s
   test_fdiv_d_d_d(TyDF);
   test_fdiv_s_s_s(TySF);
   test_fdiv_2d_2d_2d(TyDF);
   test_fdiv_4s_4s_4s(TySF);
   test_fdiv_2s_2s_2s(TySF);

   // fmadd     d,s
   // fnmadd    d,s
   // fmsub     d,s
   // fnmsub    d,s

   // fnmul     d,s
   test_fnmul_d_d_d(TyDF);
   test_fnmul_s_s_s(TySF);

   // fmax      d,s
   // fmin      d,s
   // fmaxnm    d,s ("max number")
   // fminnm    d,s

   // fmax      2d,4s,2s
   // fmin      2d,4s,2s
   // fmaxnm    2d,4s,2s
   // fminnm    2d,4s,2s

   // fmaxnmp   d_2d,s_2s ("max number pairwise")
   // fminnmp   d_2d,s_2s

   // fmaxnmp   2d,4s,2s
   // fminnmp   2d,4s,2s

   // fmaxnmv   s_4s (maxnum across vector)
   // fminnmv   s_4s

   // fmaxp     d_2d,s_2s (max of a pair)
   // fminp     d_2d,s_2s (max of a pair)

   // fmaxp     2d,4s,2s  (max pairwise)
   // fminp     2d,4s,2s

   // fmaxv     s_4s (max across vector)
   // fminv     s_4s

   // fmla      2d,4s,2s
   // fmls      2d,4s,2s
   test_fmla_2d_2d_2d(TyDF);
   test_fmla_4s_4s_4s(TySF);
   test_fmla_2s_2s_2s(TySF);
   test_fmls_2d_2d_2d(TyDF);
   test_fmls_4s_4s_4s(TySF);
   test_fmls_2s_2s_2s(TySF);

   // fmla      d_d_d[],s_s_s[] (by element)
   // fmls      d_d_d[],s_s_s[] (by element)

   // fmla      2d_2d_d[],4s_4s_s[],2s_2s_s[]
   // fmls      2d_2d_d[],4s_4s_s[],2s_2s_s[]

   // fmov      2d,4s,2s #imm (part of the MOVI/MVNI/ORR/BIC imm group)
   // INCOMPLETE
   test_fmov_2d_imm_01(TyD);
   test_fmov_2d_imm_02(TyD);
   test_fmov_2d_imm_03(TyD);
   if (0) test_fmov_4s_imm_01(TyS);
   if (0) test_fmov_4s_imm_02(TyS);
   if (0) test_fmov_4s_imm_03(TyS);
   if (0) test_fmov_2s_imm_01(TyS);
   if (0) test_fmov_2s_imm_02(TyS);
   if (0) test_fmov_2s_imm_03(TyS);

   // fmov      d_d,s_s
   test_fmov_d_d(TyDF);
   test_fmov_s_s(TySF);

   // fmov      s_w,w_s,d_x,d[1]_x,x_d,x_d[1]
   test_fmov_s_w(TyS);
   test_fmov_d_x(TyD);
   test_fmov_d1_x(TyD);
   test_fmov_w_s(TyS);
   test_fmov_x_d(TyD);
   test_fmov_x_d1(TyD);

   // fmov      d,s #imm
   test_fmov_d_imm_01(TyNONE);
   test_fmov_d_imm_02(TyNONE);
   test_fmov_d_imm_03(TyNONE);
   test_fmov_s_imm_01(TyNONE);
   test_fmov_s_imm_02(TyNONE);
   test_fmov_s_imm_03(TyNONE);

   // fmul      d_d_d[],s_s_s[]
   // fmul      2d_2d_d[],4s_4s_s[],2s_2s_s[]

   // fmul      2d,4s,2s
   // fmul      d,s
   test_fmul_d_d_d(TyDF);
   test_fmul_s_s_s(TySF);
   test_fmul_2d_2d_2d(TyDF);
   test_fmul_4s_4s_4s(TySF);
   test_fmul_2s_2s_2s(TySF);

   // fmulx     d_d_d[],s_s_s[]
   // fmulx     2d_2d_d[],4s_4s_s[],2s_2s_s[]

   // fmulx     d,s
   // fmulx     2d,4s,2s

   // frecpe    d,s (recip estimate)
   // frecpe    2d,4s,2s

   // frecps    d,s (recip step)
   // frecps    2d,4s,2s

   // frecpx    d,s (recip exponent)

   // frinta    d,s
   // frinti    d,s
   // frintm    d,s
   // frintn    d,s
   // frintp    d,s
   // frintx    d,s
   // frintz    d,s

   // frinta    2d,4s,2s (round to integral, nearest away)
   // frinti    2d,4s,2s (round to integral, per FPCR)
   // frintm    2d,4s,2s (round to integral, minus inf)
   // frintn    2d,4s,2s (round to integral, nearest, to even)
   // frintp    2d,4s,2s (round to integral, plus inf)
   // frintx    2d,4s,2s (round to integral exact, per FPCR)
   // frintz    2d,4s,2s (round to integral, zero)

   // frsqrte   d,s (est)
   // frsqrte   2d,4s,2s

   // frsqrts   d,s (step)
   // frsqrts   2d,4s,2s

   // ======================== CONV ========================

   // fcvt      s_h,d_h,h_s,d_s,h_d,s_d (fp convert, scalar)

   // fcvtl{2}  4s/4h, 4s/8h, 2d/2s, 2d/4s (float convert to longer form)

   // fcvtn{2}  4h/4s, 8h/4s, 2s/2d, 4s/2d (float convert to narrower form)
   // INCOMPLETE
   test_fcvtn_2s_2d(TyDF);
   test_fcvtn_4s_2d(TyDF);

   // fcvtas    d,s  (fcvt to signed int,   nearest, ties away)
   // fcvtau    d,s  (fcvt to unsigned int, nearest, ties away)
   // fcvtas    2d,4s,2s
   // fcvtau    2d,4s,2s
   // fcvtas    w_s,x_s,w_d,x_d
   // fcvtau    w_s,x_s,w_d,x_d

   // fcvtms    d,s  (fcvt to signed int,   minus inf)
   // fcvtmu    d,s  (fcvt to unsigned int, minus inf)
   // fcvtms    2d,4s,2s
   // fcvtmu    2d,4s,2s
   // fcvtms    w_s,x_s,w_d,x_d
   // fcvtmu    w_s,x_s,w_d,x_d

   // fcvtns    d,s  (fcvt to signed int,   nearest)
   // fcvtnu    d,s  (fcvt to unsigned int, nearest)
   // fcvtns    2d,4s,2s
   // fcvtnu    2d,4s,2s
   // fcvtns    w_s,x_s,w_d,x_d
   // fcvtnu    w_s,x_s,w_d,x_d

   // fcvtps    d,s  (fcvt to signed int,   plus inf)
   // fcvtpu    d,s  (fcvt to unsigned int, plus inf)
   // fcvtps    2d,4s,2s
   // fcvtpu    2d,4s,2s
   // fcvtps    w_s,x_s,w_d,x_d
   // fcvtpu    w_s,x_s,w_d,x_d

   // fcvtzs    d,s (fcvt to signed integer,   to zero)
   // fcvtzu    d,s (fcvt to unsigned integer, to zero)
   // fcvtzs    2d,4s,2s
   // fcvtzu    2d,4s,2s
   // fcvtzs    w_s,x_s,w_d,x_d
   // fcvtzu    w_s,x_s,w_d,x_d

   // fcvtzs    d,s (fcvt to signed fixedpt,   to zero) (w/ #fbits)
   // fcvtzu    d,s (fcvt to unsigned fixedpt, to zero) (w/ #fbits)
   // fcvtzs    2d,4s,2s
   // fcvtzu    2d,4s,2s
   // fcvtzs    w_s,x_s,w_d,x_d (fcvt to signed fixedpt,   to zero) (w/ #fbits)
   // fcvtzu    w_s,x_s,w_d,x_d (fcvt to unsigned fixedpt, to zero) (w/ #fbits)

   // fcvtxn    s_d (fcvt to lower prec narrow, rounding to odd)
   // fcvtxn    2s_2d,4s_2d

   // scvtf     d,s        _#fbits
   // ucvtf     d,s        _#fbits

   // scvtf     2d,4s,2s   _#fbits
   // ucvtf     2d,4s,2s   _#fbits

   // scvtf     d,s
   // ucvtf     d,s

   // scvtf     2d,4s,2s
   // ucvtf     2d,4s,2s

   // scvtf     s_w, d_w, s_x, d_x,   _#fbits
   // ucvtf     s_w, d_w, s_x, d_x,   _#fbits

   // scvtf     s_w, d_w, s_x, d_x
   // ucvtf     s_w, d_w, s_x, d_x
   test_scvtf_s_w(TyS);
   test_scvtf_d_w(TyS);
   test_scvtf_s_x(TyD);
   test_scvtf_d_x(TyD);
   test_ucvtf_s_w(TyS);
   test_ucvtf_d_w(TyS);
   test_ucvtf_s_x(TyD);
   test_ucvtf_d_x(TyD);

   // ======================== INT ========================

   // abs       d
   // neg       d
   test_abs_d_d(TyD);
   test_neg_d_d(TyD);

   // abs       2d,4s,2s,8h,4h,16b,8b
   // neg       2d,4s,2s,8h,4h,16b,8b
   test_abs_2d_2d(TyD);
   test_abs_4s_4s(TyS);
   test_abs_2s_2s(TyS);
   test_abs_8h_8h(TyH);
   test_abs_4h_4h(TyH);
   test_abs_16b_16b(TyB);
   test_abs_8b_8b(TyB);
   test_neg_2d_2d(TyD);
   test_neg_4s_4s(TyS);
   test_neg_2s_2s(TyS);
   test_neg_8h_8h(TyH);
   test_neg_4h_4h(TyH);
   test_neg_16b_16b(TyB);
   test_neg_8b_8b(TyB);

   // add       d
   // sub       d
   test_add_d_d_d(TyD);
   test_sub_d_d_d(TyD);

   // add       2d,4s,2s,8h,4h,16b,8b
   // sub       2d,4s,2s,8h,4h,16b,8b
   test_add_2d_2d_2d(TyD);
   test_add_4s_4s_4s(TyS);
   test_add_2s_2s_2s(TyS);
   test_add_8h_8h_8h(TyH);
   test_add_4h_4h_4h(TyH);
   test_add_16b_16b_16b(TyB);
   test_add_8b_8b_8b(TyB);
   test_sub_2d_2d_2d(TyD);
   test_sub_4s_4s_4s(TyS);
   test_sub_2s_2s_2s(TyS);
   test_sub_8h_8h_8h(TyH);
   test_sub_4h_4h_4h(TyH);
   test_sub_16b_16b_16b(TyB);
   test_sub_8b_8b_8b(TyB);

   // addhn{2}   2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   // subhn{2}   2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   // raddhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   // rsubhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   test_addhn_2s_2d_2d(TyD);
   test_addhn2_4s_2d_2d(TyD);
   test_addhn_4h_4s_4s(TyS);
   test_addhn2_8h_4s_4s(TyS);
   test_addhn_8b_8h_8h(TyH);
   test_addhn2_16b_8h_8h(TyH);
   test_subhn_2s_2d_2d(TyD);
   test_subhn2_4s_2d_2d(TyD);
   test_subhn_4h_4s_4s(TyS);
   test_subhn2_8h_4s_4s(TyS);
   test_subhn_8b_8h_8h(TyH);
   test_subhn2_16b_8h_8h(TyH);
   test_raddhn_2s_2d_2d(TyD);
   test_raddhn2_4s_2d_2d(TyD);
   test_raddhn_4h_4s_4s(TyS);
   test_raddhn2_8h_4s_4s(TyS);
   test_raddhn_8b_8h_8h(TyH);
   test_raddhn2_16b_8h_8h(TyH);
   test_rsubhn_2s_2d_2d(TyD);
   test_rsubhn2_4s_2d_2d(TyD);
   test_rsubhn_4h_4s_4s(TyS);
   test_rsubhn2_8h_4s_4s(TyS);
   test_rsubhn_8b_8h_8h(TyH);
   test_rsubhn2_16b_8h_8h(TyH);

   // addp     d (add pairs, across)
   test_addp_d_2d(TyD);

   // addp     2d,4s,2s,8h,4h,16b,8b
   test_addp_2d_2d_2d(TyD);
   test_addp_4s_4s_4s(TyS);
   test_addp_2s_2s_2s(TyS);
   test_addp_8h_8h_8h(TyH);
   test_addp_4h_4h_4h(TyH);
   test_addp_16b_16b_16b(TyB);
   test_addp_8b_8b_8b(TyB);

   // addv     4s,8h,4h,16b,18b (reduce across vector)
   test_addv_s_4s(TyS);
   test_addv_h_8h(TyH);
   test_addv_h_4h(TyH);
   test_addv_b_16b(TyB);
   test_addv_b_8b(TyB);

   // and      16b,8b
   // bic      16b,8b
   // orn      16b,8b
   // orr      16b,8b
   test_and_16b_16b_16b(TyB);
   test_and_8b_8b_8b(TyB);
   test_bic_16b_16b_16b(TyB);
   test_bic_8b_8b_8b(TyB);
   test_orr_16b_16b_16b(TyB);
   test_orr_8b_8b_8b(TyB);
   test_orn_16b_16b_16b(TyB);
   test_orn_8b_8b_8b(TyB);

   // orr      8h,4h   #imm8, LSL #0 or 8
   // orr      4s,2s   #imm8, LSL #0, 8, 16 or 24
   // bic      8h,4h   #imm8, LSL #0 or 8
   // bic      4s,2s   #imm8, LSL #0, 8, 16 or 24
   // movi and mvni are very similar, a superset of these.
   // Cases are below.
   test_orr_8h_0x5A_lsl0(TyH);
   test_orr_8h_0xA5_lsl8(TyH);
   test_orr_4h_0x5A_lsl0(TyH);
   test_orr_4h_0xA5_lsl8(TyH);
   test_orr_4s_0x5A_lsl0(TyS);
   test_orr_4s_0x6B_lsl8(TyS);
   test_orr_4s_0x49_lsl16(TyS);
   test_orr_4s_0x3D_lsl24(TyS);
   test_orr_2s_0x5A_lsl0(TyS);
   test_orr_2s_0x6B_lsl8(TyS);
   test_orr_2s_0x49_lsl16(TyS);
   test_orr_2s_0x3D_lsl24(TyS);
   test_bic_8h_0x5A_lsl0(TyH);
   test_bic_8h_0xA5_lsl8(TyH);
   test_bic_4h_0x5A_lsl0(TyH);
   test_bic_4h_0xA5_lsl8(TyH);
   test_bic_4s_0x5A_lsl0(TyS);
   test_bic_4s_0x6B_lsl8(TyS);
   test_bic_4s_0x49_lsl16(TyS);
   test_bic_4s_0x3D_lsl24(TyS);
   test_bic_2s_0x5A_lsl0(TyS);
   test_bic_2s_0x6B_lsl8(TyS);
   test_bic_2s_0x49_lsl16(TyS);
   test_bic_2s_0x3D_lsl24(TyS);

   // bif      16b,8b (vector) (bit insert if false)
   // bit      16b,8b (vector) (bit insert if true)
   // bsl      16b,8b (vector) (bit select)
   // eor      16b,8b (vector)
   test_bif_16b_16b_16b(TyB);
   test_bif_8b_8b_8b(TyB);
   test_bit_16b_16b_16b(TyB);
   test_bit_8b_8b_8b(TyB);
   test_bsl_16b_16b_16b(TyB);
   test_bsl_8b_8b_8b(TyB);
   test_eor_16b_16b_16b(TyB);
   test_eor_8b_8b_8b(TyB);

   // cls      4s,2s,8h,4h,16b,8b (count leading sign bits)
   // clz      4s,2s,8h,4h,16b,8b (count leading zero bits)
   test_cls_4s_4s(TyS);
   test_cls_2s_2s(TyS);
   test_cls_8h_8h(TyH);
   test_cls_4h_4h(TyH);
   test_cls_16b_16b(TyB);
   test_cls_8b_8b(TyB);
   test_clz_4s_4s(TyS);
   test_clz_2s_2s(TyS);
   test_clz_8h_8h(TyH);
   test_clz_4h_4h(TyH);
   test_clz_16b_16b(TyB);
   test_clz_8b_8b(TyB);

   // cmeq     d
   // cmge     d
   // cmgt     d
   // cmhi     d
   // cmhs     d
   // cmtst    d
   test_cmeq_d_d_d(TyD);
   test_cmge_d_d_d(TyD);
   test_cmgt_d_d_d(TyD);
   test_cmhi_d_d_d(TyD);
   test_cmhs_d_d_d(TyD);
   test_cmtst_d_d_d(TyD);

   // cmeq     2d,4s,2s,8h,4h,16b,8b
   // cmge     2d,4s,2s,8h,4h,16b,8b
   // cmgt     2d,4s,2s,8h,4h,16b,8b
   // cmhi     2d,4s,2s,8h,4h,16b,8b
   // cmhs     2d,4s,2s,8h,4h,16b,8b
   // cmtst    2d,4s,2s,8h,4h,16b,8b
   test_cmeq_2d_2d_2d(TyD);
   test_cmeq_4s_4s_4s(TyS);
   test_cmeq_2s_2s_2s(TyS);
   test_cmeq_8h_8h_8h(TyH);
   test_cmeq_4h_4h_4h(TyH);
   test_cmeq_16b_16b_16b(TyB);
   test_cmeq_8b_8b_8b(TyB);
   test_cmge_2d_2d_2d(TyD);
   test_cmge_4s_4s_4s(TyS);
   test_cmge_2s_2s_2s(TyS);
   test_cmge_8h_8h_8h(TyH);
   test_cmge_4h_4h_4h(TyH);
   test_cmge_16b_16b_16b(TyB);
   test_cmge_8b_8b_8b(TyB);
   test_cmgt_2d_2d_2d(TyD);
   test_cmgt_4s_4s_4s(TyS);
   test_cmgt_2s_2s_2s(TyS);
   test_cmgt_8h_8h_8h(TyH);
   test_cmgt_4h_4h_4h(TyH);
   test_cmgt_16b_16b_16b(TyB);
   test_cmgt_8b_8b_8b(TyB);
   test_cmhi_2d_2d_2d(TyD);
   test_cmhi_4s_4s_4s(TyS);
   test_cmhi_2s_2s_2s(TyS);
   test_cmhi_8h_8h_8h(TyH);
   test_cmhi_4h_4h_4h(TyH);
   test_cmhi_16b_16b_16b(TyB);
   test_cmhi_8b_8b_8b(TyB);
   test_cmhs_2d_2d_2d(TyD);
   test_cmhs_4s_4s_4s(TyS);
   test_cmhs_2s_2s_2s(TyS);
   test_cmhs_8h_8h_8h(TyH);
   test_cmhs_4h_4h_4h(TyH);
   test_cmhs_16b_16b_16b(TyB);
   test_cmhs_8b_8b_8b(TyB);
   test_cmtst_2d_2d_2d(TyD);
   test_cmtst_4s_4s_4s(TyS);
   test_cmtst_2s_2s_2s(TyS);
   test_cmtst_8h_8h_8h(TyH);
   test_cmtst_4h_4h_4h(TyH);
   test_cmtst_16b_16b_16b(TyB);
   test_cmtst_8b_8b_8b(TyB);

   // cmeq_z   d
   // cmge_z   d
   // cmgt_z   d
   // cmle_z   d
   // cmlt_z   d
   test_cmeq_zero_d_d(TyD);
   test_cmge_zero_d_d(TyD);
   test_cmgt_zero_d_d(TyD);
   test_cmle_zero_d_d(TyD);
   test_cmlt_zero_d_d(TyD);

   // cmeq_z   2d,4s,2s,8h,4h,16b,8b
   // cmge_z   2d,4s,2s,8h,4h,16b,8b
   // cmgt_z   2d,4s,2s,8h,4h,16b,8b
   // cmle_z   2d,4s,2s,8h,4h,16b,8b
   // cmlt_z   2d,4s,2s,8h,4h,16b,8b
   test_cmeq_zero_2d_2d(TyD);
   test_cmeq_zero_4s_4s(TyS);
   test_cmeq_zero_2s_2s(TyS);
   test_cmeq_zero_8h_8h(TyH);
   test_cmeq_zero_4h_4h(TyH);
   test_cmeq_zero_16b_16b(TyB);
   test_cmeq_zero_8b_8b(TyB);
   test_cmge_zero_2d_2d(TyD);
   test_cmge_zero_4s_4s(TyS);
   test_cmge_zero_2s_2s(TyS);
   test_cmge_zero_8h_8h(TyH);
   test_cmge_zero_4h_4h(TyH);
   test_cmge_zero_16b_16b(TyB);
   test_cmge_zero_8b_8b(TyB);
   test_cmgt_zero_2d_2d(TyD);
   test_cmgt_zero_4s_4s(TyS);
   test_cmgt_zero_2s_2s(TyS);
   test_cmgt_zero_8h_8h(TyH);
   test_cmgt_zero_4h_4h(TyH);
   test_cmgt_zero_16b_16b(TyB);
   test_cmgt_zero_8b_8b(TyB);
   test_cmle_zero_2d_2d(TyD);
   test_cmle_zero_4s_4s(TyS);
   test_cmle_zero_2s_2s(TyS);
   test_cmle_zero_8h_8h(TyH);
   test_cmle_zero_4h_4h(TyH);
   test_cmle_zero_16b_16b(TyB);
   test_cmle_zero_8b_8b(TyB);
   test_cmlt_zero_2d_2d(TyD);
   test_cmlt_zero_4s_4s(TyS);
   test_cmlt_zero_2s_2s(TyS);
   test_cmlt_zero_8h_8h(TyH);
   test_cmlt_zero_4h_4h(TyH);
   test_cmlt_zero_16b_16b(TyB);
   test_cmlt_zero_8b_8b(TyB);

   // cnt      16b,8b (population count per byte)
   test_cnt_16b_16b(TyB);
   test_cnt_8b_8b(TyB);

   // dup      d,s,h,b (vec elem to scalar)
   test_dup_d_d0(TyD);
   test_dup_d_d1(TyD);
   test_dup_s_s0(TyS);
   test_dup_s_s3(TyS);
   test_dup_h_h0(TyH);
   test_dup_h_h6(TyH);
   test_dup_b_b0(TyB);
   test_dup_b_b13(TyB);

   // dup      2d,4s,2s,8h,4h,16b,8b (vec elem to vector)
   test_dup_2d_d0(TyD);
   test_dup_2d_d1(TyD);
   test_dup_4s_s0(TyS);
   test_dup_4s_s3(TyS);
   test_dup_2s_s0(TyS);
   test_dup_2s_s2(TyS);
   test_dup_8h_h0(TyH);
   test_dup_8h_h6(TyH);
   test_dup_4h_h1(TyH);
   test_dup_4h_h5(TyH);
   test_dup_16b_b2(TyB);
   test_dup_16b_b12(TyB);
   test_dup_8b_b3(TyB);
   test_dup_8b_b13(TyB);

   // dup      2d,4s,2s,8h,4h,16b,8b (general reg to vector)
   test_dup_2d_x(TyD);
   test_dup_4s_w(TyS);
   test_dup_2s_w(TyS);
   test_dup_8h_w(TyH);
   test_dup_4h_w(TyH);
   test_dup_16b_w(TyB);
   test_dup_8b_w(TyB);

   // ext      16b,8b,#imm4 (concat 2 vectors, then slice)
   test_ext_16b_16b_16b_0x0(TyB);
   test_ext_16b_16b_16b_0x7(TyB);
   test_ext_16b_16b_16b_0x8(TyB);
   test_ext_16b_16b_16b_0x9(TyB);
   test_ext_16b_16b_16b_0xF(TyB);
   test_ext_8b_8b_8b_0x0(TyB);
   test_ext_8b_8b_8b_0x1(TyB);
   test_ext_8b_8b_8b_0x6(TyB);
   test_ext_8b_8b_8b_0x7(TyB);

   // ins      d[]_d[],s[]_s[],h[]_h[],b[]_b[]
   test_ins_d0_d0(TyD);
   test_ins_d0_d1(TyD);
   test_ins_d1_d0(TyD);
   test_ins_d1_d1(TyD);
   test_ins_s0_s2(TyS);
   test_ins_s3_s0(TyS);
   test_ins_s2_s1(TyS);
   test_ins_s1_s3(TyS);
   test_ins_h0_h6(TyH);
   test_ins_h7_h0(TyH);
   test_ins_h6_h1(TyH);
   test_ins_h1_h7(TyH);
   test_ins_b0_b14(TyB);
   test_ins_b15_b8(TyB);
   test_ins_b13_b9(TyB);
   test_ins_b5_b12(TyB);

   // ins      d[]_x, s[]_w, h[]_w, b[]_w
   test_INS_general();

   // mla   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   // mls   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   // mul   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   test_mla_4s_4s_s0(TyS);
   test_mla_4s_4s_s3(TyS);
   test_mla_2s_2s_s0(TyS);
   test_mla_2s_2s_s3(TyS);
   test_mla_8h_8h_h1(TyH);
   test_mla_8h_8h_h5(TyH);
   test_mla_4h_4h_h2(TyH);
   test_mla_4h_4h_h7(TyH);
   test_mls_4s_4s_s0(TyS);
   test_mls_4s_4s_s3(TyS);
   test_mls_2s_2s_s0(TyS);
   test_mls_2s_2s_s3(TyS);
   test_mls_8h_8h_h1(TyH);
   test_mls_8h_8h_h5(TyH);
   test_mls_4h_4h_h2(TyH);
   test_mls_4h_4h_h7(TyH);
   test_mul_4s_4s_s0(TyS);
   test_mul_4s_4s_s3(TyS);
   test_mul_2s_2s_s0(TyS);
   test_mul_2s_2s_s3(TyS);
   test_mul_8h_8h_h1(TyH);
   test_mul_8h_8h_h5(TyH);
   test_mul_4h_4h_h2(TyH);
   test_mul_4h_4h_h7(TyH);

   // mla   4s,2s,8h,4h,16b,8b
   // mls   4s,2s,8h,4h,16b,8b
   // mul   4s,2s,8h,4h,16b,8b
   test_mla_4s_4s_4s(TyS);
   test_mla_2s_2s_2s(TyS);
   test_mla_8h_8h_8h(TyH);
   test_mla_4h_4h_4h(TyH);
   test_mla_16b_16b_16b(TyB);
   test_mla_8b_8b_8b(TyB);
   test_mls_4s_4s_4s(TyS);
   test_mls_2s_2s_2s(TyS);
   test_mls_8h_8h_8h(TyH);
   test_mls_4h_4h_4h(TyH);
   test_mls_16b_16b_16b(TyB);
   test_mls_8b_8b_8b(TyB);
   test_mul_4s_4s_4s(TyS);
   test_mul_2s_2s_2s(TyS);
   test_mul_8h_8h_8h(TyH);
   test_mul_4h_4h_4h(TyH);
   test_mul_16b_16b_16b(TyB);
   test_mul_8b_8b_8b(TyB);

   // Some of these movi and mvni cases are similar to orr and bic
   // cases with immediates.  Maybe they should be moved together.
   // movi  16b,8b   #imm8, LSL #0
   test_movi_16b_0x9C_lsl0(TyB);
   test_movi_8b_0x8B_lsl0(TyB);

   // movi  8h,4h    #imm8, LSL #0 or 8
   // mvni  8h,4h    #imm8, LSL #0 or 8
   test_movi_8h_0x5A_lsl0(TyH);
   test_movi_8h_0xA5_lsl8(TyH);
   test_movi_4h_0x5A_lsl0(TyH);
   test_movi_4h_0xA5_lsl8(TyH);
   test_mvni_8h_0x5A_lsl0(TyH);
   test_mvni_8h_0xA5_lsl8(TyH);
   test_mvni_4h_0x5A_lsl0(TyH);
   test_mvni_4h_0xA5_lsl8(TyH);

   // movi  4s,2s    #imm8, LSL #0, 8, 16, 24
   // mvni  4s,2s    #imm8, LSL #0, 8, 16, 24
   test_movi_4s_0x5A_lsl0(TyS);
   test_movi_4s_0x6B_lsl8(TyS);
   test_movi_4s_0x49_lsl16(TyS);
   test_movi_4s_0x3D_lsl24(TyS);
   test_movi_2s_0x5A_lsl0(TyS);
   test_movi_2s_0x6B_lsl8(TyS);
   test_movi_2s_0x49_lsl16(TyS);
   test_movi_2s_0x3D_lsl24(TyS);
   test_mvni_4s_0x5A_lsl0(TyS);
   test_mvni_4s_0x6B_lsl8(TyS);
   test_mvni_4s_0x49_lsl16(TyS);
   test_mvni_4s_0x3D_lsl24(TyS);
   test_mvni_2s_0x5A_lsl0(TyS);
   test_mvni_2s_0x6B_lsl8(TyS);
   test_mvni_2s_0x49_lsl16(TyS);
   test_mvni_2s_0x3D_lsl24(TyS);

   // movi  4s,2s    #imm8, MSL #8 or 16
   // mvni  4s,2s    #imm8, MSL #8 or 16
   test_movi_4s_0x6B_msl8(TyS);
   test_movi_4s_0x94_msl16(TyS);
   test_movi_2s_0x7A_msl8(TyS);
   test_movi_2s_0xA5_msl16(TyS);
   test_mvni_4s_0x6B_msl8(TyS);
   test_mvni_4s_0x94_msl16(TyS);
   test_mvni_2s_0x7A_msl8(TyS);
   test_mvni_2s_0xA5_msl16(TyS);

   // movi  d,       #imm64
   // movi  2d,      #imm64
   test_movi_d_0xA5(TyD);
   test_movi_2d_0xB4(TyD);

   // not   16b,8b
   test_not_16b_16b(TyB);
   test_not_8b_8b(TyB);

   // pmul  16b,8b
   test_pmul_16b_16b_16b(TyB);
   test_pmul_8b_8b_8b(TyB);

   // pmull{2}  8h_8b_8b,8h_16b_16b,1q_1d_1d,1q_2d_2d
   test_pmull_8h_8b_8b(TyB);
   test_pmull2_8h_16b_16b(TyB);
   //test_pmull_1q_1d_1d(TyD);
   //test_pmull_1q_2d_2d(TyD);

   // rbit    16b,8b
   // rev16   16b,8b
   // rev32   16b,8b,8h,4h
   // rev64   16b,8b,8h,4h,4s,2s
   test_rbit_16b_16b(TyB);
   test_rbit_8b_8b(TyB);
   test_rev16_16b_16b(TyB);
   test_rev16_8b_8b(TyB);
   test_rev32_16b_16b(TyB);
   test_rev32_8b_8b(TyB);
   test_rev32_8h_8h(TyH);
   test_rev32_4h_4h(TyH);
   test_rev64_16b_16b(TyB);
   test_rev64_8b_8b(TyB);
   test_rev64_8h_8h(TyH);
   test_rev64_4h_4h(TyH);
   test_rev64_4s_4s(TyS);
   test_rev64_2s_2s(TyS);

   // saba      16b,8b,8h,4h,4s,2s
   // uaba      16b,8b,8h,4h,4s,2s
   test_saba_4s_4s_4s(TyS);
   test_saba_2s_2s_2s(TyS);
   test_saba_8h_8h_8h(TyH);
   test_saba_4h_4h_4h(TyH);
   test_saba_16b_16b_16b(TyB);
   test_saba_8b_8b_8b(TyB);
   test_uaba_4s_4s_4s(TyS);
   test_uaba_2s_2s_2s(TyS);
   test_uaba_8h_8h_8h(TyH);
   test_uaba_4h_4h_4h(TyH);
   test_uaba_16b_16b_16b(TyB);
   test_uaba_8b_8b_8b(TyB);

   // sabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // uabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   test_sabal_2d_2s_2s(TyS);
   test_sabal2_2d_4s_4s(TyS);
   test_sabal_4s_4h_4h(TyH);
   test_sabal2_4s_8h_8h(TyH);
   test_sabal_8h_8b_8b(TyB);
   test_sabal2_8h_16b_16b(TyB);
   test_uabal_2d_2s_2s(TyS);
   test_uabal2_2d_4s_4s(TyS);
   test_uabal_4s_4h_4h(TyH);
   test_uabal2_4s_8h_8h(TyH);
   test_uabal_8h_8b_8b(TyB);
   test_uabal2_8h_16b_16b(TyB);

   // sabd      16b,8b,8h,4h,4s,2s
   // uabd      16b,8b,8h,4h,4s,2s
   test_sabd_4s_4s_4s(TyS);
   test_sabd_2s_2s_2s(TyS);
   test_sabd_8h_8h_8h(TyH);
   test_sabd_4h_4h_4h(TyH);
   test_sabd_16b_16b_16b(TyB);
   test_sabd_8b_8b_8b(TyB);
   test_uabd_4s_4s_4s(TyS);
   test_uabd_2s_2s_2s(TyS);
   test_uabd_8h_8h_8h(TyH);
   test_uabd_4h_4h_4h(TyH);
   test_uabd_16b_16b_16b(TyB);
   test_uabd_8b_8b_8b(TyB);

   // sabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // uabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   test_sabdl_2d_2s_2s(TyS);
   test_sabdl2_2d_4s_4s(TyS);
   test_sabdl_4s_4h_4h(TyH);
   test_sabdl2_4s_8h_8h(TyH);
   test_sabdl_8h_8b_8b(TyB);
   test_sabdl2_8h_16b_16b(TyB);
   test_uabdl_2d_2s_2s(TyS);
   test_uabdl2_2d_4s_4s(TyS);
   test_uabdl_4s_4h_4h(TyH);
   test_uabdl2_4s_8h_8h(TyH);
   test_uabdl_8h_8b_8b(TyB);
   test_uabdl2_8h_16b_16b(TyB);

   // sadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   // uadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   test_sadalp_1d_2s(TyS);
   test_sadalp_2d_4s(TyS);
   test_sadalp_2s_4h(TyH);
   test_sadalp_4s_8h(TyH);
   test_sadalp_4h_8b(TyB);
   test_sadalp_8h_16b(TyB);
   test_uadalp_1d_2s(TyS);
   test_uadalp_2d_4s(TyS);
   test_uadalp_2s_4h(TyH);
   test_uadalp_4s_8h(TyH);
   test_uadalp_4h_8b(TyB);
   test_uadalp_8h_16b(TyB);

   // saddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // uaddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // ssubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // usubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   test_saddl_2d_2s_2s(TyS);
   test_saddl2_2d_4s_4s(TyS);
   test_saddl_4s_4h_4h(TyH);
   test_saddl2_4s_8h_8h(TyH);
   test_saddl_8h_8b_8b(TyB);
   test_saddl2_8h_16b_16b(TyB);
   test_uaddl_2d_2s_2s(TyS);
   test_uaddl2_2d_4s_4s(TyS);
   test_uaddl_4s_4h_4h(TyH);
   test_uaddl2_4s_8h_8h(TyH);
   test_uaddl_8h_8b_8b(TyB);
   test_uaddl2_8h_16b_16b(TyB);
   test_ssubl_2d_2s_2s(TyS);
   test_ssubl2_2d_4s_4s(TyS);
   test_ssubl_4s_4h_4h(TyH);
   test_ssubl2_4s_8h_8h(TyH);
   test_ssubl_8h_8b_8b(TyB);
   test_ssubl2_8h_16b_16b(TyB);
   test_usubl_2d_2s_2s(TyS);
   test_usubl2_2d_4s_4s(TyS);
   test_usubl_4s_4h_4h(TyH);
   test_usubl2_4s_8h_8h(TyH);
   test_usubl_8h_8b_8b(TyB);
   test_usubl2_8h_16b_16b(TyB);

   // saddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   // uaddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   test_saddlp_1d_2s(TyS);
   test_saddlp_2d_4s(TyS);
   test_saddlp_2s_4h(TyH);
   test_saddlp_4s_8h(TyH);
   test_saddlp_4h_8b(TyB);
   test_saddlp_8h_16b(TyB);
   test_uaddlp_1d_2s(TyS);
   test_uaddlp_2d_4s(TyS);
   test_uaddlp_2s_4h(TyH);
   test_uaddlp_4s_8h(TyH);
   test_uaddlp_4h_8b(TyB);
   test_uaddlp_8h_16b(TyB);

   // saddlv    h_16b/8b, s_8h/4h, d_4s
   // uaddlv    h_16b/8b, s_8h/4h, d_4s
   test_saddlv_h_16b(TyB);
   test_saddlv_h_8b(TyB);
   test_saddlv_s_8h(TyH);
   test_saddlv_s_4h(TyH);
   test_saddlv_d_4s(TyH);
   test_uaddlv_h_16b(TyB);
   test_uaddlv_h_8b(TyB);
   test_uaddlv_s_8h(TyH);
   test_uaddlv_s_4h(TyH);
   test_uaddlv_d_4s(TyH);

   // saddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_4s/2s
   // uaddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_4s/2s
   // ssubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_4s/2s
   // usubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_4s/2s
   test_saddw2_8h_8h_16b(TyB);
   test_saddw_8h_8h_8b(TyB);
   test_saddw2_4s_4s_8h(TyH);
   test_saddw_4s_4s_4h(TyH);
   test_saddw2_2d_2d_4s(TyS);
   test_saddw_2d_2d_2s(TyS);
   test_uaddw2_8h_8h_16b(TyB);
   test_uaddw_8h_8h_8b(TyB);
   test_uaddw2_4s_4s_8h(TyH);
   test_uaddw_4s_4s_4h(TyH);
   test_uaddw2_2d_2d_4s(TyS);
   test_uaddw_2d_2d_2s(TyS);
   test_ssubw2_8h_8h_16b(TyB);
   test_ssubw_8h_8h_8b(TyB);
   test_ssubw2_4s_4s_8h(TyH);
   test_ssubw_4s_4s_4h(TyH);
   test_ssubw2_2d_2d_4s(TyS);
   test_ssubw_2d_2d_2s(TyS);
   test_usubw2_8h_8h_16b(TyB);
   test_usubw_8h_8h_8b(TyB);
   test_usubw2_4s_4s_8h(TyH);
   test_usubw_4s_4s_4h(TyH);
   test_usubw2_2d_2d_4s(TyS);
   test_usubw_2d_2d_2s(TyS);

   // shadd        16b,8b,8h,4h,4s,2s
   // uhadd        16b,8b,8h,4h,4s,2s
   // shsub        16b,8b,8h,4h,4s,2s
   // uhsub        16b,8b,8h,4h,4s,2s
   test_shadd_4s_4s_4s(TyS);
   test_shadd_2s_2s_2s(TyS);
   test_shadd_8h_8h_8h(TyH);
   test_shadd_4h_4h_4h(TyH);
   test_shadd_16b_16b_16b(TyB);
   test_shadd_8b_8b_8b(TyB);
   test_uhadd_4s_4s_4s(TyS);
   test_uhadd_2s_2s_2s(TyS);
   test_uhadd_8h_8h_8h(TyH);
   test_uhadd_4h_4h_4h(TyH);
   test_uhadd_16b_16b_16b(TyB);
   test_uhadd_8b_8b_8b(TyB);
   test_shsub_4s_4s_4s(TyS);
   test_shsub_2s_2s_2s(TyS);
   test_shsub_8h_8h_8h(TyH);
   test_shsub_4h_4h_4h(TyH);
   test_shsub_16b_16b_16b(TyB);
   test_shsub_8b_8b_8b(TyB);
   test_uhsub_4s_4s_4s(TyS);
   test_uhsub_2s_2s_2s(TyS);
   test_uhsub_8h_8h_8h(TyH);
   test_uhsub_4h_4h_4h(TyH);
   test_uhsub_16b_16b_16b(TyB);
   test_uhsub_8b_8b_8b(TyB);

   // shll{2}      8h_8b/16b_#8, 4s_4h/8h_#16, 2d_2s/4s_#32
   test_shll_8h_8b_8(TyB);
   test_shll2_8h_16b_8(TyB);
   test_shll_4s_4h_16(TyH);
   test_shll2_4s_8h_16(TyH);
   test_shll_2d_2s_32(TyS);
   test_shll2_2d_4s_32(TyS);

   // shrn{2}      2s/4s_2d, 8h/4h_4s, 8b/16b_8h,   #imm in 1 .. elem_bits
   // rshrn{2}     2s/4s_2d, 8h/4h_4s, 8b/16b_8h,   #imm in 1 .. elem_bits
   test_shrn_2s_2d_1(TyD);
   test_shrn_2s_2d_32(TyD);
   test_shrn2_4s_2d_1(TyD);
   test_shrn2_4s_2d_32(TyD);
   test_shrn_4h_4s_1(TyS);
   test_shrn_4h_4s_16(TyS);
   test_shrn2_8h_4s_1(TyS);
   test_shrn2_8h_4s_16(TyS);
   test_shrn_8b_8h_1(TyH);
   test_shrn_8b_8h_8(TyH);
   test_shrn2_16b_8h_1(TyH);
   test_shrn2_16b_8h_8(TyH);
   test_rshrn_2s_2d_1(TyD);
   test_rshrn_2s_2d_32(TyD);
   test_rshrn2_4s_2d_1(TyD);
   test_rshrn2_4s_2d_32(TyD);
   test_rshrn_4h_4s_1(TyS);
   test_rshrn_4h_4s_16(TyS);
   test_rshrn2_8h_4s_1(TyS);
   test_rshrn2_8h_4s_16(TyS);
   test_rshrn_8b_8h_1(TyH);
   test_rshrn_8b_8h_8(TyH);
   test_rshrn2_16b_8h_1(TyH);
   test_rshrn2_16b_8h_8(TyH);

   // sli          d_#imm
   // sri          d_#imm
   test_sli_d_d_0(TyD);
   test_sli_d_d_32(TyD);
   test_sli_d_d_63(TyD);
   test_sri_d_d_1(TyD);
   test_sri_d_d_33(TyD);
   test_sri_d_d_64(TyD);

   // sli          2d,4s,2s,8h,4h,16b,8b  _#imm
   // sri          2d,4s,2s,8h,4h,16b,8b  _#imm
   test_sli_2d_2d_0(TyD);
   test_sli_2d_2d_32(TyD);
   test_sli_2d_2d_63(TyD);
   test_sli_4s_4s_0(TyS);
   test_sli_4s_4s_16(TyS);
   test_sli_4s_4s_31(TyS);
   test_sli_2s_2s_0(TyS);
   test_sli_2s_2s_16(TyS);
   test_sli_2s_2s_31(TyS);
   test_sli_8h_8h_0(TyH);
   test_sli_8h_8h_8(TyH);
   test_sli_8h_8h_15(TyH);
   test_sli_4h_4h_0(TyH);
   test_sli_4h_4h_8(TyH);
   test_sli_4h_4h_15(TyH);
   test_sli_16b_16b_0(TyB);
   test_sli_16b_16b_3(TyB);
   test_sli_16b_16b_7(TyB);
   test_sli_8b_8b_0(TyB);
   test_sli_8b_8b_3(TyB);
   test_sli_8b_8b_7(TyB);
   test_sri_2d_2d_1(TyD);
   test_sri_2d_2d_33(TyD);
   test_sri_2d_2d_64(TyD);
   test_sri_4s_4s_1(TyS);
   test_sri_4s_4s_17(TyS);
   test_sri_4s_4s_32(TyS);
   test_sri_2s_2s_1(TyS);
   test_sri_2s_2s_17(TyS);
   test_sri_2s_2s_32(TyS);
   test_sri_8h_8h_1(TyH);
   test_sri_8h_8h_8(TyH);
   test_sri_8h_8h_16(TyH);
   test_sri_4h_4h_1(TyH);
   test_sri_4h_4h_8(TyH);
   test_sri_4h_4h_16(TyH);
   test_sri_16b_16b_1(TyB);
   test_sri_16b_16b_4(TyB);
   test_sri_16b_16b_8(TyB);
   test_sri_8b_8b_1(TyB);
   test_sri_8b_8b_4(TyB);
   test_sri_8b_8b_8(TyB);

   // smax         4s,2s,8h,4h,16b,8b
   // umax         4s,2s,8h,4h,16b,8b
   // smin         4s,2s,8h,4h,16b,8b
   // umin         4s,2s,8h,4h,16b,8b
   test_smax_4s_4s_4s(TyS);
   test_smax_2s_2s_2s(TyS);
   test_smax_8h_8h_8h(TyH);
   test_smax_4h_4h_4h(TyH);
   test_smax_16b_16b_16b(TyB);
   test_smax_8b_8b_8b(TyB);
   test_umax_4s_4s_4s(TyS);
   test_umax_2s_2s_2s(TyS);
   test_umax_8h_8h_8h(TyH);
   test_umax_4h_4h_4h(TyH);
   test_umax_16b_16b_16b(TyB);
   test_umax_8b_8b_8b(TyB);
   test_smin_4s_4s_4s(TyS);
   test_smin_2s_2s_2s(TyS);
   test_smin_8h_8h_8h(TyH);
   test_smin_4h_4h_4h(TyH);
   test_smin_16b_16b_16b(TyB);
   test_smin_8b_8b_8b(TyB);
   test_umin_4s_4s_4s(TyS);
   test_umin_2s_2s_2s(TyS);
   test_umin_8h_8h_8h(TyH);
   test_umin_4h_4h_4h(TyH);
   test_umin_16b_16b_16b(TyB);
   test_umin_8b_8b_8b(TyB);

   // smaxp        4s,2s,8h,4h,16b,8b
   // umaxp        4s,2s,8h,4h,16b,8b
   // sminp        4s,2s,8h,4h,16b,8b
   // uminp        4s,2s,8h,4h,16b,8b
   test_smaxp_4s_4s_4s(TyS);
   test_smaxp_2s_2s_2s(TyS);
   test_smaxp_8h_8h_8h(TyH);
   test_smaxp_4h_4h_4h(TyH);
   test_smaxp_16b_16b_16b(TyB);
   test_smaxp_8b_8b_8b(TyB);
   test_umaxp_4s_4s_4s(TyS);
   test_umaxp_2s_2s_2s(TyS);
   test_umaxp_8h_8h_8h(TyH);
   test_umaxp_4h_4h_4h(TyH);
   test_umaxp_16b_16b_16b(TyB);
   test_umaxp_8b_8b_8b(TyB);
   test_sminp_4s_4s_4s(TyS);
   test_sminp_2s_2s_2s(TyS);
   test_sminp_8h_8h_8h(TyH);
   test_sminp_4h_4h_4h(TyH);
   test_sminp_16b_16b_16b(TyB);
   test_sminp_8b_8b_8b(TyB);
   test_uminp_4s_4s_4s(TyS);
   test_uminp_2s_2s_2s(TyS);
   test_uminp_8h_8h_8h(TyH);
   test_uminp_4h_4h_4h(TyH);
   test_uminp_16b_16b_16b(TyB);
   test_uminp_8b_8b_8b(TyB);

   // smaxv        s_4s,h_8h,h_4h,b_16b,b_8b
   // umaxv        s_4s,h_8h,h_4h,b_16b,b_8b
   // sminv        s_4s,h_8h,h_4h,b_16b,b_8b
   // uminv        s_4s,h_8h,h_4h,b_16b,b_8b
   test_SMAXV();
   test_UMAXV();
   test_SMINV();
   test_UMINV();

   // smlal{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   // umlal{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   // smlsl{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   // umlsl{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   // smull{2}     2d_2s/4s_s[]. 4s_4h/8h_h[]
   // umull{2}     2d_2s/4s_s[]. 4s_4h/8h_h[]
   test_smlal_2d_2s_s0(TyS);
   test_smlal_2d_2s_s3(TyS);
   test_smlal2_2d_4s_s1(TyS);
   test_smlal2_2d_4s_s2(TyS);
   test_smlal_4s_4h_h0(TyH);
   test_smlal_4s_4h_h7(TyH);
   test_smlal2_4s_8h_h1(TyH);
   test_smlal2_4s_8h_h4(TyH);
   test_umlal_2d_2s_s0(TyS);
   test_umlal_2d_2s_s3(TyS);
   test_umlal2_2d_4s_s1(TyS);
   test_umlal2_2d_4s_s2(TyS);
   test_umlal_4s_4h_h0(TyH);
   test_umlal_4s_4h_h7(TyH);
   test_umlal2_4s_8h_h1(TyH);
   test_umlal2_4s_8h_h4(TyH);
   test_smlsl_2d_2s_s0(TyS);
   test_smlsl_2d_2s_s3(TyS);
   test_smlsl2_2d_4s_s1(TyS);
   test_smlsl2_2d_4s_s2(TyS);
   test_smlsl_4s_4h_h0(TyH);
   test_smlsl_4s_4h_h7(TyH);
   test_smlsl2_4s_8h_h1(TyH);
   test_smlsl2_4s_8h_h4(TyH);
   test_umlsl_2d_2s_s0(TyS);
   test_umlsl_2d_2s_s3(TyS);
   test_umlsl2_2d_4s_s1(TyS);
   test_umlsl2_2d_4s_s2(TyS);
   test_umlsl_4s_4h_h0(TyH);
   test_umlsl_4s_4h_h7(TyH);
   test_umlsl2_4s_8h_h1(TyH);
   test_umlsl2_4s_8h_h4(TyH);
   test_smull_2d_2s_s0(TyS);
   test_smull_2d_2s_s3(TyS);
   test_smull2_2d_4s_s1(TyS);
   test_smull2_2d_4s_s2(TyS);
   test_smull_4s_4h_h0(TyH);
   test_smull_4s_4h_h7(TyH);
   test_smull2_4s_8h_h1(TyH);
   test_smull2_4s_8h_h4(TyH);
   test_umull_2d_2s_s0(TyS);
   test_umull_2d_2s_s3(TyS);
   test_umull2_2d_4s_s1(TyS);
   test_umull2_2d_4s_s2(TyS);
   test_umull_4s_4h_h0(TyH);
   test_umull_4s_4h_h7(TyH);
   test_umull2_4s_8h_h1(TyH);
   test_umull2_4s_8h_h4(TyH);

   // smlal{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // umlal{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // smlsl{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // umlsl{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // smull{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   // umull{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   test_smlal_2d_2s_2s(TyS);
   test_smlal2_2d_4s_4s(TyS);
   test_smlal_4s_4h_4h(TyH);
   test_smlal2_4s_8h_8h(TyH);
   test_smlal_8h_8b_8b(TyB);
   test_smlal2_8h_16b_16b(TyB);
   test_umlal_2d_2s_2s(TyS);
   test_umlal2_2d_4s_4s(TyS);
   test_umlal_4s_4h_4h(TyH);
   test_umlal2_4s_8h_8h(TyH);
   test_umlal_8h_8b_8b(TyB);
   test_umlal2_8h_16b_16b(TyB);
   test_smlsl_2d_2s_2s(TyS);
   test_smlsl2_2d_4s_4s(TyS);
   test_smlsl_4s_4h_4h(TyH);
   test_smlsl2_4s_8h_8h(TyH);
   test_smlsl_8h_8b_8b(TyB);
   test_smlsl2_8h_16b_16b(TyB);
   test_umlsl_2d_2s_2s(TyS);
   test_umlsl2_2d_4s_4s(TyS);
   test_umlsl_4s_4h_4h(TyH);
   test_umlsl2_4s_8h_8h(TyH);
   test_umlsl_8h_8b_8b(TyB);
   test_umlsl2_8h_16b_16b(TyB);
   test_smull_2d_2s_2s(TyS);
   test_smull2_2d_4s_4s(TyS);
   test_smull_4s_4h_4h(TyH);
   test_smull2_4s_8h_8h(TyH);
   test_smull_8h_8b_8b(TyB);
   test_smull2_8h_16b_16b(TyB);
   test_umull_2d_2s_2s(TyS);
   test_umull2_2d_4s_4s(TyS);
   test_umull_4s_4h_4h(TyH);
   test_umull2_4s_8h_8h(TyH);
   test_umull_8h_8b_8b(TyB);
   test_umull2_8h_16b_16b(TyB);

   // smov         w_b[], w_h[], x_b[], x_h[], x_s[]
   // umov         w_b[], w_h[],               w_s[], x_d[]
   test_umov_x_d0(TyD);
   test_umov_x_d1(TyD);
   test_umov_w_s0(TyS);
   test_umov_w_s3(TyS);
   test_umov_w_h0(TyH);
   test_umov_w_h7(TyH);
   test_umov_w_b0(TyB);
   test_umov_w_b15(TyB);
   test_smov_x_s0(TyS);
   test_smov_x_s3(TyS);
   test_smov_x_h0(TyH);
   test_smov_x_h7(TyH);
   test_smov_w_h0(TyH);
   test_smov_w_h7(TyH);
   test_smov_x_b0(TyB);
   test_smov_x_b15(TyB);
   test_smov_w_b0(TyB);
   test_smov_w_b15(TyB);

   // sqabs        d,s,h,b
   // sqneg        d,s,h,b
   test_sqabs_d_d(TyD);
   test_sqabs_s_s(TyS);
   test_sqabs_h_h(TyH);
   test_sqabs_b_b(TyB);
   test_sqneg_d_d(TyD);
   test_sqneg_s_s(TyS);
   test_sqneg_h_h(TyH);
   test_sqneg_b_b(TyB);

   // sqabs        2d,4s,2s,8h,4h,16b,8b
   // sqneg        2d,4s,2s,8h,4h,16b,8b
   test_sqabs_2d_2d(TyD);
   test_sqabs_4s_4s(TyS);
   test_sqabs_2s_2s(TyS);
   test_sqabs_8h_8h(TyH);
   test_sqabs_4h_4h(TyH);
   test_sqabs_16b_16b(TyB);
   test_sqabs_8b_8b(TyB);
   test_sqneg_2d_2d(TyD);
   test_sqneg_4s_4s(TyS);
   test_sqneg_2s_2s(TyS);
   test_sqneg_8h_8h(TyH);
   test_sqneg_4h_4h(TyH);
   test_sqneg_16b_16b(TyB);
   test_sqneg_8b_8b(TyB);

   // sqadd        d,s,h,b
   // uqadd        d,s,h,b
   // sqsub        d,s,h,b
   // uqsub        d,s,h,b
   test_sqadd_d_d_d(TyD);
   test_sqadd_s_s_s(TyS);
   test_sqadd_h_h_h(TyH);
   test_sqadd_b_b_b(TyB);
   test_uqadd_d_d_d(TyD);
   test_uqadd_s_s_s(TyS);
   test_uqadd_h_h_h(TyH);
   test_uqadd_b_b_b(TyB);
   test_sqsub_d_d_d(TyD);
   test_sqsub_s_s_s(TyS);
   test_sqsub_h_h_h(TyH);
   test_sqsub_b_b_b(TyB);
   test_uqsub_d_d_d(TyD);
   test_uqsub_s_s_s(TyS);
   test_uqsub_h_h_h(TyH);
   test_uqsub_b_b_b(TyB);

   // sqadd        2d,4s,2s,8h,4h,16b,8b
   // uqadd        2d,4s,2s,8h,4h,16b,8b
   // sqsub        2d,4s,2s,8h,4h,16b,8b
   // uqsub        2d,4s,2s,8h,4h,16b,8b
   test_sqadd_2d_2d_2d(TyD);
   test_sqadd_4s_4s_4s(TyS);
   test_sqadd_2s_2s_2s(TyS);
   test_sqadd_8h_8h_8h(TyH);
   test_sqadd_4h_4h_4h(TyH);
   test_sqadd_16b_16b_16b(TyB);
   test_sqadd_8b_8b_8b(TyB);
   test_uqadd_2d_2d_2d(TyD);
   test_uqadd_4s_4s_4s(TyS);
   test_uqadd_2s_2s_2s(TyS);
   test_uqadd_8h_8h_8h(TyH);
   test_uqadd_4h_4h_4h(TyH);
   test_uqadd_16b_16b_16b(TyB);
   test_uqadd_8b_8b_8b(TyB);
   test_sqsub_2d_2d_2d(TyD);
   test_sqsub_4s_4s_4s(TyS);
   test_sqsub_2s_2s_2s(TyS);
   test_sqsub_8h_8h_8h(TyH);
   test_sqsub_4h_4h_4h(TyH);
   test_sqsub_16b_16b_16b(TyB);
   test_sqsub_8b_8b_8b(TyB);
   test_uqsub_2d_2d_2d(TyD);
   test_uqsub_4s_4s_4s(TyS);
   test_uqsub_2s_2s_2s(TyS);
   test_uqsub_8h_8h_8h(TyH);
   test_uqsub_4h_4h_4h(TyH);
   test_uqsub_16b_16b_16b(TyB);
   test_uqsub_8b_8b_8b(TyB);

   // sqdmlal      d_s_s[], s_h_h[]
   // sqdmlsl      d_s_s[], s_h_h[]
   // sqdmull      d_s_s[], s_h_h[]
   test_sqdmlal_d_s_s0(TyS);
   test_sqdmlal_d_s_s3(TyS);
   test_sqdmlal_s_h_h1(TyH);
   test_sqdmlal_s_h_h5(TyH);
   test_sqdmlsl_d_s_s0(TyS);
   test_sqdmlsl_d_s_s3(TyS);
   test_sqdmlsl_s_h_h1(TyH);
   test_sqdmlsl_s_h_h5(TyH);
   test_sqdmull_d_s_s0(TyS);
   test_sqdmull_d_s_s3(TyS);
   test_sqdmull_s_h_h1(TyH);
   test_sqdmull_s_h_h5(TyH);

   // sqdmlal{2}   2d_2s/4s_s[], 4s_4h/8h_h[]
   // sqdmlsl{2}   2d_2s/4s_s[], 4s_4h/8h_h[]
   // sqdmull{2}   2d_2s/4s_s[], 4s_4h/2h_h[]
   test_sqdmlal_2d_2s_s0(TyS);
   test_sqdmlal_2d_2s_s3(TyS);
   test_sqdmlal2_2d_4s_s1(TyS);
   test_sqdmlal2_2d_4s_s2(TyS);
   test_sqdmlal_4s_4h_h0(TyH);
   test_sqdmlal_4s_4h_h7(TyH);
   test_sqdmlal2_4s_8h_h1(TyH);
   test_sqdmlal2_4s_8h_h4(TyH);
   test_sqdmlsl_2d_2s_s0(TyS);
   test_sqdmlsl_2d_2s_s3(TyS);
   test_sqdmlsl2_2d_4s_s1(TyS);
   test_sqdmlsl2_2d_4s_s2(TyS);
   test_sqdmlsl_4s_4h_h0(TyH);
   test_sqdmlsl_4s_4h_h7(TyH);
   test_sqdmlsl2_4s_8h_h1(TyH);
   test_sqdmlsl2_4s_8h_h4(TyH);
   test_sqdmull_2d_2s_s0(TyS);
   test_sqdmull_2d_2s_s3(TyS);
   test_sqdmull2_2d_4s_s1(TyS);
   test_sqdmull2_2d_4s_s2(TyS);
   test_sqdmull_4s_4h_h0(TyH);
   test_sqdmull_4s_4h_h7(TyH);
   test_sqdmull2_4s_8h_h1(TyH);
   test_sqdmull2_4s_8h_h4(TyH);

   // sqdmlal      d_s_s, s_h_h
   // sqdmlsl      d_s_s, s_h_h
   // sqdmull      d_s_s, s_h_h
   test_sqdmlal_d_s_s(TyS);
   test_sqdmlal_s_h_h(TyH);
   test_sqdmlsl_d_s_s(TyS);
   test_sqdmlsl_s_h_h(TyH);
   test_sqdmull_d_s_s(TyS);
   test_sqdmull_s_h_h(TyH);

   // sqdmlal{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)
   // sqdmlsl{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)
   // sqdmull{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)
   test_sqdmlal_2d_2s_2s(TyS);
   test_sqdmlal2_2d_4s_4s(TyS);
   test_sqdmlal_4s_4h_4h(TyH);
   test_sqdmlal2_4s_8h_8h(TyH);
   test_sqdmlsl_2d_2s_2s(TyS);
   test_sqdmlsl2_2d_4s_4s(TyS);
   test_sqdmlsl_4s_4h_4h(TyH);
   test_sqdmlsl2_4s_8h_8h(TyH);
   test_sqdmull_2d_2s_2s(TyS);
   test_sqdmull2_2d_4s_4s(TyS);
   test_sqdmull_4s_4h_4h(TyH);
   test_sqdmull2_4s_8h_8h(TyH);

   // sqdmulh      s_s_s[], h_h_h[]
   // sqrdmulh     s_s_s[], h_h_h[]
   test_sqdmulh_s_s_s1(TyS);
   test_sqdmulh_s_s_s3(TyS);
   test_sqdmulh_h_h_h2(TyH);
   test_sqdmulh_h_h_h7(TyH);
   test_sqrdmulh_s_s_s1(TyS);
   test_sqrdmulh_s_s_s3(TyS);
   test_sqrdmulh_h_h_h2(TyH);
   test_sqrdmulh_h_h_h7(TyH);

   // sqdmulh      4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]
   // sqrdmulh     4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]
   test_sqdmulh_4s_4s_s1(TyS);
   test_sqdmulh_4s_4s_s3(TyS);
   test_sqdmulh_2s_2s_s1(TyS);
   test_sqdmulh_2s_2s_s3(TyS);
   test_sqdmulh_8h_8h_h2(TyH);
   test_sqdmulh_8h_8h_h7(TyH);
   test_sqdmulh_4h_4h_h2(TyH);
   test_sqdmulh_4h_4h_h7(TyH);
   test_sqrdmulh_4s_4s_s1(TyS);
   test_sqrdmulh_4s_4s_s3(TyS);
   test_sqrdmulh_2s_2s_s1(TyS);
   test_sqrdmulh_2s_2s_s3(TyS);
   test_sqrdmulh_8h_8h_h2(TyH);
   test_sqrdmulh_8h_8h_h7(TyH);
   test_sqrdmulh_4h_4h_h2(TyH);
   test_sqrdmulh_4h_4h_h7(TyH);

   // sqdmulh      h,s
   // sqrdmulh     h,s
   test_sqdmulh_s_s_s(TyS);
   test_sqdmulh_h_h_h(TyH);
   test_sqrdmulh_s_s_s(TyS);
   test_sqrdmulh_h_h_h(TyH);

   // sqdmulh      4s,2s,8h,4h
   // sqrdmulh     4s,2s,8h,4h
   test_sqdmulh_4s_4s_4s(TyS);
   test_sqdmulh_2s_2s_2s(TyS);
   test_sqdmulh_8h_8h_8h(TyH);
   test_sqdmulh_4h_4h_4h(TyH);
   test_sqrdmulh_4s_4s_4s(TyS);
   test_sqrdmulh_2s_2s_2s(TyS);
   test_sqrdmulh_8h_8h_8h(TyH);
   test_sqrdmulh_4h_4h_4h(TyH);

   // sqshl (reg)  d,s,h,b
   // uqshl (reg)  d,s,h,b
   // sqrshl (reg) d,s,h,b
   // uqrshl (reg) d,s,h,b
   test_sqshl_d_d_d(TyD);
   test_sqshl_s_s_s(TyS);
   test_sqshl_h_h_h(TyH);
   test_sqshl_b_b_b(TyB);
   test_uqshl_d_d_d(TyD);
   test_uqshl_s_s_s(TyS);
   test_uqshl_h_h_h(TyH);
   test_uqshl_b_b_b(TyB);
   test_sqrshl_d_d_d(TyD);
   test_sqrshl_s_s_s(TyS);
   test_sqrshl_h_h_h(TyH);
   test_sqrshl_b_b_b(TyB);
   test_uqrshl_d_d_d(TyD);
   test_uqrshl_s_s_s(TyS);
   test_uqrshl_h_h_h(TyH);
   test_uqrshl_b_b_b(TyB);

   // sqshl (reg)  2d,4s,2s,8h,4h,16b,8b
   // uqshl (reg)  2d,4s,2s,8h,4h,16b,8b
   // sqrshl (reg) 2d,4s,2s,8h,4h,16b,8b
   // uqrshl (reg) 2d,4s,2s,8h,4h,16b,8b
   test_sqshl_2d_2d_2d(TyD);
   test_sqshl_4s_4s_4s(TyS);
   test_sqshl_2s_2s_2s(TyS);
   test_sqshl_8h_8h_8h(TyH);
   test_sqshl_4h_4h_4h(TyH);
   test_sqshl_16b_16b_16b(TyB);
   test_sqshl_8b_8b_8b(TyB);
   test_uqshl_2d_2d_2d(TyD);
   test_uqshl_4s_4s_4s(TyS);
   test_uqshl_2s_2s_2s(TyS);
   test_uqshl_8h_8h_8h(TyH);
   test_uqshl_4h_4h_4h(TyH);
   test_uqshl_16b_16b_16b(TyB);
   test_uqshl_8b_8b_8b(TyB);
   test_sqrshl_2d_2d_2d(TyD);
   test_sqrshl_4s_4s_4s(TyS);
   test_sqrshl_2s_2s_2s(TyS);
   test_sqrshl_8h_8h_8h(TyH);
   test_sqrshl_4h_4h_4h(TyH);
   test_sqrshl_16b_16b_16b(TyB);
   test_sqrshl_8b_8b_8b(TyB);
   test_uqrshl_2d_2d_2d(TyD);
   test_uqrshl_4s_4s_4s(TyS);
   test_uqrshl_2s_2s_2s(TyS);
   test_uqrshl_8h_8h_8h(TyH);
   test_uqrshl_4h_4h_4h(TyH);
   test_uqrshl_16b_16b_16b(TyB);
   test_uqrshl_8b_8b_8b(TyB);

   // sqrshrn      s_d, h_s, b_h   #imm
   // uqrshrn      s_d, h_s, b_h   #imm
   // sqshrn       s_d, h_s, b_h   #imm
   // uqshrn       s_d, h_s, b_h   #imm
   // sqrshrun     s_d, h_s, b_h   #imm
   // sqshrun      s_d, h_s, b_h   #imm
   test_sqrshrn_s_d_1(TyD);
   test_sqrshrn_s_d_17(TyD);
   test_sqrshrn_s_d_32(TyD);
   test_sqrshrn_h_s_1(TyS);
   test_sqrshrn_h_s_9(TyS);
   test_sqrshrn_h_s_16(TyS);
   test_sqrshrn_b_h_1(TyH);
   test_sqrshrn_b_h_4(TyH);
   test_sqrshrn_b_h_8(TyH);
   test_uqrshrn_s_d_1(TyD);
   test_uqrshrn_s_d_17(TyD);
   test_uqrshrn_s_d_32(TyD);
   test_uqrshrn_h_s_1(TyS);
   test_uqrshrn_h_s_9(TyS);
   test_uqrshrn_h_s_16(TyS);
   test_uqrshrn_b_h_1(TyH);
   test_uqrshrn_b_h_4(TyH);
   test_uqrshrn_b_h_8(TyH);
   test_sqshrn_s_d_1(TyD);
   test_sqshrn_s_d_17(TyD);
   test_sqshrn_s_d_32(TyD);
   test_sqshrn_h_s_1(TyS);
   test_sqshrn_h_s_9(TyS);
   test_sqshrn_h_s_16(TyS);
   test_sqshrn_b_h_1(TyH);
   test_sqshrn_b_h_4(TyH);
   test_sqshrn_b_h_8(TyH);
   test_uqshrn_s_d_1(TyD);
   test_uqshrn_s_d_17(TyD);
   test_uqshrn_s_d_32(TyD);
   test_uqshrn_h_s_1(TyS);
   test_uqshrn_h_s_9(TyS);
   test_uqshrn_h_s_16(TyS);
   test_uqshrn_b_h_1(TyH);
   test_uqshrn_b_h_4(TyH);
   test_uqshrn_b_h_8(TyH);
   test_sqrshrun_s_d_1(TyD);
   test_sqrshrun_s_d_17(TyD);
   test_sqrshrun_s_d_32(TyD);
   test_sqrshrun_h_s_1(TyS);
   test_sqrshrun_h_s_9(TyS);
   test_sqrshrun_h_s_16(TyS);
   test_sqrshrun_b_h_1(TyH);
   test_sqrshrun_b_h_4(TyH);
   test_sqrshrun_b_h_8(TyH);
   test_sqshrun_s_d_1(TyD);
   test_sqshrun_s_d_17(TyD);
   test_sqshrun_s_d_32(TyD);
   test_sqshrun_h_s_1(TyS);
   test_sqshrun_h_s_9(TyS);
   test_sqshrun_h_s_16(TyS);
   test_sqshrun_b_h_1(TyH);
   test_sqshrun_b_h_4(TyH);
   test_sqshrun_b_h_8(TyH);

   // sqrshrn{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   // uqrshrn{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   // sqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   // uqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   // sqrshrun{2}  2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   // sqshrun{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   test_sqrshrn_2s_2d_1(TyD);
   test_sqrshrn_2s_2d_17(TyD);
   test_sqrshrn_2s_2d_32(TyD);
   test_sqrshrn2_4s_2d_1(TyD);
   test_sqrshrn2_4s_2d_17(TyD);
   test_sqrshrn2_4s_2d_32(TyD);
   test_sqrshrn_4h_4s_1(TyS);
   test_sqrshrn_4h_4s_9(TyS);
   test_sqrshrn_4h_4s_16(TyS);
   test_sqrshrn2_8h_4s_1(TyS);
   test_sqrshrn2_8h_4s_9(TyS);
   test_sqrshrn2_8h_4s_16(TyS);
   test_sqrshrn_8b_8h_1(TyH);
   test_sqrshrn_8b_8h_4(TyH);
   test_sqrshrn_8b_8h_8(TyH);
   test_sqrshrn2_16b_8h_1(TyH);
   test_sqrshrn2_16b_8h_4(TyH);
   test_sqrshrn2_16b_8h_8(TyH);
   test_uqrshrn_2s_2d_1(TyD);
   test_uqrshrn_2s_2d_17(TyD);
   test_uqrshrn_2s_2d_32(TyD);
   test_uqrshrn2_4s_2d_1(TyD);
   test_uqrshrn2_4s_2d_17(TyD);
   test_uqrshrn2_4s_2d_32(TyD);
   test_uqrshrn_4h_4s_1(TyS);
   test_uqrshrn_4h_4s_9(TyS);
   test_uqrshrn_4h_4s_16(TyS);
   test_uqrshrn2_8h_4s_1(TyS);
   test_uqrshrn2_8h_4s_9(TyS);
   test_uqrshrn2_8h_4s_16(TyS);
   test_uqrshrn_8b_8h_1(TyH);
   test_uqrshrn_8b_8h_4(TyH);
   test_uqrshrn_8b_8h_8(TyH);
   test_uqrshrn2_16b_8h_1(TyH);
   test_uqrshrn2_16b_8h_4(TyH);
   test_uqrshrn2_16b_8h_8(TyH);
   test_sqshrn_2s_2d_1(TyD);
   test_sqshrn_2s_2d_17(TyD);
   test_sqshrn_2s_2d_32(TyD);
   test_sqshrn2_4s_2d_1(TyD);
   test_sqshrn2_4s_2d_17(TyD);
   test_sqshrn2_4s_2d_32(TyD);
   test_sqshrn_4h_4s_1(TyS);
   test_sqshrn_4h_4s_9(TyS);
   test_sqshrn_4h_4s_16(TyS);
   test_sqshrn2_8h_4s_1(TyS);
   test_sqshrn2_8h_4s_9(TyS);
   test_sqshrn2_8h_4s_16(TyS);
   test_sqshrn_8b_8h_1(TyH);
   test_sqshrn_8b_8h_4(TyH);
   test_sqshrn_8b_8h_8(TyH);
   test_sqshrn2_16b_8h_1(TyH);
   test_sqshrn2_16b_8h_4(TyH);
   test_sqshrn2_16b_8h_8(TyH);
   test_uqshrn_2s_2d_1(TyD);
   test_uqshrn_2s_2d_17(TyD);
   test_uqshrn_2s_2d_32(TyD);
   test_uqshrn2_4s_2d_1(TyD);
   test_uqshrn2_4s_2d_17(TyD);
   test_uqshrn2_4s_2d_32(TyD);
   test_uqshrn_4h_4s_1(TyS);
   test_uqshrn_4h_4s_9(TyS);
   test_uqshrn_4h_4s_16(TyS);
   test_uqshrn2_8h_4s_1(TyS);
   test_uqshrn2_8h_4s_9(TyS);
   test_uqshrn2_8h_4s_16(TyS);
   test_uqshrn_8b_8h_1(TyH);
   test_uqshrn_8b_8h_4(TyH);
   test_uqshrn_8b_8h_8(TyH);
   test_uqshrn2_16b_8h_1(TyH);
   test_uqshrn2_16b_8h_4(TyH);
   test_uqshrn2_16b_8h_8(TyH);
   test_sqrshrun_2s_2d_1(TyD);
   test_sqrshrun_2s_2d_17(TyD);
   test_sqrshrun_2s_2d_32(TyD);
   test_sqrshrun2_4s_2d_1(TyD);
   test_sqrshrun2_4s_2d_17(TyD);
   test_sqrshrun2_4s_2d_32(TyD);
   test_sqrshrun_4h_4s_1(TyS);
   test_sqrshrun_4h_4s_9(TyS);
   test_sqrshrun_4h_4s_16(TyS);
   test_sqrshrun2_8h_4s_1(TyS);
   test_sqrshrun2_8h_4s_9(TyS);
   test_sqrshrun2_8h_4s_16(TyS);
   test_sqrshrun_8b_8h_1(TyH);
   test_sqrshrun_8b_8h_4(TyH);
   test_sqrshrun_8b_8h_8(TyH);
   test_sqrshrun2_16b_8h_1(TyH);
   test_sqrshrun2_16b_8h_4(TyH);
   test_sqrshrun2_16b_8h_8(TyH);
   test_sqshrun_2s_2d_1(TyD);
   test_sqshrun_2s_2d_17(TyD);
   test_sqshrun_2s_2d_32(TyD);
   test_sqshrun2_4s_2d_1(TyD);
   test_sqshrun2_4s_2d_17(TyD);
   test_sqshrun2_4s_2d_32(TyD);
   test_sqshrun_4h_4s_1(TyS);
   test_sqshrun_4h_4s_9(TyS);
   test_sqshrun_4h_4s_16(TyS);
   test_sqshrun2_8h_4s_1(TyS);
   test_sqshrun2_8h_4s_9(TyS);
   test_sqshrun2_8h_4s_16(TyS);
   test_sqshrun_8b_8h_1(TyH);
   test_sqshrun_8b_8h_4(TyH);
   test_sqshrun_8b_8h_8(TyH);
   test_sqshrun2_16b_8h_1(TyH);
   test_sqshrun2_16b_8h_4(TyH);
   test_sqshrun2_16b_8h_8(TyH);

   // sqshl (imm)  d,s,h,b   _#imm
   // uqshl (imm)  d,s,h,b   _#imm
   // sqshlu (imm) d,s,h,b   _#imm
   test_sqshl_d_d_0(TyD);
   test_sqshl_d_d_32(TyD);
   test_sqshl_d_d_63(TyD);
   test_sqshl_s_s_0(TyS);
   test_sqshl_s_s_16(TyS);
   test_sqshl_s_s_31(TyS);
   test_sqshl_h_h_0(TyH);
   test_sqshl_h_h_8(TyH);
   test_sqshl_h_h_15(TyH);
   test_sqshl_b_b_0(TyB);
   test_sqshl_b_b_4(TyB);
   test_sqshl_b_b_7(TyB);
   test_uqshl_d_d_0(TyD);
   test_uqshl_d_d_32(TyD);
   test_uqshl_d_d_63(TyD);
   test_uqshl_s_s_0(TyS);
   test_uqshl_s_s_16(TyS);
   test_uqshl_s_s_31(TyS);
   test_uqshl_h_h_0(TyH);
   test_uqshl_h_h_8(TyH);
   test_uqshl_h_h_15(TyH);
   test_uqshl_b_b_0(TyB);
   test_uqshl_b_b_4(TyB);
   test_uqshl_b_b_7(TyB);
   test_sqshlu_d_d_0(TyD);
   test_sqshlu_d_d_32(TyD);
   test_sqshlu_d_d_63(TyD);
   test_sqshlu_s_s_0(TyS);
   test_sqshlu_s_s_16(TyS);
   test_sqshlu_s_s_31(TyS);
   test_sqshlu_h_h_0(TyH);
   test_sqshlu_h_h_8(TyH);
   test_sqshlu_h_h_15(TyH);
   test_sqshlu_b_b_0(TyB);
   test_sqshlu_b_b_4(TyB);
   test_sqshlu_b_b_7(TyB);

   // sqshl (imm)  2d,4s,2s,8h,4h,16b,8b   _#imm
   // uqshl (imm)  2d,4s,2s,8h,4h,16b,8b   _#imm
   // sqshlu (imm) 2d,4s,2s,8h,4h,16b,8b   _#imm
   test_sqshl_2d_2d_0(TyD);
   test_sqshl_2d_2d_32(TyD);
   test_sqshl_2d_2d_63(TyD);
   test_sqshl_4s_4s_0(TyS);
   test_sqshl_4s_4s_16(TyS);
   test_sqshl_4s_4s_31(TyS);
   test_sqshl_2s_2s_0(TyS);
   test_sqshl_2s_2s_16(TyS);
   test_sqshl_2s_2s_31(TyS);
   test_sqshl_8h_8h_0(TyH);
   test_sqshl_8h_8h_8(TyH);
   test_sqshl_8h_8h_15(TyH);
   test_sqshl_4h_4h_0(TyH);
   test_sqshl_4h_4h_8(TyH);
   test_sqshl_4h_4h_15(TyH);
   test_sqshl_16b_16b_0(TyB);
   test_sqshl_16b_16b_3(TyB);
   test_sqshl_16b_16b_7(TyB);
   test_sqshl_8b_8b_0(TyB);
   test_sqshl_8b_8b_3(TyB);
   test_sqshl_8b_8b_7(TyB);
   test_uqshl_2d_2d_0(TyD);
   test_uqshl_2d_2d_32(TyD);
   test_uqshl_2d_2d_63(TyD);
   test_uqshl_4s_4s_0(TyS);
   test_uqshl_4s_4s_16(TyS);
   test_uqshl_4s_4s_31(TyS);
   test_uqshl_2s_2s_0(TyS);
   test_uqshl_2s_2s_16(TyS);
   test_uqshl_2s_2s_31(TyS);
   test_uqshl_8h_8h_0(TyH);
   test_uqshl_8h_8h_8(TyH);
   test_uqshl_8h_8h_15(TyH);
   test_uqshl_4h_4h_0(TyH);
   test_uqshl_4h_4h_8(TyH);
   test_uqshl_4h_4h_15(TyH);
   test_uqshl_16b_16b_0(TyB);
   test_uqshl_16b_16b_3(TyB);
   test_uqshl_16b_16b_7(TyB);
   test_uqshl_8b_8b_0(TyB);
   test_uqshl_8b_8b_3(TyB);
   test_uqshl_8b_8b_7(TyB);
   test_sqshlu_2d_2d_0(TyD);
   test_sqshlu_2d_2d_32(TyD);
   test_sqshlu_2d_2d_63(TyD);
   test_sqshlu_4s_4s_0(TyS);
   test_sqshlu_4s_4s_16(TyS);
   test_sqshlu_4s_4s_31(TyS);
   test_sqshlu_2s_2s_0(TyS);
   test_sqshlu_2s_2s_16(TyS);
   test_sqshlu_2s_2s_31(TyS);
   test_sqshlu_8h_8h_0(TyH);
   test_sqshlu_8h_8h_8(TyH);
   test_sqshlu_8h_8h_15(TyH);
   test_sqshlu_4h_4h_0(TyH);
   test_sqshlu_4h_4h_8(TyH);
   test_sqshlu_4h_4h_15(TyH);
   test_sqshlu_16b_16b_0(TyB);
   test_sqshlu_16b_16b_3(TyB);
   test_sqshlu_16b_16b_7(TyB);
   test_sqshlu_8b_8b_0(TyB);
   test_sqshlu_8b_8b_3(TyB);
   test_sqshlu_8b_8b_7(TyB);

   // sqxtn        s_d,h_s,b_h
   // uqxtn        s_d,h_s,b_h
   // sqxtun       s_d,h_s,b_h
   test_sqxtn_s_d(TyD);
   test_sqxtn_h_s(TyS);
   test_sqxtn_b_h(TyH);
   test_uqxtn_s_d(TyD);
   test_uqxtn_h_s(TyS);
   test_uqxtn_b_h(TyH);
   test_sqxtun_s_d(TyD);
   test_sqxtun_h_s(TyS);
   test_sqxtun_b_h(TyH);

   // sqxtn{2}     2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   // uqxtn{2}     2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   // sqxtun{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   test_sqxtn_2s_2d(TyD);
   test_sqxtn2_4s_2d(TyD);
   test_sqxtn_4h_4s(TyS);
   test_sqxtn2_8h_4s(TyS);
   test_sqxtn_8b_8h(TyH);
   test_sqxtn2_16b_8h(TyH);
   test_uqxtn_2s_2d(TyD);
   test_uqxtn2_4s_2d(TyD);
   test_uqxtn_4h_4s(TyS);
   test_uqxtn2_8h_4s(TyS);
   test_uqxtn_8b_8h(TyH);
   test_uqxtn2_16b_8h(TyH);
   test_sqxtun_2s_2d(TyD);
   test_sqxtun2_4s_2d(TyD);
   test_sqxtun_4h_4s(TyS);
   test_sqxtun2_8h_4s(TyS);
   test_sqxtun_8b_8h(TyH);
   test_sqxtun2_16b_8h(TyH);

   // srhadd       4s,2s,8h,4h,16b,8b
   // urhadd       4s,2s,8h,4h,16b,8b
   test_srhadd_4s_4s_4s(TyS);
   test_srhadd_2s_2s_2s(TyS);
   test_srhadd_8h_8h_8h(TyH);
   test_srhadd_4h_4h_4h(TyH);
   test_srhadd_16b_16b_16b(TyB);
   test_srhadd_8b_8b_8b(TyB);
   test_urhadd_4s_4s_4s(TyS);
   test_urhadd_2s_2s_2s(TyS);
   test_urhadd_8h_8h_8h(TyH);
   test_urhadd_4h_4h_4h(TyH);
   test_urhadd_16b_16b_16b(TyB);
   test_urhadd_8b_8b_8b(TyB);

   // sshl (reg)   d
   // ushl (reg)   d
   test_sshl_d_d_d(TyD);
   test_ushl_d_d_d(TyD);

   // sshl (reg)   2d,4s,2s,8h,4h,16b,8b
   // ushl (reg)   2d,4s,2s,8h,4h,16b,8b
   test_sshl_2d_2d_2d(TyD);
   test_sshl_4s_4s_4s(TyS);
   test_sshl_2s_2s_2s(TyS);
   test_sshl_8h_8h_8h(TyH);
   test_sshl_4h_4h_4h(TyH);
   test_sshl_16b_16b_16b(TyB);
   test_sshl_8b_8b_8b(TyB);
   test_ushl_2d_2d_2d(TyD);
   test_ushl_4s_4s_4s(TyS);
   test_ushl_2s_2s_2s(TyS);
   test_ushl_8h_8h_8h(TyH);
   test_ushl_4h_4h_4h(TyH);
   test_ushl_16b_16b_16b(TyB);
   test_ushl_8b_8b_8b(TyB);

   // shl  (imm)   d
   // sshr (imm)   d
   // ushr (imm)   d
   test_shl_d_d_0(TyD);
   test_shl_d_d_32(TyD);
   test_shl_d_d_63(TyD);
   test_sshr_d_d_1(TyD);
   test_sshr_d_d_32(TyD);
   test_sshr_d_d_64(TyD);
   test_ushr_d_d_1(TyD);
   test_ushr_d_d_32(TyD);
   test_ushr_d_d_64(TyD);

   // shl  (imm)   16b,8b,8h,4h,4s,2s,2d
   // sshr (imm)   2d,4s,2s,8h,4h,16b,8b
   // ushr (imm)   2d,4s,2s,8h,4h,16b,8b
   test_shl_2d_2d_0(TyD);
   test_shl_2d_2d_13(TyD);
   test_shl_2d_2d_63(TyD);
   test_shl_4s_4s_0(TyS);
   test_shl_4s_4s_13(TyS);
   test_shl_4s_4s_31(TyS);
   test_shl_2s_2s_0(TyS);
   test_shl_2s_2s_13(TyS);
   test_shl_2s_2s_31(TyS);
   test_shl_8h_8h_0(TyH);
   test_shl_8h_8h_13(TyH);
   test_shl_8h_8h_15(TyH);
   test_shl_4h_4h_0(TyH);
   test_shl_4h_4h_13(TyH);
   test_shl_4h_4h_15(TyH);
   test_shl_16b_16b_0(TyB);
   test_shl_16b_16b_7(TyB);
   test_shl_8b_8b_0(TyB);
   test_shl_8b_8b_7(TyB);
   test_sshr_2d_2d_1(TyD);
   test_sshr_2d_2d_13(TyD);
   test_sshr_2d_2d_64(TyD);
   test_sshr_4s_4s_1(TyS);
   test_sshr_4s_4s_13(TyS);
   test_sshr_4s_4s_32(TyS);
   test_sshr_2s_2s_1(TyS);
   test_sshr_2s_2s_13(TyS);
   test_sshr_2s_2s_32(TyS);
   test_sshr_8h_8h_1(TyH);
   test_sshr_8h_8h_13(TyH);
   test_sshr_8h_8h_16(TyH);
   test_sshr_4h_4h_1(TyH);
   test_sshr_4h_4h_13(TyH);
   test_sshr_4h_4h_16(TyH);
   test_sshr_16b_16b_1(TyB);
   test_sshr_16b_16b_8(TyB);
   test_sshr_8b_8b_1(TyB);
   test_sshr_8b_8b_8(TyB);
   test_ushr_2d_2d_1(TyD);
   test_ushr_2d_2d_13(TyD);
   test_ushr_2d_2d_64(TyD);
   test_ushr_4s_4s_1(TyS);
   test_ushr_4s_4s_13(TyS);
   test_ushr_4s_4s_32(TyS);
   test_ushr_2s_2s_1(TyS);
   test_ushr_2s_2s_13(TyS);
   test_ushr_2s_2s_32(TyS);
   test_ushr_8h_8h_1(TyH);
   test_ushr_8h_8h_13(TyH);
   test_ushr_8h_8h_16(TyH);
   test_ushr_4h_4h_1(TyH);
   test_ushr_4h_4h_13(TyH);
   test_ushr_4h_4h_16(TyH);
   test_ushr_16b_16b_1(TyB);
   test_ushr_16b_16b_8(TyB);
   test_ushr_8b_8b_1(TyB);
   test_ushr_8b_8b_8(TyB);

   // ssra (imm)   d
   // usra (imm)   d
   test_ssra_d_d_1(TyD);
   test_ssra_d_d_32(TyD);
   test_ssra_d_d_64(TyD);
   test_usra_d_d_1(TyD);
   test_usra_d_d_32(TyD);
   test_usra_d_d_64(TyD);

   // ssra (imm)   2d,4s,2s,8h,4h,16b,8b
   // usra (imm)   2d,4s,2s,8h,4h,16b,8b
   test_ssra_2d_2d_1(TyD);
   test_ssra_2d_2d_32(TyD);
   test_ssra_2d_2d_64(TyD);
   test_ssra_4s_4s_1(TyS);
   test_ssra_4s_4s_16(TyS);
   test_ssra_4s_4s_32(TyS);
   test_ssra_2s_2s_1(TyS);
   test_ssra_2s_2s_16(TyS);
   test_ssra_2s_2s_32(TyS);
   test_ssra_8h_8h_1(TyH);
   test_ssra_8h_8h_8(TyH);
   test_ssra_8h_8h_16(TyH);
   test_ssra_4h_4h_1(TyH);
   test_ssra_4h_4h_8(TyH);
   test_ssra_4h_4h_16(TyH);
   test_ssra_16b_16b_1(TyB);
   test_ssra_16b_16b_3(TyB);
   test_ssra_16b_16b_8(TyB);
   test_ssra_8b_8b_1(TyB);
   test_ssra_8b_8b_3(TyB);
   test_ssra_8b_8b_8(TyB);
   test_usra_2d_2d_1(TyD);
   test_usra_2d_2d_32(TyD);
   test_usra_2d_2d_64(TyD);
   test_usra_4s_4s_1(TyS);
   test_usra_4s_4s_16(TyS);
   test_usra_4s_4s_32(TyS);
   test_usra_2s_2s_1(TyS);
   test_usra_2s_2s_16(TyS);
   test_usra_2s_2s_32(TyS);
   test_usra_8h_8h_1(TyH);
   test_usra_8h_8h_8(TyH);
   test_usra_8h_8h_16(TyH);
   test_usra_4h_4h_1(TyH);
   test_usra_4h_4h_8(TyH);
   test_usra_4h_4h_16(TyH);
   test_usra_16b_16b_1(TyB);
   test_usra_16b_16b_3(TyB);
   test_usra_16b_16b_8(TyB);
   test_usra_8b_8b_1(TyB);
   test_usra_8b_8b_3(TyB);
   test_usra_8b_8b_8(TyB);

   // srshl (reg)  d
   // urshl (reg)  d
   test_srshl_d_d_d(TyD);
   test_urshl_d_d_d(TyD);

   // srshl (reg)  2d,4s,2s,8h,4h,16b,8b
   // urshl (reg)  2d,4s,2s,8h,4h,16b,8b
   test_srshl_2d_2d_2d(TyD);
   test_srshl_4s_4s_4s(TyS);
   test_srshl_2s_2s_2s(TyS);
   test_srshl_8h_8h_8h(TyH);
   test_srshl_4h_4h_4h(TyH);
   test_srshl_16b_16b_16b(TyB);
   test_srshl_8b_8b_8b(TyB);
   test_urshl_2d_2d_2d(TyD);
   test_urshl_4s_4s_4s(TyS);
   test_urshl_2s_2s_2s(TyS);
   test_urshl_8h_8h_8h(TyH);
   test_urshl_4h_4h_4h(TyH);
   test_urshl_16b_16b_16b(TyB);
   test_urshl_8b_8b_8b(TyB);

   // srshr (imm)  d
   // urshr (imm)  d
   test_srshr_d_d_1(TyD);
   test_srshr_d_d_32(TyD);
   test_srshr_d_d_64(TyD);
   test_urshr_d_d_1(TyD);
   test_urshr_d_d_32(TyD);
   test_urshr_d_d_64(TyD);

   // srshr (imm)  2d,4s,2s,8h,4h,16b,8b
   // urshr (imm)  2d,4s,2s,8h,4h,16b,8b
   test_srshr_2d_2d_1(TyD);
   test_srshr_2d_2d_32(TyD);
   test_srshr_2d_2d_64(TyD);
   test_srshr_4s_4s_1(TyS);
   test_srshr_4s_4s_16(TyS);
   test_srshr_4s_4s_32(TyS);
   test_srshr_2s_2s_1(TyS);
   test_srshr_2s_2s_16(TyS);
   test_srshr_2s_2s_32(TyS);
   test_srshr_8h_8h_1(TyH);
   test_srshr_8h_8h_8(TyH);
   test_srshr_8h_8h_16(TyH);
   test_srshr_4h_4h_1(TyH);
   test_srshr_4h_4h_8(TyH);
   test_srshr_4h_4h_16(TyH);
   test_srshr_16b_16b_1(TyB);
   test_srshr_16b_16b_3(TyB);
   test_srshr_16b_16b_8(TyB);
   test_srshr_8b_8b_1(TyB);
   test_srshr_8b_8b_3(TyB);
   test_srshr_8b_8b_8(TyB);
   test_urshr_2d_2d_1(TyD);
   test_urshr_2d_2d_32(TyD);
   test_urshr_2d_2d_64(TyD);
   test_urshr_4s_4s_1(TyS);
   test_urshr_4s_4s_16(TyS);
   test_urshr_4s_4s_32(TyS);
   test_urshr_2s_2s_1(TyS);
   test_urshr_2s_2s_16(TyS);
   test_urshr_2s_2s_32(TyS);
   test_urshr_8h_8h_1(TyH);
   test_urshr_8h_8h_8(TyH);
   test_urshr_8h_8h_16(TyH);
   test_urshr_4h_4h_1(TyH);
   test_urshr_4h_4h_8(TyH);
   test_urshr_4h_4h_16(TyH);
   test_urshr_16b_16b_1(TyB);
   test_urshr_16b_16b_3(TyB);
   test_urshr_16b_16b_8(TyB);
   test_urshr_8b_8b_1(TyB);
   test_urshr_8b_8b_3(TyB);
   test_urshr_8b_8b_8(TyB);

   // srsra (imm)  d
   // ursra (imm)  d
   test_srsra_d_d_1(TyD);
   test_srsra_d_d_32(TyD);
   test_srsra_d_d_64(TyD);
   test_ursra_d_d_1(TyD);
   test_ursra_d_d_32(TyD);
   test_ursra_d_d_64(TyD);

   // srsra (imm)  2d,4s,2s,8h,4h,16b,8b
   // ursra (imm)  2d,4s,2s,8h,4h,16b,8b
   test_srsra_2d_2d_1(TyD);
   test_srsra_2d_2d_32(TyD);
   test_srsra_2d_2d_64(TyD);
   test_srsra_4s_4s_1(TyS);
   test_srsra_4s_4s_16(TyS);
   test_srsra_4s_4s_32(TyS);
   test_srsra_2s_2s_1(TyS);
   test_srsra_2s_2s_16(TyS);
   test_srsra_2s_2s_32(TyS);
   test_srsra_8h_8h_1(TyH);
   test_srsra_8h_8h_8(TyH);
   test_srsra_8h_8h_16(TyH);
   test_srsra_4h_4h_1(TyH);
   test_srsra_4h_4h_8(TyH);
   test_srsra_4h_4h_16(TyH);
   test_srsra_16b_16b_1(TyB);
   test_srsra_16b_16b_3(TyB);
   test_srsra_16b_16b_8(TyB);
   test_srsra_8b_8b_1(TyB);
   test_srsra_8b_8b_3(TyB);
   test_srsra_8b_8b_8(TyB);
   test_ursra_2d_2d_1(TyD);
   test_ursra_2d_2d_32(TyD);
   test_ursra_2d_2d_64(TyD);
   test_ursra_4s_4s_1(TyS);
   test_ursra_4s_4s_16(TyS);
   test_ursra_4s_4s_32(TyS);
   test_ursra_2s_2s_1(TyS);
   test_ursra_2s_2s_16(TyS);
   test_ursra_2s_2s_32(TyS);
   test_ursra_8h_8h_1(TyH);
   test_ursra_8h_8h_8(TyH);
   test_ursra_8h_8h_16(TyH);
   test_ursra_4h_4h_1(TyH);
   test_ursra_4h_4h_8(TyH);
   test_ursra_4h_4h_16(TyH);
   test_ursra_16b_16b_1(TyB);
   test_ursra_16b_16b_3(TyB);
   test_ursra_16b_16b_8(TyB);
   test_ursra_8b_8b_1(TyB);
   test_ursra_8b_8b_3(TyB);
   test_ursra_8b_8b_8(TyB);

   // sshll{2} (imm)  2d_2s/4s, 4s_4h/8h, 8h_8b/16b
   // ushll{2} (imm)  2d_2s/4s, 4s_4h/8h, 8h_8b/16b
   test_sshll_2d_2s_0(TyS);
   test_sshll_2d_2s_15(TyS);
   test_sshll_2d_2s_31(TyS);
   test_sshll2_2d_4s_0(TyS);
   test_sshll2_2d_4s_15(TyS);
   test_sshll2_2d_4s_31(TyS);
   test_sshll_4s_4h_0(TyH);
   test_sshll_4s_4h_7(TyH);
   test_sshll_4s_4h_15(TyH);
   test_sshll2_4s_8h_0(TyH);
   test_sshll2_4s_8h_7(TyH);
   test_sshll2_4s_8h_15(TyH);
   test_sshll_8h_8b_0(TyB);
   test_sshll_8h_8b_3(TyB);
   test_sshll_8h_8b_7(TyB);
   test_sshll2_8h_16b_0(TyB);
   test_sshll2_8h_16b_3(TyB);
   test_sshll2_8h_16b_7(TyB);
   test_ushll_2d_2s_0(TyS);
   test_ushll_2d_2s_15(TyS);
   test_ushll_2d_2s_31(TyS);
   test_ushll2_2d_4s_0(TyS);
   test_ushll2_2d_4s_15(TyS);
   test_ushll2_2d_4s_31(TyS);
   test_ushll_4s_4h_0(TyH);
   test_ushll_4s_4h_7(TyH);
   test_ushll_4s_4h_15(TyH);
   test_ushll2_4s_8h_0(TyH);
   test_ushll2_4s_8h_7(TyH);
   test_ushll2_4s_8h_15(TyH);
   test_ushll_8h_8b_0(TyB);
   test_ushll_8h_8b_3(TyB);
   test_ushll_8h_8b_7(TyB);
   test_ushll2_8h_16b_0(TyB);
   test_ushll2_8h_16b_3(TyB);
   test_ushll2_8h_16b_7(TyB);

   // suqadd  d,s,h,b
   // usqadd  d,s,h,b
   test_suqadd_d_d(TyD);
   test_suqadd_s_s(TyS);
   test_suqadd_h_h(TyH);
   test_suqadd_b_b(TyB);
   test_usqadd_d_d(TyD);
   test_usqadd_s_s(TyS);
   test_usqadd_h_h(TyH);
   test_usqadd_b_b(TyB);

   // suqadd  2d,4s,2s,8h,4h,16b,8b
   // usqadd  2d,4s,2s,8h,4h,16b,8b
   test_suqadd_2d_2d(TyD);
   test_suqadd_4s_4s(TyS);
   test_suqadd_2s_2s(TyS);
   test_suqadd_8h_8h(TyH);
   test_suqadd_4h_4h(TyH);
   test_suqadd_16b_16b(TyB);
   test_suqadd_8b_8b(TyB);
   test_usqadd_2d_2d(TyD);
   test_usqadd_4s_4s(TyS);
   test_usqadd_2s_2s(TyS);
   test_usqadd_8h_8h(TyH);
   test_usqadd_4h_4h(TyH);
   test_usqadd_16b_16b(TyB);
   test_usqadd_8b_8b(TyB);

   // tbl     8b_{16b}_8b, 16b_{16b}_16b
   // tbl     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   // tbl     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   // tbl     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b
   test_tbl_16b_1reg(TyB);
   test_tbl_16b_2reg(TyB);
   test_tbl_16b_3reg(TyB);
   test_tbl_16b_4reg(TyB);
   test_tbl_8b_1reg(TyB);
   test_tbl_8b_2reg(TyB);
   test_tbl_8b_3reg(TyB);
   test_tbl_8b_4reg(TyB);

   // tbx     8b_{16b}_8b, 16b_{16b}_16b
   // tbx     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   // tbx     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   // tbx     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b
   test_tbx_16b_1reg(TyB);
   test_tbx_16b_2reg(TyB);
   test_tbx_16b_3reg(TyB);
   test_tbx_16b_4reg(TyB);
   test_tbx_8b_1reg(TyB);
   test_tbx_8b_2reg(TyB);
   test_tbx_8b_3reg(TyB);
   test_tbx_8b_4reg(TyB);

   // trn1    2d,4s,2s,8h,4h,16b,8b
   // trn2    2d,4s,2s,8h,4h,16b,8b
   test_trn1_2d_2d_2d(TyD);
   test_trn1_4s_4s_4s(TyS);
   test_trn1_2s_2s_2s(TyS);
   test_trn1_8h_8h_8h(TyH);
   test_trn1_4h_4h_4h(TyH);
   test_trn1_16b_16b_16b(TyB);
   test_trn1_8b_8b_8b(TyB);
   test_trn2_2d_2d_2d(TyD);
   test_trn2_4s_4s_4s(TyS);
   test_trn2_2s_2s_2s(TyS);
   test_trn2_8h_8h_8h(TyH);
   test_trn2_4h_4h_4h(TyH);
   test_trn2_16b_16b_16b(TyB);
   test_trn2_8b_8b_8b(TyB);

   // urecpe      4s,2s
   // ursqrte     4s,2s
   test_urecpe_4s_4s(TyS);
   test_urecpe_2s_2s(TyS);
   test_ursqrte_4s_4s(TyS);
   test_ursqrte_2s_2s(TyS);

   // uzp1      2d,4s,2s,8h,4h,16b,8b
   // uzp2      2d,4s,2s,8h,4h,16b,8b
   // zip1      2d,4s,2s,8h,4h,16b,8b
   // zip2      2d,4s,2s,8h,4h,16b,8b
   test_uzp1_2d_2d_2d(TyD);
   test_uzp1_4s_4s_4s(TyS);
   test_uzp1_2s_2s_2s(TyS);
   test_uzp1_8h_8h_8h(TyH);
   test_uzp1_4h_4h_4h(TyH);
   test_uzp1_16b_16b_16b(TyB);
   test_uzp1_8b_8b_8b(TyB);
   test_uzp2_2d_2d_2d(TyD);
   test_uzp2_4s_4s_4s(TyS);
   test_uzp2_2s_2s_2s(TyS);
   test_uzp2_8h_8h_8h(TyH);
   test_uzp2_4h_4h_4h(TyH);
   test_uzp2_16b_16b_16b(TyB);
   test_uzp2_8b_8b_8b(TyB);
   test_zip1_2d_2d_2d(TyD);
   test_zip1_4s_4s_4s(TyS);
   test_zip1_2s_2s_2s(TyS);
   test_zip1_8h_8h_8h(TyH);
   test_zip1_4h_4h_4h(TyH);
   test_zip1_16b_16b_16b(TyB);
   test_zip1_8b_8b_8b(TyB);
   test_zip2_2d_2d_2d(TyD);
   test_zip2_4s_4s_4s(TyS);
   test_zip2_2s_2s_2s(TyS);
   test_zip2_8h_8h_8h(TyH);
   test_zip2_4h_4h_4h(TyH);
   test_zip2_16b_16b_16b(TyB);
   test_zip2_8b_8b_8b(TyB);

   // xtn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   test_xtn_2s_2d(TyD);
   test_xtn2_4s_2d(TyD);
   test_xtn_4h_4s(TyS);
   test_xtn2_8h_4s(TyS);
   test_xtn_8b_8h(TyH);
   test_xtn2_16b_8h(TyH);

   // ======================== MEM ========================

   // ld1  (multiple 1-element structures to 1/2/3/4 regs)
   // ld1  (single 1-element structure to one lane of 1 reg)
   // ld1r (single 1-element structure and rep to all lanes of 1 reg)

   // ld2  (multiple 2-element structures to 2 regs)
   // ld2  (single 2-element structure to one lane of 2 regs)
   // ld2r (single 2-element structure and rep to all lanes of 2 regs)

   // ld3  (multiple 3-element structures to 3 regs)
   // ld3  (single 3-element structure to one lane of 3 regs)
   // ld3r (single 3-element structure and rep to all lanes of 3 regs)

   // ld4  (multiple 4-element structures to 4 regs)
   // ld4  (single 4-element structure to one lane of 4 regs)
   // ld4r (single 4-element structure and rep to all lanes of 4 regs)

   // ldnp  q_q_addr,d_d_addr,s_s_addr  (load pair w/ non-temporal hint)
   //       addr = reg + uimm7 * reg_size

   // ldp   q_q_addr,d_d_addr,s_s_addr  (load pair)
   //       addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   // ldr   q,d,s,h,b from addr
   //       addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   // ldr   q,d,s from  pc+#imm19

   // ldr   q,d,s,h,b from addr
   //       addr = [Xn|SP, R <extend> <shift]

   // ldur  q,d,s,h,b from addr
   //       addr = [Xn|SP,#imm] (unscaled offset)

   // st1 (multiple 1-element structures from 1/2/3/4 regs)
   // st1 (single 1-element structure for 1 lane of 1 reg)

   // st2 (multiple 2-element structures from 2 regs)
   // st2 (single 2-element structure from 1 lane of 2 regs)

   // st3 (multiple 3-element structures from 3 regs)
   // st3 (single 3-element structure from 1 lane of 3 regs)

   // st4 (multiple 4-element structures from 4 regs)
   // st4 (single 4-element structure from one lane of 4 regs)

   // stnp q_q_addr, d_d_addr, s_s_addr
   //      addr = [Xn|SP, #imm]

   // stp  q_q_addr, d_d_addr, s_s_addr
   //      addr = [Xn|SP], #imm  or [Xn|SP, #imm]!  or [Xn|SP, #imm]

   // str  q,d,s,h,b_addr
   //      addr = [Xn|SP], #simm  or [Xn|SP, #simm]!  or [Xn|SP, #pimm]

   // str   q,d,s,h,b_addr
   //       addr = [Xn|SP, R <extend> <shift]

   // stur  q,d,s,h,b_addr
   //       addr = [Xn|SP,#imm] (unscaled offset)

   // ======================== CRYPTO ========================

   // aesd       16b (aes single round decryption)
   // aese       16b (aes single round encryption)
   // aesimc     16b (aes inverse mix columns)
   // aesmc      16b (aes mix columns)

   // sha1c      q_s_4s
   // sha1h      s_s
   // sha1m      q_s_4s
   // sha1p      q_s_4s
   // sha1su0    4s_4s_4s
   // sha1su1    4s_4s

   // sha256h2   q_q_4s
   // sha256h    q_q_4s
   // sha256su0  4s_4s
   // sha256su1  4s_4s_4s

   return 0;
}


/* ---------------------------------------------------------------- */
/* -- Alphabetical list of insns                                 -- */
/* ---------------------------------------------------------------- */
/*
   abs      d
   abs      2d,4s,2s,8h,4h,16b,8b
   add      d
   add      2d,4s,2s,8h,4h,16b,8b
   addhn    2s.2d.2d, 4s.2d.2d, h_from_s and b_from_h (add and get high half)
   addp     d (add pairs, across)
   addp     2d,4s,2s,8h,4h,16b,8b
   addv     4s,8h,4h,16b,18b (reduce across vector)
   aesd     16b (aes single round decryption)
   aese     16b (aes single round encryption)
   aesimc   16b (aes inverse mix columns)
   aesmc    16b (aes mix columns)
   and      16b,8b

   bic      4s,2s,8h,4h (vector, imm)
   also movi, mvni, orr

   bic      16b,8b (vector,reg) (bit clear)
   bif      16b,8b (vector) (bit insert if false)
   bit      16b,8b (vector) (bit insert if true)
   bsl      16b,8b (vector) (bit select)

   cls      4s,2s,8h,4h,16b,8b (count leading sign bits)
   clz      4s,2s,8h,4h,16b,8b (count leading zero bits)

   cmeq     d
   cmeq     2d,4s,2s,8h,4h,16b,8b
   cmeq_z   d
   cmeq_z   2d,4s,2s,8h,4h,16b,8b

   cmge     d
   cmge     2d,4s,2s,8h,4h,16b,8b
   cmge_z   d
   cmge_z   2d,4s,2s,8h,4h,16b,8b

   cmgt     d
   cmgt     2d,4s,2s,8h,4h,16b,8b
   cmgt_z   d
   cmgt_z   2d,4s,2s,8h,4h,16b,8b

   cmhi     d
   cmhi     2d,4s,2s,8h,4h,16b,8b

   cmhs     d
   cmhs     2d,4s,2s,8h,4h,16b,8b

   cmle_z   d
   cmle_z   2d,4s,2s,8h,4h,16b,8b

   cmlt_z   d
   cmlt_z   2d,4s,2s,8h,4h,16b,8b

   cmtst    d
   cmtst    2d,4s,2s,8h,4h,16b,8b

   cnt      16b,8b (population count per byte)

   dup      d,s,h,b (vec elem to scalar)
   dup      2d,4s,2s,8h,4h,16b,8b (vec elem to vector)
   dup      2d,4s,2s,8h,4h,16b,8b (general reg to vector)

   eor      16b,8b (vector)
   ext      16b,8b,#imm4 (concat 2 vectors, then slice)

   fabd     d,s
   fabd     2d,4s,2s

   fabs     d,s
   fabs     2d,4s,2s

   facge    s,d  (floating abs compare GE)
   facge    2d,4s,2s

   facgt    s,d  (floating abs compare GE)
   facgt    2d,4s,2s

   fadd     d,s
   fadd     2d,4s,2s

   faddp    d,s (floating add pair)
   faddp    2d,4s,2s

   fccmp    d,s (floating point conditional quiet compare)
   fccmpe   d,s (floating point conditional signaling compare)

   fcmeq    d,s
   fcmeq    2d,4s,2s
   fcmeq_z  d,s
   fcmeq_z  2d,4s,2s

   fcmge    d,s
   fcmge    2d,4s,2s
   fcmge_z  d,s
   fcmge_z  2d,4s,2s

   fcmgt    d,s
   fcmgt    2d,4s,2s
   fcmgt_z  d,s
   fcmgt_z  2d,4s,2s

   fcmle_z  d,s
   fcmle_z  2d,4s,2s

   fcmlt_z  d,s
   fcmlt_z  2d,4s,2s

   fcmp     d,s (floating point quiet, set flags)
   fcmp_z   d,s
   fcmpe    d,s (floating point signaling, set flags)
   fcmpe_z  d,s

   fcsel    d,s (fp cond select)

   fcvt     s_h,d_h,h_s,d_s,h_d,s_d (fp convert, scalar)

   fcvtas   d,s  (fcvt to signed int, nearest, ties away)
   fcvtas   2d,4s,2s
   fcvtas   w_s,x_s,w_d,x_d

   fcvtau   d,s  (fcvt to unsigned int, nearest, ties away)
   fcvtau   2d,4s,2s
   fcvtau   w_s,x_s,w_d,x_d

   fcvtl{2} 4s/4h, 4s/8h, 2d/2s, 2d/4s (float convert to longer form)

   fcvtms   d,s  (fcvt to signed int, minus inf)
   fcvtms   2d,4s,2s
   fcvtms   w_s,x_s,w_d,x_d

   fcvtmu   d,s  (fcvt to unsigned int, minus inf)
   fcvtmu   2d,4s,2s
   fcvtmu   w_s,x_s,w_d,x_d

   fcvtn{2} 4h/4s, 8h/4s, 2s/2d, 4s/2d (float convert to narrower form)

   fcvtns   d,s  (fcvt to signed int, nearest)
   fcvtns   2d,4s,2s
   fcvtns   w_s,x_s,w_d,x_d

   fcvtnu   d,s  (fcvt to unsigned int, nearest)
   fcvtnu   2d,4s,2s
   fcvtnu   w_s,x_s,w_d,x_d

   fcvtps   d,s  (fcvt to signed int, plus inf)
   fcvtps   2d,4s,2s
   fcvtps   w_s,x_s,w_d,x_d

   fcvtpu   d,s  (fcvt to unsigned int, plus inf)
   fcvtpu   2d,4s,2s
   fcvtpu   w_s,x_s,w_d,x_d

   fcvtxn   s_d (fcvt to lower prec narrow, rounding to odd)
   fcvtxn   2s_2d,4s_2d

   fcvtzs   s,d (fcvt to signed fixedpt, to zero) (w/ #fbits)
   fcvtzs   2d,4s,2s

   fcvtzs   s,d (fcvt to signed integer, to zero)
   fcvtzs   2d,4s,2s

   fcvtzs   w_s,x_s,w_d,x_d (fcvt to signed fixedpt, to zero) (w/ #fbits)

   fcvtzs   w_s,x_s,w_d,x_d (fcvt to signed integer, to zero)

   fcvtzu   s,d (fcvt to unsigned fixedpt, to zero) (w/ #fbits)
   fcvtzu   2d,4s,2s

   fcvtzu   s,d (fcvt to unsigned integer, to zero)
   fcvtzu   2d,4s,2s

   fcvtzu   w_s,x_s,w_d,x_d (fcvt to unsigned fixedpt, to zero) (w/ #fbits)

   fcvtzu   w_s,x_s,w_d,x_d (fcvt to unsigned integer, to zero)

   fdiv     d,s
   fdiv     2d,4s,2s

   fmadd    d,s
   fnmadd   d,s
   fnmsub   d,s
   fnmul    d,s

   fmax     d,s
   fmin     d,s

   fmax     2d,4s,2s
   fmin     2d,4s,2s

   fmaxnm   d,s ("max number")
   fminnm   d,s

   fmaxnm   2d,4s,2s
   fminnm   2d,4s,2s

   fmaxnmp  d_2d,s_2s ("max number pairwise")
   fminnmp  d_2d,s_2s

   fmaxnmp  2d,4s,2s
   fminnmp  2d,4s,2s

   fmaxnmv  s_4s (maxnum across vector)
   fminnmv  s_4s

   fmaxp    d_2d,s_2s (max of a pair)
   fminp    d_2d,s_2s (max of a pair)

   fmaxp    2d,4s,2s  (max pairwise)
   fminp    2d,4s,2s

   fmaxv    s_4s (max across vector)
   fminv    s_4s

   fmla     d_d_d[],s_s_s[] (by element)
   fmla     2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmla     2d,4s,2s

   fmls     d_d_d[],s_s_s[] (by element)
   fmls     2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmls     2d,4s,2s

   fmov     2d,4s,2s #imm (part of the MOVI/MVNI/ORR/BIC imm group)

   fmov     d_d,s_s

   fmov     s_w,w_s,d_x,d[1]_x,x_d,x_d[1]

   fmov     d,s #imm

   fmsub    d,s

   fmul     d_d_d[],s_s_s[]
   fmul     2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmul     2d,4s,2s
   fmul     d,s

   fmulx    d_d_d[],s_s_s[]
   fmulx    2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmulx    d,s
   fmulx    2d,4s,2s

   fneg     d,s
   fneg     2d,4s,2s

   frecpe   d,s (recip estimate)
   frecpe   2d,4s,2s

   frecps   d,s (recip step)
   frecps   2d,4s,2s

   frecpx   d,s (recip exponent)

   frinta   2d,4s,2s (round to integral, nearest away)
   frinta   d,s

   frinti   2d,4s,2s (round to integral, per FPCR)
   frinti   d,s

   frintm   2d,4s,2s (round to integral, minus inf)
   frintm   d,s

   frintn   2d,4s,2s (round to integral, nearest, to even)
   frintn   d,s

   frintp   2d,4s,2s (round to integral, plus inf)
   frintp   d,s

   frintx   2d,4s,2s (round to integral exact, per FPCR)
   frintx   d,s

   frintz   2d,4s,2s (round to integral, zero)
   frintz   d,s

   frsqrte  d,s (est)
   frsqrte  2d,4s,2s

   frsqrts  d,s (step)
   frsqrts  2d,4s,2s

   fsqrt    d,s
   fsqrt    2d,4s,2s

   fsub     d,s
   fsub     2d,4s,2s

   ins      d[]_d[],s[]_s[],h[]_h[],b[]_b[]

   ins      d[]_x, s[]_w, h[]_w, b[]_w

   ld1  (multiple 1-element structures to 1/2/3/4 regs)
   ld1  (single 1-element structure to one lane of 1 reg)
   ld1r (single 1-element structure and rep to all lanes of 1 reg)

   ld2  (multiple 2-element structures to 2 regs)
   ld2  (single 2-element structure to one lane of 2 regs)
   ld2r (single 2-element structure and rep to all lanes of 2 regs)

   ld3  (multiple 3-element structures to 3 regs)
   ld3  (single 3-element structure to one lane of 3 regs)
   ld3r (single 3-element structure and rep to all lanes of 3 regs)

   ld4  (multiple 4-element structures to 4 regs)
   ld4  (single 4-element structure to one lane of 4 regs)
   ld4r (single 4-element structure and rep to all lanes of 4 regs)

   ldnp  q_q_addr,d_d_addr,s_s_addr  (load pair w/ non-temporal hint)
         addr = reg + uimm7 * reg_size

   ldp   q_q_addr,d_d_addr,s_s_addr  (load pair)
         addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   ldr   q,d,s,h,b from addr
         addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   ldr   q,d,s from  pc+#imm19

   ldr   q,d,s,h,b from addr
         addr = [Xn|SP, R <extend> <shift]

   ldur  q,d,s,h,b from addr
         addr = [Xn|SP,#imm] (unscaled offset)

   mla   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mla   4s,2s,8h,4h,16b,8b

   mls   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mls   4s,2s,8h,4h,16b,8b

   movi  16b,8b   #imm8, LSL #0
   movi  8h,4h    #imm8, LSL #0 or 8
   movi  4s,2s    #imm8, LSL #0, 8, 16, 24
   movi  4s,2s    #imm8, MSL #8 or 16
   movi  d,       #imm64
   movi  2d,      #imm64

   mul   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mul   4s,2s,8h,4h,16b,8b

   mvni  8h,4h    #imm8, LSL #0 or 8
   mvni  4s,2s    #imm8, LSL #0, 8, 16, 24
   mvni  4s,2s    #imm8, MSL #8 or 16

   neg   d
   neg   2d,4s,2s,8h,4h,16b,8b

   not   16b,8b

   orn   16b,8b

   orr   8h,4h   #imm8, LSL #0 or 8
   orr   4s,2s   #imm8, LSL #0, 8, 16 or 24

   orr   16b,8b

   pmul  16b,8b

   pmull{2}  8h_8b_8b,8h_16b_16b,1q_1d_1d,1d_2d_2d

   raddhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h

   rbit    16b,8b
   rev16   16b,8b
   rev32   16b,8b,8h,4h
   rev64   16b,8b,8h,4h,4s,2s

   rshrn{2}  2s/4s_2d, 8h/4h_4s, 2s/4s_2d,   #imm in 1 .. elem_bits

   rsubhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h

   saba      16b,8b,8h,4h,4s,2s
   sabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   sabd      16b,8b,8h,4h,4s,2s
   sabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   sadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   saddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   saddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   saddlv    h_16b/8b, s_8h/4h, d_4s

   saddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s

   scvtf     d,s        _#fbits
   scvtf     2d,4s,2s   _#fbits

   scvtf     d,s
   scvtf     2d,4s,2s

   scvtf     s_w, d_w, s_x, d_x,   _#fbits
   scvtf     s_w, d_w, s_x, d_x

   sha1c       q_s_4s
   sha1h       s_s
   sha1m       q_s_4s
   sha1p       q_s_4s
   sha1su0     4s_4s_4s
   sha1su1     4s_4s
   sha256h2    q_q_4s
   sha256h     q_q_4s
   sha256su0   4s_4s
   sha256su1   4s_4s_4s

   shadd       16b,8b,8h,4h,4s,2s

   shl         d_#imm
   shl         16b,8b,8h,4h,4s,2s,2d  _#imm

   shll{2}   8h_8b/16b_#8, 4s_4h/8h_#16, 2d_2s/4s_#32

   shrn{2}  2s/4s_2d, 8h/4h_4s, 2s/4s_2d,   #imm in 1 .. elem_bits

   shsub       16b,8b,8h,4h,4s,2s

   sli         d_#imm
   sli         2d,4s,2s,8h,4h,16b,8b  _#imm

   smax        4s,2s,8h,4h,16b,8b

   smaxp       4s,2s,8h,4h,16b,8b

   smaxv       s_4s,h_8h,h_4h,b_16b,b_8b

   smin        4s,2s,8h,4h,16b,8b

   sminp       4s,2s,8h,4h,16b,8b

   sminv       s_4s,h_8h,h_4h,b_16b,b_8b

   smlal{2}    2d_2s/4s_s[], 4s_4h/8h_h[]
   smlal{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   smlsl{2}    2d_2s/4s_s[], 4s_4h/8h_h[]
   smlsl{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   smov        w_b[], w_h[], x_b[], x_h[], x_s[]

   smull{2}    2d_2s/4s_s[]. 4s_4h/8h_h[]
   smull{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   sqabs       d,s,h,b
   sqabs       2d,4s,2s,8h,4h,16b,8b

   sqadd       d,s,h,b
   sqadd       2d,4s,2s,8h,4h,16b,8b

   sqdmlal     d_s_s[], s_h_h[]
   sqdmlal{2}  2d_2s/4s_s[], 4s_4h/8h_h[]

   sqdmlal     d_s_s, s_h_h
   sqdmlal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)

   sqdmlsl     d_s_s[], s_h_h[]
   sqdmlsl{2}  2d_2s/4s_s[], 4s_4h/8h_h[]

   sqdmlsl     d_s_s, s_h_h
   sqdmlsl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)

   sqdmulh     s_s_s[], h_h_h[]
   sqdmulh     4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]

   sqdmulh     h,s
   sqdmulh     4s,2s,8h,4h

   sqdmull     d_s_s[], s_h_h[]
   sqdmull{2}  2d_2s/4s_s[], 4s_4h/2h_h[]

   sqdmull     d_s_s,s_h_h
   sqdmull{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)

   sqneg       d,s,h,b
   sqneg       2d,4s,2s,8h,4h,16b,8b

   sqrdmulh    s_s_s[], h_h_h[]
   sqrdmulh    4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]

   sqrdmulh    h,s
   sqrdmulh    4s,2s,8h,4h

   sqrshl      d,s,h,b
   sqrshl      2d,4s,2s,8h,4h,16b,8b

   sqrshrn     s_d, h_s, b_h   #imm
   sqrshrn{2}  2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqrshrun     s_d, h_s, b_h   #imm
   sqrshrun{2}  2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqshl        d,s,h,b   _#imm
   sqshl        2d,4s,2s,8h,4h,16b,8b   _#imm

   sqshl        d,s,h,b
   sqshl        2d,4s,2s,8h,4h,16b,8b

   sqshlu       d,s,h,b  _#imm
   sqshlu       2d,4s,2s,8h,4h,16b,8b  _#imm

   sqshrn       s_d, h_s, b_h   #imm
   sqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqshrun      s_d, h_s, b_h   #imm
   sqshrun{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqsub       d,s,h,b
   sqsub       2d,4s,2s,8h,4h,16b,8b

   sqxtn       s_d,h_s,b_h
   sqxtn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   sqxtun      s_d,h_s,b_h
   sqxtun{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   srhadd      4s,2s,8h,4h,16b,8b

   sri         d_#imm
   sri         2d,4s,2s,8h,4h,16b,8b  _#imm

   srshl (reg) d
   srshl       2d,4s,2s,8h,4h,16b,8b

   srshr (imm) d
   srshr       2d,4s,2s,8h,4h,16b,8b

   srsra (imm) d
   srsra       2d,4s,2s,8h,4h,16b,8b

   sshl (reg)  d
   sshl        2d,4s,2s,8h,4h,16b,8b

   sshll{2} (imm)  2d_2s/4s  4s_4h/8h, 8h_8b/16b

   sshr (imm)  d
   sshr        2d,4s,2s,8h,4h,16b,8b

   ssra (imm)  d
   ssra        2d,4s,2s,8h,4h,16b,8b

   ssubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   ssubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s

   st1 (multiple 1-element structures from 1/2/3/4 regs)
   st1 (single 1-element structure for 1 lane of 1 reg)

   st2 (multiple 2-element structures from 2 regs)
   st2 (single 2-element structure from 1 lane of 2 regs)

   st3 (multiple 3-element structures from 3 regs)
   st3 (single 3-element structure from 1 lane of 3 regs)

   st4 (multiple 4-element structures from 4 regs)
   st4 (single 4-element structure from one lane of 4 regs)

   stnp q_q_addr, d_d_addr, s_s_addr
        addr = [Xn|SP, #imm]

   stp  q_q_addr, d_d_addr, s_s_addr
        addr = [Xn|SP], #imm  or [Xn|SP, #imm]!  or [Xn|SP, #imm]

   str  q,d,s,h,b_addr
        addr = [Xn|SP], #simm  or [Xn|SP, #simm]!  or [Xn|SP, #pimm]

   str   q,d,s,h,b_addr
         addr = [Xn|SP, R <extend> <shift]

   stur  q,d,s,h,b_addr
         addr = [Xn|SP,#imm] (unscaled offset)

   sub   d
   sub   2d,4s,2s,8h,4h,16b,8b

   subhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h

   suqadd  d,s,h,b
   suqadd  2d,4s,2s,8h,4h,16b,8b

   tbl     8b_{16b}_8b, 16b_{16b}_16b
   tbl     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   tbl     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   tbl     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b

   tbx     8b_{16b}_8b, 16b_{16b}_16b
   tbx     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   tbx     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   tbx     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b

   trn1    2d,4s,2s,8h,4h,16b,8b
   trn2    2d,4s,2s,8h,4h,16b,8b

   uaba      16b,8b,8h,4h,4s,2s
   uabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   uabd      16b,8b,8h,4h,4s,2s
   uabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   uadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   uaddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   uaddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   uaddlv    h_16b/8b, s_8h/4h, d_4s

   uaddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s

   ucvtf     d,s        _#fbits
   ucvtf     2d,4s,2s   _#fbits

   ucvtf     d,s
   ucvtf     2d,4s,2s

   ucvtf     s_w, d_w, s_x, d_x,   _#fbits
   ucvtf     s_w, d_w, s_x, d_x

   uhadd       16b,8b,8h,4h,4s,2s

   uhsub       16b,8b,8h,4h,4s,2s

   umax        4s,2s,8h,4h,16b,8b

   umaxp       4s,2s,8h,4h,16b,8b

   umaxv       s_4s,h_8h,h_4h,b_16b,b_8b

   umin        4s,2s,8h,4h,16b,8b

   uminp       4s,2s,8h,4h,16b,8b

   uminv       s_4s,h_8h,h_4h,b_16b,b_8b

   umlal{2}    2d_2s/4s_s[], 4s_4h/8h_h[]
   umlal{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   umlsl{2}    2d_2s/4s_s[], 4s_4h/8h_h[]
   umlsl{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   umov        w_b[], w_h[], x_b[], x_h[], x_s[]

   umull{2}    2d_2s/4s_s[]. 4s_4h/8h_h[]
   umull{2}    2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   uqadd       d,s,h,b
   uqadd       2d,4s,2s,8h,4h,16b,8b

   uqrshl      d,s,h,b
   uqrshl      2d,4s,2s,8h,4h,16b,8b

   uqrshrn     s_d, h_s, b_h   #imm
   uqrshrn{2}  2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   uqshl        d,s,h,b   _#imm
   uqshl        2d,4s,2s,8h,4h,16b,8b   _#imm

   uqshl        d,s,h,b
   uqshl        2d,4s,2s,8h,4h,16b,8b

   uqshrn       s_d, h_s, b_h   #imm
   uqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   uqsub       d,s,h,b
   uqsub       2d,4s,2s,8h,4h,16b,8b

   uqxtn       s_d,h_s,b_h
   uqxtn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   urecpe      4s,2s

   urhadd      4s,2s,8h,4h,16b,8b

   urshl (reg) d
   urshl       2d,4s,2s,8h,4h,16b,8b

   urshr (imm) d
   urshr       2d,4s,2s,8h,4h,16b,8b

   ursqrte     4s,2s

   ursra (imm) d
   ursra       2d,4s,2s,8h,4h,16b,8b

   ushl (reg)  d
   ushl        2d,4s,2s,8h,4h,16b,8b

   ushll{2} (imm)  2d_2s/4s  4s_4h/8h, 8h_8b/16b

   ushr (imm)  d
   ushr        2d,4s,2s,8h,4h,16b,8b

   usqadd      d,s,h,b
   usqadd      2d,4s,2s,8h,4h,16b,8b

   usra (imm)  d
   usra        2d,4s,2s,8h,4h,16b,8b

   usubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   usubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s

   uzp1      2d,4s,2s,8h,4h,16b,8b
   uzp2      2d,4s,2s,8h,4h,16b,8b

   xtn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   zip1      2d,4s,2s,8h,4h,16b,8b
   zip2      2d,4s,2s,8h,4h,16b,8b
*/


/* ---------------------------------------------------------------- */
/* -- List of insns, grouped somewhat by laneage configuration   -- */
/* ---------------------------------------------------------------- */
/*
   ======================== FP ========================

   fabs      d,s
   fabs      2d,4s,2s

   fneg      d,s
   fneg      2d,4s,2s

   fsqrt     d,s
   fsqrt     2d,4s,2s

   fadd      d,s
   fsub      d,s

   fadd      2d,4s,2s
   fsub      2d,4s,2s

   fabd      d,s
   fabd      2d,4s,2s

   faddp     d,s (floating add pair)
   faddp     2d,4s,2s

   fccmp     d,s (floating point conditional quiet compare)
   fccmpe    d,s (floating point conditional signaling compare)

   fcmeq     d,s
   fcmge     d,s
   fcmgt     d,s
   facgt     d,s  (floating abs compare GE)
   facge     d,s  (floating abs compare GE)

   fcmeq     2d,4s,2s
   fcmge     2d,4s,2s
   fcmgt     2d,4s,2s
   facge     2d,4s,2s
   facgt     2d,4s,2s

   fcmeq_z   d,s
   fcmge_z   d,s
   fcmgt_z   d,s
   fcmle_z   d,s
   fcmlt_z   d,s

   fcmeq_z   2d,4s,2s
   fcmge_z   2d,4s,2s
   fcmgt_z   2d,4s,2s
   fcmle_z   2d,4s,2s
   fcmlt_z   2d,4s,2s

   fcmp_z    d,s
   fcmpe_z   d,s
   fcmp      d,s (floating point quiet, set flags)
   fcmpe     d,s (floating point signaling, set flags)

   fcsel     d,s (fp cond select)

   fdiv      d,s
   fdiv      2d,4s,2s

   fmadd     d,s
   fnmadd    d,s
   fmsub     d,s
   fnmsub    d,s

   fnmul     d,s

   fmax      d,s
   fmin      d,s
   fmaxnm    d,s ("max number")
   fminnm    d,s

   fmax      2d,4s,2s
   fmin      2d,4s,2s
   fmaxnm    2d,4s,2s
   fminnm    2d,4s,2s

   fmaxnmp   d_2d,s_2s ("max number pairwise")
   fminnmp   d_2d,s_2s

   fmaxnmp   2d,4s,2s
   fminnmp   2d,4s,2s

   fmaxnmv   s_4s (maxnum across vector)
   fminnmv   s_4s

   fmaxp     d_2d,s_2s (max of a pair)
   fminp     d_2d,s_2s (max of a pair)

   fmaxp     2d,4s,2s  (max pairwise)
   fminp     2d,4s,2s

   fmaxv     s_4s (max across vector)
   fminv     s_4s

   fmla      2d,4s,2s
   fmls      2d,4s,2s

   fmla      d_d_d[],s_s_s[] (by element)
   fmls      d_d_d[],s_s_s[] (by element)

   fmla      2d_2d_d[],4s_4s_s[],2s_2s_s[]
   fmls      2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmov      2d,4s,2s #imm (part of the MOVI/MVNI/ORR/BIC imm group)

   fmov      d_d,s_s

   fmov      s_w,w_s,d_x,d[1]_x,x_d,x_d[1]

   fmov      d,s #imm

   fmul      d_d_d[],s_s_s[]
   fmul      2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmul      2d,4s,2s
   fmul      d,s

   fmulx     d_d_d[],s_s_s[]
   fmulx     2d_2d_d[],4s_4s_s[],2s_2s_s[]

   fmulx     d,s
   fmulx     2d,4s,2s

   frecpe    d,s (recip estimate)
   frecpe    2d,4s,2s

   frecps    d,s (recip step)
   frecps    2d,4s,2s

   frecpx    d,s (recip exponent)

   frinta    d,s
   frinti    d,s
   frintm    d,s
   frintn    d,s
   frintp    d,s
   frintx    d,s
   frintz    d,s

   frinta    2d,4s,2s (round to integral, nearest away)
   frinti    2d,4s,2s (round to integral, per FPCR)
   frintm    2d,4s,2s (round to integral, minus inf)
   frintn    2d,4s,2s (round to integral, nearest, to even)
   frintp    2d,4s,2s (round to integral, plus inf)
   frintx    2d,4s,2s (round to integral exact, per FPCR)
   frintz    2d,4s,2s (round to integral, zero)

   frsqrte   d,s (est)
   frsqrte   2d,4s,2s

   frsqrts   d,s (step)
   frsqrts   2d,4s,2s

   ======================== CONV ========================

   fcvt      s_h,d_h,h_s,d_s,h_d,s_d (fp convert, scalar)

   fcvtl{2}  4s/4h, 4s/8h, 2d/2s, 2d/4s (float convert to longer form)

   fcvtn{2}  4h/4s, 8h/4s, 2s/2d, 4s/2d (float convert to narrower form)

   fcvtas    d,s  (fcvt to signed int,   nearest, ties away)
   fcvtau    d,s  (fcvt to unsigned int, nearest, ties away)
   fcvtas    2d,4s,2s
   fcvtau    2d,4s,2s
   fcvtas    w_s,x_s,w_d,x_d
   fcvtau    w_s,x_s,w_d,x_d

   fcvtms    d,s  (fcvt to signed int,   minus inf)
   fcvtmu    d,s  (fcvt to unsigned int, minus inf)
   fcvtms    2d,4s,2s
   fcvtmu    2d,4s,2s
   fcvtms    w_s,x_s,w_d,x_d
   fcvtmu    w_s,x_s,w_d,x_d

   fcvtns    d,s  (fcvt to signed int,   nearest)
   fcvtnu    d,s  (fcvt to unsigned int, nearest)
   fcvtns    2d,4s,2s
   fcvtnu    2d,4s,2s
   fcvtns    w_s,x_s,w_d,x_d
   fcvtnu    w_s,x_s,w_d,x_d

   fcvtps    d,s  (fcvt to signed int,   plus inf)
   fcvtpu    d,s  (fcvt to unsigned int, plus inf)
   fcvtps    2d,4s,2s
   fcvtpu    2d,4s,2s
   fcvtps    w_s,x_s,w_d,x_d
   fcvtpu    w_s,x_s,w_d,x_d

   fcvtzs    d,s (fcvt to signed integer,   to zero)
   fcvtzu    d,s (fcvt to unsigned integer, to zero)
   fcvtzs    2d,4s,2s
   fcvtzu    2d,4s,2s
   fcvtzs    w_s,x_s,w_d,x_d
   fcvtzu    w_s,x_s,w_d,x_d

   fcvtzs    d,s (fcvt to signed fixedpt,   to zero) (w/ #fbits)
   fcvtzu    d,s (fcvt to unsigned fixedpt, to zero) (w/ #fbits)
   fcvtzs    2d,4s,2s
   fcvtzu    2d,4s,2s
   fcvtzs    w_s,x_s,w_d,x_d (fcvt to signed fixedpt,   to zero) (w/ #fbits)
   fcvtzu    w_s,x_s,w_d,x_d (fcvt to unsigned fixedpt, to zero) (w/ #fbits)

   fcvtxn    s_d (fcvt to lower prec narrow, rounding to odd)
   fcvtxn    2s_2d,4s_2d

   scvtf     d,s        _#fbits
   ucvtf     d,s        _#fbits

   scvtf     2d,4s,2s   _#fbits
   ucvtf     2d,4s,2s   _#fbits

   scvtf     d,s
   ucvtf     d,s

   scvtf     2d,4s,2s
   ucvtf     2d,4s,2s

   scvtf     s_w, d_w, s_x, d_x,   _#fbits
   ucvtf     s_w, d_w, s_x, d_x,   _#fbits

   scvtf     s_w, d_w, s_x, d_x
   ucvtf     s_w, d_w, s_x, d_x

   ======================== INT ========================

   abs       d
   neg       d

   abs       2d,4s,2s,8h,4h,16b,8b
   neg       2d,4s,2s,8h,4h,16b,8b

   add       d
   sub       d

   add       2d,4s,2s,8h,4h,16b,8b
   sub       2d,4s,2s,8h,4h,16b,8b

   addhn{2}   2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   subhn{2}   2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   raddhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h
   rsubhn{2}  2s/4s_2d_2d, 4h/8h_4s_4s, 8b/16b_8h_8h

   addp     d (add pairs, across)
   addp     2d,4s,2s,8h,4h,16b,8b
   addv     4s,8h,4h,16b,18b (reduce across vector)

   and      16b,8b

   orr      8h,4h   #imm8, LSL #0 or 8
   orr      4s,2s   #imm8, LSL #0, 8, 16 or 24
   bic      8h,4h   #imm8, LSL #0 or 8
   bic      4s,2s   #imm8, LSL #0, 8, 16 or 24
   also movi, mvni

   bic      16b,8b (vector,reg) (bit clear)
   bif      16b,8b (vector) (bit insert if false)
   bit      16b,8b (vector) (bit insert if true)
   bsl      16b,8b (vector) (bit select)

   cls      4s,2s,8h,4h,16b,8b (count leading sign bits)
   clz      4s,2s,8h,4h,16b,8b (count leading zero bits)

   cmeq     d
   cmge     d
   cmgt     d
   cmhi     d
   cmhs     d
   cmtst    d

   cmeq     2d,4s,2s,8h,4h,16b,8b
   cmge     2d,4s,2s,8h,4h,16b,8b
   cmgt     2d,4s,2s,8h,4h,16b,8b
   cmhi     2d,4s,2s,8h,4h,16b,8b
   cmhs     2d,4s,2s,8h,4h,16b,8b
   cmtst    2d,4s,2s,8h,4h,16b,8b

   cmeq_z   d
   cmge_z   d
   cmgt_z   d
   cmle_z   d
   cmlt_z   d

   cmeq_z   2d,4s,2s,8h,4h,16b,8b
   cmge_z   2d,4s,2s,8h,4h,16b,8b
   cmgt_z   2d,4s,2s,8h,4h,16b,8b
   cmle_z   2d,4s,2s,8h,4h,16b,8b
   cmlt_z   2d,4s,2s,8h,4h,16b,8b

   cnt      16b,8b (population count per byte)

   dup      d,s,h,b (vec elem to scalar)
   dup      2d,4s,2s,8h,4h,16b,8b (vec elem to vector)
   dup      2d,4s,2s,8h,4h,16b,8b (general reg to vector)

   eor      16b,8b (vector)
   ext      16b,8b,#imm4 (concat 2 vectors, then slice)

   ins      d[]_d[],s[]_s[],h[]_h[],b[]_b[]

   ins      d[]_x, s[]_w, h[]_w, b[]_w

   mla   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mla   4s,2s,8h,4h,16b,8b

   mls   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mls   4s,2s,8h,4h,16b,8b

   movi  16b,8b   #imm8, LSL #0
   movi  8h,4h    #imm8, LSL #0 or 8
   movi  4s,2s    #imm8, LSL #0, 8, 16, 24
   movi  4s,2s    #imm8, MSL #8 or 16
   movi  d,       #imm64
   movi  2d,      #imm64

   mul   4s_4s_s[],2s_2s_s[],8h_8h_h[],4h_4h_h[]
   mul   4s,2s,8h,4h,16b,8b

   mvni  8h,4h    #imm8, LSL #0 or 8
   mvni  4s,2s    #imm8, LSL #0, 8, 16, 24
   mvni  4s,2s    #imm8, MSL #8 or 16

   not   16b,8b

   orn   16b,8b
   orr   16b,8b

   pmul  16b,8b

   pmull{2}  8h_8b_8b,8h_16b_16b,1q_1d_1d,1d_2d_2d

   rbit    16b,8b
   rev16   16b,8b
   rev32   16b,8b,8h,4h
   rev64   16b,8b,8h,4h,4s,2s

   saba      16b,8b,8h,4h,4s,2s
   uaba      16b,8b,8h,4h,4s,2s

   sabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   uabal{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   sabd      16b,8b,8h,4h,4s,2s
   uabd      16b,8b,8h,4h,4s,2s

   sabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   uabdl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   sadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   uadalp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   saddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   uaddl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   ssubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   usubl{2}  2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   saddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s
   uaddlp    4h_8b,8h_16b,2s_4h,4s_8h,1d_2s,2d_4s

   saddlv    h_16b/8b, s_8h/4h, d_4s
   uaddlv    h_16b/8b, s_8h/4h, d_4s

   saddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s
   uaddw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s
   ssubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s
   usubw{2}  8h_8h_16b/8b, 4s_4s_8h/4h, 2d_2d_2s/4s

   shadd        16b,8b,8h,4h,4s,2s
   uhadd        16b,8b,8h,4h,4s,2s
   shsub        16b,8b,8h,4h,4s,2s
   uhsub        16b,8b,8h,4h,4s,2s

   shl          d_#imm
   shl          16b,8b,8h,4h,4s,2s,2d  _#imm

   shll{2}      8h_8b/16b_#8, 4s_4h/8h_#16, 2d_2s/4s_#32

   shrn{2}      2s/4s_2d, 8h/4h_4s, 2s/4s_2d,   #imm in 1 .. elem_bits
   rshrn{2}     2s/4s_2d, 8h/4h_4s, 2s/4s_2d,   #imm in 1 .. elem_bits

   sli          d_#imm
   sri          d_#imm

   sli          2d,4s,2s,8h,4h,16b,8b  _#imm
   sri          2d,4s,2s,8h,4h,16b,8b  _#imm

   smax         4s,2s,8h,4h,16b,8b
   umax         4s,2s,8h,4h,16b,8b
   smin         4s,2s,8h,4h,16b,8b
   umin         4s,2s,8h,4h,16b,8b

   smaxp        4s,2s,8h,4h,16b,8b
   umaxp        4s,2s,8h,4h,16b,8b
   sminp        4s,2s,8h,4h,16b,8b
   uminp        4s,2s,8h,4h,16b,8b

   smaxv        s_4s,h_8h,h_4h,b_16b,b_8b
   umaxv        s_4s,h_8h,h_4h,b_16b,b_8b
   sminv        s_4s,h_8h,h_4h,b_16b,b_8b
   uminv        s_4s,h_8h,h_4h,b_16b,b_8b

   smlal{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   umlal{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   smlsl{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   umlsl{2}     2d_2s/4s_s[], 4s_4h/8h_h[]
   smull{2}     2d_2s/4s_s[]. 4s_4h/8h_h[]
   umull{2}     2d_2s/4s_s[]. 4s_4h/8h_h[]

   smlal{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   umlal{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   smlsl{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   umlsl{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   smull{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)
   umull{2}     2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h), 8h_(8b_8b)/(16b_16b)

   smov         w_b[], w_h[], x_b[], x_h[], x_s[]
   umov         w_b[], w_h[], x_b[], x_h[], x_s[]

   sqabs        d,s,h,b
   sqneg        d,s,h,b

   sqabs        2d,4s,2s,8h,4h,16b,8b
   sqneg        2d,4s,2s,8h,4h,16b,8b

   sqadd        d,s,h,b
   uqadd        d,s,h,b
   sqsub        d,s,h,b
   uqsub        d,s,h,b

   sqadd        2d,4s,2s,8h,4h,16b,8b
   uqadd        2d,4s,2s,8h,4h,16b,8b
   sqsub        2d,4s,2s,8h,4h,16b,8b
   uqsub        2d,4s,2s,8h,4h,16b,8b

   sqdmlal      d_s_s[], s_h_h[]
   sqdmlsl      d_s_s[], s_h_h[]
   sqdmull      d_s_s[], s_h_h[]

   sqdmlal{2}   2d_2s/4s_s[], 4s_4h/8h_h[]
   sqdmlsl{2}   2d_2s/4s_s[], 4s_4h/8h_h[]
   sqdmull{2}   2d_2s/4s_s[], 4s_4h/2h_h[]

   sqdmlal      d_s_s, s_h_h
   sqdmlsl      d_s_s, s_h_h
   sqdmull      d_s_s, s_h_h

   sqdmlal{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)
   sqdmlsl{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)
   sqdmull{2}   2d_(2s_2s)/(4s_4s), 4s_(4h_4h)/(8h_8h)

   sqdmulh      s_s_s[], h_h_h[]
   sqrdmulh     s_s_s[], h_h_h[]

   sqdmulh      4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]
   sqrdmulh     4s_4s_s[], 2s_2s_s[], 8h_8h_h[], 4h_4h_h[]

   sqdmulh      h,s
   sqrdmulh     h,s

   sqdmulh      4s,2s,8h,4h
   sqrdmulh     4s,2s,8h,4h

   sqshl        d,s,h,b
   uqshl        d,s,h,b
   sqrshl       d,s,h,b
   uqrshl       d,s,h,b

   sqshl        2d,4s,2s,8h,4h,16b,8b
   uqshl        2d,4s,2s,8h,4h,16b,8b
   sqrshl       2d,4s,2s,8h,4h,16b,8b
   uqrshl       2d,4s,2s,8h,4h,16b,8b

   sqrshrn      s_d, h_s, b_h   #imm
   uqrshrn      s_d, h_s, b_h   #imm
   sqshrn       s_d, h_s, b_h   #imm
   uqshrn       s_d, h_s, b_h   #imm

   sqrshrun     s_d, h_s, b_h   #imm
   sqshrun      s_d, h_s, b_h   #imm

   sqrshrn{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   uqrshrn{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   sqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   uqshrn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqrshrun{2}  2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm
   sqshrun{2}   2s/4s_2d, 4h/8h_4s, 8b/16b_8h,  #imm

   sqshl        d,s,h,b   _#imm
   uqshl        d,s,h,b   _#imm
   sqshlu       d,s,h,b   _#imm

   sqshl        2d,4s,2s,8h,4h,16b,8b   _#imm
   uqshl        2d,4s,2s,8h,4h,16b,8b   _#imm
   sqshlu       2d,4s,2s,8h,4h,16b,8b   _#imm

   sqxtn        s_d,h_s,b_h
   uqxtn        s_d,h_s,b_h
   sqxtun       s_d,h_s,b_h

   sqxtn{2}     2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   uqxtn{2}     2s/4s_2d, 4h/8h_4s, 8b/16b_8h
   sqxtun{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   srhadd       4s,2s,8h,4h,16b,8b
   urhadd       4s,2s,8h,4h,16b,8b

   sshl (reg)   d
   ushl (reg)   d
   sshr (imm)   d
   ushr (imm)   d
   ssra (imm)   d
   usra (imm)   d

   srshl (reg)  d
   urshl (reg)  d
   srshr (imm)  d
   urshr (imm)  d
   srsra (imm)  d
   ursra (imm)  d

   sshl         2d,4s,2s,8h,4h,16b,8b
   ushl         2d,4s,2s,8h,4h,16b,8b
   sshr         2d,4s,2s,8h,4h,16b,8b
   ushr         2d,4s,2s,8h,4h,16b,8b
   ssra         2d,4s,2s,8h,4h,16b,8b
   usra         2d,4s,2s,8h,4h,16b,8b

   srshl        2d,4s,2s,8h,4h,16b,8b
   urshl        2d,4s,2s,8h,4h,16b,8b
   srshr        2d,4s,2s,8h,4h,16b,8b
   urshr        2d,4s,2s,8h,4h,16b,8b
   srsra        2d,4s,2s,8h,4h,16b,8b
   ursra        2d,4s,2s,8h,4h,16b,8b

   sshll{2} (imm)  2d_2s/4s  4s_4h/8h, 8h_8b/16b
   ushll{2} (imm)  2d_2s/4s  4s_4h/8h, 8h_8b/16b

   suqadd  d,s,h,b
   suqadd  2d,4s,2s,8h,4h,16b,8b

   tbl     8b_{16b}_8b, 16b_{16b}_16b
   tbl     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   tbl     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   tbl     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b

   tbx     8b_{16b}_8b, 16b_{16b}_16b
   tbx     8b_{16b,16b}_8b, 16b_{16b,16b}_16b
   tbx     8b_{16b,16b,16b}_8b, 16b_{16b,16b,16b}_16b
   tbx     8b_{16b,16b,16b,16b}_8b, 16b_{16b,16b,16b,16b}_16b

   trn1    2d,4s,2s,8h,4h,16b,8b
   trn2    2d,4s,2s,8h,4h,16b,8b

   urecpe      4s,2s

   ursqrte     4s,2s

   usqadd      d,s,h,b
   usqadd      2d,4s,2s,8h,4h,16b,8b

   uzp1      2d,4s,2s,8h,4h,16b,8b
   uzp2      2d,4s,2s,8h,4h,16b,8b

   xtn{2}    2s/4s_2d, 4h/8h_4s, 8b/16b_8h

   zip1      2d,4s,2s,8h,4h,16b,8b
   zip2      2d,4s,2s,8h,4h,16b,8b

   ======================== MEM ========================

   ld1  (multiple 1-element structures to 1/2/3/4 regs)
   ld1  (single 1-element structure to one lane of 1 reg)
   ld1r (single 1-element structure and rep to all lanes of 1 reg)

   ld2  (multiple 2-element structures to 2 regs)
   ld2  (single 2-element structure to one lane of 2 regs)
   ld2r (single 2-element structure and rep to all lanes of 2 regs)

   ld3  (multiple 3-element structures to 3 regs)
   ld3  (single 3-element structure to one lane of 3 regs)
   ld3r (single 3-element structure and rep to all lanes of 3 regs)

   ld4  (multiple 4-element structures to 4 regs)
   ld4  (single 4-element structure to one lane of 4 regs)
   ld4r (single 4-element structure and rep to all lanes of 4 regs)

   ldnp  q_q_addr,d_d_addr,s_s_addr  (load pair w/ non-temporal hint)
         addr = reg + uimm7 * reg_size

   ldp   q_q_addr,d_d_addr,s_s_addr  (load pair)
         addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   ldr   q,d,s,h,b from addr
         addr = [Xn|SP],#imm   or [Xn|SP,#imm]!  or [Xn|SP,#imm]

   ldr   q,d,s from  pc+#imm19

   ldr   q,d,s,h,b from addr
         addr = [Xn|SP, R <extend> <shift]

   ldur  q,d,s,h,b from addr
         addr = [Xn|SP,#imm] (unscaled offset)

   st1 (multiple 1-element structures from 1/2/3/4 regs)
   st1 (single 1-element structure for 1 lane of 1 reg)

   st2 (multiple 2-element structures from 2 regs)
   st2 (single 2-element structure from 1 lane of 2 regs)

   st3 (multiple 3-element structures from 3 regs)
   st3 (single 3-element structure from 1 lane of 3 regs)

   st4 (multiple 4-element structures from 4 regs)
   st4 (single 4-element structure from one lane of 4 regs)

   stnp q_q_addr, d_d_addr, s_s_addr
        addr = [Xn|SP, #imm]

   stp  q_q_addr, d_d_addr, s_s_addr
        addr = [Xn|SP], #imm  or [Xn|SP, #imm]!  or [Xn|SP, #imm]

   str  q,d,s,h,b_addr
        addr = [Xn|SP], #simm  or [Xn|SP, #simm]!  or [Xn|SP, #pimm]

   str   q,d,s,h,b_addr
         addr = [Xn|SP, R <extend> <shift]

   stur  q,d,s,h,b_addr
         addr = [Xn|SP,#imm] (unscaled offset)

   ======================== CRYPTO ========================

   aesd       16b (aes single round decryption)
   aese       16b (aes single round encryption)
   aesimc     16b (aes inverse mix columns)
   aesmc      16b (aes mix columns)

   sha1c      q_s_4s
   sha1h      s_s
   sha1m      q_s_4s
   sha1p      q_s_4s
   sha1su0    4s_4s_4s
   sha1su1    4s_4s

   sha256h2   q_q_4s
   sha256h    q_q_4s
   sha256su0  4s_4s
   sha256su1  4s_4s_4s
*/
