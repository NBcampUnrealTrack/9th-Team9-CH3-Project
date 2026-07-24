# Parcel Knight HTML 발표 자료

`index.html`을 브라우저에서 직접 여는 15장짜리 16:9 프레젠테이션입니다. 외부 CDN, 별도 서버, 빌드 과정이 필요하지 않습니다.

본문은 현재 저장소의 C++·설정·콘텐츠 경로와 Git 이력에서 확인된 사실만 사용했습니다. 실행 또는 Blueprint 편집기에서 다시 확인해야 하는 내용은 `[에디터 확인 필요]`로 남겼습니다.

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

## 실행과 조작

1. `Presentation/index.html`을 Chrome 또는 Edge로 엽니다.
2. 브라우저 확대/축소는 100%를 권장합니다.
3. 상단 오른쪽의 `⛶` 버튼 또는 `F` 키로 전체 화면을 시작합니다.
4. 현재 슬라이드는 `#slide-5` 같은 URL 해시에 기록됩니다.

| 입력 | 동작 |
|---|---|
| `←`, `PageUp` | 이전 슬라이드 |
| `→`, `PageDown`, `Space` | 다음 슬라이드 |
| `Home` / `End` | 처음 / 마지막 슬라이드 |
| 숫자 `1`~`15` | 해당 번호로 이동. 두 자리 수는 연속 입력 |
| `F` 또는 `⛶` | 전체 화면 시작/종료 |
| `Esc` | 전체 화면 또는 슬라이드 목록 종료 |
| 화면 좌우 버튼 / 터치 스와이프 | 이전/다음 |
| 상단 `목록` | 원하는 슬라이드로 이동 |

첫 장의 이전 버튼과 마지막 장의 다음 버튼은 자동으로 비활성화됩니다.

## 슬라이드 구성과 팀 작업

| 번호 | 섹션 | 내용 | 발표 전 확인 |
|---:|---|---|---|
| 1 | INTRO | 표지 | 팀명·발표자·날짜·담당 역할·대표 화면 |
| 2 | INTRO | 게임 개요와 플레이 루프 | 공식 장르·개발 목표 |
| 3 | ARCHITECTURE | 서버·복제·소유 클라이언트 구조 | Blueprint 기본값 |
| 4 | FEATURES | 핵심 기능 3개 요약 | 발표 연결 문장 |
| 5 | FEATURES | 인벤토리 및 상점 | 실제 구매·저장·장착 시연 |
| 6 | FEATURES | 기존 배송 파이프라인 유지와 연동 | StageData·BoxData·ZoneTag 연결 |
| 7 | FEATURES | DeliveryBox와 캐릭터 물리 충돌 | Blueprint collision preset과 PIE 결과 |
| 8 | TROUBLESHOOTING | 트러블슈팅 3개 요약 | 해결 상태 갱신 |
| 9 | TROUBLESHOOTING | 호스트 네임플레이트 | 표시 정책 수정 후 2인·3인 검증 |
| 10 | TROUBLESHOOTING | C++ / Blueprint 노드 불일치 | Editor 빌드·Refresh Nodes·Compile 결과 |
| 11 | TROUBLESHOOTING | DeliveryBox 충돌 | 들기·놓기·던지기·배송 회귀 확인 |
| 12 | TECH | 서버와 클라이언트 역할 분리 | Listen Server 동작 설명 합의 |
| 13 | TECH | GameplayTags State / Action | DataAsset·DataTable 태그 연결 |
| 14 | TECH | Lyra 스타일 Component / Interface | “스타일” 표현과 팀 의도 확인 |
| 15 | WRAP-UP | 팀 프로젝트 회고와 Q&A | 회고 문장·핵심 결론 최종 합의 |

먼저 1·2·15번을 팀이 함께 확정하고, 각 담당자가 5~7번의 시연 포인트와 9~11번의 실제 검증 결과를 채우는 순서를 권장합니다.

## 텍스트 수정 방법

