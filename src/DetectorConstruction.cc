/// description of stix instrument
// Author: Hualin Xiao(hualin.xiao@fhnw.ch)
// Date: Fri Jun 10, 2022
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <G4EllipticalTube.hh>
#include <G4GenericTrap.hh>
#include <G4Transform3D.hh>
#include <G4TwoVector.hh>
#include <vector>

#include <G4AssemblyVolume.hh>
#include <G4Box.hh>
#include <G4Cons.hh>
#include <G4Element.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4PVPlacement.hh>
#include <G4PVReplica.hh>
#include <G4Polycone.hh>
#include <G4Polyhedra.hh>
#include <G4RotationMatrix.hh>
#include <G4SubtractionSolid.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>

#include <G4Colour.hh>
#include <G4NistManager.hh>
#include <G4Transform3D.hh>
#include <G4VisAttributes.hh>

#include <G4LogicalVolumeStore.hh>
#include <G4RunManager.hh>
#include <G4SolidStore.hh>
#include <G4UImanager.hh>

#include <G4GDMLParser.hh>

#include "AnalysisManager.hh"
#include "G4ExtrudedSolid.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "GridParameters.hh"
#include <TFile.h>
#include <TTree.h>
const G4double pi = CLHEP::pi;
const G4ThreeVector singleDetectorPosition(0, 0, -0.8 * mm);

DetectorConstruction::DetectorConstruction() {
  // for (int i = 0; i < 32; i++)G4cout <<Grid::getDetectorCenterCoordsCAD(i) <<
  // G4endl;
  fWorldFile = "";
  gridsEnabled = true;
  activatedDetectorFlag = 100;
  // all detectors will be constructed  if it is not between 0 -- 31

  G4RotationMatrix rotY;
  rotY.rotateY(-90 * deg);
  G4RotationMatrix rotX;
  rotX.rotateX(90 * deg);
  rotMatrix = rotX * rotY;
  ;
  detMsg = new DetectorMessenger(this);
}
void DetectorConstruction::ConstructSpacecraft() {
  // definition moved to gdml
  /*
     G4double scWidth= 2.5 *m;
     G4double scHeight= 2.7 *m;
     G4double scLength= 3.1*m;
  //taken from wikipedia
  G4double scWallThickness=1*mm;

  G4Box *spacecraftBox= new G4Box("spacecaft",
  scLength/2,
  scHeight/2,
  scWidth/2
  );//in stix coordinate frame
    //stix_x
    //
    G4Box *spacecraftBoxInner= new G4Box("spacecaft",
    scLength/2,
    scHeight/2-1,
    scWidth/2-1
    );//in stix coordinate frame
          //

          G4SubtractionSolid *spacecraftHollowBox= new
          G4SubtractionSolid("SpaceLab", SL_123, SL3hole_cons); G4LogicalVolume
   *spacecraftLog=new G4LogicalVolume(spacecraftBox, Alum, "spacecraftLog", 0,
  0, 0); G4Box *spacecraftBox= new G4Box("spacecaft", scLength/2, scHeight/2,
   scWidth/2
   );//in stix coordinate frame

*/
}

