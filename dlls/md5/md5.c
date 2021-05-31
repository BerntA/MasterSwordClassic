/************************************************************************* 
 * MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 * From RFC 1321, MD5 Message-Digest Algorithm, April 1992
 *
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 * 
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 ************************************************************************/

#ifndef NOMEMCPY
#include <mem.h>        /* Needed for memcpy() and memset() */
#endif /* NOMEMCPY */
#include "global.h"
#include "md5.h"

/* Constants for MD5Transform routine. */

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void MD5Transform PROTO_LIST ((UINT4 [4], unsigned char [64]));
static void Encode PROTO_LIST ((unsigned char *, UINT4 *, unsigned int));
static void Decode PROTO_LIST ((UINT4 *, unsigned char *, unsigned int));

#ifdef NOMEMCPY
static void MD5_memcpy PROTO_LIST ((POINTER, POINTER, unsigned int));
static void MD5_memset PROTO_LIST ((POINTER, int, unsigned int));
#else
#define MD5_memcpy memcpy
#define MD5_memset memset
#endif /* NOMEMCPY */

static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}

/*************************************************************************
 * MD5 initialization. Begins an MD5 operation, writing a new context. 
 ************************************************************************/
void 
MD5Init (context)
MD5_CTX *context;                                        /* context */
{
  context->count[0] = context->count[1] = 0;
  /* Load magic initialization constants. */
  context->state[0] = 0x67452301L;
  context->state[1] = 0xefcdab89L;
  context->state[2] = 0x98badcfeL;
  context->state[3] = 0x10325476L;
  return;
}

/************************************************************************
 * MD5 block update operation. Continues an MD5 message-digest
 * operation, processing another message block, and updating the
 * context.
 ************************************************************************/