각 슬라이드는 `index.html`의 `<section class="slide" id="slide-N">` 블록 하나입니다.

```html
<section class="slide" id="slide-6" data-section="FEATURES">
  ...이 슬라이드에 보이는 내용...
</section>
```

- 제목: `<h1>` 또는 `<h2>`
- 본문: `<p>`, `<li>`, `<dd>`
- 코드: `<pre><code>` 내부. `<`는 `&lt;`, `>`는 `&gt;`, `&`는 `&amp;`로 작성
- 조사 근거: 발표 화면에는 표시하지 않으며, 이 문서의 `주요 코드 근거` 절에서 관리
- 섹션: `INTRO`, `ARCHITECTURE`, `FEATURES`, `TROUBLESHOOTING`, `TECH`, `WRAP-UP`

각 슬라이드 위의 HTML 주석은 해당 블록의 목적을 설명합니다. 구조를 유지하고 문구만 바꾸면 레이아웃이 가장 안정적입니다.

## 이미지 넣는 방법

1. 발표용 이미지 복사본을 `Presentation/assets/images/`에 넣습니다.
2. 표지 이미지는 `game-cover.png` 파일을 같은 이름으로 교체하면 자동 반영됩니다. 파일이 없거나 읽히지 않으면 깨진 아이콘 대신 플레이스홀더가 표시됩니다.
3. 다른 슬라이드 이미지는 `index.html`에서 교체할 placeholder를 찾아 아래처럼 바꿉니다.

```html
<img
  class="slide-image"
  src="assets/images/delivery-box-collision.png"
  alt="택배 상자와 캐릭터 충돌 장면">
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

- 플레이 화면을 꽉 채울 때: `object-fit: cover`
- UI 전체·구조도·Blueprint 캡처를 자르지 않을 때: `object-fit: contain; background: #050a12`
- 로컬 `C:\...` 경로가 아니라 `assets/images/...` 상대 경로를 사용합니다.
- 원본 Unreal 에셋을 옮기거나 수정하지 말고 내보낸 발표용 복사본만 넣습니다.

현재 자동 삽입한 실제 플레이 이미지나 가짜 오류 화면은 없습니다.

## 영상 넣는 방법

1. 필요하면 `Presentation/assets/videos/` 폴더를 만들고 MP4 파일을 넣습니다.
2. 원하는 슬라이드 안에 다음 요소를 추가합니다.

```html
<video class="slide-video" controls preload="metadata">
  <source src="assets/videos/gameplay-demo.mp4" type="video/mp4">
  이 브라우저는 영상을 재생할 수 없습니다.
</video>
```

```css
.slide-video {
  width: 100%;
  aspect-ratio: 16 / 9;
  border-radius: 18px;
  background: #050a12;
  object-fit: contain;
}
```

발표 PC에서도 재생되도록 영상 파일을 HTML과 함께 복사하고, H.264/AAC MP4 형식을 권장합니다.

## 슬라이드 추가·삭제

1. 가장 비슷한 `<section class="slide">`를 복사하거나 삭제합니다.
2. `id="slide-N"`, `aria-labelledby`, 제목의 `id`를 1부터 연속 번호로 정리합니다.
3. `data-section`을 기존 섹션 중 하나로 지정합니다.
4. JavaScript가 `.slide`를 자동 수집하므로 슬라이드 배열·전체 수·진행률은 따로 수정하지 않습니다.

## PDF로 저장

1. Chrome 또는 Edge에서 `index.html`을 엽니다.
2. 인쇄에서 `PDF로 저장`을 선택합니다.
3. `배경 그래픽`을 켭니다.
4. 여백은 `없음`, 배율은 `기본값` 또는 `페이지에 맞춤`을 사용합니다.
5. 인쇄 CSS가 슬라이드마다 한 페이지로 분리합니다.

## 주요 코드 근거

### 인벤토리 및 상점