void DetectorConstruction::ConstructGrids() {

  if (!gridsEnabled)
    return;
  // don't construct if disabled by macro

  // created by grid_data_creator.py in
  // /home/xiaohl/FHNW/STIX/SolarFlareAnalysis/stix_simulator/pySimulator/export_grid_data.inpy
  G4String fname = "./grid_data/stix_grid_parameters.root";
  G4cout << "Loading grid parameters from ROOT file: " << fname << G4endl;
  TFile f(fname);
  TTree *grids = (TTree *)f.Get("grids");
  Int_t is_front;
  Int_t det_idx;
  Int_t i_strip;
  Float_t polygon_x[5];
  Float_t polygon_y[5];
  Int_t is_nominal_parameters;

  // Set branch addresses.
  grids->SetBranchAddress("is_front", &is_front);
  grids->SetBranchAddress("det_idx", &det_idx);
  grids->SetBranchAddress("i_strip", &i_strip);
  grids->SetBranchAddress("polygon_x", polygon_x);
  grids->SetBranchAddress("polygon_y", polygon_y);
  grids->SetBranchAddress("is_nominal_parameters", &is_nominal_parameters);

  Long64_t nentries = grids->GetEntries();

  G4ThreeVector pos;

  G4bool nominalOrRealFlag[32] = {0};
  G4Box *frontGridBox =
      new G4Box("frontGridBox", 11.05 * mm, 10.05 * mm, 0.205 * mm);
  G4Box *rearGridBox =
      new G4Box("rearGridBox", 6.505 * mm, 6.505 * mm, 0.205 * mm);
  // dummy boxes
  // added extra 50 um to avoid overlapping

  G4LogicalVolume *frontGridContainerLog[32];
  G4LogicalVolume *rearGridContainerLog[32];
  for (int i = 0; i < 32; i++) {
    frontGridContainerLog[i] = new G4LogicalVolume(
        frontGridBox, Vacuum, "frontGridContainer", 0, 0, 0);
    rearGridContainerLog[i] =
        new G4LogicalVolume(rearGridBox, Vacuum, "rearGridContainer", 0, 0, 0);
  }

  for (int i = 0; i < nentries; i++) {
    grids->GetEntry(i);
    nominalOrRealFlag[det_idx] = is_nominal_parameters;
    std::vector<G4TwoVector> vertexCoords;
    for (int j = 0; j < 4; j++) {
      vertexCoords.push_back(G4TwoVector(polygon_x[j] * mm, polygon_y[j] * mm));
    }
    // Left wedge Solid and logical Volume
    G4ExtrudedSolid *strip =
        new G4ExtrudedSolid("strip", vertexCoords,
                            0.4 * 0.5 * mm, // 0.4 mm thick, halfz
                            G4TwoVector(), 1, G4TwoVector(), 1);
    G4LogicalVolume *gridStripLog =
        new G4LogicalVolume(strip, Tungsten, "gridStripLog", 0, 0, 0);
    G4cout << "creating grids:" << i << " " << pos << G4endl;
    if (is_front == 1) {
      new G4PVPlacement(0, G4ThreeVector(), gridStripLog, "gridStrip",
                        frontGridContainerLog[det_idx], false, i, false);
    } else {
      new G4PVPlacement(0, G4ThreeVector(), gridStripLog, "gridStrip",
                        rearGridContainerLog[det_idx], false, i, false);
    }
  }
  for (int i = 0; i < 32; i++) {
    if (i == 8 || i == 9)
      continue;
    // don't construct CFL and BKG
    G4String gtype = nominalOrRealFlag[i] == 1 ? "NOMINAL" : "REAL";
    G4cout << "Grid #" << i << " parameter type: " << gtype << G4endl;
    bool singleDetector = false;
    if (activatedDetectorFlag >= 0 && activatedDetectorFlag < 32) {
      if (i != activatedDetectorFlag)
        continue;
      singleDetector = true;
    }

    pos = singleDetector ? G4ThreeVector(-21 * mm, 0, 0)
                         : Grid::getGridCenterCAD(i, 1);
    new G4PVPlacement(G4Transform3D(rotMatrix, pos), frontGridContainerLog[i],
                      "frontGrid", worldLogical, false, i, false);

    pos = singleDetector ? G4ThreeVector(-19 * mm, 0, 0)
                         : Grid::getGridCenterCAD(i, 0);
    new G4PVPlacement(G4Transform3D(rotMatrix, pos), rearGridContainerLog[i],
                      "rearGrid", worldLogical, false, i, false);
  }

  // fluorescence test

  G4cout << "Grid construction finished." << G4endl;
  f.Close();
}

