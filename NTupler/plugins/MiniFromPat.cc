// -*- C++ -*-
//
// Package:    PhaseTwoAnalysis/NTupler
// Class:      MiniFromPat
// 
/**\class MiniFromPat MiniFromPat.cc PhaseTwoAnalysis/NTupler/plugins/MiniFromPat.cc

Description: produces flat ntuples from PAT collections
   - storing gen, reco, and pf leptons with pT > 10 GeV and |eta| < 3
   - storing gen and reco jets with pT > 20 GeV and |eta| < 5

Implementation:
   - lepton isolation might need to be refined
   - muon ID comes from https://twiki.cern.ch/twiki/bin/viewauth/CMS/UPGTrackerTDRStudies#Muon_identification
   - electron ID comes from https://indico.cern.ch/event/623893/contributions/2531742/attachments/1436144/2208665/UPSG_EGM_Workshop_Mar29.pdf
      /!\ no ID is implemented for forward electrons as:
      - PFClusterProducer does not run on miniAOD
      - jurassic isolation needs tracks
   - PF jet ID comes from Run-2 https://github.com/cms-sw/cmssw/blob/CMSSW_9_1_1_patch1/PhysicsTools/SelectorUtils/interface/PFJetIDSelectionFunctor.h
   - b-tagging WP come from Run-2 https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation80XReReco#Supported_Algorithms_and_Operati 
      - for pfCombinedInclusiveSecondaryVertexV2BJetTags: L = 0.5426, M = 0.8484, T = 0.9535)
      - for deepCSV: L = 0.2219, M = 0.6324, T = 0.8958
*/

//
// Original Author:  Elvire Bouvier
//         Created:  Tue, 20 Jun 2017 11:27:12 GMT
//
//


// system include files
#include <memory>
#include <cmath>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"//
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "RecoEgamma/EgammaTools/interface/ConversionTools.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "PhysicsTools/SelectorUtils/interface/PFJetIDSelectionFunctor.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/JetReco/interface/GenJet.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "RecoVertex/VertexPrimitives/interface/TransientVertex.h"
#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "SimTracker/Records/interface/TrackAssociatorRecord.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "FWCore/Framework/interface/ESHandle.h"

#include "RecoVertex/KinematicFitPrimitives/interface/ParticleMass.h"
#include <RecoVertex/KinematicFitPrimitives/interface/KinematicParticleFactoryFromTransientTrack.h>
#include "RecoVertex/KinematicFitPrimitives/interface/KinematicVertex.h"
#include "RecoVertex/KinematicFit/interface/KinematicParticleVertexFitter.h"
#include "RecoVertex/KinematicFit/interface/KinematicParticleFitter.h"
#include "PhysicsTools/CandUtils/interface/AddFourMomenta.h"
#include "DataFormats/Math/interface/deltaR.h"

#include "PhaseTwoAnalysis/NTupler/interface/MiniEvent.h"

#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "Math/GenVector/VectorUtil.h"

