// -*- C++ -*-
//
// Package:    PhaseTwoAnalysis/PatMuonFilter
// Class:      PatMuonFilter
// 
/**\class PatMuonFilter PatMuonFilter.cc PhaseTwoAnalysis/PatMuonFilter/plugins/PatMuonFilter.cc

Description: adds a vector of pat muons

Implementation:
- muon ID comes from https://twiki.cern.ch/twiki/bin/viewauth/CMS/UPGTrackerTDRStudies#Muon_identification
*/
//
// Original Author:  Elvire Bouvier
//         Created:  Sun, 02 Jul 2017 20:54:15 GMT
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "Geometry/GEMGeometry/interface/ME0EtaPartitionSpecs.h"
#include "Geometry/GEMGeometry/interface/ME0Geometry.h"

#include <vector>

//
// class declaration
//

class PatMuonFilter : public edm::stream::EDProducer<> {
    public:
        explicit PatMuonFilter(const edm::ParameterSet&);
        ~PatMuonFilter();

        static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

    private:
        virtual void beginStream(edm::StreamID) override;
        virtual void produce(edm::Event&, const edm::EventSetup&) override;
        virtual void endStream() override;

        bool isME0MuonSel(reco::Muon muon, double pullXCut, double dXCut, double pullYCut, double dYCut, double dPhi);
        bool isME0MuonSelNew(reco::Muon, double, double, double);

        virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
        //virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
        //virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
        //virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

        // ----------member data ---------------------------
        edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken_;
        edm::EDGetTokenT<std::vector<pat::Muon>> muonsToken_;
        const ME0Geometry* ME0Geometry_; 
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
PatMuonFilter::PatMuonFilter(const edm::ParameterSet& iConfig):
    verticesToken_(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("vertices"))),
    muonsToken_(consumes<std::vector<pat::Muon>>(iConfig.getParameter<edm::InputTag>("muons")))
{
    produces<std::vector<pat::Muon>>("LooseMuons");
    produces<std::vector<double>>("LooseMuonRelIso");
    produces<std::vector<pat::Muon>>("MediumMuons");
    produces<std::vector<double>>("MediumMuonRelIso");
    produces<std::vector<pat::Muon>>("TightMuons");
    produces<std::vector<double>>("TightMuonRelIso");

}

void
PatMuonFilter::beginRun(edm::Run const& iRun, edm::EventSetup const& iSetup)
{
  edm::ESHandle<ME0Geometry> hGeom;
  iSetup.get<MuonGeometryRecord>().get(hGeom);
  ME0Geometry_ =( &*hGeom);
}
    
PatMuonFilter::~PatMuonFilter()
{

    // do anything here that needs to be done at destruction time
    // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to produce the data  ------------
    void
PatMuonFilter::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    using namespace edm;
    Handle<std::vector<reco::Vertex>> vertices;
    iEvent.getByToken(verticesToken_, vertices);
    // Vertices
    int prVtx = -1;
    for (size_t i = 0; i < vertices->size(); i++) {
        if (vertices->at(i).isFake()) continue;
        if (vertices->at(i).ndof() <= 4) continue;
        if (prVtx < 0) prVtx = i;
    }

    Handle<std::vector<pat::Muon>> muons;
    iEvent.getByToken(muonsToken_, muons);
    std::unique_ptr<std::vector<pat::Muon>> filteredLooseMuons;
    std::unique_ptr<std::vector<double>> filteredLooseMuonRelIso;
    std::unique_ptr<std::vector<pat::Muon>> filteredMediumMuons;
    std::unique_ptr<std::vector<double>> filteredMediumMuonRelIso;
    std::unique_ptr<std::vector<pat::Muon>> filteredTightMuons;
    std::unique_ptr<std::vector<double>> filteredTightMuonRelIso;
    std::vector<pat::Muon> looseVec;
    std::vector<double> looseIsoVec;
    std::vector<pat::Muon> mediumVec;
    std::vector<double> mediumIsoVec;
    std::vector<pat::Muon> tightVec;
    std::vector<double> tightIsoVec;

    for (size_t i = 0; i < muons->size(); i++) {
      if (muons->at(i).pt() < 2.) continue;
      if (fabs(muons->at(i).eta()) > 3.) continue;

      auto priVertex = vertices->at(prVtx);
      auto muon = muons->at(i);
    
      bool isLoose = muon::isLooseMuon(muon);
      bool isMedium = muon::isMediumMuon(muon);
      bool isTight = (prVtx > -0.5 && muon::isTightMuon(muon,priVertex));

      double mom = muon.p();
      double dPhiCut_ = std::min(std::max(1.2/mom,1.2/100),0.056);
      double dPhiBendCut_ = std::min(std::max(0.2/mom,0.2/100),0.0096);
      bool isLooseME0 = isME0MuonSelNew(muon, 0.077, dPhiCut_, dPhiBendCut_);
      
      bool ipxy = false, ipz = false, validPxlHit = false, highPurity = false;
      if (muon.innerTrack().isNonnull()){
      	ipxy = std::abs(muon.muonBestTrack()->dxy(priVertex.position())) < 0.2;
      	ipz = std::abs(muon.muonBestTrack()->dz((priVertex.position()))) < 0.5;
      	validPxlHit = muon.innerTrack()->hitPattern().numberOfValidPixelHits() > 0;
      	highPurity = muon.innerTrack()->quality(reco::Track::highPurity);
      }
      // isMediumME0 - just loose with track requirements for now, this needs to be updated
      bool isMediumME0 = isME0MuonSelNew(muon, 0.077, dPhiCut_, dPhiBendCut_) && ipxy && validPxlHit && highPurity;

      // tighter cuts for tight ME0
      dPhiCut_ = std::min(std::max(1.2/mom,1.2/100),0.032);
      dPhiBendCut_ = std::min(std::max(0.2/mom,0.2/100),0.0041);
      bool isTightME0 = isME0MuonSelNew(muon, 0.048, dPhiCut_, dPhiBendCut_) && ipxy && ipz && validPxlHit && highPurity;
      
      double relIso = (muon.puppiNoLeptonsChargedHadronIso() + muon.puppiNoLeptonsNeutralHadronIso() + muon.puppiNoLeptonsPhotonIso()) / muon.pt();
      
      if (isLoose || (fabs(muon.eta()) > 2.4 && isLooseME0) ){
	looseVec.push_back(muon);
	looseIsoVec.push_back(relIso);
      }

      if (isMedium || (fabs(muon.eta()) > 2.4 && isMediumME0) ){
	mediumVec.push_back(muon);
	mediumIsoVec.push_back(relIso);
      }
    
      if (isTight || (fabs(muon.eta()) > 2.4 && isTightME0) ){
	tightVec.push_back(muon);
	tightIsoVec.push_back(relIso);
      }

    }

    filteredLooseMuons.reset(new std::vector<pat::Muon>(looseVec));
    filteredLooseMuonRelIso.reset(new std::vector<double>(looseIsoVec));
    filteredMediumMuons.reset(new std::vector<pat::Muon>(mediumVec));
    filteredMediumMuonRelIso.reset(new std::vector<double>(mediumIsoVec));
    filteredTightMuons.reset(new std::vector<pat::Muon>(tightVec));
    filteredTightMuonRelIso.reset(new std::vector<double>(tightIsoVec));

    iEvent.put(std::move(filteredLooseMuons), "LooseMuons");
    iEvent.put(std::move(filteredLooseMuonRelIso), "LooseMuonRelIso");
    iEvent.put(std::move(filteredMediumMuons), "MediumMuons");
    iEvent.put(std::move(filteredMediumMuonRelIso), "MediumMuonRelIso");
    iEvent.put(std::move(filteredTightMuons), "TightMuons");
    iEvent.put(std::move(filteredTightMuonRelIso), "TightMuonRelIso");

    return;
}

