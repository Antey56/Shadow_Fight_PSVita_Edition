# PT_DYNAMIC parser на C

`vita/src/arm_dynamic.c` и `vita/include/arm_dynamic.h` предоставляют bounded parser ARM32 ET_DYN для file bytes. Он подключён к Vita target и отдельной host shared library CMake. Android код не выполняется, модуль не отображается в executable memory.

API двухпроходный: сначала вызов `arm_dynamic_read(file,size,NULL,0,&info)`, затем выделение `info.relocation_count` записей `ArmRawRel` и повторный вызов с буфером. Между вызовами вход должен оставаться неизменным. Выходные буферы и info не должны перекрывать вход. На ошибке info обнуляется; частично заполненные выходные записи отбрасываются. Это не transactional image writer: image вообще не изменяется.

Проверяются заголовок ELF, ET_DYN/EM_ARM/LE, program tables, LOAD bounds/overlap/alignment, единственный file-backed PT_DYNAMIC, DT_NULL, singleton tags, STRTAB bounds, SYMENT, размеры REL/JMPREL, target bounds/alignment и referenced symbol/name bounds. Packed relocations, RELA/RELR, неизвестные types и RELATIVE с ненулевым symbol index отклоняются. Поддержаны NONE, ABS32, GLOB_DAT, JUMP_SLOT и symbol-zero RELATIVE.

C-парсер возвращает сырые vaddr/type/symbol_index, **не адреса разрешённых символов**. Для `arm_reloc_apply` по-прежнему нужен отдельный resolver и перевод target_vaddr в offset буфера. Bound check символов использует file-backed LOAD; сертификация размеров таблицы через GNU/SYSV HASH, symbol versioning и ELF search scope ещё не реализована. Дубликаты relocation targets должен отклонять следующий этап `arm_reloc_apply` (Python parser также отклоняет их). Hard-float flag проверяет `sf2_elf_inspect`, который следует вызвать до этого parser.

## Проверки

- **333366 relocation records из всех 10 библиотек APK** совпали с независимым Python PT_DYNAMIC parser: target, type, symbol_index и порядок.
- В копиях файлов обнулены section-header metadata; C-результат не изменился.
- Результаты: `reports/special-1.0.12/c-dynamic-verification.json`.
- **41 Python tests PASS**, включая восемь тестов C через ctypes: корректный ELF, truncated file, unknown type, symbol bounds, missing DT_NULL, nonzero-symbol RELATIVE, insufficient capacity и packed encoding.
- Host GCC компилирует код с Wall/Wextra/Werror. Три CTest targets проходят.

```sh
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
python tools/verify_dynamic_c.py --host-library build-host/arm_dynamic_host.dll --modules local-input/1.0.12 --out reports/special-1.0.12/c-dynamic-verification.json
python -m unittest discover -s tests -v
```

На Linux CMake создаст `.so`; указать его в --host-library. Тест Python для C пока ориентирован на путь Windows DLL; при отсутствии он явно skipped. Проверка не является полной защитой парсера от произвольных враждебных файлов и не заменяет sandbox.

Исполнение Unity, constructors, JNI_OnLoad и графика на Vita по-прежнему не проверены. Этот этап делает parser доступным C-загрузчику, а не решает 451 внешний импорт и JNI runtime.
