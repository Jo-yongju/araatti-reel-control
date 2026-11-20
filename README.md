# 아라아띠 낚시릴 제어

낚시 게임의 저항 명령을 모터 출력으로 연결하기 위한 STM32G431 코드입니다. 엔코더 시험 프로젝트와 MCSDK FOC 연동 소스를 구분해 보관합니다.

| 경로 | 내용 |
|---|---|
| `firmware/encoder_test` | CubeIDE 엔코더 하드웨어 시험 프로젝트 |
| `firmware/encoder_test/App` | 초기 명령 파서·출력 변화율 제한·통신 타임아웃·모터 모사 계층 |
| `firmware/encoder_test/Tests` | 초기 제어 계층의 호스트 시험 |
| `foc/App` | BL4270E1K FOC 프로젝트의 UART·MCSDK 연결 소스 |
| `foc/Inc`, `foc/bl4270e1k.ioc` | 당시 모터·드라이버 설정 |
| `docs` | 버전 구분과 MCSDK 연결 지점 |

## 구현 내용

- 줄 단위 저항 명령과 STOP 처리
- 출력 변화율 제한과 300 ms 명령 타임아웃
- 엔코더 카운트 누적, 릴 회전량 환산, 텔레메트리 포맷
- FOC 버전의 부호 있는 전류 지령과 MCSDK 상태·fault 확인
- 정렬 중 카운터 재설정 구간 처리

`firmware/encoder_test`의 main은 엔코더 관측용입니다. 이 프로젝트를 빌드했다고 FOC 전류 제어가 실행되는 것은 아닙니다. 초기 `motor_if.c`는 모사 구현이며 실제 모터 제어는 `foc/App/Src/motor_if.c`에서 확인할 수 있습니다.

## 확인 방법

엔코더 시험은 STM32CubeIDE에서 `firmware/encoder_test`를 가져옵니다. 프로젝트 이름은 `ffb_reel_g431`입니다. 측정값은 `encoder_raw_count`, `encoder_total_count`, `encoder_delta`, `encoder_direction`으로 확인합니다.

초기 제어 계층의 호스트 시험은 Python 3와 GCC로 실행합니다.

```text
python tests/run_tests.py
```

FOC 소스는 2026-10-03 기준 코드입니다. MCSDK 전체 배포본과 생성 프로젝트 전체를 포함하지 않아 이 폴더만으로 바로 빌드되지 않습니다. [연동 문서](docs/foc-integration.md)에 필요한 환경과 호출 지점을 정리했습니다.

기존 전류별 실험 영상과 별개로, 이번 저장소 정리에서는 새 모터 구동이나 게임 통합 시험을 수행하지 않았습니다. 초기 호스트 시험의 결과를 FOC 실물 검증 결과로 해석하지 않습니다.

외부 코드의 저작권과 라이선스는 [THIRD_PARTY.md](THIRD_PARTY.md)를 참고하세요.
