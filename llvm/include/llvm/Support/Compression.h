//===-- llvm/Support/Compression.h ---Compression----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains basic functions for compression/uncompression.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_COMPRESSION_H
#define LLVM_SUPPORT_COMPRESSION_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
class Error;

namespace compression {

struct CompressionAlgorithm {
  const StringRef Name;
  const int BestSpeedLevel;
  const int DefaultLevel;
  const int BestSizeLevel;
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer,
                        int Level) = 0;
  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) = 0;
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer) {
    return compress(Input, CompressedBuffer, this->DefaultLevel);
  }

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }

protected:
  CompressionAlgorithm(StringRef Name, int BestSpeedLevel, int DefaultLevel,
                       int BestSizeLevel)
      : Name(Name), BestSpeedLevel(BestSpeedLevel), DefaultLevel(DefaultLevel),
        BestSizeLevel(BestSizeLevel) {}
};

class CompressionKind {
private:
  uint8_t CompressionID;

protected:
  friend constexpr llvm::Optional<CompressionKind>
  getOptionalCompressionKind(uint8_t OptionalCompressionID);
  // because getOptionalCompressionKind is the only friend:
  // we can trust the value of y is valid
  constexpr CompressionKind(uint8_t CompressionID)
      : CompressionID(CompressionID) {}

public:
  constexpr operator uint8_t() const { return CompressionID; }
  CompressionAlgorithm *operator->() const;

  constexpr operator bool() const;

  static const llvm::compression::CompressionKind Unknown, Zlib, ZStd;
};
constexpr inline const llvm::compression::CompressionKind
    llvm::compression::CompressionKind::Unknown{255}, ///< Abstract compression
    llvm::compression::CompressionKind::Zlib{1}, ///< zlib style complession
    llvm::compression::CompressionKind::ZStd{2}; ///< zstd style complession
typedef llvm::Optional<CompressionKind> OptionalCompressionKind;

constexpr CompressionKind::operator bool() const {
  switch (uint8_t(CompressionID)) {
  case uint8_t(CompressionKind::Zlib):
    return LLVM_ENABLE_ZLIB;
  case uint8_t(CompressionKind::ZStd):
    return LLVM_ENABLE_ZSTD;
  default:
    return false;
  }
}

constexpr bool operator==(CompressionKind Left, CompressionKind Right) {
  return uint8_t(Left) == uint8_t(Right);
}

constexpr OptionalCompressionKind
getOptionalCompressionKind(uint8_t OptionalCompressionID) {
  switch (OptionalCompressionID) {
  case uint8_t(0):
    return NoneType();
  case uint8_t(CompressionKind::Zlib):
  case uint8_t(CompressionKind::ZStd):
    return CompressionKind(OptionalCompressionID);
  default:
    return CompressionKind::Unknown;
  }
}

} // End of namespace compression

} // End of namespace llvm

#endif
