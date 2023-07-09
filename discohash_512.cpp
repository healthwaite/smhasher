// DISCoHAsH 512 - crypto hash 
// https://github.com/dosyago/discohash
//   Copyright 2023 Cris Stringfellow & The Dosyago Corporation
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
#include <cstdio>
#include <inttypes.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include "discohash.h"

constexpr int STATE = 64;
constexpr int STATEM = STATE-1;
constexpr int STATE64 = STATE >> 3;
constexpr int HSTATE64 = STATE64 >> 1;
constexpr int STATE64M = STATE64-1;
constexpr int HSTATE64M = HSTATE64-1;
alignas(uint64_t) uint8_t disco_c_buf[STATE] = {0};
constexpr uint64_t P = 0xFFFFFFFFFFFFFFFF - 58;
constexpr uint64_t Q = UINT64_C(13166748625691186689);
alignas(uint64_t) uint8_t *dcs8 = (uint8_t *)disco_c_buf;
uint64_t *dcs = (uint64_t *)disco_c_buf;

static inline uint64_t
rot (uint64_t v, int n)
{
  n = n & 63U;
  if (n)
    v = (v >> n) | (v << (64 - n));
  return v;
}

static inline uint8_t
rot8 (uint8_t v, int n)
{
  n = n & 7U;
  if (n)
    v = (v >> n) | (v << (8 - n));
  return v;
}

static inline void
mix (const int A)
{
  const int B = A + 1;
  dcs[A] *= P;
  dcs[A] = rot (dcs[A], 23);
  dcs[A] *= Q;

  dcs[B] ^= dcs[A];

  dcs[B] *= P;
  dcs[B] = rot (dcs[B], 23);
  dcs[B] *= Q;
}

static inline void
round (const uint64_t *m64, const uint8_t *m8, int len)
{
  int index = 0;
  int sindex = 0;
  uint64_t counter = 0xfaccadaccad09997;
  uint8_t counter8 = 137;

  for (int Len = len >> 3; index < Len; index++)
    {
      dcs[sindex] += rot (m64[index] + index + counter + 1, 23);
      counter += ~m64[index] + 1;
      switch (sindex)
        {
        case 1:
          mix (0);
          sindex++;
          break;
        case 3:
          mix (2);
          sindex++;
          break;
        case 5:
          mix (4);
          sindex++;
          break;
        case 7:
          mix (6);
          sindex = 0;
          break;
        default:
          sindex++;
          break;
        }
    }

  mix (1);
  mix (3);
  mix (5);

  index <<= 3;
  sindex = index & (STATEM);
  for (; index < len; index++)
    {
      dcs8[sindex] += rot8 (m8[index] + index + counter8 + 1, 23);
      counter8 += ~m8[index] + 1;
      mix (index % STATE64M);
      if (sindex >= STATEM)
        {
          sindex = 0;
        }
      else
        {
          sindex++;
        }
    }

  mix (0);
  mix (1);
  mix (2);
  mix (3);
  mix (4);
  mix (5);
  mix (6);
}

