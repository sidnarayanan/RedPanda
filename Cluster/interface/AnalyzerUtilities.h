#ifndef ANALYZERUTILS_H
#define ANALYZERUTILS_H

// PandaProd Objects
#include "PandaTree/Objects/interface/Event.h"
#include "PandaTree/Objects/interface/EventRed.h"

// PANDACore
#include "PandaCore/Tools/interface/Common.h"

// fastjet
#include "fastjet/PseudoJet.hh"
#include "fastjet/JetDefinition.hh"
#include "fastjet/GhostedAreaSpec.hh"
#include "fastjet/AreaDefinition.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/contrib/SoftDrop.hh"
#include "fastjet/contrib/MeasureDefinition.hh"

////////////////////////////////////////////////////////////////////////////////////
typedef std::vector<fastjet::PseudoJet> VPseudoJet;

inline VPseudoJet ConvertPFCands(std::vector<const panda::PFCand*> &incoll, bool puppi, double minPt=0.001) {
  VPseudoJet vpj;
  vpj.reserve(incoll.size());
  for (auto *incand : incoll) {
    double factor = puppi ? incand->puppiW() : 1;
    if (factor*incand->pt()<minPt)
      continue;
    vpj.emplace_back(factor*incand->px(),factor*incand->py(),
                     factor*incand->pz(),factor*incand->e());
  }
  return vpj;
}

inline VPseudoJet ConvertPFCands(panda::RefVector<panda::PFCand> &incoll, bool puppi, double minPt=0.001) {
  std::vector<const panda::PFCand*> outcoll;
  outcoll.reserve(incoll.size());
  for (auto incand : incoll)
    outcoll.push_back(incand.get());

  return ConvertPFCands(outcoll, puppi, minPt);
}

inline VPseudoJet ConvertPFCands(panda::PFCandCollection &incoll, bool puppi, double minPt=0.001) {
  std::vector<const panda::PFCand*> outcoll;
  outcoll.reserve(incoll.size());
  for (auto &incand : incoll)
    outcoll.push_back(&incand);

  return ConvertPFCands(outcoll, puppi, minPt);
}

inline VPseudoJet 
ConvertGenParticles(std::vector<panda::GenParticle*> &incoll, 
                    std::map<fastjet::PseudoJet*, panda::GenParticle*> &assoc,
                    double minPt=0.001) 
{
  VPseudoJet vpj; vpj.reserve(incoll.size());
  for (auto *ingen : incoll) {
    if (ingen->pt() < minPt) 
      continue;
    vpj.emplace_back(ingen->px(),ingen->py(),ingen->pz(),ingen->e());
    assoc[&(vpj.back())] = ingen;
  }
  return vpj;
}


////////////////////////////////////////////////////////////////////////////////////

#endif
