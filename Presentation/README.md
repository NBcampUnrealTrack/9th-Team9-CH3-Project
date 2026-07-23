# Parcel Knight HTML 발표 자료

`index.html`을 브라우저에서 직접 열어 사용하는 15장짜리 16:9 발표 자료입니다. 외부 CDN, 별도 서버, 빌드 과정이 필요하지 않습니다.

발표 본문은 현재 저장소의 C++·설정·콘텐츠 폴더와 Git 변경 이력에서 확인되는 사실만 사용했습니다. 팀의 의도나 실행 결과를 코드만으로 확정할 수 없는 곳은 `[팀 확인 필요]` 또는 `[에디터 확인 필요]`로 남겼습니다.

## 파일 구성

```text
Presentation/
├─ index.html
├─ styles.css
├─ presentation.js
├─ README.md
└─ assets/
   └─ images/
      └─ .gitkeep
```

Unreal 프로젝트의 `Source/`, `Content/`, `Config/`, 플러그인과 에셋은 이 발표 자료에서 수정하지 않습니다.

## 실행 방법

1. `Presentation/index.html`을 Chrome 또는 Edge로 엽니다.
2. 브라우저 확대/축소는 100%를 권장합니다.
3. 상단 오른쪽의 `⛶` 버튼 또는 `F` 키로 전체 화면을 시작합니다.
4. 현재 슬라이드는 URL의 `#slide-5` 같은 해시에 기록됩니다. 링크를 다시 열면 해당 장에서 시작합니다.

## 조작 방법

| 입력 | 동작 |
|---|---|
| `←`, `PageUp` | 이전 슬라이드 |
| `→`, `PageDown`, `Space` | 다음 슬라이드 |
| `Home` | 첫 슬라이드 |
| `End` | 마지막 슬라이드 |
| 숫자 `1`~`15` | 해당 번호로 이동. 두 자리 수는 연속 입력 |
| `F` 또는 `⛶` 버튼 | 전체 화면 시작/종료 |
| `Esc` | 전체 화면 또는 슬라이드 목록 종료 |
| 화면 좌우 버튼 | 이전/다음 |
| 터치 좌우 스와이프 | 이전/다음 |
| 상단 `목록` | 15장 목록에서 바로 이동 |

첫 장에서 이전 버튼, 마지막 장에서 다음 버튼은 자동으로 비활성화됩니다.

## 팀이 먼저 채울 내용

발표를 합칠 때 아래 순서로 채우면 전체 문맥이 빨리 정리됩니다.

1. **슬라이드 1**: 팀명, 발표자, 날짜, 팀원 5명의 이름과 담당 역할, 실제 대표 화면
2. **슬라이드 2**: 팀이 합의한 공식 장르와 개발 목표
3. **슬라이드 15**: 한 문장으로 정리한 발표 핵심 결론
4. **슬라이드 5~7**: 담당자가 실제 시연에서 강조할 사용자 경험
5. **슬라이드 9~11**: 수정 뒤 직접 실행한 재현 조건과 확인 결과
6. **슬라이드 13~15**: 실제 당시 비교한 대안과 선택 배경

대괄호가 들어간 문구는 검색해서 교체하면 됩니다. HTML에서 `[팀 입력 필요`, `[팀 확인 필요`, `[에디터 확인 필요`를 검색하세요.

## 슬라이드 구성

| 번호 | 섹션 | 내용 | 팀 작업 |
|---:|---|---|---|
| 1 | INTRO | 표지와 5명 역할 | 모든 대괄호 항목, 대표 화면 |
| 2 | INTRO | 플레이 루프와 확인된 기본 정보 | 공식 장르, 개발 목표 |
| 3 | ARCHITECTURE | 서버·복제 상태·소유 클라이언트 구조 | Blueprint 기본값 확인 |
| 4 | FEATURES | 핵심 기능 3개 요약 | 발표 연결 문장 조정 |
| 5 | FEATURES | Steam 세션 생명주기 | 실제 두 PC 확인 결과 |
| 6 | FEATURES | 배송 판정과 점수 전달 | Box/Zone 태그 연결 확인 |
| 7 | FEATURES | 로드아웃 제출과 서버 검증 | 서버 프로필 한계 설명 합의 |
| 8 | TROUBLESHOOTING | 문제 3개 요약 | 실행 확인 상태 갱신 |
| 9 | TROUBLESHOOTING | 사망 처리 단일화 | death event 2회 검증 결과 |
| 10 | TROUBLESHOOTING | 이동 속도 상태 합성 | Carry/Sprint/Slow 순서 검증 |
| 11 | TROUBLESHOOTING | 점수·콤보 계산 순서 | 실제 DataTable 조건으로 수치 확인 |
| 12 | DECISIONS | 기술 선택 3개 요약 | 팀이 실제 선택한 배경 확인 |
| 13 | DECISIONS | Framework와 컴포넌트 책임 | 비교 대안과 장단점 보완 |
| 14 | DECISIONS | Replication과 RPC 구분 | 전용 서버·통신량 검증 여부 |
| 15 | DECISIONS | Tags·DataAssets와 기술 요약 | 핵심 결론, DataAsset 연결 확인 |

