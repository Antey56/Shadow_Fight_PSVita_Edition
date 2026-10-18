# Special Edition — фактический аудит

APK: `com.nekki.shadowfight2.paid--30000019.apk`
SHA-256: `ac570008f56922dd1df5bd66934cb4dd44ab3eff0e2961d4f7b02ca398fc969f`
Размер: 125481320 байт.

ARMv7: True. Библиотек: 10; уникальных импортов: 518; native JNI declarations: 63.

| Модуль | Размер | LOAD span | Импортов |
|---|---:|---:|---:|
| lib/armeabi-v7a/libcrashlytics-common.so | 456204 | 461484 | 205 |
| lib/armeabi-v7a/libcrashlytics-handler.so | 72324 | 75353 | 40 |
| lib/armeabi-v7a/libcrashlytics-trampoline.so | 7220 | 5896 | 6 |
| lib/armeabi-v7a/libcrashlytics.so | 78336 | 81385 | 46 |
| lib/armeabi-v7a/libFirebaseCppApp-10_2_0.so | 5132780 | 3101080 | 144 |
| lib/armeabi-v7a/libFirebaseCppCrashlytics.so | 41040 | 28776 | 39 |
| lib/armeabi-v7a/libFirebaseCppMessaging.so | 87540 | 53376 | 52 |
| lib/armeabi-v7a/libil2cpp.so | 38534968 | 39084628 | 266 |
| lib/armeabi-v7a/libmain.so | 18464 | 20757 | 30 |
| lib/armeabi-v7a/libunity.so | 13250628 | 13890408 | 382 |

Assets: 1464 файлов, 252973065 байт; LOAD spans всех модулей суммарно 56803143 байт. Это не измерение RAM.

## Решение

ARMv7 gate пройден. Unity runtime gate НЕ пройден: необходима отдельная реализация Android/Unity platform layer. Наличие ELF32 не подтверждает работоспособность.

Полные JNI descriptors: JNI-METHODS.md. Карта импортов: imports.csv; UNIMPLEMENTED означает, что адаптер проектом не предоставлен. Native declarations не доказывают фактическую регистрацию или вызов.

Ошибок базового парсинга: 0.
