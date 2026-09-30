# DAMSA_CsI 시뮬레이션

검출기·트리거·선원 조건은 `src/DetectorConstruction.cc` 생성자의
**USER CONFIGURATION**에서 설정합니다. 수정 후 다시 빌드하세요.
배치와 GUI는 동일한 조건을 사용합니다. 매크로는 초기화, 출력 파일 이름,
이벤트 수, 시각화만 제어하며 `/damsa/` 설정 명령은 제거했습니다.

## 실행

프로젝트 폴더에서:

```bash
cmake -S . -B build
cmake --build build -j 4
cd build
```

배치 실행:

```bash
./DAMSA_CsI batch.mac
```

`macros/batch.mac`의 `/run/beamOn 1000000`이 이벤트 수입니다.
원본 매크로를 수정하면 위 CMake 명령을 다시 실행해 build에 복사하세요.
`build/batch.mac`을 직접 수정하면 즉시 적용되지만 CMake 재실행 시 덮어씁니다.

GUI 실행:

```bash
./DAMSA_CsI
```

`vis.mac`으로 검출기를 표시하고 명령 입력을 기다립니다. GUI 명령창에서:

```text
/run/beamOn 100
```

다음 런을 별도 저장하려면 실행 전에 `/analysis/setFileName run2.root`를 입력하세요.
기본 출력은 실행 폴더의 `output.root`, 설정 기록은 `output.root.config.txt`입니다.
`beamOn`은 이벤트 생성 명령입니다. 기본 선원은 감마 빔이 아니라 Sr-90 이온입니다.

## 조건 수정

모든 길이는 전체 크기입니다. S13은 +z, S14는 -z입니다.

| 조건 | USER CONFIGURATION 항목 |
|---|---|
| SiPM 사용·크기·간격 | `s13`, `s14`: enabled, width, height, thickness, gap, gapMaterial, lightGuide, guideLength, guideGap, guideMaterial 순서 |
| 반사체 | `reflector`: `none`, `aluminum`, `teflon` |
| 측면 간격·포일 두께 | `sideGap`, `foilThickness` |
| 콜리메이터·나트륨 구형 구조물 | `collimator`, `sourceBead` |
| 트리거 | `trigger`: `none`, `crystal`, `external` |
| 셀프 트리거 채널 | `selfChannel`: `sum`, `s13`, `s14`, `coincidence` |
| 트리거 임계값 | `selfThreshold`, `externalThreshold` (엄격한 `>` 비교) |
| 선원 모드 | `sourceMode`: `ion`, `beam`, `Sr90_collimator` |
| 이온 | `ionZ`, `ionA`, `ionPosition` |
| 단색 입자 빔 | `beamParticle`, `beamEnergy`, `beamPosition`, `beamDirection` |
| Sr/Y 베타 스펙트럼 모드 | `spectrumPosition`, `sourceConeLength`, `sourceConeRadius` |

예: 외부 트리거를 사용하려면 `fConfig.trigger = "external";`로 바꾸세요.
양쪽 SiPM과 콜리메이터도 필요하면 `s13`의 첫 값을 true,
`collimator`를 true로 설정합니다. 외부 트리거 선택 시 PS 카운터가 자동 배치됩니다.

결합 물질은 `air`, `grease`, `cookie`입니다. 가이드가 없으면
CsI → gap → SiPM, 있으면 CsI → gap → guide → guideGap → SiPM입니다.
PDE·굴절률·흡수길이·발광 스펙트럼 같은 물질 데이터는 `src/Materials.cc`,
물리 프로세스 등록은 `src/PhysicsList.cc`에 있습니다.
CsI 크기 및 고정 구조물 치수는 DetectorConstruction의 `Construct()`에 있습니다.

## 이전 구조에서 달라진 점

기존 예제 매크로는 코드의 기본값을 덮어써 실행 파일별로 검출 조건이 달랐습니다.
이제 조건을 덮어쓰지 않습니다. `single_sipm.mac`, `external_trigger.mac`,
`light_guide.mac`은 호환용으로 `batch.mac`을 실행하며 이름에 따른 조건 변경은 없습니다.
기존 `/damsa/` 명령이 있는 개인 매크로는 해당 줄을 제거하고 조건을 C++에 옮겨야 합니다.
선원 조건을 DetectorConstruction으로 옮겼으며 현재 기본값은 유지했습니다.
기존 `validation/check_output.C`는 과거 세 프리셋 결과 파일 전용 검증입니다.
현재 공통 배치 실행에 대한 검증으로 사용하지 마세요.

