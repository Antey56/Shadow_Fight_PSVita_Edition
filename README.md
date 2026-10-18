# Shadow Fight PSVita Edition — research

**Весь этот проект навайбкожен с помощью Codex (OpenAI).** Это экспериментальная работа человека и ИИ: счётчики тестов и наличие VPK не означают готовность игрового порта. На реальной PS Vita приложение пока не проверено.

## Текущий результат

- **Собран диагностический VPK 0.5.** Бинарный файл пока хранится локально и не опубликован; исходники сборки доступны в этом репозитории.
- Это проверка графики vitaGL, физических контролов, журнала ввода и небольших внутренних self-checks. **Shadow Fight / Unity не запускаются.**
- Цель исследования: Shadow Fight 2 Special Edition **1.0.12 / 30000019**, ARMv7, Unity 2020.3.48f1.
- Реализованы C-проверка ELF, PT_DYNAMIC parser, ARM REL relocation core и подготовка data image с resolver callback.
- 161 Python/integration test и 8 CTest targets проходят локально. 333366 записей релокаций реального APK совпали с независимым анализом; арифметика 317859 RELATIVE записей проверена отдельно.
- 451 уникальный обязательный внешний импорт пока требует реализации/проверки. Android bionic/JNI adapters, Unity lifecycle и запуск игры ещё впереди.

APK, игровые .so, assets, DRM/лицензионные патчи и установленный SDK в этот репозиторий не входят. Имена импортов, манифест и технические отчёты получены статическим анализом предоставленных APK. Подпись/происхождение этих APK не сертифицированы.

## Проверка VPK на своей Vita

Нужна настроенная для homebrew Vita и `libshacccg.suprx`, необходимая vitaGL. После собственной сборки установить `build-vita/sf2-diagnostic.vpk` через обычный установщик VPK. Title ID: `SF2AUD001`. При старте также выполняется JNI/JavaVM/memory probe без игровых данных. Ожидаются надпись `UNITY NOT STARTED`, статус self-check, подсветка виртуальных touch-точек при движении левого стика/крестовины и нажатиях CROSS/SQUARE/TRIANGLE/CIRCLE. START завершает приложение.

Журнал: `ux0:data/sf2vita-input.log`; каждый запуск перезаписывает только этот диагностический журнал. Игровые данные не нужны. Эта инструкция описывает ожидаемое поведение собранного приложения; аппаратная проверка остаётся необходимой.

## Сборка и тесты на компьютере

```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
python -m unittest discover -s tests -v
```

На Windows указать генератор `-G "MinGW Makefiles"` или Ninja. На Linux shared libraries имеют расширение .so. Тесты C-интеграции требуют предварительной host сборки; без неё будут явно skipped. GitHub Actions выполняет эти проверки без APK и без SDK.

## Сборка VPK

VitaSDK 2026.08, GCC/G++ 15.2.0, vitaGL, vitaShaRK, SceShaccCgExt, mathneon и taihen. C++ linker нужен для C++-частей vitaGL, хотя наш код написан на C.

```sh
vdpm install --noconfirm vitaGL vitaShaRK taihen
cmake -S vita -B build-vita -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-vita -j2
```

На Windows также можно: `./tools/build_vita.ps1 -SdkPath C:/path/to/vitasdk`. Результат: `build-vita/sf2-diagnostic.vpk`. Историческая сборка 0.2 и её SHA-256 — [отчёт сборки](reports/VITA-BUILD.md). Текущие исходники собирают диагностическую версию 0.5; бинарники пока остаются локальными. Текущий диагностический VPK собран в обычном SDK ABI; будущая интеграция Android softfp потребует согласованного toolchain/адаптеров. Диагностическую сборку нельзя считать подтверждением ABI игры.

## Анализ собственного APK

```sh
python tools/prepare_audit.py game.apk --out reports/my-audit
python tools/check_runtime_gate.py reports/my-audit/apk-report.json --out reports/my-audit/runtime-gate.json
python tools/plan_memory.py reports/my-audit/apk-report.json --budget-mib 64 --out reports/my-audit/memory-plan.json
```

Runtime gate сейчас возвращает код 1 намеренно: запуск не разрешён. Бюджет 64 MiB — условный параметр расчёта, не измерение памяти Vita. В GitHub-копии APK-отчёты публикуются как читаемые технические сводки по разрешённым полям. Произвольные ELF/DEX-строки, metadata/tokens, полный список exports и asset paths исключены. Полные отчёты остаются в локальном архиве и могут быть сформированы анализатором из собственного APK.

## Документы

- [Технический проект](docs/DESIGN.md)
- [Аудит Special Edition 1.0.12](docs/SPECIAL-1.0.12-NEXT.md)
- [Раскладка памяти](docs/MEMORY-PLAN.md)
- [Релокации](docs/ARM-RELOCATIONS.md)
- [C PT_DYNAMIC parser](docs/C-DYNAMIC-PARSER.md)
- [C link pipeline](docs/LINK-PIPELINE.md)
- [Порядок разрешения символов](docs/SYMBOL-SCOPE.md)
- [Первый Android logging backend](docs/ANDROID-LOGGING.md)
- [Чтение Android assets](docs/ANDROID-ASSETS.md)
- [JNI: аудит и подготовка регистрации](docs/JNI-SURFACE.md)
- [Модель Context и AssetManager](docs/JNI-OBJECTS.md)
- [Ограниченная таблица JNIEnv](docs/JNI-BRIDGE.md)
- [JavaVM и привязка потока](docs/JNI-VM.md)
- [Объекты ожидающих исключений](docs/JNI-EXCEPTIONS.md)
- [Реальные регистрации Unity](docs/UNITY-BOOTSTRAP.md)
- [Статус шести направлений интеграции](docs/INTEGRATION-STATUS.md)
- [Регистрация Unity, строки и reflection](docs/JNI-REGISTRATION-REFLECTION.md)
- [Участники и роль ИИ](CONTRIBUTORS.md)
- [Исходники и лицензии зависимостей](THIRD_PARTY.md)

Код проекта предоставляется без гарантий; детали лицензий сторонних компонентов сохранены отдельно. Shadow Fight, NEKKI и Unity — собственность соответствующих правообладателей; проект не является их официальным продуктом.