## 텍스트 수정 방법

모든 슬라이드는 `index.html` 안의 `<section class="slide" id="slide-N">` 블록 하나에 대응합니다.

```html
<section class="slide" id="slide-6" data-section="FEATURES">
  ...이 슬라이드에 보이는 내용...
</section>
```

- 제목: 각 슬라이드의 `<h1>` 또는 `<h2>`
- 본문: `<p>`, `<li>`, `<dd>`
- 코드: `<pre><code>` 내부. `<`는 `&lt;`, `>`는 `&gt;`, `&`는 `&amp;`로 작성
- 근거 표시: 각 슬라이드 맨 아래 `<footer class="slide-evidence">`
- 섹션: `data-section` 값. 현재 값은 `INTRO`, `ARCHITECTURE`, `FEATURES`, `TROUBLESHOOTING`, `DECISIONS`

각 슬라이드 바로 위에는 용도를 설명하는 HTML 주석이 있습니다. 구조를 유지하고 글자만 바꾸면 레이아웃이 가장 안정적입니다.

## 이미지 넣는 방법

1. 파일을 `Presentation/assets/images/`에 복사합니다.
2. 영문 소문자, 숫자, 하이픈을 사용한 파일명을 권장합니다. 예: `cover.webp`, `delivery-zone.png`.
3. `index.html`에서 해당 placeholder 바로 위의 `이미지 교체 위치` 주석을 찾습니다.
4. placeholder `<div>`를 아래처럼 `<img>`로 바꿉니다.

```html
<img
  class="slide-image cover-media"
  src="assets/images/cover.webp"
  alt="두 플레이어가 택배 상자를 운반하는 실제 게임 화면">
```

필요하면 `styles.css`에 다음 규칙을 추가합니다.

```css
.slide-image {
  width: 100%;
  height: 100%;
  border-radius: 22px;
  object-fit: cover;
}
```

- 화면을 꽉 채우는 캡처: `object-fit: cover`
- UI 전체나 구조도를 잘리지 않게 표시: `object-fit: contain; background: #050a12`
- 이미지의 `alt`에는 화면에서 발표자가 설명할 핵심을 짧게 적습니다.
- 로컬 경로는 `C:\...`가 아니라 `assets/images/...`처럼 HTML 기준 상대 경로를 사용합니다.
- 원본 Unreal 에셋을 이동하거나 수정하지 말고, 발표용으로 내보낸 복사본만 이 폴더에 둡니다.

현재 자동으로 넣은 실제 플레이 캡처는 없습니다. 저장소에서 전체 게임 화면을 입증할 로컬 이미지가 확인되지 않아, 표지에는 레이아웃이 유지되는 placeholder만 두었습니다.

## 슬라이드 추가·삭제 방법

### 추가

1. `index.html`에서 가장 비슷한 슬라이드 `<section>`을 통째로 복사합니다.
2. `id="slide-N"`을 순서대로 다시 번호 매깁니다.
3. `aria-labelledby`와 제목 요소의 `id`도 같은 번호로 바꿉니다.
4. `data-section`을 기존 5개 섹션 중 하나로 지정합니다.
5. JavaScript는 `.slide` 요소를 자동 수집하므로 슬라이드 배열을 따로 수정하지 않습니다.

### 삭제

1. 해당 `<section class="slide">` 블록을 제거합니다.
2. 남은 `slide-N` 번호를 1부터 연속되게 정리합니다.
3. 상단 목록·진행률·전체 수는 자동 계산됩니다.

## 디자인 수정 위치

`styles.css` 맨 위 `:root`의 CSS 변수만 바꾸면 주요 색을 일괄 수정할 수 있습니다.

```css
:root {
  --bg: #050a12;
  --surface: #08111f;
  --text: #f2f7fc;
  --cyan: #40d7ff;
  --amber: #ffbd59;
  --violet: #ae8cff;
}
```