G4LogicalVolume *DetectorConstruction::ConstructCdTeDetector() {
  G4double bigW = 2.15 * mm;
  G4double bigH = 4.55 * mm;
  G4double bigSW = 1.1 * mm;
  G4double bigSH = 0.455 * mm;
  G4double smallW = 1.05 * mm;
  G4double smallH = 0.86 * mm;
  G4double pixel0CenterX = -3.3 * mm;
  G4double pixel0CenterY = 2.3 * mm;
  G4double pixel4CenterX = -3.3 * mm;
  G4double pixel4CenterY = -2.3 * mm;
  G4double deltaW = 2.2 * mm;
  G4double pixel8CenterX = -3.85 * mm;
  G4double cdTeCalisteOffset =
      0.8 * mm; //  (top margion) 0.2 + 10 + 1.8 (Bottom margin)  = 12
  G4double pixel8CenterY = 0 * mm;
  G4double cdteThickness = 1 * mm;
  G4double anodeThickness = (15 + 15 + 100) * 1e-9 * m; // 130 nm
  G4double cathodeThickness =
      15 * 1e-9 *
      m; // 15 nm
         // thickness negligible  compared to the thickness  uncertainty

  bool checkOverlaps = true;
  // caliste
  G4double calisteWidth = 11 * mm;
  G4double calisteLength = 12 * mm;
  G4double calisteBaseHeight = 14.4 * mm;

  G4double padHeight = 0.2 * mm;  // pads underneath the  CdTe
  G4double pinsHeight = 1.2 * mm; //

  G4double calisteTotalHight = calisteBaseHeight + cdteThickness +
                               anodeThickness + cathodeThickness + padHeight +
                               pinsHeight;

  G4Box *calisteWorld = new G4Box("CdTeModuleWorld", calisteWidth / 2,
                                  calisteLength / 2, calisteTotalHight / 2);

  ////
  ///// calisteResin base with different coating layers

  G4Box *calisteBaseOuter = new G4Box("CdTeModuleBaseOuter", calisteWidth / 2,
                                      calisteLength / 2, calisteBaseHeight / 2);

  G4double coatingThickness[4] = {2e-3 * mm, 2.5e-3 * mm, 20e-3 * mm,
                                  2e-3 * mm};
  G4Material *coatingMateris[4] = {Nickle, Copper, Nickle, Resin};
  // Au+Ni+Cu+2u
  // different layer of the coating material is  considered as mixture

  G4Box *calisteBaseInner[4];
  G4String calisteBaseInnerLayerName[4] = {"nickle", "copper", "nickle",
                                           "resin"};

  G4LogicalVolume *calisteBaseInnerLog[4];
  G4double totalCoatingThickness = 0;
  G4LogicalVolume *calisteBaseOuterLog =
      new G4LogicalVolume(calisteBaseOuter, Gold, "calisteBaseOuter", 0, 0, 0);

G4LogicalVolume  *coatingMotherLog;

  for (int i = 0; i < 4; i++) {
    totalCoatingThickness += coatingThickness[i];
    calisteBaseInner[i] = new G4Box(
        G4String("CdTeModuleBaseInner_") + calisteBaseInnerLayerName[i],
        calisteWidth / 2 - totalCoatingThickness,
        calisteLength / 2 - totalCoatingThickness, calisteBaseHeight / 2);
    calisteBaseInnerLog[i] = new G4LogicalVolume(
        calisteBaseInner[i], coatingMateris[i],
        G4String("log_CalisteBaseInner_") + calisteBaseInnerLayerName[i], 0, 0,
        0);
	if(i==0)coatingMotherLog=calisteBaseOuterLog;
	else{
		coatingMotherLog=calisteBaseInnerLog[i-1];
	}

    new G4PVPlacement(0, G4ThreeVector(0, 0, 0), calisteBaseInnerLog[i],
                      calisteBaseInnerLayerName[i] + "_phys",
                      coatingMotherLog, false, 0, checkOverlaps);
  }

  // caliste base material, is unknown
  // can be considered filled resin

  /////construct CdTe and pixels
  G4Box *CdTeBox = new G4Box("CdTeDet", 5 * mm, 5 * mm, cdteThickness / 2.);
  G4Box *CdTeAnodeBox =
      new G4Box("CdTeAnode", 5 * mm, 5 * mm, anodeThickness / 2.);
  G4Box *CdTeCathodeBox =
      new G4Box("CdTeCathode", 5 * mm, 5 * mm, cathodeThickness / 2.);

  // Top and Small pixels
  std::vector<G4TwoVector> vertexCoordsTop;
  vertexCoordsTop.push_back(G4TwoVector(-bigW / 2, bigH / 2));
  vertexCoordsTop.push_back(G4TwoVector(bigW / 2, bigH / 2));
  vertexCoordsTop.push_back(G4TwoVector(bigW / 2, -bigH / 2));
  vertexCoordsTop.push_back(G4TwoVector(-bigW / 2 + bigSW, -bigH / 2));
  vertexCoordsTop.push_back(G4TwoVector(-bigW / 2 + bigSW, -bigH / 2 + bigSH));
  vertexCoordsTop.push_back(G4TwoVector(-bigW / 2, -bigH / 2 + bigSH));

  G4ExtrudedSolid *bigPixelTopGeo =
      new G4ExtrudedSolid("topBigPixel", vertexCoordsTop, cdteThickness / 2.,
                          G4TwoVector(), 1, G4TwoVector(), 1);
  std::vector<G4TwoVector> vertexCoordsBott;
  vertexCoordsBott.push_back(G4TwoVector(-bigW / 2, bigH / 2 - bigSH));
  vertexCoordsBott.push_back(G4TwoVector(-bigW / 2 + bigSW, bigH / 2 - bigSH));
  vertexCoordsBott.push_back(G4TwoVector(-bigW / 2 + bigSW, bigH / 2));
  vertexCoordsBott.push_back(G4TwoVector(bigW / 2, bigH / 2));
  vertexCoordsBott.push_back(G4TwoVector(bigW / 2, -bigH / 2));
  vertexCoordsBott.push_back(G4TwoVector(-bigW / 2, -bigH / 2));

  G4ExtrudedSolid *bigPixelBottomGeo =
      new G4ExtrudedSolid("bottBigPixel", vertexCoordsBott, cdteThickness / 2.,
                          G4TwoVector(), 1, G4TwoVector(), 1);
  G4Box *smallPixelGeo =
      new G4Box("smallPixel", smallW / 2, smallH / 2, cdteThickness / 2.);
  G4LogicalVolume *calisteLog =
      new G4LogicalVolume(calisteWorld, Vacuum, "calisteWorld", 0, 0, 0);
  // CdTe detector module world
  G4LogicalVolume *CdTeDetLog =
      new G4LogicalVolume(CdTeBox, CdTe, "CdTeDetLog", 0, 0, 0);
  G4LogicalVolume *CdTeAnodeLog = new G4LogicalVolume(
      CdTeAnodeBox, goldLayerMaterial, "CdTeAnodeLog", 0, 0, 0);
  G4LogicalVolume *CdTeCathodeLog =
      new G4LogicalVolume(CdTeCathodeBox, Platinum, "CdTeCathodeLog", 0, 0, 0);
  G4LogicalVolume *bigPixelTopLog =
      new G4LogicalVolume(bigPixelTopGeo, CdTe, "bigPixelTopLog", 0, 0, 0);
  G4LogicalVolume *bigPixelBottomLog = new G4LogicalVolume(
      bigPixelBottomGeo, CdTe, "bigPixelBottomLog", 0, 0, 0);
  G4LogicalVolume *smallPixelLog =
      new G4LogicalVolume(smallPixelGeo, CdTe, "smallPixelLog", 0, 0, 0);
  G4int copyNb;
  G4double detZ = 0;
  // place electrode
  G4String name = "pixel";
  for (int i = 0; i < 4; i++) {
    // it becomes pixel4 after rotation
    copyNb = i;
    G4ThreeVector posBigPixelTop(pixel0CenterX + deltaW * i, pixel0CenterY,
                                 detZ);
    new G4PVPlacement(0, posBigPixelTop, bigPixelTopLog, name, CdTeDetLog,
                      false, copyNb, checkOverlaps);

    // it becomes pixel4 after rotation
    copyNb = i + 4;
    G4ThreeVector posBigPixelBottom(pixel4CenterX + deltaW * i, pixel4CenterY,
                                    detZ);
    new G4PVPlacement(0, posBigPixelBottom, bigPixelBottomLog, name, CdTeDetLog,
                      false, copyNb, checkOverlaps);

    // small pixels
    copyNb = i + 8;
    G4ThreeVector posSmallPixel(pixel8CenterX + deltaW * i, pixel8CenterY,
                                detZ);

    new G4PVPlacement(0, posSmallPixel, smallPixelLog, name, CdTeDetLog, false,
                      copyNb, checkOverlaps);
  }

  //////construct pads
  //////
  G4Tubs *roundPadGeo =
      new G4Tubs("roundPadGeo", 0, 0.3 * mm, 0.1 * mm, 0, 360. * deg);
  // 0.6 mm and 0.2 mm high
  G4LogicalVolume *roundPadLog =
      new G4LogicalVolume(roundPadGeo, SilverEpoxy, "roundPadLog", 0, 0, 0);

  G4EllipticalTube *ellipticalPadGeo =
      new G4EllipticalTube("elliptical", 0.3, 0.4 * mm, 0.1 * mm);
  G4LogicalVolume *ellipticalPadLog = new G4LogicalVolume(
      ellipticalPadGeo, SilverEpoxy, "ellipticalPadLog", 0, 0, 0);
  G4Box *rectPadGeo =
      new G4Box("rectPadGeo", 0.3 * mm, 4.45 / 2 * mm, 0.1 * mm);
  G4LogicalVolume *rectPadLog =
      new G4LogicalVolume(rectPadGeo, SilverEpoxy, "rectPadLog", 0, 0, 0);

  G4double roundPadX[] = {
      -3.744, -2.832, -1.541, -0.646, 0.714,  1.489,  2.849,  3.744,  -3.727,
      -2.832, -1.541, -0.628, 0.680,  1.558,  2.832,  3.744,  -3.744, -2.849,
      -1.523, -0.646, 0.646,  1.541,  2.832,  3.727,  -3.865, 2.746,  -3.710,
      -2.832, -1.558, -0.628, 1.541,  2.832,  3.744,  3.727,  2.815,  1.523,
      0.611,  -0.663, -1.523, -2.832, -3.710, 3.744,  2.849,  1.523,  0.628,
      -0.663, -1.523, -2.832, -3.744, 0.559,  -1.627, 0.628};
  G4double roundPadY[] = {
      -4.678, -4.678, -4.695, -4.678, -4.678, -4.695, -4.644, -4.661, -3.768,
      -3.768, -3.785, -3.768, -3.768, -3.751, -3.768, -3.768, -2.858, -2.876,
      -2.910, -2.893, -2.893, -2.876, -2.876, -2.893, -0.747, -0.798, 1.296,
      1.313,  1.313,  1.313,  1.348,  1.348,  1.365,  2.258,  2.240,  2.240,
      2.258,  2.223,  2.240,  2.240,  2.240,  3.150,  3.150,  3.150,  3.150,
      3.133,  3.116,  3.133,  3.133,  -0.764, -0.764, 1.365};
  G4double ellipitalPadX[] = {-2.746, -2.763, -0.525, -0.542,
                              1.644,  1.644,  3.830,  3.847};
  G4double ellipitalPadY[] = {-1.433, -0.060, -1.451, -0.077,
                              -1.451, -0.060, -1.433, -0.077};
  G4double rectPadX[]={4.71,-4.75};
  G4double rectPadY[]={-0.755, -0.755};
  //positions extracted from the graph and verified using pyplot
  G4double padZ=calisteTotalHight / 2 - cathodeThickness -
                                      cdteThickness - anodeThickness - padHeight/2.; 

  //placing pads
  //
  for(int i=0;i<52;i++){
	  new G4PVPlacement(0,       G4ThreeVector(roundPadX[i]*mm, -roundPadY[i]*mm, padZ),
                    roundPadLog, "roundPadPhys", calisteLog, false, i,
                    checkOverlaps);
					//reverse Y needed
  }
  for(int i=0;i<8;i++){
	  new G4PVPlacement(0,       G4ThreeVector(ellipitalPadX[i]*mm, -ellipitalPadY[i]*mm, padZ),
                    ellipticalPadLog, "ellipticalPadPhys", calisteLog, false, i,
                    checkOverlaps);
  }
  for(int i=0;i<2;i++){
	  new G4PVPlacement(0,       G4ThreeVector(rectPadX[i]*mm, -rectPadY[i]*mm, padZ),
                    rectPadLog, "rectPadPhys", calisteLog, false, i,
                    checkOverlaps);
  }

  //y reversed

  // see email from olivier, sent on Feb. 09 2023: CAD model Caliste-SO

  // place all logical volume into CdTe

  new G4PVPlacement(0,
                    G4ThreeVector(0, cdTeCalisteOffset,
                                  calisteTotalHight / 2 - cathodeThickness / 2),
                    CdTeCathodeLog, "cathode", calisteLog, false, 0, true);
  new G4PVPlacement(0,
                    G4ThreeVector(0, cdTeCalisteOffset,
                                  calisteTotalHight / 2 - cathodeThickness -
                                      cdteThickness / 2),
                    CdTeDetLog, "CdTeDetLog", calisteLog, false, 0,
                    checkOverlaps);

  new G4PVPlacement(0,
                    G4ThreeVector(0, cdTeCalisteOffset,
                                  calisteTotalHight / 2 - cathodeThickness -
                                      cdteThickness - anodeThickness / 2),
                    CdTeAnodeLog, "anode", calisteLog, false, 0, true);
  /// end of CdTe detector

  new G4PVPlacement(
      0,
      G4ThreeVector(0, 0,
                    calisteTotalHight / 2 - cathodeThickness - anodeThickness -
                        padHeight - cdteThickness - calisteBaseHeight / 2),
      calisteBaseOuterLog, "calisteOuter", calisteLog, false, 0, checkOverlaps);

  return calisteLog;
}