//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class MiniFromPat : public edm::one::EDAnalyzer<edm::one::SharedResources>  {
  public:
    explicit MiniFromPat(const edm::ParameterSet&);
    ~MiniFromPat();

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

    enum ElectronMatchType {UNMATCHED = 0,
      TRUE_PROMPT_ELECTRON,
      TRUE_ELECTRON_FROM_TAU,
      TRUE_NON_PROMPT_ELECTRON};  


  private:
    virtual void beginJob() override;
    void genAnalysis(const edm::Event& iEvent, const edm::EventSetup& iSetup);
    void recoAnalysis(const edm::Event& iEvent, const edm::EventSetup& iSetup);
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override;

    bool isLooseElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot); 
    bool isMediumElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot); 
    bool isTightElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot); 
    bool isME0MuonSel(reco::Muon, double pullXCut, double dXCut, double pullYCut, double dYCut, double dPhi);

    // ----------member data ---------------------------
    edm::Service<TFileService> fs_;

    edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken_;
    edm::EDGetTokenT<std::vector<pat::Electron>> elecsToken_;
    edm::EDGetTokenT<reco::BeamSpot> bsToken_;
    edm::EDGetTokenT<std::vector<reco::Conversion>> convToken_;
    edm::EDGetTokenT<std::vector<pat::Muon>> muonsToken_;
    edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;
    PFJetIDSelectionFunctor jetIDLoose_;
    PFJetIDSelectionFunctor jetIDTight_;
    edm::EDGetTokenT<std::vector<pat::MET>> metsToken_;
    edm::EDGetTokenT<std::vector<pat::PackedCandidate>> pfCandsToken_;
    edm::EDGetTokenT<std::vector<reco::GenJet>> genJetsToken_;
    edm::EDGetTokenT<std::vector<pat::PackedGenParticle>> genPartsToken_;

    TTree *tree_;
    MiniEvent_t ev_;
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
MiniFromPat::MiniFromPat(const edm::ParameterSet& iConfig):
  verticesToken_(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("vertices"))),
  elecsToken_(consumes<std::vector<pat::Electron>>(iConfig.getParameter<edm::InputTag>("electrons"))),
  bsToken_(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("beamspot"))),
  convToken_(consumes<std::vector<reco::Conversion>>(iConfig.getParameter<edm::InputTag>("conversions"))),  
  muonsToken_(consumes<std::vector<pat::Muon>>(iConfig.getParameter<edm::InputTag>("muons"))),
  jetsToken_(consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jets"))),
  jetIDLoose_(PFJetIDSelectionFunctor::FIRSTDATA, PFJetIDSelectionFunctor::LOOSE), 
  jetIDTight_(PFJetIDSelectionFunctor::FIRSTDATA, PFJetIDSelectionFunctor::TIGHT), 
  metsToken_(consumes<std::vector<pat::MET>>(iConfig.getParameter<edm::InputTag>("mets"))),
  pfCandsToken_(consumes<std::vector<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("pfCands"))),
  genJetsToken_(consumes<std::vector<reco::GenJet>>(iConfig.getParameter<edm::InputTag>("genJets"))),
  genPartsToken_(consumes<std::vector<pat::PackedGenParticle>>(iConfig.getParameter<edm::InputTag>("genParts")))
{
  //now do what ever initialization is needed
  usesResource("TFileService");

  tree_ = fs_->make<TTree>("events","events");
  createMiniEventTree(tree_,ev_);

}


