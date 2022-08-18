//===- llvm/unittest/Support/CompressionTest.cpp - Compression tests ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements unit tests for the Compression functions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Compression.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::compression;

namespace {

static void testCompressionAlgorithm(
    StringRef Input, int Level, CompressionSpec *CSpec,
    std::string ExpectedDestinationBufferTooSmallErrorMessage) {
  SmallVector<uint8_t, 0> Compressed;
  SmallVector<uint8_t, 0> Uncompressed;
  CSpec->Implementation->compress(arrayRefFromStringRef(Input), Compressed,
                                  Level);

  // Check that uncompressed buffer is the same as original.
  Error E =
      CSpec->Implementation->decompress(Compressed, Uncompressed, Input.size());
  consumeError(std::move(E));

  EXPECT_EQ(Input, toStringRef(Uncompressed));
  if (Input.size() > 0) {
    // Uncompression fails if expected length is too short.
    E = CSpec->Implementation->decompress(Compressed, Uncompressed,
                                          Input.size() - 1);
    EXPECT_EQ(ExpectedDestinationBufferTooSmallErrorMessage,
              llvm::toString(std::move(E)));
  }
}

#if LLVM_ENABLE_ZLIB
static void testZlibCompression(StringRef Input, int Level) {
  testCompressionAlgorithm(Input, Level,
                           getCompressionSpec(CompressionKind::Zlib),
                           "zlib error: Z_BUF_ERROR");
}

TEST(CompressionTest, Zlib) {
  CompressionSpec *CSpec = getCompressionSpec(CompressionKind::Zlib);
  CompressionImpl *CImpl = CSpec->Implementation;
  testZlibCompression("", CImpl->DefaultLevel);

  testZlibCompression("hello, world!", CImpl->BestSizeLevel);
  testZlibCompression("hello, world!", CImpl->BestSpeedLevel);
  testZlibCompression("hello, world!", CImpl->DefaultLevel);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZlibCompression(BinaryDataStr, CImpl->BestSizeLevel);
  testZlibCompression(BinaryDataStr, CImpl->BestSpeedLevel);
  testZlibCompression(BinaryDataStr, CImpl->DefaultLevel);
}
#endif

#if LLVM_ENABLE_ZSTD
static void testZStdCompression(StringRef Input, int Level) {
  testCompressionAlgorithm(Input, Level,
                           getCompressionSpec(CompressionKind::ZStd),
                           "Destination buffer is too small");
}

TEST(CompressionTest, ZStd) {
  CompressionSpec *CSpec = getCompressionSpec(CompressionKind::ZStd);
  CompressionImpl *CImpl = CSpec->Implementation;
  testZStdCompression("", CImpl->DefaultLevel);

  testZStdCompression("hello, world!", CImpl->BestSizeLevel);
  testZStdCompression("hello, world!", CImpl->BestSpeedLevel);
  testZStdCompression("hello, world!", CImpl->DefaultLevel);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZStdCompression(BinaryDataStr, CImpl->BestSizeLevel);
  testZStdCompression(BinaryDataStr, CImpl->BestSpeedLevel);
  testZStdCompression(BinaryDataStr, CImpl->DefaultLevel);
}
#endif
} // namespace