DetectorConstruction::~DetectorConstruction() { delete detMsg; }

G4VPhysicalVolume *DetectorConstruction::Construct() {

  // AnalysisManager *analysisManager = AnalysisManager::GetInstance();
  // AnalysisManager->SetAttenuatorStatus(attenuatorIn);

  G4NistManager *nist_manager = G4NistManager::Instance();
  Alum = nist_manager->FindOrBuildMaterial("G4_Al");
  Vacuum = nist_manager->FindOrBuildMaterial("G4_Galactic");
  Tungsten = nist_manager->FindOrBuildMaterial("G4_W");
  Platinum = nist_manager->FindOrBuildMaterial("G4_Pt");
  Copper = nist_manager->FindOrBuildMaterial("G4_Cu");

  Siliver = nist_manager->FindOrBuildMaterial("G4_Ag");
  Gold = nist_manager->FindOrBuildMaterial("G4_Au");
  Nickle = nist_manager->FindOrBuildMaterial("G4_Ni");

  G4Element *elAl = nist_manager->FindOrBuildElement("Al");
  G4Element *elH = nist_manager->FindOrBuildElement("H");
  G4Element *elC = nist_manager->FindOrBuildElement("C");
  G4Element *elTi = nist_manager->FindOrBuildElement("Ti");
  G4Element *elCd = nist_manager->FindOrBuildElement("Cd");
  G4Element *elTe = nist_manager->FindOrBuildElement("Te");
  G4Element *elAu = nist_manager->FindOrBuildElement("Au");
  G4Element *elAg = nist_manager->FindOrBuildElement("Ag");

  G4Element *elCr = nist_manager->FindOrBuildElement("Cr");
  G4Element *elZn = nist_manager->FindOrBuildElement("Zn");
  G4Element *elMn = nist_manager->FindOrBuildElement("Mn");
  G4Element *elCu = nist_manager->FindOrBuildElement("Cu");
  G4Element *elFe = nist_manager->FindOrBuildElement("Fe");
  G4Element *elO = nist_manager->FindOrBuildElement("O");
  G4Element *elSi = nist_manager->FindOrBuildElement("Si");
  G4Element *elMg = nist_manager->FindOrBuildElement("Mg");

  G4double density = 5.85 * g / cm3; // STIX-DS-0017-PSI
  G4int nelements, natoms;
  G4double fractionmass;
  CdTe = new G4Material("CdTe", density, nelements = 2);
  CdTe->AddElement(elCd, natoms = 1);
  CdTe->AddElement(elTe, natoms = 1);

  goldLayerMaterial =
      new G4Material("goldLayerMaterial", density, nelements = 3);
  goldLayerMaterial->AddElement(elAu, 0.94700687); // mass fraction
  goldLayerMaterial->AddElement(elTi, 0.033120707);
  goldLayerMaterial->AddElement(elAl, 0.019872424);

  SiO2 = new G4Material("SiO2", density = 2.2 * g / cm3, nelements = 2);
  SiO2->AddElement(elSi, 1);
  SiO2->AddElement(elO, 2);

  Alum7075 = new G4Material("Alum7075", density = 2.8 * g / cm3, nelements = 7);
  Alum7075->AddElement(elAl, 0.876);
  Alum7075->AddElement(elZn, 0.056);
  Alum7075->AddElement(elCr, 0.023);
  Alum7075->AddElement(elMg, 0.025);
  Alum7075->AddElement(elCu, 0.016);
  Alum7075->AddElement(elFe, 0.0025);
  Alum7075->AddElement(elMn, 0.0015);

  density = 1.2 * g / cm3;

  Epoxy = new G4Material("Epoxy", 1.2 * g / cm3, nelements = 2);
  Epoxy->AddElement(elH, natoms = 2);
  Epoxy->AddElement(elC, natoms = 2);

  density = 1.86 * g / cm3;
  FR4 = new G4Material("FR4", density, nelements = 2);
  FR4->AddMaterial(SiO2, 0.528);
  FR4->AddMaterial(Epoxy, 0.472);

  // 2/ Molding resin
  // Use the following mixture
  //+ density of the mixture 1.77
  //+ 73% resin (C2H2)
  //+ 27% SiO2

  density = 1.77 * g / cm3;
  Resin = new G4Material("Resin", density, nelements = 2);
  Resin->AddMaterial(SiO2, fractionmass = 0.27);
  Resin->AddMaterial(Epoxy, fractionmass = 0.73);

  SilverEpoxy =
      new G4Material("SilverEpoxy", density = 3.3 * g / cm3, nelements = 2);
  SilverEpoxy->AddMaterial(Epoxy, fractionmass = 0.7);
  SilverEpoxy->AddMaterial(Siliver, fractionmass = 0.3);

  // MetalCoating = new G4Material("MetalCoating", density = 9 * g / cm3,
  // nelements = 3); MetalCoating->AddMaterial(Nickle, fractionmass = 0.155);
  // MetalCoating->AddMaterial(Gold, fractionmass = 0.15);
  // MetalCoating->AddMaterial(Copper, fractionmass = 0.695);

  LeadPadMat =
      new G4Material("LeadPadMat", density = 11 * g / cm3, nelements = 2);
  LeadPadMat->AddMaterial(Nickle, fractionmass = 0.587);
  LeadPadMat->AddMaterial(Gold, fractionmass = 0.413);

  // construct world
  G4GDMLParser parser;
  if (fWorldFile != "") {

    G4cout << "==== === Loading mass model from gdml files " << fWorldFile
           << "..." << G4endl;
    parser.Read("gdml/" + fWorldFile + ".gdml");
    worldPhysical = parser.GetWorldVolume();
    worldLogical = worldPhysical->GetLogicalVolume();
  } else {
    G4cout << "## CAD models will not be imported!" << G4endl;

    G4Box *worldSolid = new G4Box("worldSolid", 500 * cm, 500 * cm, 500 * cm);
    worldLogical =
        new G4LogicalVolume(worldSolid, Vacuum, "worldLogical", 0, 0, 0);
    worldPhysical = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), worldLogical,
                                      "worldPhysical", 0, false, 0);
  }

  // construct world
  G4cout << "Constructing Caliste..." << G4endl;
  G4LogicalVolume *CalisteLog = ConstructCdTeDetector();

  G4cout << "Placing detector Caliste..." << G4endl;

  for (int i = 0; i < 32; i++) {
    // placing detector
    G4ThreeVector pos = Grid::getCalisteCenterCoordsCAD(i);
    if (activatedDetectorFlag >= 0 && activatedDetectorFlag < 32) {
      // single detector only, for testing
      if (i != activatedDetectorFlag)
        continue;
      else {
        pos.setY(0);
        pos.setZ(-0.8 * mm);
        // move the detector to the center
      }
    }
    G4cout << ">> detector:  " << i << "  , position: " << pos << G4endl;
    new G4PVPlacement(G4Transform3D(rotMatrix, pos), CalisteLog, "Caliste",
                      worldLogical, false, i, true);
    // break;
  }

  G4cout << "World construction completed" << G4endl;

  // X-ray window

  ConstructCFL();
  ConstructBKG();

  ConstructGrids();

  SetVisColors();

  // ConstructSpaceCraft();

  CalisteLog->SetVisAttributes(G4VisAttributes::Invisible);
  worldLogical->SetVisAttributes(G4VisAttributes::Invisible);
  return worldPhysical;
}