void
DISCoHAsH_512_64 (const void *key, int len, seed_t seed, void *out)
{
  int tempLen = len;
  if (tempLen == 0)
    {
      tempLen = 1;
    }
  alignas (uint64_t) uint8_t *tempBuf = new uint8_t[tempLen];
  const uint8_t *key8Arr = (uint8_t *)key;

  alignas (uint64_t) const uint8_t seedbuf[16] = { 0 };
  const uint8_t *seed8Arr = (uint8_t *)seedbuf;
  const uint64_t *seed64Arr = (uint64_t *)seedbuf;
  uint32_t *seed32Arr = (uint32_t *)seedbuf;

  // the cali number from the Matrix (1999)
  seed32Arr[0] = UINT32_C (0xc5550690);
  seed32Arr[0] -= seed;
  seed32Arr[1] = 1 + seed;
  seed32Arr[2] = ~(1 - seed);
  seed32Arr[3] = (1 + seed) * UINT32_C (0xf00dacca);

  // nothing up my sleeve
  dcs[0] = UINT64_C (0x123456789abcdef0);
  dcs[1] = UINT64_C (0x0fedcba987654321);
  dcs[2] = UINT64_C (0xaccadacca80081e5);
  dcs[3] = UINT64_C (0xf00baaf00f00baaa);
  dcs[4] = UINT64_C (0xbeefdeadbeefc0de);
  dcs[5] = UINT64_C (0xabad1deafaced00d);
  dcs[6] = UINT64_C (0xfaceb00cfacec0de);
  dcs[7] = UINT64_C (0xdeadc0dedeadbeef);

  memcpy (tempBuf, key, len);
  uint64_t *temp64 = reinterpret_cast<uint64_t *> (tempBuf);

  round (temp64, key8Arr, len);
  round (seed64Arr, seed8Arr, 16);
  round (dcs, dcs8, STATE);

  // 512-bit internal state 256-bit output
  uint64_t h[4] = { 0 }; // This will hold the final output

  h[0] -= dcs[2];
  h[0] -= dcs[3];
  h[0] -= dcs[6];
  h[0] -= dcs[7];

  // full 256-bit output
  /*
    h[1] = dcs[2] ^ dcs[3] ^ dcs[6] ^ dcs[7];

    h[2] -= dcs[0];
    h[2] -= dcs[1];
    h[2] -= dcs[4];
    h[2] -= dcs[5];

    h[3] = dcs[0] ^ dcs[1] ^ dcs[4] ^ dcs[5];
  */

  memcpy (out, h, sizeof (h) / 4); // divide by 4 as smhasher only uses 64 bits

  delete[] tempBuf;
}

