# Shadow Fight 2 Special Edition 3.0.3 — инженерный проект

**Обновление: текущая цель — предоставленная Special Edition 1.0.12 ARMv7.** См. `docs/SPECIAL-1.0.12-NEXT.md` и `reports/special-1.0.12/SUMMARY.md`. Ниже сохранена история исходного аудита 3.0.3; её ARM64-вердикт не относится к 1.0.12. Новый маршрут анализа: `python tools/prepare_audit.py "game.apk" --out reports/new-audit`.

**Следующий этап реализован:** C-проверка ELF, host CMake/CTest и строгий отчёт зависимостей. Подробности: `docs/PROGRESS-RUNTIME-GATE.md`. Архив пересобирается командой `python tools/package_project.py` без игровых бинарников.

Добавлен планировщик относительных модульных диапазонов с проверкой выравнивания и пересечений: `docs/MEMORY-PLAN.md`. Фактическая раскладка 1.0.12 занимает 54,24 МиБ без runtime heap/graphics/stacks. Всего 26 Python tests PASS.

Реализовано ядро ARM REL relocations: `docs/ARM-RELOCATIONS.md`. C-результаты 317859 RELATIVE записей реального APK совпали с независимым Python-расчётом; инструкции игры не исполнялись.

Чтение PT_DYNAMIC без section headers реализовано и связано с C-проверкой относительных релокаций: `docs/DYNAMIC-PARSER.md`. Все 10 библиотек прошли сравнение двух методов анализа; 33 Python tests PASS.

PT_DYNAMIC parser перенесён в C и подключён к Vita target: `docs/C-DYNAMIC-PARSER.md`. Все 333366 сырых relocation records совпали с Python; 41 Python tests PASS. Resolver и запуск Unity ещё не реализованы.

Добавлен C pipeline создания data image с resolver callback: `docs/LINK-PIPELINE.md`. 51 Python tests PASS; все 10 настоящих модулей корректно блокируются при пустом resolver. Сам callback-интерфейс готов; Android adapters и полный symbol lookup/runtime ещё не реализованы.

Аудит выполнен 18 октября 2026 года над реально доступным вложением. **Вердикт: данный APK непригоден для прямого Android2Vita/so-loader: только AArch64, нет ARMv7.** Это исследовательский проект, не готовый порт. APK и извлечённые игровые данные в архив проекта не включены.

## Состав

- `reports/apk-report.json`: фактический манифест, ELF-зависимости, импорты/экспорты, JNI-экспорты, релокации, строки, перечень assets и DEX.
- `reports/AUDIT.md`: выводы и ограничения для конкретного файла.
- `docs/DESIGN.md`: архитектура, этапы интеграции, graceful fallback, критерии приёмки.
- `tools/analyze_apk.py`: автономный анализатор на стандартной библиотеке Python 3.10+, без выполнения Android-кода.
- `vita/`: диагностический C/CMake-каркас VitaSDK с vitaGL и примером multitouch; настоящая загрузка игры намеренно не реализована.
- `tests/`: проверка последовательности touch и проверки анализатора.

## Анализ APK

Из корня распакованного проекта:

```powershell
python tools/analyze_apk.py "C:\путь\игра.apk" --out reports/my-report.json
python tools/analyze_apk.py "C:\путь\игра.apk" --out reports/my-report.json --extract extracted-new
python -m unittest discover -s tests -v
```

На Linux та же команда с Linux-путём. Первоначальное `/mnt/data/...apk` — путь другого окружения; в этой сессии файл получен из временного каталога вложения чата. Скрипт принимает любой реальный путь, не зависит от него.

Код возврата: 0 — анализ завершён без ошибок парсинга (это НЕ подтверждение совместимости); 1 — JSON создан с ошибками отдельных элементов; 2 — общая ошибка. Распаковка требует нового каталога, ограничена 2 GiB и не использует ZIP-имена как команды. Перезапись существующего каталога запрещена; при ошибке может остаться частичная распаковка. Враждебные APK следует анализировать в изолированной среде с ограничением RAM/CPU: это исследовательский парсер, не промышленная песочница.

## Диагностическая сборка Vita

Нужны установленный VitaSDK, CMake, генератор Make/Ninja, vitaGL и его зависимости (в том числе vitaShaRK, taiHEN и shader compiler stub). Следуйте инструкциям текущей версии vitaGL; набор статических зависимостей может отличаться. Исходники сторонних библиотек здесь не поставляются. Для будущего ARMv7 so-loader дополнительно нужны согласованные версии so_util и kubridge, ARM softfp-совместимые адаптеры. Не смешивайте float ABI библиотек.

В среде Linux/WSL с настроенным SDK:

```sh
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"
cmake -S vita -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

Ожидаемый артефакт — `build/sf2-diagnostic.vpk`; это пустой графический экран и журнал ввода `ux0:data/sf2vita-input.log`, START завершает приложение. Для установки используйте обычный установщик VPK на своей настроенной Vita. kubridge не требуется этому диагностическому экрану; для реального so_util с RWX/cache — требуется. Инициализация vitaGL выполняет графическую настройку поверх GXM; kubridge графику не инициализирует.

**Сборка VPK здесь не выполнена: VitaSDK и vitaGL в окружении не установлены.** Проверены анализатор на исходном APK и переносимое ядро управления компилятором host GCC. Каркас не выдаётся за проверенную на устройстве сборку. При изменении vitaGL сверяйте CMake link dependencies с её официальными примерами.

Проверка touch на host:

```sh
gcc -std=c99 -Wall -Wextra -Werror -Ivita/include vita/src/touch.c tests/touch_test.c -lm -o touch_test
./touch_test
```

## Первичные источники

- Android ABI: https://developer.android.com/ndk/guides/abis
- VitaSDK: https://vitasdk.org/ и https://github.com/vitasdk/samples
- so_util API: https://github.com/Rinnegatamante/so_util/blob/master/so_util.h
- vitaGL: https://github.com/Rinnegatamante/vitaGL
- Unity IL2CPP: https://docs.unity.com/en-us/engine/6000.6/manual/programming-environment/scripting-backends/il2cpp/il2cpp-introduction

Источники проверены при подготовке; ветки master изменяются. Для воспроизводимой интеграции сохраните точные commit SHA SDK, so_util, kubridge, vitaGL и параметры сборки. Репозиторий не изменяет DRM, лицензии, покупки или подпись APK.
