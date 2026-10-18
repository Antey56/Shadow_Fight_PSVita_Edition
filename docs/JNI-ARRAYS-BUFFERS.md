# Primitive arrays and borrowed direct buffers

Добавлены 44 JNI entries: GetArrayLength (171), конструкторы восьми примитивных массивов (175–182), Elements/Release (183–198), Region Get/Set (199–214) и direct ByteBuffer API (229–231). Всего 70 непустых slots. Индексы и контракты сверены с [JNI specification](https://docs.oracle.com/en/java/javase/26/docs/specs/jni/functions.html). Смещения групп и direct-buffer functions проверяются при компиляции.

## Массивы

Поддерживаются boolean/byte/char/short/int/long/float/double. Нативные ширины — 1/1/2/2/4/8/4/8 байт; boolean хранится по байту, без битовой упаковки. Новый массив заполнен нулями. Длина — signed 32-bit jsize; отрицательная длина отклоняется. Один массив ограничен 1 MiB, сумма уникальных backing allocations одной среды — 8 MiB. Проверка длины выполняется до умножения, переполнение не допускается. Копия local/global ref разделяет refcounted backing и не расходует бюджет второй раз.

Get/SetRegion проверяют тип, start/count и диапазон через count <= length-start. Никаких частичных записей при ошибке. Нулевой region в конце массива разрешён; NULL buffer принимается только для нулевого count. Ошибки выставляют синтетическое pending exception, а не настоящие Java exception classes. Array GetObjectClass и FindClass для восьми примитивных descriptors поддержаны; полная Java array class hierarchy отсутствует.

GetArrayElements всегда создаёт copy и устанавливает isCopy=true. Поддерживается максимум 16 одновременно выданных копий, с общим бюджетом 4 MiB. Release сверяет pointer, тип и identity массива. Режим 0 копирует изменения назад и освобождает copy; JNI_COMMIT=1 копирует назад и сохраняет copy для дальнейшего Release; JNI_ABORT=2 освобождает без записи. Неверный owner, mode или stale pointer отклоняется. Release разрешён при pending error для cleanup.

Все leases должны быть завершены до detach. Explicit local cleanup/detach освобождает оставшиеся копии без copyback; старые pointers становятся недействительными. Уничтожение среды очищает оставшуюся память. Это однопоточная модель: нет совместных Java worker arrays или GC pinning. GetPrimitiveArrayCritical/ReleasePrimitiveArrayCritical пока не поддержаны и остаются NULL slots; обычные Elements не следует выдавать за critical API.

Object arrays, проверка Java ArrayStoreException и class assignability пока не реализованы. Этот этап не закрывает ReflectionHelper.invoke с Object[] и не разрешает запуск Unity.

## DirectByteBuffer

NewDirectByteBuffer сохраняет **заимствованный** адрес и capacity, не копирует и не освобождает память. Адрес должен быть ненулевым и оставаться доступным до завершения всех операций с объектом; caller отвечает за readable/writable region. Capacity ограничена 0..INT32_MAX; проверяется переполнение address+capacity. Действительность ОС-отображения памяти не проверяется.

GetDirectBufferAddress/Capacity возвращают сохранённые значения. Для валидного объекта другого типа возвращаются NULL/-1 без ошибки; неверный reference отклоняется. Local/global copies разделяют metadata. Удаление ByteBuffer не освобождает пользовательский PCM buffer. Никакие Java ByteBuffer методы, position/limit/order views или Java byte-order операции не реализованы: передаются только native memory и capacity. Синтетический GetObjectClass возвращает DirectByteBuffer class token, а не полную Android class implementation.

Эти функции нужны для дальнейшего исследования FMODAudioDevice JNI; звукового вывода, audio thread и fmodProcess запуска здесь нет.

## Проверки

22 новых host integration tests покрывают все восемь типов, значения/ширины, copyback/COMMIT/ABORT, границы и большие индексы, ошибочный тип, ref lifetime, бюджеты и direct memory aliasing. CTest дополнительно проверяет 64-битные массивы и direct-buffer capacity через typed JNI function pointers.

Диагностика Vita 0.6 содержит IntArray + COMMIT + readback и direct ByteBuffer probe поверх собственного stack buffer; он жив до уничтожения среды. Аппаратного запуска ещё нет. Проверены 183 Python tests и 8 CTest targets, Vita cross-build успешен. ARM guest code и Unity не исполнялись. Игровой port, первый кадр, звук и бой этим результатом не подтверждаются.