- `Source/Parcel_Knight/Public/Data/ItemData.h` — `FItemData`, `FItemEffect`
- `Source/Parcel_Knight/Private/UI/ParcelShopInventoryWidget.cpp` — `RequestPurchase`, `RefreshShopUI`, 목록 재구축
- `Source/Parcel_Knight/Private/Core/ParcelGameInstance.cpp` — `TryPurchaseConsumable`, `SaveData`, 로컬 재화·보유 목록
- `Source/Parcel_Knight/Private/Core/ParcelPlayerController.cpp` — `SubmitLocalLoadoutToServer`, `Server_SubmitLoadout`
- `Source/Parcel_Knight/Private/Core/InventoryComponent.cpp` — `SetValidatedLoadout`, `OnRep_Items`

### 배송 파이프라인과 물리 충돌

- `Source/Parcel_Knight/Private/Delivery/DeliveryBoxSpawner.cpp` — 서버 타이머 스폰
- `Source/Parcel_Knight/Private/Delivery/DeliverySubsystem.cpp` — 상자 생성·등록·제거
- `Source/Parcel_Knight/Private/Delivery/DeliveryBox.cpp` — 상태·물리·Interface 구현
- `Source/Parcel_Knight/Private/Character/CharacterCarryComponent.cpp` — Attach/Detach와 충돌 전환
- `Source/Parcel_Knight/Private/Delivery/DeliveryZone.cpp` — 성공/실패 판정
- `Source/Parcel_Knight/Public/Delivery/CarryableInterface.h`
- `Source/Parcel_Knight/Public/Delivery/InteractableInterface.h`
- Git `594f37c` — Pawn/PhysicsBody 충돌 응답 명시 전후

### 트러블슈팅

- `Source/Parcel_Knight/Private/Character/ParcelCharacter.cpp::UpdateOverheadNameplate` — 로컬 Pawn 숨김 분기
- `Source/Parcel_Knight/Private/UI/ParcelNameplateWidget.cpp::SetPlayerName` — 이름 텍스트 적용
- `Source/Parcel_Knight/Public/Core/ParcelGameInstance.h::EquipSkin`
- `Source/Parcel_Knight/Public/Character/ParcelCharacter.h::ApplySkin`
- `Content/UI/Hud/WBP_SkinCustomization.uasset` — 노드 갱신 대상
- `Content/Blueprints/BP_ParcelCharacter.uasset`, `Content/Delivery/Boxes/BP_DeliveryBox.uasset` — 기본값 확인 대상

### 기술 이슈

- `Source/Parcel_Knight/Private/Core/HealthComponent.cpp`
- `Source/Parcel_Knight/Private/Core/TeamScoreComponent.cpp`
- `Source/Parcel_Knight/Private/Character/ParcelPlayerStateComponent.cpp`
- `Config/Tags/CharacterGameplayTags.ini`, `Config/Tags/DeliveryGameplayTags.ini`
- `Source/Parcel_Knight/Private/Character/ParcelInteractionComponent.cpp`
- `Source/Parcel_Knight/Private/Character/ParcelCharacter.cpp`

## 발표 전에 반드시 확인할 항목

- `BP_ParcelCharacter → NameplateWidgetComp`: Widget Class, Visibility, Hidden in Game, Owner No See, Only Owner See
- `WBP_SkinCustomization`: Refresh All Nodes 후 Apply Skin / Equip Skin 핀과 Compile 결과
- `BP_DeliveryBox → CollisionComponent`: PhysicsBody, Query and Physics, Pawn Block
- `BP_ParcelCharacter → CapsuleComponent`: Pawn profile, PhysicsBody Block
- PIE Listen Server + Client 1~2명: 네임플레이트, 구매/장착, 상자 들기·놓기·던지기, 배송 성공/실패
- StageData, DT_BoxData, DT_ConsumableItems, ZoneTag의 실제 에셋 연결

확인 전에는 `[에디터 확인 필요]` 문구를 해결 완료 표현으로 바꾸지 마세요.
