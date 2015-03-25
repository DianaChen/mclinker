//===- MergeString.cpp ----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "mcld/LD/MergeString.h"

#include "mcld/Fragment/FragmentRef.h"
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

uint64_t MergeString::getOutputOffset(const Fragment& pFrag) const {
  const Entry& entry = llvm::cast<const Entry>(pFrag);
  return entry.getOutputEntry().getOffset();
}

uint64_t MergeString::getOutputOffset(uint64_t pInputOffset,
                                      const Fragment& pFrag) const {
  return doGetOutputOffset(pInputOffset, pFrag);
}

Fragment& MergeString::getOutputFragment(FragmentRef& pFragRef) {
  return doGetOutputFragment(pFragRef);
}

const Fragment&
MergeString::getOutputFragment(const FragmentRef& pFragRef) const {
  return doGetOutputFragment(pFragRef);
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
  // Map the first fragment to the input MergeString
  if (!pOther.getSectionData().empty())
    m_FragSectMap[&pOther.getSectionData().front()] = &pOther;

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

uint64_t MergeStringOutput::doGetOutputOffset(uint64_t pInputOffset,
                                              const Fragment& pFrag) const {
  // get the input MergeString
  assert(m_FragSectMap.find(&pFrag) != m_FragSectMap.end());
  return m_FragSectMap.at(&pFrag)->getOutputOffset(pInputOffset, pFrag);
}

Fragment& MergeStringOutput::doGetOutputFragment(FragmentRef& pFragRef) {
  return llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
}

const Fragment&
MergeStringOutput::doGetOutputFragment(const FragmentRef& pFragRef) const {
  return llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
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

uint64_t MergeStringInput::doGetOutputOffset(uint64_t pInputOffset,
                                             const Fragment& pFrag) const {
  assert(m_InOffsetMap.find(pInputOffset) != m_InOffsetMap.end());
  return m_InOffsetMap.at(pInputOffset)->getOutputEntry().getOffset();
}

Fragment& MergeStringInput::doGetOutputFragment(FragmentRef& pFragRef) {
  // If this MergeStringInput is created during MergeSections,
  // the data in the target section has been read into a SectionData as a
  // normal section. And then the data will be read again to MergeString during
  // MergeSections, while this will be done after read symbols. Hence the symbol
  // will point to the fragment in the original SectionData. In this case, we
  // can only get the output fragment according to the fragment offset (In
  // finalizeSymbolValue, those symbols defined in this section still hold
  // FragmentRef to m_pPreservedData)
  // FIXME: this is a trick that we assume the original SectionData contains
  // only one RegionFragment (so the offset could be correct)
  if (pFragRef.frag()->getParent() != m_pSectionData) {
    assert(m_InOffsetMap.find(pFragRef.offset()) != m_InOffsetMap.end());
    return m_InOffsetMap.at(pFragRef.offset())->getOutputEntry();
  }
  return llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
}

const Fragment&
MergeStringInput::doGetOutputFragment(const FragmentRef& pFragRef) const {
  if (pFragRef.frag()->getParent() != m_pSectionData) {
    assert(m_InOffsetMap.find(pFragRef.offset()) != m_InOffsetMap.end());
    return m_InOffsetMap.at(pFragRef.offset())->getOutputEntry();
  }
  return llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
}

const LDSection&
MergeStringInput::getOutputSection(const Fragment& pFrag) const {
  const Entry& entry = llvm::cast<const Entry>(pFrag);
  return entry.getOutputEntry().getParent()->getSection();
}

LDSection& MergeStringInput::getOutputSection(Fragment& pFrag) {
  // get the section of output
  Entry& entry = llvm::cast<Entry>(pFrag);
  return entry.getOutputEntry().getParent()->getSection();
}

}  // namespace mcld
