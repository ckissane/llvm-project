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
  Base = 255, ///< Abstract compression
  None = 0,   ///< No compression
  Zlib = 1,   ///< zlib style complession
  ZStd = 2,   ///< zstd style complession
};

// This is the base class of all compression algorithms that llvm support
// handles.
class CompressionAlgorithm {
public:
  static constexpr SupportCompressionType AlgorithmId =
      SupportCompressionType::Base;
  static constexpr StringRef name = "base";
  static constexpr int BestSpeedCompression = -999;
  static constexpr int DefaultCompression = -999;
  static constexpr int BestSizeCompression = -999;

  // // estimates level to achive compression speed around scale*(speed at level
  // which has max (speed*ratio) on mozilla-unified Bundle) int
  // levelToTargetCompressionSpeed(float scale){
  //   return 1;
  // };
  bool supported();

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer,
                int Level = DefaultCompression);

  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize);

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }
  constexpr CompressionAlgorithm(){};
};

class NoneCompressionAlgorithm : public CompressionAlgorithm {
  using super = CompressionAlgorithm;

public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::None;
  constexpr static StringRef name = "none";
  constexpr static int BestSpeedCompression = 0;
  constexpr static int DefaultCompression = 0;
  constexpr static int BestSizeCompression = 0;

  bool supported();
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer,
                int Level = DefaultCompression);

  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize);

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }

  constexpr NoneCompressionAlgorithm() : super(){};
};

class ZlibCompressionAlgorithm : public CompressionAlgorithm {
  using super = CompressionAlgorithm;

public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::Zlib;
  constexpr static StringRef name = "zlib";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 6;
  constexpr static int BestSizeCompression = 9;

  bool supported();
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer,
                int Level = DefaultCompression);

  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize);

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }
  constexpr ZlibCompressionAlgorithm() : super(){};
};

class ZStdCompressionAlgorithm : public CompressionAlgorithm {
  using super = CompressionAlgorithm;

public:
  constexpr static SupportCompressionType AlgorithmId =
      SupportCompressionType::ZStd;
  constexpr static StringRef name = "zstd";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 5;
  constexpr static int BestSizeCompression = 12;

  bool supported();
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer,
                int Level = DefaultCompression);

  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize);

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }
  constexpr ZStdCompressionAlgorithm() : super(){};
};
llvm::compression::CompressionAlgorithm
CompressionAlgorithmFromId(uint8_t CompressionSchemeId);

} // End of namespace compression

} // End of namespace llvm

#endif
