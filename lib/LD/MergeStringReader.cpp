//===- MergeStringReader.cpp ----------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "mcld/LD/MergeStringReader.h"

#include "mcld/Fragment/NullFragment.h"
#include "mcld/LD/LDSection.h"
#include "mcld/LD/SectionData.h"
#include "mcld/MC/Input.h"
#include "mcld/Support/MemoryArea.h"

#include <llvm/ADT/StringRef.h>

namespace mcld {

//===----------------------------------------------------------------------===//
// Non-member functions
//===----------------------------------------------------------------------===//
static inline size_t string_length(const char* pStr) {
  const char* p = pStr;
  size_t len = 0;
  for (; *p != 0; ++p)
    ++len;
  return len + 1;
}

//===----------------------------------------------------------------------===//
// EhFrameReader
//===----------------------------------------------------------------------===//
template <>
bool MergeStringReader::read<32, true>(Input& pInput,
                                       MergeString& pMergeString) {
  LDSection& section= pMergeString.getSection();
  if (section.size() == 0x0) {
    NullFragment* frag = new NullFragment();
    pMergeString.getSectionData().getFragmentList().push_back(frag);
    return true;
  }
  // get the section contents
  uint64_t file_off = pInput.fileOffset() + section.offset();
  llvm::StringRef sect_region =
      pInput.memArea()->request(file_off, section.size());

  size_t frag_off = 0x0;
  const char* str = sect_region.begin();
  // split the section contents into fragments those with one string each
  while (str < sect_region.end()) {
    size_t len = string_length(str);
    pMergeString.addString(llvm::StringRef(str, len), frag_off);
    frag_off += len;
  }
  return true;
}

}  // namespace mcld
