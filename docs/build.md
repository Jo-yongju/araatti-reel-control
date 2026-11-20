# 다른 PC에서 빌드하기

## 준비

- Windows 10/11, Windows PowerShell 5.1 이상
- STM32CubeIDE 1.17.0, 설치에 포함된 GNU Tools for STM32 12.3.rel1
- 저장소 전체 ZIP 또는 Git clone

공식 설치 페이지: https://www.st.com/en/development-tools/stm32cubeide.html

MCSDK 6.4.2 생성 코드와 빌드용 소스가 포함되어 있습니다. 현재 소스를 빌드하는 데 Workbench 설치·코드 재생성·Python은 필요하지 않습니다. 모터 설정을 바꾸고 다시 생성하려면 해당 버전의 Workbench가 필요합니다.

## 한 번에 빌드

ZIP을 전체 압축 해제하고 저장소 루트에서 실행합니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-Firmware.ps1
```

기본값은 두 프로젝트의 Debug 빌드입니다. 각 실행은 `build/ide-.../`에 새 워크스페이스와 로그를 만듭니다. 설치 위치를 프로젝트에 저장하지 않습니다. 컴파일러 실행, 빌드 로그와 새 ELF 생성을 모두 확인하며 플래시는 수행하지 않습니다. 같은 프로젝트를 IDE에서 동시에 빌드하지 마세요.

프로젝트·구성·설치 경로를 지정할 수도 있습니다.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-Firmware.ps1 -Project foc -Configuration Release
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-Firmware.ps1 -Project encoder -CubeIdePath 'D:\Tools\STM32CubeIDE'
```

설치 경로는 `headless-build.bat`이 있는 폴더입니다. 기본 설치 위치 `C:\ST\STM32CubeIDE_1.17.0\STM32CubeIDE`는 자동으로 찾습니다.

## IDE에서 가져올 위치

File → Import → General → Existing Projects into Workspace를 사용합니다.

| 용도 | 가져올 폴더 | 프로젝트 이름 |
|---|---|---|
| 엔코더 하드웨어 시험 | `firmware/encoder_test` | `ffb_reel_g431` |
| FOC 낚시릴 제어 | `foc/STM32CubeIDE` | `bl4270e1k` |

FOC는 상위 `foc/App`, `foc/Src`, `foc/Inc`, `foc/Drivers`, `foc/MCSDK_v6.4.2-Full`을 상대 경로로 참조합니다. 전체 구조를 유지하고 **Copy projects into workspace는 선택하지 않습니다.** `STM32CubeIDE` 폴더만 따로 옮기면 참조가 끊깁니다.

결과 ELF는 각 프로젝트 폴더의 `Debug` 또는 `Release` 아래에 생성됩니다. 엔코더 시험 ELF와 실제 FOC ELF는 기능이 다릅니다.

## 검증 기록

2026-11-20, Windows / CubeIDE 1.17.0 / ARM GCC 12.3.1. 각 구성을 클린 빌드했습니다.

| 프로젝트 | 구성 | 오류 / 경고 | ELF text / data / bss (bytes) |
|---|---|---|---|
| 엔코더 | Debug | 0 / 0 | 7772 / 12 / 1668 |
| 엔코더 | Release | 0 / 0 | 4836 / 12 / 1668 |
| FOC | Debug | 0 / 0 | 42708 / 1068 / 7856 |
| FOC | Release | 0 / 0 | 42708 / 1068 / 7856 |

엔코더 Release 구성에서 빠져 있던 `HW_Test/Src`를 빌드 대상에 추가했습니다. FOC의 두 구성은 원본 설정을 유지하며 최적화 수준이 같아 위 ELF 크기가 같습니다. 모터 제어 로직은 변경하지 않았습니다. 빌드 통과는 실제 모터·전력 보드의 동작 검증을 대신하지 않습니다.
