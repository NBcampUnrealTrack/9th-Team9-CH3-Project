# 9th-Team9-CH3-Project

<img width="2814" height="1536" alt="Parcel_Knight_Title" src="https://github.com/user-attachments/assets/8a0dbff0-f63e-4110-b49d-4b8a9d1a217f" />


## 최초 실행
cmd에 입력
```bash
git clone https://github.com/NBcampUnrealTrack/9th-Team9-CH3-Project.git
```

## 브랜치 설명
| 브랜치      | 용도            |
| -------- | ------------- |
| main     | 최종 제출 / 안정 버전 |
| develop  | 개발 통합 브랜치     |
| feat/기능명 | 개인 기능 개발 브랜치  |

## 기본 규칙
```bash
main 브랜치에 직접 작업하지 않습니다.
develop 브랜치에 직접 작업하지 않습니다.
각자 feat/기능명 브랜치를 만들어 작업합니다.
작업 완료 후 Pull Request로 develop에 병합합니다.
발표 또는 제출 가능한 버전만 main에 병합합니다.
```

## 브랜치 이름 규칙
```bash
feat/기능명

좋은 예시:
feat/player-movement
feat/player-interaction
feat/delivery-box
feat/delivery-zone
나쁜 예시:
feat/PlayerMovement
feat/player movement
feat/내기능
feat/test
feat/asdf
```

## 커밋 메세지 규칙
커밋 메세지는 아래 형식을 권장합니다.
```bash
타입: 작업 내용
```
타입종류
| 타입       | 의미          |
| -------- | ----------- |
| Add      | 새로운 기능 추가   |
| Fix      | 버그 수정       |
| Update   | 기존 기능 수정    |
| Refactor | 코드 구조 개선    |
| Remove   | 파일 또는 기능 삭제 |
| Docs     | 문서 수정       |
| Merge    | 브랜치 병합      |

## 작업 시작 전 체크 리스트
```bash
[ ] develop 브랜치로 이동했는가?
[ ] git pull origin develop을 했는가?
[ ] feat/기능명 브랜치를 새로 만들었는가?
[ ] main 브랜치에서 직접 작업하고 있지 않은가?
[ ] 다른 팀원 작업 파일과 겹치지 않는가?
```

## 작업 후 체크 리스트
```bash
[ ] Unreal Editor에서 프로젝트가 정상 실행되는가?
[ ] 내가 만든 기능이 정상 작동하는가?
[ ] 불필요한 테스트 파일이 포함되지 않았는가?
[ ] 빌드 오류가 없는가?
[ ] git status로 변경 파일을 확인했는가?
[ ] 커밋 메시지를 알아보기 쉽게 작성했는가?
[ ] feat 브랜치로 push했는가?
[ ] Pull Request를 develop으로 보냈는가?
```
