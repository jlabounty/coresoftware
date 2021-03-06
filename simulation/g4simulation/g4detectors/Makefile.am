AUTOMAKE_OPTIONS = foreign

# List of shared libraries to produce
lib_LTLIBRARIES = \
    libg4detectors_io.la \
    libg4detectors.la

AM_CPPFLAGS = -DCGAL_DISABLE_ROUNDING_MATH_CHECK

AM_LDFLAGS = \
    -L$(libdir) \
    -L$(OFFLINE_MAIN)/lib

INCLUDES = \
    -I$(includedir) \
    -I$(OFFLINE_MAIN)/include \
    -I$(ROOTSYS)/include \
    -I${G4_MAIN}/include \
    -I${G4_MAIN}/include/Geant4

libg4detectors_io_la_LIBADD = \
  -lphool

libg4detectors_la_LIBADD = \
  -lphool \
  -lSubsysReco \
  -lg4testbench \
  -lCGAL \
  libg4detectors_io.la

pkginclude_HEADERS = \
  PHG4BlockGeom.h \
  PHG4BlockGeomContainer.h \
  PHG4CylinderCell.h \
  PHG4CylinderCellv1.h \
  PHG4CylinderCellv2.h \
  PHG4CylinderCellContainer.h \
  PHG4CylinderGeom.h \
  PHG4CylinderGeomContainer.h \
  PHG4CylinderCellDefs.h \
  PHG4CylinderCellGeom.h \
  PHG4CylinderCellGeomContainer.h

libg4detectors_io_la_SOURCES = \
  PHG4BlockCellGeom.cc \
  PHG4BlockCellGeom_Dict.cc \
  PHG4BlockCellGeomContainer.cc \
  PHG4BlockCellGeomContainer_Dict.cc \
  PHG4BlockGeom.cc \
  PHG4BlockGeom_Dict.cc \
  PHG4BlockGeomv1.cc \
  PHG4BlockGeomv1_Dict.cc \
  PHG4BlockGeomContainer.cc \
  PHG4BlockGeomContainer_Dict.cc \
  PHG4CylinderGeom.cc \
  PHG4CylinderGeom_Dict.cc \
  PHG4CylinderGeomv1.cc \
  PHG4CylinderGeomv1_Dict.cc \
  PHG4CylinderGeomv2.cc \
  PHG4CylinderGeomv2_Dict.cc \
  PHG4CylinderGeomv3.cc \
  PHG4CylinderGeomv3_Dict.cc \
  PHG4CylinderGeomv4.cc \
  PHG4CylinderGeomv4_Dict.cc \
  PHG4CylinderGeom_Spacalv1.cc \
  PHG4CylinderGeom_Spacalv1_Dict.cc \
  PHG4CylinderGeom_Spacalv2.cc \
  PHG4CylinderGeom_Spacalv2_Dict.cc \
  PHG4CylinderGeomContainer.cc \
  PHG4CylinderGeomContainer_Dict.cc \
  PHG4CylinderCell.cc \
  PHG4CylinderCell_Dict.cc \
  PHG4CylinderCellv1.cc \
  PHG4CylinderCellv1_Dict.cc \
  PHG4CylinderCellv2.cc \
  PHG4CylinderCellv2_Dict.cc \
  PHG4CylinderCellContainer.cc \
  PHG4CylinderCellContainer_Dict.cc \
  PHG4CylinderCellGeom.cc \
  PHG4CylinderCellGeom_Dict.cc \
  PHG4CylinderCellGeomContainer.cc \
  PHG4CylinderCellGeomContainer_Dict.cc

