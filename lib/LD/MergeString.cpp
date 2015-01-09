//===- MergeString.cpp ----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "mcld/LD/MergeString.h"

#include "mcld/LD/LDSection.h"
#include "mcld/LD/SectionData.h"
#include "mcld/Support/GCFactory.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>

namespace mcld {

typedef GCFactory<MergeStringOutput, 1> MSOutputFactory;
typedef GCFactory<MergeStringInput, MCLD_SECTIONS_PER_INPUT> MSInputFactory;

static llvm::ManagedStatic<MSOutputFactory> g_MSOutputFactory;
static llvm::ManagedStatic<MSInputFactory> g_MSInputFactory;

//===----------------------------------------------------------------------===//
// MergeString
//===----------------------------------------------------------------------===//
MergeString::MergeString(LDSection& pSection)
    : m_pSection(&pSection) {
  m_pSectionData = SectionData::Create(pSection);
}

uint64_t MergeString::getOutputOffset(RegionFragment& pFrag) const {
  Entry& entry = llvm::cast<Entry>(pFrag);
  return entry.getOutputEntry().getOffset();
}

//===----------------------------------------------------------------------===//
// MergeStringOutput
//===----------------------------------------------------------------------===//
MergeStringOutput* MergeStringOutput::Create(LDSection& pSection) {
  MergeStringOutput* result = g_MSOutputFactory->allocate();
  new (result) MergeStringOutput(pSection);
  return result;
}

void MergeStringOutput::Destroy(MergeStringOutput*& pMS) {
  pMS->~MergeStringOutput();
  g_MSOutputFactory->deallocate(pMS);
  pMS = NULL;
}

void MergeStringOutput::Clear() {
  g_MSOutputFactory->clear();
}

void MergeStringOutput::clearStringPool() {
  m_StringPool.clear();
}

MergeString& MergeStringOutput::merge(MergeString& pOther) {
  assert(!pOther.isOutput());
  // traverse the strings in pOther
  uint64_t offset = m_pSection->size();
  SectionData::iterator it = pOther.getSectionData().begin();
  while (it != pOther.getSectionData().end()) {
    if (it->getKind() != Fragment::Region)
      continue;
    Entry* string = &(llvm::cast<Entry>(*it));
    std::pair<StringPoolTy::iterator, bool> res = m_StringPool.insert(string);
    // set the OutputEntry to this string
    string->setOutputEntry(**res.first);
    // if the string inserted (which means there is no an identical string in
    // the pool), then move this string to output SectionData
    if (res.second) {
      SectionData::iterator to_be_splice = it;
      ++it;
      m_pSectionData->getFragmentList().splice(m_pSectionData->end(),
          pOther.getSectionData().getFragmentList(), to_be_splice);
      string->setParent(m_pSectionData);
      // set fragment offset
      string->setOffset(offset);
      offset += string->getRegion().size();
    } else {
      ++it;
    }
  }
  // set section size
  m_pSection->setSize(offset);
  return *this;
}

//===----------------------------------------------------------------------===//
// MergeStringInput
//===----------------------------------------------------------------------===//
MergeStringInput* MergeStringInput::Create(LDSection& pSection) {
  MergeStringInput* result = g_MSInputFactory->allocate();
  new (result) MergeStringInput(pSection);
  return result;
}

void MergeStringInput::Destroy(MergeStringInput*& pMS) {
  pMS->~MergeStringInput();
  g_MSInputFactory->deallocate(pMS);
  pMS = NULL;
}

void MergeStringInput::Clear() {
  g_MSInputFactory->clear();
}

void MergeStringInput::addString(llvm::StringRef pString,
                                 uint64_t pInputOffset) {
  // create an entry
  Entry* entry = new Entry(pString, m_pSectionData);
  entry->setOffset(pInputOffset);
  // record the input offset
  m_InOffsetMap[pInputOffset] = entry;
}

uint64_t MergeStringInput::getOutputOffset(uint64_t pInputOffset) const {
  assert(m_InOffsetMap.find(pInputOffset) != m_InOffsetMap.end());
  return m_InOffsetMap.at(pInputOffset)->getOutputEntry().getOffset();
}

}  // namespace mcld
