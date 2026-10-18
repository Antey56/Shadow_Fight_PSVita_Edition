# C pipeline подготовки связанного image

`vita/src/arm_link.c` связывает PT_DYNAMIC parser, формирование LOAD image, symbol resolver callback и transactional ARM REL core. Код включён в Vita target, но игровой цикл его пока не вызывает.

API `arm_link_image` получает исходные file bytes, условный runtime_start, лимит image и callback, возвращающий **32-битный целевой адрес** символа. Host pointer в callback возвращать нельзя. Создаётся только обычный data buffer, без executable permissions, constructors, cache flush и JNI. Загруженные Android инструкции не вызываются. Caller обязан обеспечить отсутствие пересечений входных/выходных структур и передавать пустой result; для освобождения применяется `arm_link_dispose` из того же модуля/CRT.

Обязательные global/default-visible ссылки, включая определения текущего модуля, передаются resolver: это позволяет последующему реализованному resolver выбирать требуемый ELF scope. Local/hidden/internal/protected definitions используют текущий модуль. Определения проверяются на принадлежность LOAD (ABS разрешены отдельно); TLS/IFUNC/COMMON и неизвестные bindings/types отклоняются. Unresolved undefined weak получает ноль; это политика подготовки данных, не разрешение вызывать нулевой weak function pointer. R_ARM_NONE/symbol-zero обрабатываются отдельно.

При отсутствии обязательного символа возвращается -20 с именем (до 127 байт) и индексом relocation; image не выдаётся, временные allocations освобождаются. Callback error → -21. Invalid definition → -14; hard-float → -15; invalid budget/overflow → -11; inconsistent mapping alignment → -13. Ошибки parser/relocation core также передаются, поэтому их числовые значения надо интерпретировать в контексте этапа. Resolver должен быть детерминированным и не производить внешних эффектов: rollback его побочных эффектов не реализован.

## Проверки

- 51 Python test PASS, включая десять новых проверок C pipeline: RELATIVE, внешний Thumb symbol, undefined weak, local definition, missing required, callback error, invalid definition, runtime overflow, budget и alignment.
- Все 10 реальных библиотек переданы pipeline с пустым resolver и условным runtime_start 0x10000000. **Каждая остановилась на -20**, имя первой отсутствующей обязательной функции записано в `reports/special-1.0.12/link-blocked-verification.json`. Ни одна библиотека не выдала image и не исполнялась.
- Host сборка GCC Wall/Wextra/Werror прошла. Это не Vita runtime проверка.

## Что остаётся

Смысл callback не заменяет реализации 451 обязательного внешнего импорта. Полная symbol visibility/versioning, lookup scope, Android bionic ABI, pthread/TLS, JVM/JNI и Unity lifecycle пока отсутствуют. Адреса целевой Vita памяти нужно получить отдельно. Size/alignment checks и арифметика подготовленного буфера не дают права запускать его как игру.

Следующий практический этап — согласованный каталог адаптеров и dependency lookup с явным статусом каждой реализации. Пока поставлять фиктивные function addresses или universal return-zero stubs было бы неверно; этот pipeline возвращает точную ошибку вместо такого поведения.
