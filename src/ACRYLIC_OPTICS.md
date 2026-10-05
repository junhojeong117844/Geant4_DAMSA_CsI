# 아크릴 광학 설정

현재 `Materials.cc`의 `G4_PLEXIGLASS`는 파장별 실측 모델이 아닌 상수 근사다.

| 항목 | 현재 설정 / 계산값 |
|---|---|
| 굴절률 | 1.49 |
| 흡수길이 | 10,000 mm (10 m) |
| 광학 테이블 범위 | 1.80–4.59 eV, 약 689–270 nm |
| 10 mm 직선 광로의 내부 투과율 | exp(-10/10000) = 99.900% |
| 20 mm 직선 광로의 내부 투과율 | exp(-20/10000) = 99.800% |
| 공기→아크릴 수직 입사면 반사율 | ((1.49-1)/(1.49+1))² = 3.873% |
| 공기–아크릴–공기, 20 mm, 첫 통과 투과율 | (1-R)² exp(-20/10000) ≈ 92.22% |
| 아크릴→공기 임계각 | asin(1/1.49) ≈ 42.16° |

첫 통과 계산은 평행 평판, 수직 입사, 다중 내부반사 제외 조건이다.
실제 테이퍼 가이드의 검출 효율은 입사각, 실제 광로 길이, 측면 반사,
결합재와 SiPM PDE까지 영향을 받으므로 위 투과율과 같지 않다.
화면의 `G4Colour(..., 0.2)`는 시각화 불투명도이며 광학 투과율이 아니다.

## 실제 재료와의 차이

- 제조사 PLEXIGLAS는 투명 제품의 **가시광** 투과율을 최대 92%로 설명한다.
  이 값에는 표면 반사 손실이 포함된다. 제품 종류와 두께에 따라 달라진다.
  https://www.plexiglas.de/en/service/product-info/light-transmission
- UV 투과 전용 PLEXIGLAS GS Clear 2458은 일반 투명 아크릴과 별도 등급이다.
  제조사 자료에는 3 mm 및 8 mm 시편의 UV 스펙트럼이 제시되어 있다.
  그 얇은 시편의 투과율을 10–20 mm 가이드에 그대로 적용하면 안 된다.
  https://www.plexiglas.de/files/plexiglas-content/pdf/technische-informationen/222-6-PLEXIGLAS-GS-UV-transmitting_Clear_2458_and_SC_EN.pdf
- 반대로 UV 차단용 ACRYLITE Gallery OP3은 200–390 nm UV를 약 99.7% 흡수한다고 명시한다.
  즉, “아크릴”이라는 이름만으로 300 nm 투과율을 정할 수 없다.
  https://www.acrylite.co/files/content/acrylite.co/documents/product-information/ACRYLITE-Extruded-Light-Transmission-Reflectance-Information.pdf

이 프로젝트의 CsI 발광 피크는 약 300 nm다. 현재 상수 흡수길이는 이 영역에서도
거의 완전 투과를 가정하므로, 실제 아크릴 가이드의 광량을 과대평가할 수 있다.
정량 모델에는 제품 등급, 시편 두께, 파장별 투과율과 굴절률이 필요하다.
측정 투과율에서 표면 반사 기여를 분리한 내부 투과율 T_bulk에 대해
`ABSLENGTH(λ) = -d / ln(T_bulk(λ))`로 변환한다.
측정이 다중 반사를 포함하면 그에 맞는 평판 광학 모델로 역산해야 한다.
원자료에 표면 반사가 이미 포함된 투과율을 바로 흡수길이로 바꾸면
Geant4 경계 반사와 손실을 중복 계산하게 된다.

제품이 정해지기 전에는 임의의 UV 곡선을 실측값처럼 넣지 않는다.
