# Чтение PT_DYNAMIC без section headers

Добавлен `tools/elf_dynamic.py`: отдельный bounded ARM32 ET_DYN parser для program headers, LOAD, PT_DYNAMIC, DT_NEEDED, DT_STRTAB/STRSZ, DT_SYMTAB/SYMENT, DT_REL/RELSZ/RELENT и DT_JMPREL/PLTRELSZ/PLTREL. Это host Python-компонент подготовки, не C-парсер Vita и не самостоятельный загрузчик.

Проверяет границы файловых данных, уникальность file-backed virtual ranges, пересечения LOAD, соответствие offset/vaddr DYNAMIC, DT_NULL, повторные singleton tags, строки, stride таблиц, выравнивание/принадлежность relocation target и повторяющиеся места записи. Неизвестные relocation type остаются явно указанными числом для последующего отклонения ядром; packed Android REL/RELA, RELA и RELR отклоняются. Требуется ровно один PT_DYNAMIC. Ошибки завершают подготовку вместо пропуска записей.

Символы извлекаются только для индексов, на которые ссылаются relocations; полного списка exports, symbol versioning и ELF lookup scopes этот компонент не предоставляет. Таблица символов ограничивается file-backed диапазоном LOAD, отдельная полная валидация числа символов через SYSV/GNU HASH пока не реализована. Поэтому результат не разрешает исполнение.

## Фактическая проверка

Все 10 библиотек 1.0.12 разобраны. DT_NEEDED и распределение relocation types совпали с независимым section-based анализатором. Дополнительно в локальных копиях обнулены e_shoff/e_shentsize/e_shnum/e_shstrndx; результаты PT_DYNAMIC не изменились. Оригинальные файлы не изменялись. Сводка — `reports/special-1.0.12/dynamic-verification.json`.

Путь проверки теперь связан: PT_DYNAMIC → список относительных relocations → C `arm_reloc_apply` → побайтовое сравнение с Python oracle, который независимо читает section headers. Проверены 317859 RELATIVE записей. Android инструкции, constructors и JNI не запускались.

```sh
python tools/elf_dynamic.py local-input/1.0.12/libmain.so --out reports/libmain-dynamic.json
python -m unittest discover -s tests -v
```

33 Python tests PASS, включая отсутствующий DT_NULL, неверный указатель, плохую relocation target, duplicate tag, truncated ELF и packed format. Host/Vita исполнение Unity, полное связывание внешних символов и первый кадр остаются непроверенными. Следующий реальный интеграционный этап — bounded C-парсер и typed symbol resolver с реализациями runtime adapters; Python prototype не заменяет их.
