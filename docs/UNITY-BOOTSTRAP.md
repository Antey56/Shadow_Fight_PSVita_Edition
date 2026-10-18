# Unity 2020.3.48f1: registration and early JNI evidence

Исследован оригинальный libunity.so из предоставленного Special Edition 1.0.12. SHA-256: 82e24ba84e18a85beb7b81f221d4405b8f3befd94fc7025d8b8282c8d1d9535b. Файл не исполнялся и не изменялся.

scan_native_tables.py сопоставляет точные пары name/descriptor из DEX с тройками указателей, для которых существуют три R_ARM_RELATIVE relocation. Function pointer должен попадать в file-backed executable PT_LOAD; младший бит сохраняется для Thumb. Найдена 51 кандидатная запись во всех десяти библиотеках, из них 49 в libunity. Эти проверки сами по себе не подтверждают class receiver или факт регистрации.

## Подтверждённые группы

Ручной просмотр Thumb disassembly JNI_OnLoad (0x26ca19, code VA 0x26ca18) установил FindClass → RegisterNatives в трёх helpers. Слот RegisterNatives загружается с offset 860, то есть JNI index 215 на ARM32. PC-relative literals и косвенные строки классов связывают следующие таблицы:

| Class | Table VA | Count | Helper code VA |
|---|---|---|---|
| com/unity3d/player/UnityPlayer | 0xc91e4c | 25 | 0x26ca80 |
| com/google/androidgamesdk/ChoreographerCallback | 0xc91f78 | 1 | 0x26cac4 |
| com/google/androidgamesdk/SwappyDisplayManager | 0xc91f84 | 2 | 0x26cb08 |

В UnityPlayer найдены initJni 0x26c26d, nativeRender 0x26c465, nativePause 0x26c305, nativeResume 0x26c337 и nativeInjectEvent 0x26c495. Это link-time VA/Thumb pointers, а не готовые адреса Vita. После загрузки нужен корректный load bias; вызывать эти значения напрямую нельзя.

audit_unity_bootstrap.py — профиль именно этого бинарника. Он отказывает при любом несовпадении SHA-256, затем заново вычисляет PC-relative адреса и проверяет строки классов, границы таблиц и совпадение каждой записи с DEX. Профиль не является автоматическим универсальным дизассемблером и не подтверждает остальные регистрации, на которые JNI_OnLoad тоже ссылается.

```sh
python tools/scan_native_tables.py local-input/1.0.12 --audit reports/special-1.0.12/deep-audit.json --out reports/special-1.0.12/native-table-candidates.json
python tools/audit_unity_bootstrap.py local-input/1.0.12/libunity.so --audit reports/special-1.0.12/deep-audit.json --out reports/special-1.0.12/unity-bootstrap.json
```

Для независимого просмотра использовать arm-vita-eabi-objdump -d -M force-thumb с start/stop-address: 0x26ca18..0x26cb4c, 0x26c26c..0x26c2a6 и 0x25cb90..0x25cc4c. Имена ближайших экспортов в выводе objdump не являются настоящими именами stripped helpers; ориентироваться на адреса и инструкции. Не интерпретировать literal pools после конца функции как код.

## Почему текущего JNI недостаточно

initJni вызывает GetJavaVM (219), затем helper 0x25cb90. В helper присутствует GetStaticMethodID: offset 452/index 113. Восстановлены строки java/lang/ClassLoader, getClassLoader, ()Ljava/lang/ClassLoader;, forName и (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;.

Также наблюдается GetObjectClass, применённый к class object. Наш ограниченный bridge поддерживает GetObjectClass только для Context/AssetManager и не создаёт java/lang/Class: даже реализованный номер слота ещё не означает нужную семантику. JNI_OnLoad требует UnityPlayer и Android Game SDK классы и RegisterNatives, которых текущая таблица ещё не предоставляет. FatalError (18) встречается в failure ветках.

Следовательно, передача текущего JNIEnv в настоящий JNI_OnLoad привела бы к отсутствующим методам или NULL slot calls. Запуск отключён по реальной зависимости, а не только из-за отсутствия теста. Требуются классы/методы reflection, статические вызовы, registry ABI, strings/arrays и полноценный потоковый контракт. Нельзя заменять их массовым успешным return 0.