void 
MD5Update (context, input, inputLen)
MD5_CTX *context;                                        /* context */
unsigned char *input;                                /* input block */
unsigned int inputLen;                     /* length of input block */
{
  unsigned int i, index, partLen;

  /* Compute number of bytes mod 64 */
  index = (unsigned int)((context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if ((context->count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3)){
     context->count[1]++;
  }
  context->count[1] += ((UINT4)inputLen >> 29);

  partLen = 64 - index;

  /* Transform as many times as possible. */
  if (inputLen >= partLen) {
     MD5_memcpy ((POINTER)&context->buffer[index], (POINTER)input, partLen);
     MD5Transform (context->state, context->buffer);

     for (i = partLen; i + 63 < inputLen; i += 64) {
        MD5Transform (context->state, &input[i]);
     }
     index = 0;
  } else {
     i = 0;
  }

  /* Buffer remaining input */
  MD5_memcpy((POINTER)&context->buffer[index],(POINTER)&input[i],inputLen-i);
  return;
}

/************************************************************************
 * MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 ************************************************************************/
void 
MD5Final (digest, context)
unsigned char digest[16];                        /* message digest */
MD5_CTX *context;                                       /* context */
{
  unsigned char bits[8];
  unsigned int index, padLen;

  /* Save number of bits */
  Encode (bits, context->count, 8);

  /* Pad out to 56 mod 64. */
  index = (unsigned int)((context->count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  MD5Update (context, PADDING, padLen);

  /* Append length (before padding) */
  MD5Update (context, bits, 8);

  /* Store state in digest */
  Encode (digest, context->state, 16);

  /* Zeroize sensitive information. */
  MD5_memset ((POINTER)context, 0, sizeof (*context));
  return;
}

/************************************************************************
 * MD5 basic transformation. Transforms state based on block. 
 ************************************************************************/
static void 
MD5Transform (state, block)
UINT4 state[4];
unsigned char block[64];
{
  UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  Decode (x, block, 64);

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478L); /* 1 */
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756L); /* 2 */
  FF (c, d, a, b, x[ 2], S13, 0x242070dbL); /* 3 */
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceeeL); /* 4 */
  FF (a, b, c, d, x[ 4], S11, 0xf57c0fafL); /* 5 */
  FF (d, a, b, c, x[ 5], S12, 0x4787c62aL); /* 6 */
  FF (c, d, a, b, x[ 6], S13, 0xa8304613L); /* 7 */
  FF (b, c, d, a, x[ 7], S14, 0xfd469501L); /* 8 */
  FF (a, b, c, d, x[ 8], S11, 0x698098d8L); /* 9 */
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7afL); /* 10 */
  FF (c, d, a, b, x[10], S13, 0xffff5bb1L); /* 11 */
  FF (b, c, d, a, x[11], S14, 0x895cd7beL); /* 12 */
  FF (a, b, c, d, x[12], S11, 0x6b901122L); /* 13 */
  FF (d, a, b, c, x[13], S12, 0xfd987193L); /* 14 */
  FF (c, d, a, b, x[14], S13, 0xa679438eL); /* 15 */
  FF (b, c, d, a, x[15], S14, 0x49b40821L); /* 16 */

  /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562L); /* 17 */
  GG (d, a, b, c, x[ 6], S22, 0xc040b340L); /* 18 */
  GG (c, d, a, b, x[11], S23, 0x265e5a51L); /* 19 */
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aaL); /* 20 */
  GG (a, b, c, d, x[ 5], S21, 0xd62f105dL); /* 21 */
  GG (d, a, b, c, x[10], S22,  0x2441453L); /* 22 */
  GG (c, d, a, b, x[15], S23, 0xd8a1e681L); /* 23 */
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8L); /* 24 */
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6L); /* 25 */
  GG (d, a, b, c, x[14], S22, 0xc33707d6L); /* 26 */
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87L); /* 27 */
  GG (b, c, d, a, x[ 8], S24, 0x455a14edL); /* 28 */
  GG (a, b, c, d, x[13], S21, 0xa9e3e905L); /* 29 */
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8L); /* 30 */
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9L); /* 31 */
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8aL); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942L); /* 33 */
  HH (d, a, b, c, x[ 8], S32, 0x8771f681L); /* 34 */
  HH (c, d, a, b, x[11], S33, 0x6d9d6122L); /* 35 */
  HH (b, c, d, a, x[14], S34, 0xfde5380cL); /* 36 */
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44L); /* 37 */
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9L); /* 38 */
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60L); /* 39 */
  HH (b, c, d, a, x[10], S34, 0xbebfbc70L); /* 40 */
  HH (a, b, c, d, x[13], S31, 0x289b7ec6L); /* 41 */
  HH (d, a, b, c, x[ 0], S32, 0xeaa127faL); /* 42 */
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085L); /* 43 */
  HH (b, c, d, a, x[ 6], S34,  0x4881d05L); /* 44 */
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039L); /* 45 */
  HH (d, a, b, c, x[12], S32, 0xe6db99e5L); /* 46 */
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8L); /* 47 */
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665L); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, 0xf4292244L); /* 49 */
  II (d, a, b, c, x[ 7], S42, 0x432aff97L); /* 50 */
  II (c, d, a, b, x[14], S43, 0xab9423a7L); /* 51 */
  II (b, c, d, a, x[ 5], S44, 0xfc93a039L); /* 52 */
  II (a, b, c, d, x[12], S41, 0x655b59c3L); /* 53 */
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92L); /* 54 */
  II (c, d, a, b, x[10], S43, 0xffeff47dL); /* 55 */
  II (b, c, d, a, x[ 1], S44, 0x85845dd1L); /* 56 */
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4fL); /* 57 */
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0L); /* 58 */
  II (c, d, a, b, x[ 6], S43, 0xa3014314L); /* 59 */
  II (b, c, d, a, x[13], S44, 0x4e0811a1L); /* 60 */
  II (a, b, c, d, x[ 4], S41, 0xf7537e82L); /* 61 */
  II (d, a, b, c, x[11], S42, 0xbd3af235L); /* 62 */
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bbL); /* 63 */
  II (b, c, d, a, x[ 9], S44, 0xeb86d391L); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information. */
  MD5_memset ((POINTER)x, 0, sizeof (x));
  return;
}

/************************************************************************
 * Encodes input (UINT4) into output (unsigned char). Assumes len is
 * a multiple of 4.
 ************************************************************************/
static void 
Encode (output, input, len)
unsigned char *output;
UINT4 *input;
unsigned int len;
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
     output[j]   = (unsigned char)(input[i] & 0xff);
     output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
     output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
     output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
  }
  return;
}

/************************************************************************
 * Decodes input (unsigned char) into output (UINT4). Assumes len is
 * a multiple of 4.
 ************************************************************************/
static void 
Decode (output, input, len)
UINT4 *output;
unsigned char *input;
unsigned int len;
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
     output[i] = ((UINT4)input[j]) | 
                 (((UINT4)input[j+1]) <<  8) |
                 (((UINT4)input[j+2]) << 16) | 
                 (((UINT4)input[j+3]) << 24);
  }
  return;
}


#ifdef NOMEMCPY

/************************************************************************
 * Note: Replace "for loop" with standard memcpy if possible. 
 ************************************************************************/
static void 
MD5_memcpy (output, input, len)
POINTER output;
POINTER input;
unsigned int len;
{
  unsigned int i;

  for (i = 0; i < len; i++) {
     output[i] = input[i];
  }
  return;
}

/************************************************************************
 * Note: Replace "for loop" with standard memset if possible. 
 ************************************************************************/
static void 
MD5_memset (output, value, len)
POINTER output;
int value;
unsigned int len;
{
  unsigned int i;

  for (i = 0; i < len; i++) {
     ((char *)output)[i] = (char)value;
  }
  return;
}

#endif /* NOMEMCPY */
