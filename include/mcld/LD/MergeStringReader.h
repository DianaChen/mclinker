//===- MergeStringReader.h ------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LD_MERGESTRINGREADER_H_
#define MCLD_LD_MERGESTRINGREADER_H_

#include "mcld/LD/MergeString.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/DataTypes.h>

namespace mcld {

class Input;

/** \class MergeStringReader
 *  \brief MergeStringReader MergeStringReader reads an input section which
 *  has SHF_MERGE and SHF_STRING. The section is split into several fragments
 *  and each of them contains a single string.
 */
class MergeStringReader {
 public:
  /// read - read an section with merge string flag, these sections will be read
  /// into a set of RegionFragment, each with only one string.
  template <size_t BITCLASS, bool SAME_ENDIAN>
  bool read(Input& pInput, MergeString& pMergeString);
};

template <>
bool MergeStringReader::read<32, true>(Input& pInput,
                                       MergeString& pMergeString);

}  // namespace mcld

#endif  // MCLD_LD_MERGESTRINGREADER_H_
