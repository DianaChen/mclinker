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

uint64_t MergeString::getOutputOffset(const FragmentRef& pFragRef) const {
  const Entry& entry = llvm::cast<const Entry>(*pFragRef.frag());
  return entry.getOutputEntry().getOffset() + pFragRef.offset();
}

uint64_t MergeString::getOutputOffset(uint64_t pInputOffset,
                                      const FragmentRef& pFragRef) const {
  return doGetOutputOffset(pInputOffset, pFragRef);
}

void MergeString::updateFragmentRef(FragmentRef& pFragRef) {
  return doUpdateFragmentRef(pFragRef);
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

MergeString& MergeStringOutput::merge(MergeString& pOther, bool pForce) {
  assert(!pOther.isOutput());
  // Map the first fragment to the input MergeString
  if (!pOther.getSectionData().empty())
    m_FragSectMap[&pOther.getSectionData().front()] = &pOther;

  uint64_t offset = m_pSection->size();
  if (pForce) {
    // set the incoming Fragments' parent and offset to the output one and then
    // add them into output SectionData
    SectionData::iterator it, end = pOther.getSectionData().end();
    for (it = pOther.getSectionData().begin(); it != end; ++it) {
      assert(it->getKind() == Fragment::Region);
      it->setParent(m_pSectionData);
      it->setOffset(offset);
      Entry* string = &(llvm::cast<Entry>(*it));
      string->setOutputEntry(*string);
      offset += string->getRegion().size();
    }
    // move all contents from pOther
    m_pSectionData->getFragmentList().splice(
        m_pSectionData->end(), pOther.getSectionData().getFragmentList());
  } else {
    // traverse the strings in pOther
    SectionData::iterator it = pOther.getSectionData().begin(),
                          end = pOther.getSectionData().end();
    while (it != end) {
      if (it->getKind() != Fragment::Region) {
        ++it;
        continue;
      }
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
  }
  // set section size
  m_pSection->setSize(offset);
  return *this;
}

uint64_t
MergeStringOutput::doGetOutputOffset(uint64_t pInputOffset,
                                     const FragmentRef& pFragRef) const {
  // get the input MergeString
  assert(m_FragSectMap.find(pFragRef.frag()) != m_FragSectMap.end());
  return m_FragSectMap.at(pFragRef.frag())->getOutputOffset(pInputOffset,
                                                            pFragRef);
}

void MergeStringOutput::doUpdateFragmentRef(FragmentRef& pFragRef) {
  Fragment& frag = llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
  pFragRef.assign(frag, pFragRef.offset());
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

uint64_t
MergeStringInput::doGetOutputOffset(uint64_t pInputOffset,
                                    const FragmentRef& pFragRef) const {
  // A symbol may not refer to the beginning of a string fragment. The beginning
  // offset of a string fragment would be pInputOffset - pFragRef.offset()
  const_offset_map_iterator it = m_InOffsetMap.find(
                                     pInputOffset - pFragRef.offset());
  if (it != m_InOffsetMap.end()) {
    return (it->second)->getOutputEntry().getOffset() + pFragRef.offset();
  }
  // if the offset does not match any one in input offset map, than we should
  // find the nearest one prior to it
  Entry* target_entry = NULL;
  uint64_t nearest = 0x0u;
  const_offset_map_iterator end = m_InOffsetMap.end();
  for (it = m_InOffsetMap.begin(); it != end; ++it) {
    if (it->first > pInputOffset)
      continue;

    if (it->first >= nearest) {
      nearest = it->first;
      target_entry = it->second;
    }
  }
  assert(target_entry != NULL);
  return target_entry->getOutputEntry().getOffset()
             + pFragRef.offset()
             + pInputOffset
             - nearest;
}

void MergeStringInput::doUpdateFragmentRef(FragmentRef& pFragRef) {
  if (pFragRef.frag()->getParent() == m_pSectionData) {
    Fragment& frag = llvm::cast<Entry>(pFragRef.frag())->getOutputEntry();
    pFragRef.assign(frag, pFragRef.offset());
    return;
  }

  // If this MergeStringInput is created during MergeSections,
  // the data in the target section has been read into a SectionData as a
  // normal section. And then the data will be read again to MergeString during
  // MergeSections, while this will be done after read symbols. Hence the symbol
  // will point to the fragment in the original SectionData. In this case, we
  // can only get the output fragment according to the fragment offset.
  // FIXME: this is a trick that we assume the original SectionData contains
  // only one RegionFragment (so the offset could be correct)
  const_offset_map_iterator it = m_InOffsetMap.find(pFragRef.offset());
  if (it != m_InOffsetMap.end()) {
    pFragRef.assign(it->second->getOutputEntry(), 0x0u);
    return;
  }
  // find the splitted fragment that this input offset belongs to
  Entry* target_entry = NULL;
  uint64_t nearest = 0x0u;
  const_offset_map_iterator end = m_InOffsetMap.end();
  for (it = m_InOffsetMap.begin(); it != end; ++it) {
    if (it->first > pFragRef.offset())
      continue;

    if (it->first >= nearest) {
      nearest = it->first;
      target_entry = it->second;
    }
  }
  assert(target_entry != NULL);
  pFragRef.assign(*target_entry, pFragRef.offset() - nearest);
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
