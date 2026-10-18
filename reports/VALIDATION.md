# Проверки, 18 октября 2026

- Python 3.12.10: полный анализ исходного APK завершён с exit 0, `errors=[]`; SHA-256 и ABI записаны в JSON.
- 4 unittest: plain XML manifest, минимальный ELF32 ARM, отклонение ZIP traversal, фиксация испорченного ELF в JSON — PASS.
- Host GCC: `-std=c99 -Wall -Wextra -Werror`, touch.c + touch_test.c — PASS.
- Исполнение host touch test: `touch sequencing PASS`.
- Vita VPK/устройство: не проверены. VITASDK указывает на C:/vitasdk, но arm-vita-eabi-gcc.exe и vitaGL.h по ожидаемым путям отсутствуют; полноценная установленная среда сборки недоступна.

Ограничения парсера: бинарный XML декодируется в плоский список start-tags; ancestry, resource references и значения resources.arsc не разрешаются. DEX — только string table; не декомпиляция методов. ELF symbols/dynamic/relocations читаются через section headers; extended section numbering, Android packed relocations и RELR не декодируются. Список обычных relocations не доказывает отсутствие packed relocations. ARM attributes не интерпретируются: для будущей ARMv7 сборки нужен `readelf -h -A -l -d -Ws -r` и отдельная проверка TLS/ABI. Assets классифицируются по имени, bundles не распаковываются, зашифрованные ресурсы не декодируются. Подпись APK не проверяется. Анализатор не является сертификатором совместимости.
