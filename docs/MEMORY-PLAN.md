# Предварительная раскладка модулей

Реализован `tools/plan_memory.py`, результат для Special Edition 1.0.12 находится в `reports/special-1.0.12/memory-plan.json`.

Планировщик выдаёт относительные смещения, учитывает page size, p_align каждого LOAD, ненулевую начальную ELF vaddr, переполнения 32-битного адресного диапазона и промежутки между модулями. Точные LOAD memory ranges внутри одного модуля не должны пересекаться; общая граничная страница без пересечения байтов допускается. Значения p_align теперь извлекаются анализатором вместе с offset, filesz и flags.

Для всех 10 библиотек запрошен **условный бюджет 64 МиБ**. Раскладка занимает 56872960 байт (54,24 МиБ), включая промежуток в одну страницу после каждого модуля. Размер выбран для проверки алгоритма и не означает, что Vita предоставит такой непрерывный диапазон. Реальные адреса не назначаются, память не резервируется, protection guard pages не создаются.

Heap Unity/IL2CPP, textures, framebuffer, stacks и Android services в расчёт не входят. Для реального загрузчика ещё нужны выделение свободного адресного пространства на устройстве, проверка пересечений с eboot/runtime/GPU, согласование адресов с so_util и relocation types, runtime import gate и JNI. Успех планирования никогда не включает execution_allowed.

```sh
python tools/prepare_audit.py game.apk --out reports/new-audit
python tools/plan_memory.py reports/new-audit/apk-report.json --budget-mib 64 --out reports/new-audit/memory-plan.json
```

Проверки: 26 Python tests PASS, включая перекрытия, лимит бюджета, overflow, неизвестную архитектуру, общий край страницы и выравнивание 64 KiB. Это host tests, не измерение доступной памяти Vita и не проверка игры на устройстве.
