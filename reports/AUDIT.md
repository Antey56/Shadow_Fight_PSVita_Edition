# Аудит предоставленного APK

## Идентификация

Имя: `[APK][Mod]_shadow-fight-2-special-edition_v.3.0.3_[androidapplications.store]_2.apk`.

Размер: 193570023 байта. SHA-256:
`762cf89afa05a7ece6a758c8503cb15c128d8faea5f7dfd10827457cf4987324`.

Манифест декларирует package `com.nekki.shadowfight2.paid`, versionName `3.0.3`, versionCode `30000097`, minSdk 24, targetSdk 35, compileSdk 36; GLES feature 0x20000 (2.0). Это декларации файла, не подтверждение происхождения и не гарантия реального набора GL-вызовов. APK не запускался, подпись и соответствие официальной сборке не проверялись.

## Стоп-фактор ABI

В ZIP 15 native-библиотек, все в `lib/arm64-v8a/`, ELF64 little endian, e_machine=183 (AArch64). `lib/armeabi-v7a/` отсутствует. Vita исполняет 32-битный ARM, so_util использует Elf32. Перемещение файлов, переименование каталога, смена ELF-заголовка, kubridge или GLES-обёртка этого не исправят. Требуется законно доступная ARMv7-сборка либо исходники для самостоятельной компиляции; ARM64-эмуляция/трансляция — отдельный исследовательский проект вне обычного so-loader.

## Движок и главная логика

| Библиотека | Размер файла, байты | Наблюдение |
|---|---:|---|
| libmain.so | 6728 | JNI_OnLoad, строки NativeLoader и libunity.so; bootstrap, не вся игра |
| libunity.so | 19018088 | Unity runtime, JNI_OnLoad/Unload; libandroid, libEGL, libmediandk |
| libil2cpp.so | 131851888 | AOT-код IL2CPP, JNI_OnLoad/Unload; вероятное место основной игровой логики |
| libbanzai-v8.so | 19583328 | дополнительная native-подсистема; роль требует исследования |
| libtocayuyu-aoc.so | 276152 | экспорт Java_aoc_androidoyunclub_apk_decryptAssets |

Остальные: `libcrashlytics-common.so`, `libcrashlytics-handler.so`, `libcrashlytics-trampoline.so`, `libcrashlytics.so`, `libdatastore_shared_counter.so`, `libFirebaseCppAnalytics.so`, `libFirebaseCppApp-12_10_1.so`, `libFirebaseCppCrashlytics.so`, `libFirebaseCppMessaging.so`, `lib_burst_generated.so`.

Для libil2cpp.so встречаются AArch64-релокации 1027 (948489), 257 (1631), 1025 (152), 1026 (584); эти записи не являются ARMv7 relocation set. Сумма виртуальных LOAD-span всех модулей — 180183819 байт, не измерение RAM: надо прибавлять heap, runtime, распакованные ресурсы, стек, framebuffer и GPU-объекты, учитывать общие страницы и стратегию загрузки. Нельзя объявлять бюджет памяти достаточным по размеру APK.

## Assets и JNI

376 файлов assets, 172847550 байт без ZIP-сжатия. Найдены 104 `.bundle`, Unity Addressables `assets/aa/catalog.json` (1525704) и `settings.json` (4414), `assets/bin/Data/boot.config`, `globalgamemanagers`, `globalgamemanagers.assets`, `Managed/Metadata/global-metadata.dat` (22916832). Отдельные текстуры не отражают все изображения: они могут находиться в bundles. По имени bundle нельзя установить формат GPU-текстур или расположение сохранений.

Файлы с точными именами users.xml/userdata среди assets не обнаружены. Каталог APK assets не является writable save-каталогом приложения. Нельзя заключать, что сохранений нет: на Android они создаются во внутреннем хранилище или через движок. В анализаторе config_candidate — эвристика имени/расширения, не декодирование содержимого.

Внешний `classes.dex` содержит только 159 строк; его `loadLibrary` и Application `aoc.androidoyunclub.apk`, библиотека с `decryptAssets` и hashed `.tmp` assets указывают на дополнительный сторонний bootstrap/упаковку. Это свидетельство изменённого запуска, не доказательство конкретного алгоритма шифрования, вредоносности или состава скрытых DEX. Ничего не расшифровывалось и не исполнялось. Упакованные Java-методы анализатор не восстанавливает.

Activity в манифесте: `com.google.firebase.MessagingUnityPlayerActivity`; присутствуют компоненты Facebook, Firebase, Google, разрешения `BILLING` и `CHECK_LICENSE`. Присутствие компонента/строки не доказывает, что он обязателен или вызывается в конкретном сценарии. JNI-экспорты приведены в JSON; динамическая регистрация RegisterNatives требует отдельного анализа вызовов и сигнатур. Отсутствие Java_* экспорта не доказывает отсутствие JNI.

## Реалистичный маршрут

1. Этот файл — NO-GO для прямого порта; не расходовать время на адресные хуки ARMv7 для AArch64.
2. Получить неизменённый законный APK с ARMv7 и повторить аудит. Нельзя смешивать native-библиотеки и assets разных версий без проверки соответствия IL2CPP metadata и Addressables.
3. Если снова Unity/IL2CPP — отдельная оценка поддержки runtime/JNI/graphics/audio. Наличие ARMv7 необходимо, но недостаточно. Раннюю сборку на другом движке оценивать заново, без переноса предположений от 3.0.3.
4. Если доступны исходники и права — native-порт с заменой платформенного слоя потенциально предпочтительнее восстановления Android/Unity окружения.

Ни работоспособность игры, ни FPS, ни расход RAM, ни возможность закончить порт этим статическим аудитом не установлены.
