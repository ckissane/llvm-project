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
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
class Error;

namespace compression {

enum class SupportCompressionType : uint8_t {
  Unknown = 255, ///< Abstract compression
  None = 0,      ///< No compression
  Zlib = 1,      ///< zlib style complession
  ZStd = 2,      ///< zstd style complession
};

// This is the base class of all compression algorithms that llvm support
// handles.

class CompressionAlgorithm {
public:
  virtual SupportCompressionType getAlgorithmId() = 0;

  virtual StringRef getName() = 0;

  virtual bool supported() = 0;

  virtual int getBestSpeedLevel() = 0;
  virtual int getDefaultLevel() = 0;
  virtual int getBestSizeLevel() = 0;

  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer,
                        int Level) = 0;
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer) = 0;

  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) = 0;
  virtual Error decompress(ArrayRef<uint8_t> Input,
                           SmallVectorImpl<uint8_t> &UncompressedBuffer,
                           size_t UncompressedSize) = 0;

  virtual CompressionAlgorithm *when(bool useCompression) = 0;
  virtual CompressionAlgorithm *whenSupported() = 0;

  virtual bool notNone() = 0;
};
class NoneCompressionAlgorithm;
class UnknownCompressionAlgorithm;
class ZStdCompressionAlgorithm;
class ZlibCompressionAlgorithm;

template <class CompressionAlgorithmType>
class CompressionAlgorithmImpl : public CompressionAlgorithm {
public:
  virtual SupportCompressionType getAlgorithmId() {
    return CompressionAlgorithmType::AlgorithmId;
  }

  virtual StringRef getName() { return CompressionAlgorithmType::Name; }

  virtual bool supported() { return CompressionAlgorithmType::Supported(); }

  virtual int getBestSpeedLevel() {
    return CompressionAlgorithmType::BestSpeedCompression;
  }
  virtual int getDefaultLevel() {
    return CompressionAlgorithmType::DefaultCompression;
  }
  virtual int getBestSizeLevel() {
    return CompressionAlgorithmType::BestSizeCompression;
  }

  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {

    return CompressionAlgorithmType::Compress(Input, CompressedBuffer, Level);
  }
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer) {
    return CompressionAlgorithmType::Compress(
        Input, CompressedBuffer, CompressionAlgorithmType::DefaultCompression);
  }

  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) {
    return CompressionAlgorithmType::Decompress(Input, UncompressedBuffer,
                                                UncompressedSize);
  }
  virtual Error decompress(ArrayRef<uint8_t> Input,
                           SmallVectorImpl<uint8_t> &UncompressedBuffer,
                           size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = CompressionAlgorithmType::Decompress(
        Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }

  virtual CompressionAlgorithm *when(bool useCompression);
  virtual CompressionAlgorithm *whenSupported() {
    return this->when(CompressionAlgorithmType::Supported());
  }

  virtual bool notNone() {
    return CompressionAlgorithmType::AlgorithmId !=
           SupportCompressionType::None;
  }
};

class ZStdCompressionAlgorithm
    : public CompressionAlgorithmImpl<ZStdCompressionAlgorithm> {
public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::ZStd;
  constexpr static StringRef Name = "zstd";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 5;
  constexpr static int BestSizeCompression = 12;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

  static ZStdCompressionAlgorithm *Instance;

protected:
  constexpr ZStdCompressionAlgorithm(){};
};

class ZlibCompressionAlgorithm
    : public CompressionAlgorithmImpl<ZlibCompressionAlgorithm> {
public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::Zlib;
  constexpr static StringRef Name = "zlib";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 6;
  constexpr static int BestSizeCompression = 9;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

  static ZlibCompressionAlgorithm *Instance;

protected:
  constexpr ZlibCompressionAlgorithm(){};
};

class UnknownCompressionAlgorithm
    : public CompressionAlgorithmImpl<UnknownCompressionAlgorithm> {
public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::Unknown;
  constexpr static StringRef Name = "unknown";
  constexpr static int BestSpeedCompression = -999;
  constexpr static int DefaultCompression = -999;
  constexpr static int BestSizeCompression = -999;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

  static UnknownCompressionAlgorithm *Instance;

protected:
  constexpr UnknownCompressionAlgorithm(){};
};
class NoneCompressionAlgorithm
    : public CompressionAlgorithmImpl<NoneCompressionAlgorithm> {

public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::None;
  constexpr static StringRef Name = "none";
  constexpr static int BestSpeedCompression = 0;
  constexpr static int DefaultCompression = 0;
  constexpr static int BestSizeCompression = 0;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

  static NoneCompressionAlgorithm *Instance;

protected:
  constexpr NoneCompressionAlgorithm(){};
};

static NoneCompressionAlgorithm *NoneCompression =
    NoneCompressionAlgorithm::Instance;
static UnknownCompressionAlgorithm *UnknownCompression =
    UnknownCompressionAlgorithm::Instance;
static ZStdCompressionAlgorithm *ZStdCompression =
    ZStdCompressionAlgorithm::Instance;
static ZlibCompressionAlgorithm *ZlibCompression =
    ZlibCompressionAlgorithm::Instance;

llvm::compression::CompressionAlgorithm *
getCompressionAlgorithm(SupportCompressionType CompressionSchemeId);
llvm::compression::CompressionAlgorithm *
getCompressionAlgorithm(uint8_t CompressionSchemeId);

} // End of namespace compression

} // End of namespace llvm

#endif