void DetectorConstruction::ConstructCFL() {

  G4ThreeVector pos = Grid::getGridCenterCAD(8, 1);
  if (activatedDetectorFlag >= 0 && activatedDetectorFlag < 32) {
    // construct single detector
    if (activatedDetectorFlag != 8)
      return;
    else {
      pos.setY(0);
      pos.setZ(0);
      // move it the center, keep the distance
      // for testing only
    }
  }

  // construct CFL
  G4Box *CFLBox = new G4Box("CFLBox", 12 * mm, 11 * mm, 0.2 * mm);
  G4LogicalVolume *CFLLog =
      new G4LogicalVolume(CFLBox, Tungsten, "CFLLog", 0, 0, 0);

  G4Box *CFLBigHole =
      new G4Box("CFLBigHole", 8.8 * mm / 2, 6.55 * mm / 2, 0.2 * mm);
  G4LogicalVolume *CFLBigHoleLog =
      new G4LogicalVolume(CFLBigHole, Vacuum, "CFLBigHoleLog", 0, 0, 0);

  G4Box *CFLSmallHole =
      new G4Box("CFLSmallHole", 1.1 * mm / 2, 6.55 * mm / 2, 0.2 * mm);
  G4LogicalVolume *CFLSmallHoleLog =
      new G4LogicalVolume(CFLSmallHole, Vacuum, "CFLSmallHoleLog", 0, 0, 0);

  new G4PVPlacement(0, G4ThreeVector(0, 6.725 * mm, 0.2 * mm), CFLBigHoleLog,
                    "CFL_BIG_TOP_HOLE", CFLLog, false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(0, -6.725 * mm, 0.2 * mm), CFLBigHoleLog,
                    "CFL_BIG_BOTTOM_HOLE", CFLLog, false, 0, false);

  new G4PVPlacement(0, G4ThreeVector(-10.45 * mm, 6.725 * mm, 0.2 * mm),
                    CFLSmallHoleLog, "CFL_SMALL_TOP_LFET_HOLE", CFLLog, false,
                    0, false);
  new G4PVPlacement(0, G4ThreeVector(-10.45 * mm, -6.725 * mm, 0.2 * mm),
                    CFLSmallHoleLog, "CFL_SMALL_BOTTOM_LFET_HOLE", CFLLog,
                    false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(10.45 * mm, -6.725 * mm, 0.2 * mm),
                    CFLSmallHoleLog, "CFL_SMALL_BOTTOM_RIGHT_HOLE", CFLLog,
                    false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(10.45 * mm, 6.725 * mm, 0.2 * mm),
                    CFLSmallHoleLog, "CFL_SMALL_TOP_RIGHT", CFLLog, false, 0,
                    false);

  new G4PVPlacement(G4Transform3D(rotMatrix, pos), CFLLog, "CFLLogical",
                    worldLogical, false, 0, false);
}