## 기본값과 광학

기본값은 `DAMSA_CsI_single_sipm`의 광학/검출기 설정입니다.

- CsI: 10 × 10 × 120 mm, S14(-z)만 사용, SiPM 6 × 6 × 2 mm.
- CsI–SiPM: 1 µm 공기층. 측면 공기층 0.15 mm, 반사체 기본 없음.
- 반사체를 켜면 측면 4개와 SiPM이 없는 끝면에 0.04 mm 포일을 배치합니다.
- `Materials.cc`는 Single_sipm 기준으로 통합했습니다. S13/S14의 `EFFICIENCY`는
  모두 `BNL_s13PDE`를 사용합니다. 배열 값은 **퍼센트**이며 코드에서 100으로 나눕니다.
  개별 PDE를 쓰려면 `mptS13`/`mptS14`의 `EFFICIENCY` 배열 연결을 변경하세요.
- CsI 발광 스펙트럼/수율/흡수길이, 매끄럽게 연마된 표면(`polished`), 테플론의
  뒷면 반사 도장 표면 모형(`groundbackpainted`)과 굴절률(`RINDEX`)도 Single_sipm과 같습니다.
- 기본 셀프 트리거는 검출 광자 수 **> 13**입니다. 기본 S14 한 개에서는 기존 조건과 같습니다.

소스 생성 알고리즘은 `PrimaryGeneratorAction.cc`에 있으며 설정은 DetectorConstruction에 있습니다(기본 Sr-90 이온,
위치 (0, 7, -55) mm). `sourceBead`는 (0, 10.6, -30.25) mm의 기존 나트륨 구형
**물질 구조물만** 켜고 끕니다. 방사선 소스 종류/위치를 바꾸는 옵션은 아닙니다.

## 트리거와 ROOT 출력

| 트리거 설정 | 배치 및 선택 조건 | 선택 이벤트 트리 |
|---|---|---|
| `none` | 카운터 없음, 모든 이벤트 기록 | `AllEvents` |
| `crystal` | 카운터 없음, 지정 SiPM 광검출 조건 | `CrystalTrigger` |
| `external` | 플라스틱 섬광체(PS) 카운터 자동 배치, 카운터 에너지 침적량 > 임계값 | `ExternalTrigger` |

`sum`은 S13+S14 합, `s13`/`s14`는 해당 채널, `coincidence`는 두 채널이
**각각** 임계값보다 커야 합니다. 사용하지 않는 채널로 트리거를 걸면 초기화 오류를 냅니다.
외부 카운터 크기는 60 × 2 × 10 mm, 중심 y=6.5 mm입니다.

공통 열은 `eventID`, `crystalEdep_keV`, `Generated_photons`입니다.

| 결과 열 | 설명 |
|---|---|
| `eventID` | 해당 런에서의 이벤트 번호 |
| `crystalEdep_keV` | 크리스탈 내부 에너지 침적량, 단위 keV |
| `Generated_photons` | 크리스탈에서 생성된 섬광 광자 수 |
| `PSEdep_keV` | 외부 플라스틱 섬광체의 에너지 침적량, 단위 keV |
| `S13`, `S14` | 각 SiPM에서 검출된 크리스탈 섬광 광자 수; PDE가 반영된 검출 수 |

외부 트리거일 때만 `PSEdep_keV`, 활성 SiPM에 대해서만 `S13`/`S14` 열이 생깁니다.
별도 `PhotoDetected` 트리는 트리거 통과 여부와 관계없이 광자가 1개 이상 검출된
이벤트를 같은 열 구성으로 기록합니다. `CsI_Wl` 히스토그램은 모든 이벤트의
CsI에서 생성된 섬광 광자의 파장(nm) 분포입니다.

설정 요약은 출력 파일 이름 뒤에 `.config.txt`를 붙여 저장합니다.
ROOT 트리/열 이름이 기존 결과와 달라졌으므로 외부 분석 스크립트도 확인하세요.
포함된 fit/histo 매크로는 새 트리 및 기존 `Trigger activated`를 자동 탐색합니다.
한 프로세스에서 여러 런(이벤트 묶음)을 실행하면 같은 출력 파일은 덮어쓰므로 각
`/run/beamOn` 앞에서 `/analysis/setFileName`을 다르게 지정하세요.