libg4detectors_la_SOURCES = \
  PHG4BlockCellReco.cc \
  PHG4BlockCellReco_Dict.cc \
  PHG4BlockDetector.cc \
  PHG4BlockEventAction.cc \
  PHG4BlockRegionSteppingAction.cc \
  PHG4BlockSteppingAction.cc \
  PHG4BlockSubsystem.cc \
  PHG4BlockSubsystem_Dict.cc \
  PHG4CEmcTestBeamDetector.cc \
  PHG4CEmcTestBeamSteppingAction.cc \
  PHG4CEmcTestBeamSubsystem.cc \
  PHG4CEmcTestBeamSubsystem_Dict.cc \
  PHG4EventActionClearZeroEdep.cc \
  PHG4ConeDetector.cc \
  PHG4ConeEventAction.cc \
  PHG4ConeRegionSteppingAction.cc \
  PHG4ConeSteppingAction.cc \
  PHG4ConeSubsystem.cc \
  PHG4ConeSubsystem_Dict.cc \
  PHG4CrystalCalorimeterDetector.cc \
  PHG4CrystalCalorimeterSteppingAction.cc \
  PHG4CrystalCalorimeterSubsystem.cc \
  PHG4CrystalCalorimeterSubsystem_Dict.cc \
  PHG4CylinderDetector.cc \
  PHG4CylinderEventAction.cc \
  PHG4CylinderRegionSteppingAction.cc \
  PHG4CylinderSubsystem.cc \
  PHG4CylinderSubsystem_Dict.cc \
  PHG4CylinderCellReco.cc \
  PHG4CylinderCellReco_Dict.cc \
  PHG4CylinderSteppingAction.cc \
  PHG4EnvelopeDetector.cc \
  PHG4EnvelopeSteppingAction.cc \
  PHG4EnvelopeSubsystem.cc \
  PHG4EnvelopeSubsystem_Dict.cc \
  PHG4FCalDetector.cc \
  PHG4FCalSteppingAction.cc \
  PHG4FCalSubsystem.cc \
  PHG4FCalSubsystem_Dict.cc \
  PHG4FPbScDetector.cc \
  PHG4FPbScEventAction.cc \
  PHG4FPbScSteppingAction.cc \
  PHG4FPbScRegionSteppingAction.cc \
  PHG4FPbScSubsystem.cc \
  PHG4FPbScSubsystem_Dict.cc \
  PHG4GenHit.cc \
  PHG4GenHit_Dict.cc \
  PHG4HcalCellReco.cc \
  PHG4HcalCellReco_Dict.cc \
  PHG4HcalDetector.cc \
  PHG4HcalSteppingAction.cc \
  PHG4HcalSubsystem.cc \
  PHG4HcalSubsystem_Dict.cc \
  PHG4HcalPrototypeDetector.cc \
  PHG4HcalPrototypeDetectorMessenger.cc \
  PHG4HcalPrototypeSteppingAction.cc \
  PHG4HcalPrototypeSubsystem.cc \
  PHG4HcalPrototypeSubsystem_Dict.cc \
  PHG4InnerHcalDetector.cc \
  PHG4InnerHcalParameters.cc \
  PHG4InnerHcalSteppingAction.cc \
  PHG4InnerHcalSubsystem.cc \
  PHG4InnerHcalSubsystem_Dict.cc \
  PHG4SlatCellReco.cc \
  PHG4SlatCellReco_Dict.cc \
  PHG4OuterHcalDetector.cc \
  PHG4OuterHcalSteppingAction.cc \
  PHG4OuterHcalSubsystem.cc \
  PHG4OuterHcalSubsystem_Dict.cc \
  PHG4RICHDetector.cc \
  PHG4RICHSteppingAction.cc \
  PHG4RICHSubsystem.cc \
  PHG4RICHSubsystem_Dict.cc \
  ePHENIXRICHConstruction.cc \
  ePHENIXRICHConstruction_Dict.cc \
  PHG4SectorConstructor.cc \
  PHG4SectorConstructor_Dict.cc \
  PHG4SectorDetector.cc \
  PHG4SectorEventAction.cc \
  PHG4SectorSteppingAction.cc \
  PHG4SectorSubsystem.cc \
  PHG4SectorSubsystem_Dict.cc \
  PHG4SiliconTrackerCellReco.cc \
  PHG4SiliconTrackerCellReco_Dict.cc \
  PHG4SiliconTrackerDetector.cc \
  PHG4SiliconTrackerSteppingAction.cc \
  PHG4SiliconTrackerSubsystem.cc \
  PHG4SiliconTrackerSubsystem_Dict.cc \
  PHG4SpacalDetector.cc \
  PHG4ProjSpacalDetector.cc \
  PHG4SpacalSteppingAction.cc \
  PHG4SpacalSubsystem.cc \
  PHG4SpacalSubsystem_Dict.cc

# Rule for generating table CINT dictionaries.
%_Dict.cc: %.h %LinkDef.h
	rootcint -f $@ -c $(DEFAULT_INCLUDES) $(INCLUDES) $^

################################################
# linking tests

noinst_PROGRAMS = \
  testexternals_g4detectors

testexternals_g4detectors_SOURCES = testexternals.cc
testexternals_g4detectors_LDADD = libg4detectors.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

##############################################
# please add new classes in alphabetical order

clean-local:
	rm -f *Dict* $(BUILT_SOURCES)