MiniFromPat::~MiniFromPat()
{

  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method to fill gen level pat -------------
  void
MiniFromPat::genAnalysis(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  Handle<std::vector<pat::PackedGenParticle>> genParts;
  iEvent.getByToken(genPartsToken_, genParts);

  Handle<std::vector<reco::GenJet>> genJets;
  iEvent.getByToken(genJetsToken_, genJets);

  // Jets
  std::vector<size_t> jGenJets;
  ev_.ngj = 0;
  for (size_t i = 0; i < genJets->size(); i++) {
    if (genJets->at(i).pt() < 20.) continue;
    if (fabs(genJets->at(i).eta()) > 5) continue;

    bool overlaps = false;
    for (size_t j = 0; j < genParts->size(); j++) {
      if (abs(genParts->at(j).pdgId()) != 11 && abs(genParts->at(j).pdgId()) != 13) continue;
      if (fabs(genJets->at(i).pt()-genParts->at(j).pt()) < 0.01*genParts->at(j).pt() && ROOT::Math::VectorUtil::DeltaR(genParts->at(j).p4(),genJets->at(i).p4()) < 0.01) {
        overlaps = true;
        break;
      }
    }
    if (overlaps) continue;
    jGenJets.push_back(i);

    ev_.gj_pt[ev_.ngj]   = genJets->at(i).pt();
    ev_.gj_phi[ev_.ngj]  = genJets->at(i).phi();
    ev_.gj_eta[ev_.ngj]  = genJets->at(i).eta();
    ev_.gj_mass[ev_.ngj] = genJets->at(i).mass();
    ev_.gj_pid[ev_.ngj]  = genJets->at(i).pdgId();
    ev_.ngj++;
  }

  // Leptons
  ev_.ngl = 0;
  for (size_t i = 0; i < genParts->size(); i++) {
    if (abs(genParts->at(i).pdgId()) != 11 && abs(genParts->at(i).pdgId()) != 13) continue;
    if (genParts->at(i).pt() < 10.) continue;
    if (fabs(genParts->at(i).eta()) > 3.) continue;
    double genIso = 0.;
    for (size_t j = 0; j < jGenJets.size(); j++) {
      if (ROOT::Math::VectorUtil::DeltaR(genParts->at(i).p4(),genJets->at(jGenJets[j]).p4()) > 0.7) continue; 
      std::vector<const reco::Candidate *> jconst = genJets->at(jGenJets[j]).getJetConstituentsQuick();
      for (size_t k = 0; k < jconst.size(); k++) {
        double deltaR = ROOT::Math::VectorUtil::DeltaR(genParts->at(i).p4(),jconst[k]->p4());
        if (deltaR < 0.01) continue;
        if (abs(genParts->at(i).pdgId()) == 13 && deltaR > 0.4) continue;
        if (abs(genParts->at(i).pdgId()) == 11 && deltaR > 0.3) continue;
        genIso = genIso + jconst[k]->pt();
      }
    }
    genIso = genIso / genParts->at(i).pt();
    ev_.gl_pid[ev_.ngl]    = genParts->at(i).pdgId();
    ev_.gl_pt[ev_.ngl]     = genParts->at(i).pt();
    ev_.gl_phi[ev_.ngl]    = genParts->at(i).phi();
    ev_.gl_eta[ev_.ngl]    = genParts->at(i).eta();
    ev_.gl_mass[ev_.ngl]   = genParts->at(i).mass();
    ev_.gl_relIso[ev_.ngl] = genIso; 
    ev_.ngl++;
  }
}

// ------------ method to fill reco level pat -------------
  void
MiniFromPat::recoAnalysis(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace edm;

  Handle<std::vector<reco::Vertex>> vertices;
  iEvent.getByToken(verticesToken_, vertices);

  Handle<std::vector<pat::Electron>> elecs;
  iEvent.getByToken(elecsToken_, elecs);
  Handle<reco::ConversionCollection> conversions;
  iEvent.getByToken(convToken_, conversions);
  Handle<reco::BeamSpot> bsHandle;
  iEvent.getByToken(bsToken_, bsHandle);
  const reco::BeamSpot &beamspot = *bsHandle.product();  

  Handle<std::vector<pat::Muon>> muons;
  iEvent.getByToken(muonsToken_, muons);

  Handle<std::vector<pat::MET>> mets;
  iEvent.getByToken(metsToken_, mets);

  Handle<std::vector<pat::PackedCandidate>> pfCands;
  iEvent.getByToken(pfCandsToken_, pfCands);

  Handle<std::vector<pat::Jet>> jets;
  iEvent.getByToken(jetsToken_, jets);

  // Vertices
  int prVtx = -1;
  for (size_t i = 0; i < vertices->size(); i++) {
    if (vertices->at(i).isFake()) continue;
    if (vertices->at(i).ndof() <= 4) continue;
    if (prVtx < 0) prVtx = i;
  }
  if (prVtx < 0) return;

  // Leptons
  ev_.nl = 0;
  for (size_t i = 0; i < muons->size(); i++) {
    if (muons->at(i).pt() < 10.) continue;
    if (fabs(muons->at(i).eta()) > 3.) continue;

    bool isLoose = (fabs(muons->at(i).eta()) < 2.4 && muon::isLooseMuon(muons->at(i))) || (fabs(muons->at(i).eta()) > 2.4 && isME0MuonSel(muons->at(i), 3, 4, 3, 4, 0.5));
    bool isMedium = (fabs(muons->at(i).eta()) < 2.4 && muon::isMediumMuon(muons->at(i))) || (fabs(muons->at(i).eta()) > 2.4 && isME0MuonSel(muons->at(i), 3, 4, 3, 4, 0.3));
    bool isTight = (fabs(muons->at(i).eta()) < 2.4 && vertices->size() > 0 && muon::isTightMuon(muons->at(i),vertices->at(prVtx))) || (fabs(muons->at(i).eta()) > 2.4 && isME0MuonSel(muons->at(i), 3, 4, 3, 4, 0.1));

    ev_.l_id[ev_.nl]     = (isTight | (isMedium<<1) | (isLoose<<2));
    ev_.l_pid[ev_.nl]    = muons->at(i).charge()*13;
    ev_.l_pt[ev_.nl]     = muons->at(i).pt();
    ev_.l_phi[ev_.nl]    = muons->at(i).phi();
    ev_.l_eta[ev_.nl]    = muons->at(i).eta();
    ev_.l_relIso[ev_.nl] = (muons->at(i).puppiNoLeptonsChargedHadronIso() + muons->at(i).puppiNoLeptonsNeutralHadronIso() + muons->at(i).puppiNoLeptonsPhotonIso()) / muons->at(i).pt();
    ev_.l_g[ev_.nl] = -1;
    for (int ig = 0; ig < ev_.ngl; ig++) {
      if (abs(ev_.gl_pid[ig]) != abs(ev_.l_pid[ev_.nl])) continue;
      if (reco::deltaR(ev_.gl_eta[ig],ev_.gl_phi[ig],ev_.l_eta[ev_.nl],ev_.l_phi[ev_.nl]) > 0.4) continue;
      ev_.l_g[ev_.nl]    = ig;
    }
    ev_.nl++;
  }

  for (size_t i = 0; i < elecs->size(); i++) {
    if (elecs->at(i).pt() < 10.) continue;
    if (fabs(elecs->at(i).eta()) > 3.) continue;

    bool isLoose = isLooseElec(elecs->at(i),conversions,beamspot);    
    bool isMedium = isMediumElec(elecs->at(i),conversions,beamspot);    
    bool isTight = isTightElec(elecs->at(i),conversions,beamspot);    

    ev_.l_id[ev_.nl]     = (isTight | (isMedium<<1) | (isLoose<<2));
    ev_.l_pid[ev_.nl]    = elecs->at(i).charge()*11;
    ev_.l_pt[ev_.nl]     = elecs->at(i).pt();
    ev_.l_phi[ev_.nl]    = elecs->at(i).phi();
    ev_.l_eta[ev_.nl]    = elecs->at(i).eta();
    ev_.l_relIso[ev_.nl] = (elecs->at(i).puppiNoLeptonsChargedHadronIso() + elecs->at(i).puppiNoLeptonsNeutralHadronIso() + elecs->at(i).puppiNoLeptonsPhotonIso()) / elecs->at(i).pt();
    ev_.l_g[ev_.nl] = -1;
    for (int ig = 0; ig < ev_.ngl; ig++) {
      if (abs(ev_.gl_pid[ig]) != abs(ev_.l_pid[ev_.nl])) continue;
      if (reco::deltaR(ev_.gl_eta[ig],ev_.gl_phi[ig],ev_.l_eta[ev_.nl],ev_.l_phi[ev_.nl]) > 0.4) continue;
      ev_.l_g[ev_.nl]    = ig;
    }
    ev_.nl++;
  }

  // Jets
  ev_.nj = 0;
  for (size_t i =0; i < jets->size(); i++) {
    if (jets->at(i).pt() < 20.) continue;
    if (fabs(jets->at(i).eta()) > 5) continue;

    bool overlaps = false;
    for (size_t j = 0; j < elecs->size(); j++) {
      if (fabs(jets->at(i).pt()-elecs->at(j).pt()) < 0.01*elecs->at(j).pt() && ROOT::Math::VectorUtil::DeltaR(elecs->at(j).p4(),jets->at(i).p4()) < 0.01) {
        overlaps = true;
        break;
      }
    }
    if (overlaps) continue;
    for (size_t j = 0; j < muons->size(); j++) {
      if (fabs(jets->at(i).pt()-muons->at(j).pt()) < 0.01*muons->at(j).pt() && ROOT::Math::VectorUtil::DeltaR(muons->at(j).p4(),jets->at(i).p4()) < 0.01) {
        overlaps = true;
        break;
      }
    }
    if (overlaps) continue;

    pat::strbitset retLoose = jetIDLoose_.getBitTemplate();
    retLoose.set(false);
    bool isLoose = jetIDLoose_(jets->at(i), retLoose);
    pat::strbitset retTight = jetIDTight_.getBitTemplate();
    retTight.set(false);
    bool isTight = jetIDTight_(jets->at(i), retTight);

    ev_.j_id[ev_.nl]      = (isTight | (isLoose<<1));
    ev_.j_pt[ev_.nj]      = jets->at(i).pt();
    ev_.j_phi[ev_.nj]     = jets->at(i).phi();
    ev_.j_eta[ev_.nj]     = jets->at(i).eta();
    ev_.j_mass[ev_.nj]    = jets->at(i).mass();
    ev_.j_csvv2[ev_.nj]   = jets->at(i).bDiscriminator("pfCombinedInclusiveSecondaryVertexV2BJetTags"); 
    ev_.j_deepcsv[ev_.nj] = jets->at(i).bDiscriminator("pfDeepCSVJetTags:probb") +
                            jets->at(i).bDiscriminator("pfDeepCSVJetTags:probbb");
    ev_.j_flav[ev_.nj]    = jets->at(i).partonFlavour();
    ev_.j_hadflav[ev_.nj] = jets->at(i).hadronFlavour();
    ev_.j_pid[ev_.nj]     = (jets->at(i).genParton() ? jets->at(i).genParton()->pdgId() : 0);
    ev_.j_g[ev_.nj] = -1;
    for (int ig = 0; ig < ev_.ngj; ig++) {
      if (reco::deltaR(ev_.gj_eta[ig],ev_.gj_phi[ig],ev_.j_eta[ev_.nj],ev_.j_phi[ev_.nj]) > 0.4) continue;
      ev_.j_g[ev_.nj]     = ig;
      break;
    }	
    ev_.nj++;

  }
  
  // MET
  ev_.nmet = 0;
  if (mets->size() > 0) {
    ev_.met_pt[ev_.nmet]  = mets->at(0).pt();
    ev_.met_phi[ev_.nmet] = mets->at(0).phi();
    ev_.nmet++;
  }

  // PF leptons
  ev_.npf = 0;
  for (size_t i = 0; i < pfCands->size(); i++) {
    if (abs(pfCands->at(i).pdgId()) != 11 and abs(pfCands->at(i).pdgId()) != 13) continue;
    if (pfCands->at(i).pt() < 10.) continue;
    if (fabs(pfCands->at(i).eta()) > 3.) continue;

    double isoPF = 0.;
    for (size_t k = 0; k < pfCands->size(); k++) {
      if (ROOT::Math::VectorUtil::DeltaR(pfCands->at(i).p4(),pfCands->at(k).p4()) > (abs(pfCands->at(i).pdgId()) == 11 ? 0.4 : 0.3)) continue;
      isoPF += pfCands->at(k).pt();
    }
    isoPF = isoPF / pfCands->at(i).pt();

    ev_.pf_pid[ev_.npf]    = pfCands->at(i).pdgId();
    ev_.pf_pt[ev_.npf]     = pfCands->at(i).pt();
    ev_.pf_eta[ev_.npf]    = pfCands->at(i).eta();
    ev_.pf_phi[ev_.npf]    = pfCands->at(i).phi();
    ev_.pf_mass[ev_.npf]   = pfCands->at(i).mass();
    ev_.pf_relIso[ev_.npf] = isoPF;
    ev_.pf_hp[ev_.npf]     = pfCands->at(i).trackHighPurity();
    ev_.npf++;

  }

}

// ------------ method called for each event  ------------
  void
MiniFromPat::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{

  //analyze the event
  if(!iEvent.isRealData()) genAnalysis(iEvent, iSetup);
  recoAnalysis(iEvent, iSetup);
  
  //save event if at least one lepton at gen or reco level
  ev_.run     = iEvent.id().run();
  ev_.lumi    = iEvent.luminosityBlock();
  ev_.event   = iEvent.id().event(); 
  tree_->Fill();

}


// ------------ method check that an e passes loose ID ----------------------------------
  bool
MiniFromPat::isLooseElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot) 
{
  if (fabs(patEl.superCluster()->eta()) > 1.479 && fabs(patEl.superCluster()->eta()) < 1.556) return false;
  if (patEl.full5x5_sigmaIetaIeta() > 0.02992) return false;
  if (fabs(patEl.deltaEtaSuperClusterTrackAtVtx()) > 0.004119) return false;
  if (fabs(patEl.deltaPhiSuperClusterTrackAtVtx()) > 0.05176) return false;
  if (patEl.hcalOverEcal() > 6.741) return false;
  if (patEl.pfIsolationVariables().sumChargedHadronPt / patEl.pt() > 2.5) return false;
  double Ooemoop = 999.;
  if (patEl.ecalEnergy() == 0) Ooemoop = 0.;
  else if (!std::isfinite(patEl.ecalEnergy())) Ooemoop = 998.;
  else Ooemoop = fabs(1./patEl.ecalEnergy() - patEl.eSuperClusterOverP()/patEl.ecalEnergy());
  if (Ooemoop > 73.76) return false;
  if (ConversionTools::hasMatchedConversion(patEl, conversions, beamspot.position())) return false;
  return true;
}

