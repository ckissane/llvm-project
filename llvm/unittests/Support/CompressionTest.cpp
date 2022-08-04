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
    StringRef Input, int Level, compression::CompressionKind CompressionScheme,
    std::string ExpectedDestinationBufferTooSmallErrorMessage) {
  SmallVector<uint8_t, 0> Compressed;
  SmallVector<uint8_t, 0> Uncompressed;
  CompressionScheme->compress(arrayRefFromStringRef(Input), Compressed, Level);

  // Check that uncompressed buffer is the same as original.
  Error E =
      CompressionScheme->decompress(Compressed, Uncompressed, Input.size());
  consumeError(std::move(E));

  EXPECT_EQ(Input, toStringRef(Uncompressed));
  if (Input.size() > 0) {
    // Uncompression fails if expected length is too short.
    E = CompressionScheme->decompress(Compressed, Uncompressed,
                                      Input.size() - 1);
    EXPECT_EQ(ExpectedDestinationBufferTooSmallErrorMessage,
              llvm::toString(std::move(E)));
  }
}

#if LLVM_ENABLE_ZLIB
static void testZlibCompression(StringRef Input, int Level) {
  testCompressionAlgorithm(Input, Level, CompressionKind::Zlib,
                           "zlib error: Z_BUF_ERROR");
}

TEST(CompressionTest, Zlib) {
  compression::CompressionKind CompressionScheme = CompressionKind::Zlib;
  testZlibCompression("", CompressionScheme->DefaultLevel);

  testZlibCompression("hello, world!", CompressionScheme->BestSizeLevel);
  testZlibCompression("hello, world!", CompressionScheme->BestSpeedLevel);
  testZlibCompression("hello, world!", CompressionScheme->DefaultLevel);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZlibCompression(BinaryDataStr, CompressionScheme->BestSizeLevel);
  testZlibCompression(BinaryDataStr, CompressionScheme->BestSpeedLevel);
  testZlibCompression(BinaryDataStr, CompressionScheme->DefaultLevel);
}
#endif

#if LLVM_ENABLE_ZSTD

static void testZStdCompression(StringRef Input, int Level) {
  testCompressionAlgorithm(Input, Level, CompressionKind::ZStd,
                           "Destination buffer is too small");
}

TEST(CompressionTest, Zstd) {
  compression::CompressionKind CompressionScheme = CompressionKind::ZStd;
  testZStdCompression("", CompressionScheme->DefaultLevel);

  testZStdCompression("hello, world!", CompressionScheme->BestSizeLevel);
  testZStdCompression("hello, world!", CompressionScheme->BestSpeedLevel);
  testZStdCompression("hello, world!", CompressionScheme->DefaultLevel);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZStdCompression(BinaryDataStr, CompressionScheme->BestSizeLevel);
  testZStdCompression(BinaryDataStr, CompressionScheme->BestSpeedLevel);
  testZStdCompression(BinaryDataStr, CompressionScheme->DefaultLevel);
}
#endif
}
