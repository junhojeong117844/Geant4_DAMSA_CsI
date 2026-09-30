// 시뮬레이션에 사용할 물리 모델을 정의하는 곳
// 광전효과, Compton, pair production 같은 EM 물리
// hadronic physics, decay, optical photon physics 등등 등록 가능

# ifndef PhysicsList_h
# define PhysicsList_h 1

#include "G4VModularPhysicsList.hh"     // 모듈 방식으로 물리 리스트를 등록할 수 있는 베이스 클래스
#include "G4EmStandardPhysics.hh"       // 표준 전자기 물리 모델 (photoelectric, Compton, pair production, ionization 등)
#include "G4DecayPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"
#include "G4OpticalPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"

class PhysicsList : public G4VModularPhysicsList
{
public:
    PhysicsList();
    ~PhysicsList();
};

#endif