// ------------ method check that an e passes medium ID ----------------------------------
  bool
MiniFromPat::isMediumElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot) 
{
  if (fabs(patEl.superCluster()->eta()) > 1.479 && fabs(patEl.superCluster()->eta()) < 1.556) return false;
  if (patEl.full5x5_sigmaIetaIeta() > 0.01609) return false;
  if (fabs(patEl.deltaEtaSuperClusterTrackAtVtx()) > 0.001766) return false;
  if (fabs(patEl.deltaPhiSuperClusterTrackAtVtx()) > 0.03130) return false;
  if (patEl.hcalOverEcal() > 7.371) return false;
  if (patEl.pfIsolationVariables().sumChargedHadronPt / patEl.pt() > 1.325) return false;
  double Ooemoop = 999.;
  if (patEl.ecalEnergy() == 0) Ooemoop = 0.;
  else if (!std::isfinite(patEl.ecalEnergy())) Ooemoop = 998.;
  else Ooemoop = fabs(1./patEl.ecalEnergy() - patEl.eSuperClusterOverP()/patEl.ecalEnergy());
  if (Ooemoop > 22.6) return false;
  if (ConversionTools::hasMatchedConversion(patEl, conversions, beamspot.position())) return false;
  return true;
}

// ------------ method check that an e passes tight ID ----------------------------------
  bool
