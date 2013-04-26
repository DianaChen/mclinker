//===- HexagonPLT.cpp -----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "HexagonPLT.h"

#include <llvm/Support/ELF.h>
#include <llvm/Support/Casting.h>

#include <mcld/LD/LDSection.h>
#include <mcld/LinkerConfig.h>
#include <mcld/Support/MsgHandling.h>

using namespace mcld;

//===----------------------------------------------------------------------===//
// PLT entry data
//===----------------------------------------------------------------------===//
HexagonPLT0::HexagonPLT0(SectionData& pParent)
  : PLT::Entry<sizeof(hexagon_plt0)>(pParent)
{
}

HexagonPLT1::HexagonPLT1(SectionData& pParent)
  : PLT::Entry<sizeof(hexagon_plt1)>(pParent)
{
}

//===----------------------------------------------------------------------===//
// HexagonPLT
//===----------------------------------------------------------------------===//
HexagonPLT::HexagonPLT(LDSection& pSection,
               HexagonGOTPLT &pGOTPLT,
               const LinkerConfig& pConfig)
  : PLT(pSection),
    m_GOTPLT(pGOTPLT),
    m_Config(pConfig)
{
  assert(LinkerConfig::DynObj == m_Config.codeGenType() ||
         LinkerConfig::Exec   == m_Config.codeGenType() ||
         LinkerConfig::Binary == m_Config.codeGenType());

  m_PLT0 = hexagon_plt0;
  m_PLT0Size = sizeof (hexagon_plt0);
  // create PLT0
  new HexagonPLT0(*m_SectionData);
  m_Last = m_SectionData->begin();
}

HexagonPLT::~HexagonPLT()
{
}

PLTEntryBase* HexagonPLT::getPLT0() const
{
  iterator first = m_SectionData->getFragmentList().begin();

  assert(first != m_SectionData->getFragmentList().end() &&
         "FragmentList is empty, getPLT0 failed!");

  PLTEntryBase* plt0 = &(llvm::cast<PLTEntryBase>(*first));

  return plt0;
}

void HexagonPLT::finalizeSectionSize()
{
  uint64_t size = 0;
  // plt0 size
  size = getPLT0()->size();

  m_Section.setSize(size);

  uint32_t offset = 0;
  SectionData::iterator frag, fragEnd = m_SectionData->end();
  for (frag = m_SectionData->begin(); frag != fragEnd; ++frag) {
    frag->setOffset(offset);
    offset += frag->size();
  }
}

bool HexagonPLT::hasPLT1() const
{
  return (m_SectionData->size() > 1);
}

void HexagonPLT::reserveEntry(size_t pNum)
{
  PLTEntryBase* plt1_entry = NULL;

  for (size_t i = 0; i < pNum; ++i) {
    plt1_entry = new HexagonPLT1(*m_SectionData);

    if (NULL == plt1_entry)
      fatal(diag::fail_allocate_memory_plt);
  }
}

HexagonPLT1* HexagonPLT::consume()
{
  ++m_Last;
  assert(m_Last != m_SectionData->end() &&
         "The number of PLT Entries and ResolveInfo doesn't match");

  return llvm::cast<HexagonPLT1>(&(*m_Last));
}

void HexagonPLT::applyPLT0()
{
  PLTEntryBase* plt0 = getPLT0();

  unsigned char* data = 0;
  data = static_cast<unsigned char*>(malloc(plt0->size()));

  if (!data)
    fatal(diag::fail_allocate_memory_plt);

  memcpy(data, m_PLT0, plt0->size());

  // uint32_t gotpltAddr = m_GOTPLT.addr();
  // SHANKARE TODO: Set the first entry and the second entry to the address
  // of the gotPltAddr

  plt0->setValue(data);
}

void HexagonPLT::applyPLT1() {

  uint64_t plt_base = m_Section.addr();
  assert(plt_base && ".plt base address is NULL!");

  uint64_t got_base = m_GOTPLT.addr();
  assert(got_base && ".got base address is NULL!");

  HexagonPLT::iterator it = m_SectionData->begin();
  HexagonPLT::iterator ie = m_SectionData->end();
  assert(it != ie && "FragmentList is empty, applyPLT1 failed!");

  uint32_t GOTEntrySize = HexagonGOTEntry::EntrySize;
  uint32_t GOTEntryAddress =
    got_base +  GOTEntrySize * 4;

  uint64_t PLTEntryAddress =
    plt_base + HexagonPLT0::EntrySize; //Offset of PLT0

  ++it; //skip PLT0
  uint64_t PLT1EntrySize = HexagonPLT1::EntrySize;
  HexagonPLT1* plt1 = NULL;

  uint32_t* Out = NULL;
  while (it != ie) {
    plt1 = &(llvm::cast<HexagonPLT1>(*it));
    Out = static_cast<uint32_t*>(malloc(HexagonPLT1::EntrySize));

    if (!Out)
      fatal(diag::fail_allocate_memory_plt);

    // Address in the PLT entries point to the corresponding GOT entries
    // TODO: Fixup plt to point to the corresponding GOTEntryAddress
    // We need to borrow the same relocation code to fix the relocation
    plt1->setValue(reinterpret_cast<unsigned char*>(Out));
    ++it;

    GOTEntryAddress += GOTEntrySize;
    PLTEntryAddress += PLT1EntrySize;
  }
}

uint64_t HexagonPLT::emit(MemoryRegion& pRegion)
{
  uint64_t result = 0x0;
  iterator it = begin();

  unsigned char* buffer = pRegion.getBuffer();
  memcpy(buffer, llvm::cast<HexagonPLT0>((*it)).getValue(), HexagonPLT0::EntrySize);
  result += HexagonPLT0::EntrySize;
  ++it;

  HexagonPLT1* plt1 = 0;
  HexagonPLT::iterator ie = end();
  while (it != ie) {
    plt1 = &(llvm::cast<HexagonPLT1>(*it));
    memcpy(buffer + result, plt1->getValue(), HexagonPLT1::EntrySize);
    result += HexagonPLT1::EntrySize;
    ++it;
  }
  return result;
}