내부 발표 캔버스는 1600×900으로 고정하고 브라우저 크기에 맞춰 비율을 유지한 채 축소합니다. 텍스트나 요소를 많이 추가하면 이 고정 캔버스를 넘을 수 있으므로, 문장을 늘리기보다 발표자 노트로 분리하는 편이 안전합니다.

## PDF로 저장

1. Chrome 또는 Edge에서 `index.html`을 엽니다.
2. 인쇄를 실행합니다.
3. 대상을 `PDF로 저장`으로 선택합니다.
4. 배경 그래픽을 켭니다.
5. 여백은 `없음`, 배율은 `기본값` 또는 `페이지에 맞춤`을 사용합니다.
6. 인쇄 CSS가 각 슬라이드를 16:9 한 페이지로 분리합니다.

## 코드 근거 전체 목록

슬라이드 안에는 가독성을 위해 각 2~4개 경로만 표시했습니다. 상세 근거는 아래와 같습니다.

### 프로젝트 기본 정보

- `Parcel_Knight.uproject` — 프로젝트 모듈과 EngineAssociation 5.5
- `Source/Parcel_Knight/Parcel_Knight.Build.cs` — UMG, GameplayTags, OnlineSubsystem, AdvancedSessions 의존성
- `Config/DefaultEngine.ini` — 기본 맵, GameMode, GameInstance, Steam OnlineSubsystem 설정
- `Config/Tags/` — Character, Trap, Box, Zone, Item 도메인 태그
- `Content/Delivery/` — 상자·구역·StageData·DataTable 에셋 폴더
- `Content/Maps/` — MainMenu, Lobby, Stage01~03 맵 에셋

### 슬라이드 3 · 전체 기술 구조

- `Source/Parcel_Knight/Public/Core/ParcelGameMode.h`
- `Source/Parcel_Knight/Public/Core/ParcelGameState.h`
- `Source/Parcel_Knight/Public/Core/ParcelPlayerState.h`
- `Source/Parcel_Knight/Private/Character/ParcelCharacter.cpp`
- `Source/Parcel_Knight/Private/UI/ParcelHUDWidget.cpp`

### 슬라이드 5 · 세션 생명주기

- `Source/Parcel_Knight/Public/Core/SessionSubsystem.h`
- `Source/Parcel_Knight/Private/Core/SessionSubsystem.cpp`
  - `CreateSession`, `BeginCreateSession`, `FindSessions`, `JoinSessionResult`
  - `OnStartSessionComplete`, `OnJoinSessionComplete`
  - `GetResolvedConnectString`, `ServerTravel`, `ClientTravel`
- `Source/Parcel_Knight/Private/Core/ParcelGameInstance.cpp`
  - `HandleSessionInviteAccepted`
- `Config/DefaultEngine.ini`

### 슬라이드 6 · 배송과 점수

- `Source/Parcel_Knight/Private/Delivery/DeliverySubsystem.cpp`
  - 서버 전용 `SpawnBox`, ActiveBoxes 등록
- `Source/Parcel_Knight/Private/Delivery/DeliveryZone.cpp`
  - `OnZoneOverlap`, `ProcessDelivery`, `MatchesTagExact`
- `Source/Parcel_Knight/Private/Core/ParcelGameMode.cpp`
  - `OnDeliveryCompleted`, `OnDeliveryFailed` 위임
- `Source/Parcel_Knight/Private/Core/DeliveryRuleComponent.cpp`
  - 성공·실패 점수와 개인 통계 반영
- `Source/Parcel_Knight/Private/Core/TeamScoreComponent.cpp`
  - 복제 점수·콤보·시간·등급
- `Source/Parcel_Knight/Private/UI/ParcelHUDWidget.cpp`
  - 복제 결과 delegate를 HUD에 연결

### 슬라이드 7 · 로드아웃

- `Source/Parcel_Knight/Private/Core/ParcelGameInstance.cpp`
  - 로컬 SaveGame 장착 태그
- `Source/Parcel_Knight/Private/Core/ParcelPlayerController.cpp`
  - `SubmitLocalLoadoutToServer`, `Server_SubmitLoadout`
- `Source/Parcel_Knight/Private/Core/InventoryComponent.cpp`
  - `SetValidatedLoadout`, `ApplyPassiveEffects`, `OnRep_Items`
- `Source/Parcel_Knight/Public/Data/ItemData.h`
  - `FItemData`와 ItemTag
