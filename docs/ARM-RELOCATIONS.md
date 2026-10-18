# ARM REL relocation core

Реализован `vita/src/arm_reloc.c`, подключён к host и Vita CMake targets. Он работает с уже подготовленным буфером, а не сам загружает ELF: caller обязан удостовериться, что каждый адрес относится к допустимому LOAD, обеспечить доступ к записи и сформировать проверенные символы. Offsets задаются относительно начала буфера, bias — разность загруженного и исходного виртуального адреса. Thumb flag передаётся отдельно от S. Это отдельный проверяемый компонент; so_util пока не подключён.

Поддерживаются Elf32_Rel: NONE (0), ABS32 (2), GLOB_DAT (21), JUMP_SLOT (22), RELATIVE (23, только symbol index zero). Выражения вычисляются modulo 2^32. Для ABS32/GLOB_DAT используется S+A с Thumb bit; для REL JUMP_SLOT A=0; для RELATIVE — bias+A. Неизвестные relocations, несогласованный RELATIVE, отсутствующие символы, невыравненные/вышедшие за буфер места и повторяющиеся места отклоняются.

Все будущие записи сначала подготавливаются отдельно, а затем применяются. При ошибке буфер остаётся неизменным; требуется внешняя синхронизация, это не атомарность относительно одновременно исполняющихся потоков. Входные записи не должны перекрывать image. Повторное применение к уже исправленному image запрещено: REL addends берутся из исходного содержимого.

Спецификация: [ARM ELF ABI](https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst), relocation codes и dynamic relocations. Поддержки TLS/IRELATIVE/COPY, Android packed relocations, RELA/RELR, symbol versioning и lazy binding здесь нет; такие записи нельзя пропускать молча.

## Проверки на реальном APK

Авторский C-код собран в host DLL для проверки, затем сравнен с независимым вычислением Python. Проверены **317859 R_ARM_RELATIVE записей из всех 10 библиотек Special Edition 1.0.12**, с symbol index zero. Каждый target проверен на принадлежность LOAD. Совпало всё итоговое содержимое каждого image, не только исправленные слова; SHA-256 результатов и число записей по модулям в `reports/special-1.0.12/relocation-verification.json`.

Использован условный bias 0x10000000 для арифметического теста. Это НЕ адрес, выбранный для реального загрузчика Vita. Никакие инструкции Android, constructors, JNI_OnLoad и callbacks не выполнялись. Остальные поддержанные типы проверены синтетическими C-тестами, не полным связыванием APK с runtime.

```sh
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
gcc -std=c99 -Wall -Wextra -Werror -shared -Ivita/include vita/src/arm_reloc.c -o build-host/arm_reloc_host.dll
python tools/verify_relative_relocs.py --host-library build-host/arm_reloc_host.dll --modules local-input/1.0.12 --out reports/special-1.0.12/relocation-verification.json
```

На Linux shared library собирать с `-fPIC -shared` и расширением .so. Android modules предоставляются пользователем локально; в архив проекта они не включены. Host DLL также не включена.

Результат: три CTest targets PASS (touch, JNI registry, ARM relocations); структурные/Python tests прошлого этапа — 26 PASS. Для перехода к настоящему исполнению остаются ELF dynamic parser, выделение памяти/cache sync на Vita, 451 обязательный внешний импорт, JNI/Unity lifecycle, графика и проверка на устройстве. Успех арифметики не подтверждает запуск игры.
