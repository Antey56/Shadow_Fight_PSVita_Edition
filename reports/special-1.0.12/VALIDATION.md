# Проверки Special Edition 1.0.12

- Полный анализ реального APK и повторный combined audit: завершены без ошибок базового парсинга.
- 8 Python unittest: PASS.
- Host GCC C99, Wall/Wextra/Werror: touch.c и native_registry.c с тестами компилируются.
- touch sequencing PASS; native registry PASS.
- Unity version 2020.3.48f1 подтверждена заголовком globalgamemanagers, не только поиском строк.
- VPK и запуск на Vita не проверены: полноценный toolchain/vitaGL и устройство недоступны.
- DEX native declarations извлечены из class_data_item/access_flags; Java control flow, фактический RegisterNatives и весь JNI runtime не реализованы.
- Новые инструменты не выполняют Android код, не модифицируют APK, не обходят лицензию.