- `Content/Data/DataTables/DT_ConsumableItems.uasset` — 실제 값은 에디터에서 확인

### 슬라이드 9 · 사망 처리

- `Source/Parcel_Knight/Private/Core/HealthComponent.cpp`
  - 서버 피해 처리와 최초 death delegate
- `Source/Parcel_Knight/Private/Character/ParcelCharacter.cpp`
  - `BindAuthoritativeDeathHandler`, `HandleCharacterDeath`, `bDeathHandled`
- `Source/Parcel_Knight/Private/Core/ParcelPlayerState.cpp`
  - DeathCount, 관전, UI 알림, 리스폰 진입
- `Source/Parcel_Knight/Private/Core/RespawnComponent.cpp`
  - 타이머와 RestartPlayer
- Git commit `b9c6bfa` — 중복 delegate 경로 제거 전후

### 슬라이드 10 · 이동 속도

- `Source/Parcel_Knight/Public/Character/ParcelMovementStatComponent.h`
- `Source/Parcel_Knight/Private/Character/ParcelMovementStatComponent.cpp`
  - `RefreshMoveSpeed`, `OnRep_MaxWalkSpeed`, `ApplyStatsToMovement`
- `Source/Parcel_Knight/Private/Character/CharacterCarryComponent.cpp`
  - Carry 이동 배율
- `Source/Parcel_Knight/Private/Components/DFStatusEffectComponent.cpp`
  - Slow 상태와 `RefreshMoveSpeed` 호출
- Git commit `7f27047` — 이전 값 복원 제거와 상태 합성 도입

### 슬라이드 11 · 점수와 콤보

- `Source/Parcel_Knight/Private/Core/TeamScoreComponent.cpp`
  - `AddTeamScore(int32, bool)`, `OnDeliverySuccess`, `OnDeliveryFail`
- `Source/Parcel_Knight/Private/Core/DeliveryRuleComponent.cpp`
  - 성공 보상 후 콤보 증가, 실패·매초 감소의 무배율 호출
- Git commit `7f27047` — 계산 순서·음수 배율·명시 변환 수정

### 슬라이드 13 · 컴포넌트 책임

- `Source/Parcel_Knight/Private/Core/ParcelGameMode.cpp`
- `Source/Parcel_Knight/Private/Core/ParcelGameState.cpp`
- `Source/Parcel_Knight/Private/Core/ParcelPlayerState.cpp`
- `Source/Parcel_Knight/Private/Character/ParcelCharacter.cpp`

### 슬라이드 14 · Replication과 RPC

- `Source/Parcel_Knight/Private/Core/TeamScoreComponent.cpp`
- `Source/Parcel_Knight/Private/Core/InventoryComponent.cpp`
- `Source/Parcel_Knight/Private/Delivery/DeliveryBox.cpp`
- `Source/Parcel_Knight/Private/Delivery/Traps/DFTrapBase.cpp`
- `Source/Parcel_Knight/Private/Core/ParcelPlayerController.cpp`

### 슬라이드 15 · Tags와 데이터

- `Config/Tags/`
- `Source/Parcel_Knight/Public/Delivery/StageData.h`
- `Source/Parcel_Knight/Public/Data/DFTrapDataAsset.h`
- `Source/Parcel_Knight/Public/Data/ItemData.h`
- `Source/Parcel_Knight/Public/Delivery/DeliveryTypes.h`
- `Content/Data/DataAsset/`, `Content/Data/TrapData/`, `Content/Data/DataTables/`의 실제 에셋은 에디터에서 연결 확인

## 반드시 실행으로 다시 확인할 항목

저장소의 최근 실행 로그는 일부 핵심 수정 커밋보다 앞선 시점이므로 아래 항목은 발표 전에 직접 확인해야 합니다.

- PIE Listen Server + Client 2인에서 사망 event 중복 호출과 리스폰
- Carry/Sprint/Slow 적용·해제 순서를 바꾼 최종 속도
- 첫 성공, 높은 콤보, 실패, 시간 감소의 실제 점수
- Box/Zone Blueprint 태그와 StageData/DataTable 연결
- Steam 로그인된 서로 다른 두 PC의 생성·초대·수락·참가·이동
- 전용 서버 또는 패키징 환경을 발표에서 주장하려면 해당 환경의 별도 실행 기록

확인 전에는 슬라이드의 “정적 확인”, `[에디터 확인 필요]` 문구를 성공 주장으로 바꾸지 마세요.
