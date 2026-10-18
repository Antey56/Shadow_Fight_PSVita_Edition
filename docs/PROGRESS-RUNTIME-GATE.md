# Реализован этап structural ELF preflight

18 октября 2026. C-модуль `vita/src/elf_preflight.c` читает файл без выполнения его кода, проверяет ELF32/LE/EM_ARM, ET_DYN, версии заголовка, размер program-header table, границы файловых сегментов, memsz/filesz, выравнивание и переполнение виртуального диапазона. Hard-float flag отклоняется. Выводит LOAD span, флаги, признаки TLS и W+X. Успех — только структурная пригодность для следующих проверок.

Все 10 настоящих библиотек 1.0.12 прошли этот этап: flags 0x05000200, по два LOAD, PT_TLS не найден, W+X LOAD не найден. Полный вывод в `reports/special-1.0.12/elf-preflight.txt`. Отсутствие PT_TLS не доказывает отсутствие pthread TLS или Android runtime TLS.

Инспектор подключён к Vita CMake и вызывается из существующего `sf2_preflight`, который по-прежнему не разрешает запуск до runtime-сертификации. Проверка сегментных пересечений, reserved address map, полной семантики динамических таблиц/relocations и constructors остаётся отдельным этапом. Это не ELF linker и не эмулятор.

## Runtime gate

`tools/check_runtime_gate.py` сравнивает импорты с экспортами упакованных библиотек и явно объявленными host adapters. Обнаружено 1123 обязательных импортных записи без адаптеров, 451 уникальное имя. Это точнее исходных 518 уникальных импортов: часть имён имеет кандидатов-поставщиков внутри APK. Кандидат ещё не доказывает видимость по ELF lookup scope и порядок загрузки. Результат хранится в `runtime-gate.json`.

По умолчанию никакие host symbols не объявлены реализованными. Даже заполненная таблица адаптеров не включает execution_allowed: нужно отдельно проверить ABI, поведение, JNI и runtime. Код возврата gate — 1 при сформированном отчёте, поскольку запуск пока блокируется; это ожидаемое состояние, не сбой инструмента.

## Повторяемая host сборка

Из корня sf2-vita, при настроенном GCC/Make:

```sh
cmake -S . -B build-host -G "MinGW Makefiles"
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
```

На Linux выбрать Unix Makefiles/Ninja вместо MinGW Makefiles. Host executable не запускает ARM-код: `build-host/elf_preflight module.so`. Python tests также умеют использовать предварительно собранный `tools/elf_preflight.exe`; без него семь structural tests будут явно skipped.

```sh
python tools/check_runtime_gate.py reports/special-1.0.12/apk-report.json --out reports/special-1.0.12/runtime-gate.json
python -m unittest discover -s tests -v
python tools/package_project.py
```

17 Python tests прошли без skips. CMake host targets собраны с Wall/Wextra/Werror; touch и native registry проверены CTest. Для будущей Vita сборки по-прежнему отсутствуют toolchain и vitaGL; VPK/первый кадр не подтверждены. APK и извлечённые `.so` сохранены только локально для тестов, упаковщик исключает их и host executables из архива.