void DetectorConstruction::ConstructBKG() {

  G4ThreeVector pos = Grid::getGridCenterCAD(9, 0);

  if (activatedDetectorFlag >= 0 && activatedDetectorFlag < 32) {
    // construct single detector
    if (activatedDetectorFlag != 9)
      return;
    else {
      pos.setY(0);
      pos.setZ(0);
      // center collimator
      // for testing only
    }
  }

  G4Box *BKGBox = new G4Box("BKGBox", 12 * mm, 11 * mm, 0.2 * mm);
  G4LogicalVolume *BKGLog =
      new G4LogicalVolume(BKGBox, Tungsten, "BKGLog", 0, 0, 0);

  G4Box *BKGBigRectHole =
      new G4Box("BKGBigRectHole", 0.5 * mm / 2, 2 * mm / 2, 0.2 * mm);
  G4LogicalVolume *BKGBigRectHoleLog =
      new G4LogicalVolume(BKGBigRectHole, Vacuum, "BKGBigHoleLog", 0, 0, 0);

  G4Tubs *BKGBigRoundHole =
      new G4Tubs("BKGBigRoundHole", 0, 0.1784 * mm, 0.2 * mm, 0, 360. * deg);
  G4Tubs *BKGSmallRoundHole =
      new G4Tubs("BKGBigSmallHole", 0, 0.0564 * mm, 0.2 * mm, 0, 360 * deg);

  G4LogicalVolume *BKGBigRoundHoleLog = new G4LogicalVolume(
      BKGBigRoundHole, Vacuum, "BKGBigRoundHoleLog", 0, 0, 0);
  G4LogicalVolume *BKGSmallRoundHoleLog = new G4LogicalVolume(
      BKGSmallRoundHole, Vacuum, "BKGSmallRoundHoleLog", 0, 0, 0);

  // estimated by looking at the picture in the STIX instrument paper, area,
  // 0.01, 0.1 and 1 mm

  new G4PVPlacement(0, G4ThreeVector(-3.5 * mm, 3 * mm, 0.2 * mm),
                    BKGBigRectHoleLog, "PIXEL_0_HOLE", BKGLog, false, 0, false);
  new G4PVPlacement(0, G4ThreeVector(-1.5 * mm, 3 * mm, 0.2 * mm),
                    BKGSmallRoundHoleLog, "PIXEL_1_HOLE", BKGLog, false, 0,
                    false);
  new G4PVPlacement(0, G4ThreeVector(1.5 * mm, 3 * mm, 0.2 * mm),
                    BKGBigRoundHoleLog, "PIXEL_2_HOLE", BKGLog, false, 0,
                    false);
  new G4PVPlacement(0, G4ThreeVector(-1.5 * mm, -3 * mm, 0.2 * mm),
                    BKGBigRoundHoleLog, "PIXEL_6_HOLE", BKGLog, false, 0,
                    false);
  new G4PVPlacement(0, G4ThreeVector(1.5 * mm, -3 * mm, 0.2 * mm),
                    BKGSmallRoundHoleLog, "PIXEL_7_HOLE", BKGLog, false, 0,
                    false);
  new G4PVPlacement(0, G4ThreeVector(3.5 * mm, -3 * mm, 0.2 * mm),
                    BKGBigRectHoleLog, "PIXEL_8_HOLE", BKGLog, false, 0, false);
  // estimated by looking at the picture in the STIX instrument paper

  new G4PVPlacement(G4Transform3D(rotMatrix, pos), BKGLog, "BKG", worldLogical,
                    false, 0, false);
}