// ------------ method called once each stream before processing any runs, lumis or events  ------------
    void
PatMuonFilter::beginStream(edm::StreamID)
{
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void
PatMuonFilter::endStream() {
}

// ------------ method to improve ME0 muon ID ----------------
    bool 
PatMuonFilter::isME0MuonSel(reco::Muon muon, double pullXCut, double dXCut, double pullYCut, double dYCut, double dPhi)
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

bool PatMuonFilter::isME0MuonSelNew(reco::Muon muon, double dEtaCut, double dPhiCut, double dPhiBendCut)
{
    
    bool result = false;
    bool isME0 = muon.isME0Muon();
    
    if(isME0){
        
        double deltaEta = 999;
        double deltaPhi = 999;
        double deltaPhiBend = 999;

        const std::vector<reco::MuonChamberMatch>& chambers = muon.matches();
        for( std::vector<reco::MuonChamberMatch>::const_iterator chamber = chambers.begin(); chamber != chambers.end(); ++chamber ){
            
            if (chamber->detector() == 5){
            
                for ( std::vector<reco::MuonSegmentMatch>::const_iterator segment = chamber->me0Matches.begin(); segment != chamber->me0Matches.end(); ++segment ){

                    LocalPoint trk_loc_coord(chamber->x, chamber->y, 0);
                    LocalPoint seg_loc_coord(segment->x, segment->y, 0);
                    LocalVector trk_loc_vec(chamber->dXdZ, chamber->dYdZ, 1);
                    
                    const ME0Chamber * me0chamber = ME0Geometry_->chamber(chamber->id);
                    
                    GlobalPoint trk_glb_coord = me0chamber->toGlobal(trk_loc_coord);
                    GlobalPoint seg_glb_coord = me0chamber->toGlobal(seg_loc_coord);
                    
                    //double segDPhi = segment->me0SegmentRef->deltaPhi();
		    // need to check if this works
		    double segDPhi = fabs(atan(chamber->dXdZ) - atan(segment->dXdZ));

                    double trackDPhi = me0chamber->computeDeltaPhi(trk_loc_coord, trk_loc_vec);
                    
                    deltaEta = fabs(trk_glb_coord.eta() - seg_glb_coord.eta() );
                    deltaPhi = fabs(trk_glb_coord.phi() - seg_glb_coord.phi() );
                    deltaPhiBend = fabs(segDPhi - trackDPhi);
                    
                    if (deltaEta < dEtaCut && deltaPhi < dPhiCut && deltaPhiBend < dPhiBendCut) result = true;
                    
                }
            }
        }
        
    }
    
    return result;
    
}

// ------------ method called when starting to processes a run  ------------
/*
   void
   PatMuonFilter::beginRun(edm::Run const&, edm::EventSetup const&)
   {
   }
   */

// ------------ method called when ending the processing of a run  ------------
/*
   void
   PatMuonFilter::endRun(edm::Run const&, edm::EventSetup const&)
   {
   }
   */

// ------------ method called when starting to processes a luminosity block  ------------
/*
   void
   PatMuonFilter::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
   {
   }
   */

// ------------ method called when ending the processing of a luminosity block  ------------
/*
   void
   PatMuonFilter::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
   {
   }
   */

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
PatMuonFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    //The following says we do not know what parameters are allowed so do no validation
    // Please change this to state exactly what you do use, even if it is no parameters
    edm::ParameterSetDescription desc;
    desc.setUnknown();
    descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(PatMuonFilter);
