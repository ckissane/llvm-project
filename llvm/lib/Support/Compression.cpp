//===--- Compression.cpp - Compression implementation ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements compression functions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Compression.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#if LLVM_ENABLE_ZLIB
#include <zlib.h>
#endif
#if LLVM_ENABLE_ZSTD
#include <zstd.h>
#endif

using namespace llvm;
using namespace llvm::compression;

#if LLVM_ENABLE_ZLIB

static StringRef convertZlibCodeToString(int Code) {
  switch (Code) {
  case Z_MEM_ERROR:
    return "zlib error: Z_MEM_ERROR";
  case Z_BUF_ERROR:
    return "zlib error: Z_BUF_ERROR";
  case Z_STREAM_ERROR:
    return "zlib error: Z_STREAM_ERROR";
  case Z_DATA_ERROR:
    return "zlib error: Z_DATA_ERROR";
  case Z_OK:
  default:
    llvm_unreachable("unknown or unexpected zlib status code");
  }
}

bool zlib::isAvailable() { return true; }

void zlib::compress(StringRef InputBuffer,
                    SmallVectorImpl<char> &CompressedBuffer, int Level) {
  unsigned long CompressedSize = ::compressBound(InputBuffer.size());
  CompressedBuffer.resize_for_overwrite(CompressedSize);
  int Res =
      ::compress2((Bytef *)CompressedBuffer.data(), &CompressedSize,
                  (const Bytef *)InputBuffer.data(), InputBuffer.size(), Level);
  if (Res == Z_MEM_ERROR)
    report_bad_alloc_error("Allocation failed");
  assert(Res == Z_OK);
  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(CompressedBuffer.data(), CompressedSize);
  CompressedBuffer.truncate(CompressedSize);
}

Error zlib::uncompress(StringRef InputBuffer, char *UncompressedBuffer,
                       size_t &UncompressedSize) {
  int Res =
      ::uncompress((Bytef *)UncompressedBuffer, (uLongf *)&UncompressedSize,
                   (const Bytef *)InputBuffer.data(), InputBuffer.size());
  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(UncompressedBuffer, UncompressedSize);
  return Res ? make_error<StringError>(convertZlibCodeToString(Res),
                                       inconvertibleErrorCode())
             : Error::success();
}

Error zlib::uncompress(StringRef InputBuffer,
                       SmallVectorImpl<char> &UncompressedBuffer,
                       size_t UncompressedSize) {
  UncompressedBuffer.resize_for_overwrite(UncompressedSize);
  Error E = zlib::uncompress(InputBuffer, UncompressedBuffer.data(),
                             UncompressedSize);
  UncompressedBuffer.truncate(UncompressedSize);
  return E;
}

#else
bool zlib::isAvailable() { return false; }
void zlib::compress(StringRef InputBuffer,
                    SmallVectorImpl<char> &CompressedBuffer, int Level) {
  llvm_unreachable("zlib::compress is unavailable");
}
Error zlib::uncompress(StringRef InputBuffer, char *UncompressedBuffer,
                       size_t &UncompressedSize) {
  llvm_unreachable("zlib::uncompress is unavailable");
}
Error zlib::uncompress(StringRef InputBuffer,
                       SmallVectorImpl<char> &UncompressedBuffer,
                       size_t UncompressedSize) {
  llvm_unreachable("zlib::uncompress is unavailable");
}
#endif

#if LLVM_ENABLE_ZSTD

bool zstd::isAvailable() { return true; }

void zstd::compress(StringRef InputBuffer,
                    SmallVectorImpl<char> &CompressedBuffer, int Level) {
  unsigned long CompressedBufferSize = ::ZSTD_compressBound(InputBuffer.size());
  CompressedBuffer.resize_for_overwrite(CompressedBufferSize);
  unsigned long CompressedSize = ::ZSTD_compress(
      (char *)CompressedBuffer.data(), CompressedBufferSize,
      (const char *)InputBuffer.data(), InputBuffer.size(), Level);
  if (ZSTD_isError(CompressedSize))
    report_bad_alloc_error("Allocation failed");
  // Tell MemorySanitizer that zstd output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(CompressedBuffer.data(), CompressedSize);
  CompressedBuffer.truncate(CompressedSize);
}

Error zstd::uncompress(StringRef InputBuffer, char *UncompressedBuffer,
                       size_t &UncompressedSize) {
  unsigned long long const rSize = ZSTD_getFrameContentSize(
      (const char *)InputBuffer.data(), InputBuffer.size());
  size_t const Res =
      ::ZSTD_decompress((char *)UncompressedBuffer, rSize,
                        (const char *)InputBuffer.data(), InputBuffer.size());
  UncompressedSize = Res;
  // Tell MemorySanitizer that zstd output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(UncompressedBuffer, UncompressedSize);
  return Res != rSize ? make_error<StringError>(ZSTD_getErrorName(Res),
                                                inconvertibleErrorCode())
                      : Error::success();
}

Error zstd::uncompress(StringRef InputBuffer,
                       SmallVectorImpl<char> &UncompressedBuffer,
                       size_t UncompressedSize) {
  UncompressedBuffer.resize_for_overwrite(UncompressedSize);
  Error E = zstd::uncompress(InputBuffer, UncompressedBuffer.data(),
                             UncompressedSize);
  UncompressedBuffer.truncate(UncompressedSize);
  return E;
}

#else
bool zstd::isAvailable() { return false; }
void zstd::compress(StringRef InputBuffer,
                    SmallVectorImpl<char> &CompressedBuffer, int Level) {
  llvm_unreachable("zstd::compress is unavailable");
}
Error zstd::uncompress(StringRef InputBuffer, char *UncompressedBuffer,
                       size_t &UncompressedSize) {
  llvm_unreachable("zstd::uncompress is unavailable");
}
Error zstd::uncompress(StringRef InputBuffer,
                       SmallVectorImpl<char> &UncompressedBuffer,
                       size_t UncompressedSize) {
  llvm_unreachable("zstd::uncompress is unavailable");
}
#endif
