//
// Monolithic PhysicsList.cc
//

#include "PhysicsList.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleWithCuts.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"
#include "G4Material.hh"
#include "G4ios.hh"

#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4UnitsTable.hh"
#include "G4ParticleTable.hh"

PhysicsList::PhysicsList():  G4VUserPhysicsList()
{
  defaultCutValue = 2.0*mm;
  cutForGamma       = 2.0*mm;
  cutForElectron    = 0.1*mm;
  cutForProton      = defaultCutValue;

 SetVerboseLevel(2); // Show informations during initialization
}


PhysicsList::~PhysicsList()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void PhysicsList::ConstructParticle()
{
  // In this method, static member functions should be called
  // for all particles which you want to use.
  // This ensures that objects of these particle types will be
  // created in the program.

  ConstructBosons();
  ConstructLeptons();
  ConstructBaryons();
}


void PhysicsList::ConstructBosons()
{
  // gamma
  G4Gamma::GammaDefinition();
}

void PhysicsList::ConstructLeptons()
{
  // leptons
  G4Electron::ElectronDefinition();
  G4Positron::PositronDefinition();
}

void PhysicsList::ConstructBaryons()
{
//  barions
  G4Proton::ProtonDefinition();
  G4AntiProton::AntiProtonDefinition();
  G4Neutron::NeutronDefinition();
  G4AntiNeutron::AntiNeutronDefinition();
}


void PhysicsList::ConstructProcess()
{
  AddTransportation();
  ConstructEM();
  ConstructGeneral();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

#include "G4ComptonScattering.hh"
#include "G4PolarizedCompton.hh"
#include "G4GammaConversion.hh"
#include "G4PhotoElectricEffect.hh"

#include "G4eMultipleScattering.hh"
#include "G4hMultipleScattering.hh"

#include "G4eIonisation.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"

#include "G4hIonisation.hh"
#include "G4ParticleTable.hh"

void PhysicsList::ConstructEM()
{
  theParticleTable = G4ParticleTable::GetParticleTable();
  aParticleIterator = theParticleTable->GetIterator();
  aParticleIterator->reset();
  while( (*aParticleIterator)() ){
    G4ParticleDefinition* particle = aParticleIterator->value();
    G4ProcessManager* pmanager = particle->GetProcessManager();
    G4String particleName = particle->GetParticleName();

    if (particleName == "gamma") {
      //gamma
      pmanager->AddDiscreteProcess(new G4PhotoElectricEffect());
      pmanager->AddDiscreteProcess(new G4ComptonScattering());
//      pmanager->AddDiscreteProcess(new G4PolarizedCompton());
      pmanager->AddDiscreteProcess(new G4GammaConversion());

    } else if (particleName == "e-") {
      //electron
      pmanager->AddProcess(new G4eMultipleScattering(),-1, 1,1);
      pmanager->AddProcess(new G4eIonisation(),       -1, 2,2);
      pmanager->AddProcess(new G4eBremsstrahlung(),   -1,-1,-1);

    } else if (particleName == "e+") {
      //positron
      pmanager->AddProcess(new G4eMultipleScattering(),-1, 1,1);
      pmanager->AddProcess(new G4eIonisation(),       -1, 2,2);
      pmanager->AddProcess(new G4eBremsstrahlung(),   -1,-1,-1);
      pmanager->AddProcess(new G4eplusAnnihilation(),  -1,-1,-1);

    } else if ((!particle->IsShortLived()) &&
	       (particle->GetPDGCharge() != 0.0) &&
	       (particle->GetParticleName() != "chargedgeantino")) {
      //all others charged particles except geantino
      pmanager->AddProcess(new G4hMultipleScattering(),-1,1,1); // confirm it 23/07/2012, Rui
      pmanager->AddProcess(new G4hIonisation(),       -1,2,2);
    }
  }
}


#include "G4Decay.hh"

void PhysicsList::ConstructGeneral()
{
  // Add Decay Process
   G4Decay* theDecayProcess = new G4Decay();
  aParticleIterator->reset();
  while( (*aParticleIterator)() ){
    G4ParticleDefinition* particle = aParticleIterator->value();
    G4ProcessManager* pmanager = particle->GetProcessManager();
    if (theDecayProcess->IsApplicable(*particle)) {
      pmanager ->AddProcess(theDecayProcess);
      // set ordering for PostStepDoIt and AtRestDoIt
      pmanager ->SetProcessOrdering(theDecayProcess, idxPostStep);
      pmanager ->SetProcessOrdering(theDecayProcess, idxAtRest);
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void PhysicsList::SetCuts()
{

  if (verboseLevel >0){
    G4cout << "PhysicsList::SetCuts:";
    G4cout << "CutLength : " << G4BestUnit(defaultCutValue,"Length") << G4endl;
  }

  // set cut values for gamma at first and for e- second and next for e+,
  // because some processes for e+/e- need cut values for gamma
  SetCutValue(cutForGamma, "gamma");
  SetCutValue(cutForElectron, "e-");
  SetCutValue(cutForElectron, "e+");

  // set cut values for proton and anti_proton before all other hadrons
  // because some processes for hadrons need cut values for proton/anti_proton
  SetCutValue(cutForProton, "proton");
  SetCutValue(cutForProton, "anti_proton");

//  SetCutValueForOthers(defaultCutValue);

  if (verboseLevel>0) DumpCutValuesTable();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....


void      PhysicsList::SetCutForGamma(G4double cut)
{
  ResetCuts();
  cutForGamma = cut;
}

void      PhysicsList::SetCutForElectron(G4double cut)
{
  ResetCuts();
  cutForElectron = cut;
}

void      PhysicsList::SetCutForProton(G4double cut)
{
  ResetCuts();
  cutForProton = cut;
}

G4double  PhysicsList::GetCutForGamma() const
{
  return cutForGamma;
}

G4double  PhysicsList::GetCutForElectron() const
{
  return cutForElectron;
}

G4double  PhysicsList::GetCutForProton() const
{
  return cutForGamma;
}