static bool DISCoHAsH_512_64_bad_seeds(std::vector<uint32_t> &seeds)
{
  seeds = std::vector<uint32_t>
    {
      0x100a32, 0x1048e7, 0x104a1, 0x106bba, 0x10a6c, 0x10c526, 0x10cd90, 0x10d00f,
      0x10e4e, 0x10e714, 0x11736, 0x11769, 0x11eac0, 0x12000a, 0x1200d,
      0x1205b, 0x12149, 0x121766, 0x12241, 0x122bf5, 0x12775, 0x128184,
      0x12852f, 0x12c69a, 0x12c74b, 0x12d5ea, 0x12e2c6, 0x12e7c6, 0x1301c8,
      0x132478, 0x133710, 0x133a36, 0x138f97, 0x13c4b7, 0x13d621, 0x13fb54,
      0x14149f, 0x141cd5, 0x1436f5, 0x1455a, 0x146fe9, 0x148428, 0x14b397,
      0x14d46b, 0x14e77, 0x14f6c2, 0x150700, 0x1519c1, 0x156442, 0x158339,
      0x15a812, 0x15b4ff, 0x15d6b2, 0x15d712, 0x15d83f, 0x15df1e, 0x15e058,
      0x161563, 0x161902, 0x161e97, 0x16325f, 0x166219, 0x16a5fa, 0x16ddf6,
      0x171163, 0x172e15, 0x174283, 0x175563, 0x175d72, 0x177697, 0x1782e1,
      0x17b06b, 0x182c1c, 0x184346, 0x18439b, 0x184a92, 0x187da6, 0x18b219,
      0x18d4af, 0x190c0c, 0x190c26, 0x197878, 0x19c49c, 0x1a1bbd, 0x1a20f0,
      0x1a2499, 0x1a2583, 0x1a5ecb, 0x1a704, 0x1adcc9, 0x1b3602, 0x1b47b5,
      0x1b5317, 0x1b9906, 0x1bb3ec, 0x1bfcf8, 0x1c2844, 0x1c3594, 0x1c3c4e,
      0x1c9f3, 0x1ca49, 0x1cc0d3, 0x1ce13e, 0x1cfee9, 0x1d0bcd, 0x1d10e6,
      0x1d31b4, 0x1d3509, 0x1d6904, 0x1e40b5, 0x1e5d7, 0x1e8180, 0x1e8abd,
      0x1f0f46, 0x1f3155, 0x1f3994, 0x1f8d44, 0x1fc9e9, 0x1fe862, 0x1feffc,
      0x20036f, 0x20119, 0x202c25, 0x205cd2, 0x20b914, 0x20c8c0, 0x20ddc2,
      0x2100b2, 0x2170f3, 0x218163, 0x2184cb, 0x2187f5, 0x219940, 0x21fabc,
      0x226cfc, 0x227954, 0x2288df, 0x229d95, 0x22e202, 0x22ef94, 0x22f23d,
      0x233fab, 0x23581d, 0x237e45, 0x238b74, 0x23a0fc, 0x23a53f, 0x23baa4,
      0x23ed32, 0x2434d2, 0x24410c, 0x245589, 0x2507ed, 0x251892, 0x256626,
      0x2571f3, 0x25c0ee, 0x25dad6, 0x261981, 0x2628cb, 0x263d9f, 0x26473f,
      0x26cf, 0x274674, 0x274a41, 0x2757f5, 0x27717e, 0x27901e, 0x27bd84,
      0x27edbd, 0x28144e, 0x282bf9, 0x2870e1, 0x28849a, 0x2898a8, 0x28a399,
      0x28d9db, 0x28d9e6, 0x28dc7d, 0x28dda3, 0x29d91, 0x2f45, 0x3664e,
      0x38712, 0x39354, 0x3a3a5, 0x3b0c8, 0x3b8a8, 0x3e80d, 0x3ec0b, 0x3f8b2,
      0x400004d6, 0x400033f6, 0x4000881d, 0x4000d9a8, 0x40014a1e, 0x4001b017,
      0x4001b206, 0x4001c007, 0x4001c114, 0x4001d1e7, 0x40020029, 0x40020d38,
      0x40021f15, 0x40026ade, 0x40026b2b, 0x40029bb6, 0x4002ae1c, 0x4002e115,
      0x4002f6d0, 0x4002fa33, 0x40032000, 0x40035983, 0x40038b86, 0x4003b1a2,
      0x4003e1cf, 0x40041638, 0x40041dfc, 0x40045884, 0x40047046, 0x40048646,
      0x40049f0d, 0x4004d953, 0x4004f293, 0x4004fe50, 0x400504a2, 0x40050ab3,
      0x4005379b, 0x40058c57, 0x40059169, 0x4005be5f, 0x40063cf6, 0x40064342,
      0x40065261, 0x4006802f, 0x4006921d, 0x4006bb13, 0x4007a366, 0x4008180d,
      0x40081b02, 0x40086951, 0x4008791c, 0x40088803, 0x4008a7c6, 0x4008a83a,
      0x4008abbd, 0x4008b35e, 0x400910f8, 0x40091aaa, 0x40091b1b, 0x40093da0,
      0x40095681, 0x40097ead, 0x4009b460, 0x400a0e27, 0x400a713b, 0x400a741f,
      0x400a8eaf, 0x400ac057, 0x400ac400, 0x400b18eb, 0x400b2f1d, 0x400b360a,
      0x400b3d0d, 0x400b48a9, 0x400b565d, 0x400bded2, 0x400c30fa, 0x400c7042,
      0x400c70ef, 0x400c86e0, 0x400c8d08, 0x400cacd7, 0x400cc605, 0x400cda80,
      0x400d0300, 0x400d1a53, 0x400d23b4, 0x400daaf8, 0x400db7b7, 0x400dfb5b,
      0x400e426c, 0x400e43d0, 0x400ed3f2, 0x400edf06, 0x400f0cb4, 0x400f4706,
      0x400f5449, 0x400f6698, 0x40100dc9, 0x40103c54, 0x401094a4, 0x4010b052,
      0x4010bc1c, 0x4010d5fd, 0x4010ea41, 0x4010fe5c, 0x40110dd9, 0x40113389,
      0x40114daa, 0x4011b20b, 0x4011b348, 0x4011e814, 0x4011f23c, 0x4012234c,
      0x40122953, 0x40126fe9, 0x40127bd3, 0x4012bbed, 0x4012d6d7, 0x4012eeb5,
      0x4012f004, 0x40130644, 0x401310be, 0x40131ecb, 0x401323ae, 0x40132985,
      0x40136463, 0x4014261d, 0x40143a2a, 0x40145451, 0x4014a39e, 0x4014d725,
      0x4014df4f, 0x4015055e, 0x40151a64, 0x401532e2, 0x40155c05, 0x401566d9,
      0x401580b8, 0x401581cc, 0x4015965f, 0x4015a86f, 0x4015a9e0, 0x4015abcd,
      0x4015ac32, 0x4015b306, 0x4015e5e2, 0x4015edd6, 0x4015ffcc, 0x401600c2,
      0x401631e1, 0x40164c67, 0x40165c1c, 0x4016ca58, 0x4016ed83, 0x40171df6,
      0x40172be9, 0x40173907, 0x4017427d, 0x40177534, 0x40178549, 0x40179138,
      0x4017a8f5, 0x4017d6a1, 0x4017e305, 0x4018444f, 0x40184bf3, 0x40186ae0,
      0x40187bc3, 0x40188cb2, 0x4018bc67, 0x4018d057, 0x40194f2a, 0x40195cda,
      0x40195ee8, 0x40195fbe, 0x401961cc, 0x40196ecc, 0x40198cc8, 0x401997cf,
      0x4019a3d7, 0x4019ba5e, 0x4019c9a2, 0x4019d266, 0x4019e5cf, 0x4019fd52,
      0x401a2a82, 0x401a45dd, 0x401a4958, 0x401a4a49, 0x401a7710, 0x401ab639,
      0x401aba09, 0x401b01d8, 0x401b15b2, 0x401b448d, 0x401b5130, 0x401b731c,
      0x401b84b4, 0x401b8f40, 0x401b9850, 0x401b9e15, 0x401ba330, 0x401bc718,
      0x401bc71b, 0x401be232, 0x401bf946, 0x401c20ec, 0x401c3244, 0x401c554d,
      0x401c7623, 0x401c9b0a, 0x401cc9fd, 0x401d03a3, 0x401d7553, 0x401d7cdb,
      0x401d7d9e, 0x401d8af5, 0x401d91f0, 0x401db4cb, 0x401dd158, 0x401de8d7,
      0x401dfeec, 0x401e089a, 0x401e20a5, 0x401e5452, 0x401e6dc3, 0x401e9700,
      0x401e9876, 0x401e9925, 0x401ea466, 0x401ea81e, 0x401f1235, 0x401f2039,
      0x401f45ca, 0x401f4901, 0x401f6f59, 0x401f71e0, 0x401f931f, 0x401f9812,
      0x401fb0b9, 0x40201647, 0x4020a180, 0x4020c81c, 0x4020d6c3, 0x4020fcfc,
      0x402142ed, 0x4021453a, 0x402162f2, 0x40219532, 0x40219621, 0x402207a9,
      0x40220bbd, 0x402210cd, 0x40222d05, 0x4022551a, 0x402256b9, 0x402296db,
      0x4022e8cc, 0x4022fffe, 0x40231460, 0x402343ee, 0x40234f10, 0x40236a7b,
      0x4024af94, 0x4024c557, 0x4024ccb1, 0x402508fd, 0x4025235f, 0x402526f9,
      0x40252dd8, 0x40252fe8, 0x40254bba, 0x4025f873, 0x4025fe67, 0x40260617,
      0x402620ef, 0x4026a247, 0x4026a6bd, 0x4026bb3b, 0x402734b0, 0x40274931,
      0x40274d47, 0x402758a3, 0x4027598e, 0x40278e15, 0x4027aabd, 0x4027b4bf,
      0x4027cb34, 0x40284c76, 0x40285372, 0x402866f5, 0x40286771, 0x40286780,
      0x402873e5, 0x4028777c, 0x4080d, 0x409c1, 0x43ecd, 0x4461c, 0x44abd,
      0x4a3ad, 0x4c806, 0x53dec, 0x58a56, 0x58adb, 0x58b0d, 0x5970d, 0x59dd7,
      0x59f2, 0x5b9f, 0x5bd4d, 0x61a0, 0x637d, 0x6765a, 0x6bf6, 0x6cf0,
      0x6d217, 0x6db4, 0x6db7, 0x6fa60, 0x70bbc, 0x71ace, 0x7239a, 0x72e69,
      0x72f3e, 0x752c, 0x78b34, 0x78fae, 0x7e95, 0x7ea8c, 0x800016e9,
      0x80002703, 0x80003878, 0x800040a5, 0x800044cc, 0x80007d31, 0x80007eed,
      0x80008379, 0x800083f4, 0x8000869f, 0x8000870d, 0x80008bba, 0x80008e74,
      0x80008f23, 0x80009070, 0x80009b85, 0x80009c6a, 0x8000a031, 0x8000a636,
      0x8000aa0b, 0x8000aabf, 0x8000ace8, 0x8000ae51, 0x8000af96, 0x8000b217,
      0x80018cec, 0x80019b4b, 0x8001c33e, 0x8001d601, 0x8001eb3c, 0x8001f9bd,
      0x800250a5, 0x80028a84, 0x8003087e, 0x800327bd, 0x800332c0, 0x800354cf,
      0x80035535, 0x8003970e, 0x8003aa27, 0x8003b59a, 0x8003be77, 0x8003e2f0,
      0x8003ffa0, 0x8003ffd8, 0x80040dd8, 0x80045f90, 0x8004abb7, 0x80057461,
      0x800579f0, 0x80057f06, 0x8006980c, 0x8006ab30, 0x8006b3b1, 0x8006f6d3,
      0x800745e4, 0x80074e41, 0x80087205, 0x8008ac93, 0x8008d6e1, 0x80092db0,
      0x80095621, 0x80096148, 0x80097827, 0x80098766, 0x800998ba, 0x8009cad9,
      0x8009cbe9, 0x800a2994, 0x800a2f27, 0x800a3403, 0x800a3ec2, 0x800a6554,
      0x800a7c25, 0x800aae01, 0x800ae4c7, 0x800af316, 0x800b42ac, 0x800b4f6f,
      0x800b5177, 0x800b6e98, 0x800b83fd, 0x800bc12b, 0x800bfdfe, 0x800c1d03,
      0x800c3d87, 0x800c6958, 0x800d0879, 0x800d4c59, 0x800d6be5, 0x800dee53,
      0x800df626, 0x800dff56, 0x800e0754, 0x800e1596, 0x800e380d, 0x800e38a5,
      0x800e5774, 0x800ec72f, 0x800ee8fb, 0x800f0b43, 0x800f2910, 0x800f441b,
      0x800f9b54, 0x800fb149, 0x800feb50, 0x800ff03f, 0x800fff06, 0x80101da5,
      0x8010440d, 0x801065ee, 0x80107e2a, 0x801086ee, 0x8010a41d, 0x8010c61e,
      0x8010c9eb, 0x8010effe, 0x8011527e, 0x80117c0b, 0x80119177, 0x80119ec8,
      0x8011a0d4, 0x8011b5df, 0x8011cda8, 0x8011d503, 0x8011d794, 0x8011d9bb,
      0x80120490, 0x80124836, 0x801273ab, 0x8012c426, 0x8012e8b1, 0x8012f077,
      0x8012f8f9, 0x8013437c, 0x80138d62, 0x8013a6c2, 0x8013dcd3, 0x8013e695,
      0x8014122e, 0x80142b1d, 0x8014a83e, 0x8014c2cc, 0x8014fe8f, 0x8015072d,
      0x8015300f, 0x80154d17, 0x80158711, 0x80161ea3, 0x80163486, 0x80164e4a,
      0x80167885, 0x8016971b, 0x80170898, 0x80171ab0, 0x80173b1c, 0x80174e07,
      0x80174f46, 0x801772a3, 0x8017965c, 0x80179b0d, 0x8017bedf, 0x8017e881,
      0x8018349d, 0x80186f29, 0x801883f3, 0x8018b4ff, 0x8018f78d, 0x80196186,
      0x801964e6, 0x80199c70, 0x801a3859, 0x801a4dbd, 0x801a56b1, 0x801a7382,
      0x801ab216, 0x801af663, 0x801af946, 0x801b02e6, 0x801bdff4, 0x801c0dc0,
      0x801c447f, 0x801c460d, 0x801c5e55, 0x801c674d, 0x801c89f5, 0x801c9cda,
      0x801d46fb, 0x801d83fb, 0x801d8563, 0x801d9b53, 0x801da9de, 0x801dedd4,
      0x801e1e96, 0x801e207a, 0x801e5507, 0x801e679d, 0x801e6c61, 0x801e78fd,
      0x801e7ea9, 0x801e9298, 0x801e9343, 0x801e9484, 0x801e9f0d, 0x801ec5ea,
      0x801efad5, 0x801f2f74, 0x801f47d4, 0x801f56a5, 0x801f8e1e, 0x801f96c6,
      0x801f9d43, 0x801febdd, 0x80201802, 0x802028dc, 0x80203a27, 0x80206041,
      0x80207514, 0x80208264, 0x80209187, 0x802095cb, 0x8020c5c7, 0x8020df99,
      0x8020f775, 0x80212100, 0x802135c2, 0x80214341, 0x80214da7, 0x80217f55,
      0x80217f93, 0x80219f52, 0x80219fa0, 0x8021b56d, 0x8021b66c, 0x8021c203,
      0x8021cebb, 0x8021e2b8, 0x8021e7b3, 0x80222aab, 0x802237f3, 0x80223d22,
      0x80225557, 0x80225e86, 0x80227e28, 0x80228466, 0x8022917c, 0x80229940,
      0x80229d55, 0x8022c4cd, 0x8022d032, 0x8022dc1a, 0x8022e8a5, 0x8022edef,
      0x802326fb, 0x80232f7d, 0x80234d15, 0x80244607, 0x80245286, 0x80248812,
      0x8024e0db, 0x8024f4f5, 0x80251ac9, 0x80251e84, 0x80251f3c, 0x80254e1c,
      0x80257048, 0x8025d064, 0x802605f8, 0x802635e2, 0x80264d90, 0x802723d5,
      0x80273429, 0x80274286, 0x80274ac6, 0x802756b5, 0x80275f1e, 0x80277f48,
      0x80280cd4, 0x80280d54, 0x80282810, 0x802832ea, 0x80285cf0, 0x802886ec,
      0x8028bdde, 0x8028c22c, 0x8028cd15, 0x844b5, 0x8587, 0x8979, 0x8a1d7,
      0x8a565, 0x8b6e2, 0x91e37, 0x923bd, 0x92925, 0x92dd9, 0x9352d, 0x94477,
      0x944b1, 0x94578, 0x980dd, 0x98e9, 0x9a45, 0x9a6f6, 0x9d262, 0x9decf,
      0x9f28, 0xa0268, 0xa103, 0xa2477, 0xa2d18, 0xa3934, 0xa5488, 0xa5d52,
      0xa61cd, 0xa87eb, 0xa8cdd, 0xa9507, 0xa9bef, 0xac840, 0xad013, 0xae24,
      0xaef5, 0xb360b, 0xb3bb4, 0xb467b, 0xbe91f, 0xc000072d, 0xc00057e7,
      0xc0008098, 0xc0009247, 0xc0009ae3, 0xc000af4c, 0xc000c097, 0xc000c6ba,
      0xc00115b7, 0xc0011bea, 0xc00125f5, 0xc0012907, 0xc001313a, 0xc00136ba,
      0xc001371b, 0xc001376d, 0xc0013bd5, 0xc0013e47, 0xc0013ff0, 0xc0014610,
      0xc0014924, 0xc0014a17, 0xc0014a47, 0xc0014c4d, 0xc0014cea, 0xc0015462,
      0xc00157a5, 0xc001586a, 0xc0015e58, 0xc00160b7, 0xc001651c, 0xc0016767,
      0xc001679e, 0xc00167e5, 0xc0016a99, 0xc001757b, 0xc0018760, 0xc00187b1,
      0xc0018858, 0xc001978a, 0xc00198dc, 0xc001a043, 0xc001c927, 0xc001dbc9,
      0xc0021297, 0xc0021d12, 0xc0027844, 0xc002cb7c, 0xc003125e, 0xc00357a9,
      0xc003849f, 0xc0038584, 0xc003a0e8, 0xc003a8d8, 0xc004350f, 0xc00466f1,
      0xc00472c6, 0xc004a741, 0xc004cb40, 0xc004cef8, 0xc004d82d, 0xc005377c,
      0xc0053ec2, 0xc0054f90, 0xc0057894, 0xc0059baf, 0xc005f045, 0xc0061c62,
      0xc0066aed, 0xc006a379, 0xc006a921, 0xc007086f, 0xc007101a, 0xc0071043,
      0xc00735b9, 0xc0073be3, 0xc0079581, 0xc007adff, 0xc007ce95, 0xc007e2bf,
      0xc007f15b, 0xc0084c35, 0xc0086865, 0xc008748a, 0xc008777d, 0xc008db4b,
      0xc008dfef, 0xc009080a, 0xc009427b, 0xc009940f, 0xc009a2ee, 0xc009bb0b,
      0xc009d27f, 0xc009e521, 0xc009f8cf, 0xc00a14bb, 0xc00a1b43, 0xc00a1ea3,
      0xc00a79fe, 0xc00a8ed0, 0xc00ad619, 0xc00af3a5, 0xc00b51de, 0xc00b60f3,
      0xc00bfd32, 0xc00c0c82, 0xc00c3267, 0xc00c4a2a, 0xc00c9912, 0xc00c9b24,
      0xc00cc69f, 0xc00cd6be, 0xc00ce4cb, 0xc00cfceb, 0xc00d1f5b, 0xc00dd183,
      0xc00df851, 0xc00e08e0, 0xc00e2012, 0xc00e28be, 0xc00e4f02, 0xc00e5618,
      0xc00e8a87, 0xc00eb451, 0xc00f110c, 0xc00f1ccf, 0xc00f1fbf, 0xc00f3b02,
      0xc00f9ffb, 0xc00fa7fc, 0xc0103527, 0xc0104c31, 0xc0105aa3, 0xc01066e8,
      0xc010aed2, 0xc010b62c, 0xc010c0e0, 0xc010f6fc, 0xc010f75c, 0xc0111b50,
      0xc011799c, 0xc011947f, 0xc0119f34, 0xc011a43d, 0xc011ab1c, 0xc011e624,
      0xc0121996, 0xc01223ea, 0xc01234b7, 0xc0124ed6, 0xc0127c7f, 0xc012cd4c,
      0xc0133dde, 0xc0133e30, 0xc013ccfc, 0xc013e50a, 0xc01420d8, 0xc014654f,
      0xc014da0b, 0xc014e924, 0xc014f718, 0xc014fdb6, 0xc01520c1, 0xc0152612,
      0xc015507d, 0xc0156f86, 0xc015959b, 0xc015b466, 0xc015cfdf, 0xc015dfb2,
      0xc0160f02, 0xc0168414, 0xc016ae71, 0xc016afc7, 0xc016c062, 0xc016c302,
      0xc0170c5e, 0xc0174100, 0xc017902b, 0xc01795e3, 0xc017b921, 0xc017fcc3,
      0xc018f22a, 0xc0190bf3, 0xc0194e86, 0xc0195cfa, 0xc01965ba, 0xc0196665,
      0xc019683a, 0xc0196fb9, 0xc0199245, 0xc0199838, 0xc019bc46, 0xc019f898,
      0xc01a0101, 0xc01a679f, 0xc01a812a, 0xc01ab4ed, 0xc01ac16a, 0xc01ae121,
      0xc01b96c2, 0xc01b9beb, 0xc01bf42f, 0xc01c0406, 0xc01c3356, 0xc01c79d4,
      0xc01ccc3d, 0xc01ce1be, 0xc01d0df7, 0xc01d6d0f, 0xc01d7689, 0xc01d9eee,
      0xc01da04c, 0xc01da1dd, 0xc01e0249, 0xc01e1203, 0xc01e4019, 0xc01e843a,
      0xc01ea9b3, 0xc01f0268, 0xc01f0544, 0xc01f3cff, 0xc01f5365, 0xc01f5922,
      0xc01f7fb5, 0xc01f8087, 0xc01fa082, 0xc0201207, 0xc0202b5a, 0xc0204f51,
      0xc0205ab7, 0xc0205f93, 0xc020df2b, 0xc021095f, 0xc0210eff, 0xc0211b2c,
      0xc02128d7, 0xc02158c5, 0xc02167df, 0xc0217784, 0xc021c2b4, 0xc021d2c9,
      0xc021db2f, 0xc021dc61, 0xc021dfbc, 0xc021ecd1, 0xc021fc20, 0xc02228fb,
      0xc02248cc, 0xc0224c97, 0xc02254a2, 0xc0227600, 0xc0229922, 0xc0229b79,
      0xc0229f19, 0xc022a82b, 0xc0231f26, 0xc02390ee, 0xc023bf0b, 0xc023d666,
      0xc024070c, 0xc024151e, 0xc02421bc, 0xc0246379, 0xc0246f96, 0xc0247152,
      0xc024882a, 0xc0249587, 0xc024a721, 0xc024c01c, 0xc024c3e9, 0xc024daa9,
      0xc024ea82, 0xc024f437, 0xc024f992, 0xc0251146, 0xc02511f4, 0xc0252000,
      0xc0253602, 0xc026131e, 0xc02656b0, 0xc02668ef, 0xc0267214, 0xc026996a,
      0xc026a83c, 0xc026c4e8, 0xc026ee57, 0xc0270111, 0xc0270790, 0xc0270914,
      0xc02738a6, 0xc02748e1, 0xc0275969, 0xc0276f3c, 0xc02772da, 0xc0277cfe,
      0xc027e7d8, 0xc027e91f, 0xc027f01b, 0xc027f193, 0xc02802f2, 0xc3b9,
      0xc49e5, 0xc4d83, 0xc62e7, 0xc6a77, 0xc6d30, 0xc6dcf, 0xc782d, 0xc7f31,
      0xc806d, 0xc8f1d, 0xc9217, 0xcb6e3, 0xccbcd, 0xcf315, 0xd184c, 0xd81a7,
      0xd9549, 0xdc47f, 0xe36ae, 0xe410b, 0xe5041, 0xe63dc, 0xe70c4, 0xe80d1,
      0xeaeb8, 0xeb039, 0xeb560, 0xed043, 0xf0436, 0xff756
    };
  return true;
}

void DISCoHAsH_512_64_seed_init (uint32_t &seed) {
  std::vector<uint32_t> bad_seeds;
  DISCoHAsH_512_64_bad_seeds(bad_seeds);
  while (std::find(bad_seeds.begin(), bad_seeds.end(), seed) != bad_seeds.end())
    seed++;
}