void DetectorConstruction::SetVisAttrib(G4LogicalVolume *log, G4double red,
                                        G4double green, G4double blue,
                                        G4double alpha, G4bool wireFrame,
                                        G4bool solid) {
  G4VisAttributes *visAttrib =
      new G4VisAttributes(G4Colour(red, green, blue, alpha));
  visAttrib->SetForceWireframe(wireFrame);
  visAttrib->SetForceSolid(solid);
  log->SetVisAttributes(visAttrib);
}
void DetectorConstruction::SetVisAttrib(G4LogicalVolume *log, G4double red,
                                        G4double green, G4double blue,
                                        G4double alpha) {
  SetVisAttrib(log, red, green, blue, alpha, true, true);
}

void DetectorConstruction::SetAttenuatorStatus(G4bool att) {
  // change attenuator status
  if (!att) {
    fWorldFile = "WorldAttOut";
    attenuatorIn = false;
    G4cout << "Attenuator is out" << G4endl;
  } else {
    fWorldFile = "WorldAttIn";
    attenuatorIn = true;
    G4cout << "Attenuator is inserted!" << G4endl;
  }
}

void DetectorConstruction::SetVisColors() {
  G4LogicalVolumeStore *lvs = G4LogicalVolumeStore::GetInstance();
  std::vector<G4LogicalVolume *>::const_iterator lvciter;
  for (lvciter = lvs->begin(); lvciter != lvs->end(); lvciter++) {
    G4String volumeName = (*lvciter)->GetName();
    G4double red = G4UniformRand() * 0.7 + 0.15;
    G4double green = G4UniformRand() * 0.7 + 0.15;
    G4double blue = G4UniformRand() * 0.7 + 0.15;
    G4double alpha = 0;

    if (volumeName == "CdTeAnodeLog" || volumeName == "CdTeCathodeLog") {
      red = 1;
      green = 0;
      blue = 0;
      alpha = 0.05;
      SetVisAttrib(*lvciter, red, green, blue, alpha, true, false);
    } else if (volumeName.contains("frontGrid") ||
               volumeName.contains("rearGrid")) {
      (*lvciter)->SetVisAttributes(G4VisAttributes::Invisible);
    } else if (volumeName.contains("grid")) {
      red = 0.28;
      green = 0.28;
      blue = 0.28;
      alpha = 0.1;
      SetVisAttrib(*lvciter, red, green, blue, alpha, true, true);
    } else {
      SetVisAttrib(*lvciter, red, green, blue, alpha);
    }
    // randomize color

    double mass = (*lvciter)->GetMass() / g;
    G4cout << "~~~ The MASS of " << volumeName << " is " << mass << " g. ~~~"
           << G4endl;
  }
}