MiniFromPat::isTightElec(const pat::Electron & patEl, edm::Handle<reco::ConversionCollection> conversions, const reco::BeamSpot beamspot) 
{
  if (fabs(patEl.superCluster()->eta()) > 1.479 && fabs(patEl.superCluster()->eta()) < 1.556) return false;
  if (patEl.full5x5_sigmaIetaIeta() > 0.01614) return false;
  if (fabs(patEl.deltaEtaSuperClusterTrackAtVtx()) > 0.001322) return false;
  if (fabs(patEl.deltaPhiSuperClusterTrackAtVtx()) > 0.06129) return false;
  if (patEl.hcalOverEcal() > 4.492) return false;
  if (patEl.pfIsolationVariables().sumChargedHadronPt / patEl.pt() > 1.255) return false;
  double Ooemoop = 999.;
  if (patEl.ecalEnergy() == 0) Ooemoop = 0.;
  else if (!std::isfinite(patEl.ecalEnergy())) Ooemoop = 998.;
  else Ooemoop = fabs(1./patEl.ecalEnergy() - patEl.eSuperClusterOverP()/patEl.ecalEnergy());
  if (Ooemoop > 18.26) return false;
  if (ConversionTools::hasMatchedConversion(patEl, conversions, beamspot.position())) return false;
  return true;
}

// ------------ method to improve ME0 muon ID ----------------
  bool 
MiniFromPat::isME0MuonSel(reco::Muon muon, double pullXCut, double dXCut, double pullYCut, double dYCut, double dPhi)
{

  bool result = false;
  bool isME0 = muon.isME0Muon();

  if(isME0){

    double deltaX = 999;
    double deltaY = 999;
    double pullX = 999;
    double pullY = 999;
    double deltaPhi = 999;

    bool X_MatchFound = false, Y_MatchFound = false, Dir_MatchFound = false;

    const std::vector<reco::MuonChamberMatch>& chambers = muon.matches();
    for(std::vector<reco::MuonChamberMatch>::const_iterator chamber = chambers.begin(); chamber != chambers.end(); ++chamber){

      for (std::vector<reco::MuonSegmentMatch>::const_iterator segment = chamber->me0Matches.begin(); segment != chamber->me0Matches.end(); ++segment){

        if (chamber->detector() == 5){

          deltaX   = fabs(chamber->x - segment->x);
          deltaY   = fabs(chamber->y - segment->y);
          pullX    = fabs(chamber->x - segment->x) / std::sqrt(chamber->xErr + segment->xErr);
          pullY    = fabs(chamber->y - segment->y) / std::sqrt(chamber->yErr + segment->yErr);
          deltaPhi = fabs(atan(chamber->dXdZ) - atan(segment->dXdZ));

        }
      }
    }

    if ((pullX < pullXCut) || (deltaX < dXCut)) X_MatchFound = true;
    if ((pullY < pullYCut) || (deltaY < dYCut)) Y_MatchFound = true;
    if (deltaPhi < dPhi) Dir_MatchFound = true;

    result = X_MatchFound && Y_MatchFound && Dir_MatchFound;

  }

  return result;

}

// ------------ method called once each job just before starting event loop  ------------
  void 
MiniFromPat::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
  void 
MiniFromPat::endJob() 
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
MiniFromPat::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(MiniFromPat);
