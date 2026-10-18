# Технический проект Android2Vita / so-loader

## Статус и границы

Текущая исследуемая сборка Special Edition 1.0.12 содержит ARMv7 ELF32 и Unity 2020.3.48f1/IL2CPP. Её библиотеки не вызываются: выполнение блокируют незавершённые JNI, bionic, graphics/audio и executable-loader слои. `sf2_preflight()` возвращает -3 для ELF64 и -4 для ещё не сертифицированного ELF32. Это предохранитель интеграции, не реализация so_util. Основной target — диагностический VPK. Актуальная оценка шести направлений и подтверждённые ограничения описаны в INTEGRATION-STATUS.md; прежний ARM64/Mod аудит не следует смешивать с этим APK.

## Слои

Vita eboot → платформа/журнал → vitaGL/GXM и audio → bionic/POSIX адаптеры → so_util/ARMv7 modules → минимальное JavaVM/JNIEnv окружение → проверенный lifecycle движка → игровой цикл.

У каждого слоя контракт и независимый журнал ошибок. Необязательные услуги отделены от файловой системы, allocator, thread/TLS, graphics и license, которые нельзя заменять общей функцией return 0.

### ELF и ABI

Сначала сверить ELF32 EM_ARM, endian, PT_LOAD диапазоны и адресные пересечения, ARM attributes, softfp/hardfp boundary, требуемые ARM/Thumb/NEON инструкции. Не назначать универсальный load address наугад. Проверить полный DT_NEEDED граф и порядок зависимостей, типы relocations, TLS, init_array, исключения/C++ ABI и allocator ownership. `DT_NEEDED libfoo.so` не означает, что Android libfoo можно заменить одноимённой Vita-библиотекой.

У каждой импортированной функции должен быть адаптер с реальной сигнатурой, calling convention, errno, размером структур и владением памятью. Особо проверить pthread_mutex/cond, bionic FILE/stat/dirent, clock, mmap, dlopen/dlsym и Android asset API. Не передавать bionic-структуры напрямую в newlib.

`so_resolve()` в проверенном upstream возвращает 0 даже при отсутствующих символах и может подставлять error stub. Поэтому успех загрузчика нельзя определять его результатом. Перед init_array нужен независимый completeness gate по обязательным импортам и собственная таблица разрешений. `so_resolve_with_dummy()` не использовать. Порядок so_file_load → relocate → resolve → gate → flush → initialize применим только после предоставления всех сервисов, которые могут понадобиться конструкторам. Ошибка до входа в игру завершает диагностический процесс; повторно не использовать частично инициализированный runtime.

### JNI и lifecycle

Проинвентаризировать фактические классы, методы и descriptors, включая RegisterNatives. Таблица JNIEnv должна иметь корректные индексы и функции, поддерживать local/global references, exception semantics, GetStringUTFChars/Release, primitive arrays, threading Attach/Detach и thread-local env. Угадывать подписи Java_* нельзя. JNI_OnLoad получает совместимый JavaVM, не NULL.

Восстановить реальные вызовы Activity/Surface/Input из Java-кода законной неупакованной сборки и тестовых логов. Графика должна существовать к нужному lifecycle событию, input доставляться на нужном потоке. NativeActivity-only bootstrap не заменяет UnityPlayer. Для Unity понадобятся также runtime/services, IL2CPP metadata matching и Burst dependencies. Проект содержит только ограниченный JavaVM/JNIEnv harness, без Java bytecode execution, и не запускает Unity одной JNI_OnLoad.

### Graphics, assets, audio

Снять реальные EGL/GLES entrypoints и extensions. Декларация GLES2 в manifest — нижняя заявленная возможность, не полный профиль. Проверить shader translation, FBO/depth/stencil, texture compression, alignment, framebuffer orientation и контекстную синхронизацию. vitaGL предоставляет API поверх GXM; kubridge — доступ к памяти/кэшам. Их роли различны.

Для assets сначала карта обращений: paths, APK zip handles, streaming/seek, Unity bundles/Addressables catalog. Виртуальный read-only mount отделить от writable `ux0:data/sf2vita/`. Сохранения писать атомарно через временный файл с последующим rename и сохранением предыдущей версии. Существующие Android saves без понимания формата не мигрировать. Не расшифровывать упаковку данного Mod APK.

Для звука определить OpenSL ES/AAudio/Java/engine path; сделать отдельный backend и ограничения очереди, latency, pause/resume. libmediandk требует проверки кодеков и video surfaces; отсутствие эквивалента может блокировать runtime. Память измерять после bootstrap, меню, боя, смены сцены и длительной сессии; не обещать FPS заранее.

## Управление

`touch.c` содержит конечный автомат пяти указателей: id 0 — виртуальный joystick, 1–4 — кнопки. Крестовина или левый стик задают joystick, CROSS/SQUARE/TRIANGLE/CIRCLE — четыре touch-точки. Координаты 960×544 примерные и не извлечены из этой игры; назначение ударов нужно калибровать по конкретному HUD. START в диагностике — выход. Правый стик, touch panel, L/R и меню зарезервированы для следующего этапа, не реализованы.

События: DOWN для первого пальца; POINTER_DOWN для следующего; MOVE только при изменении; POINTER_UP до удаления пальца с полным старым набором указателей; UP для последнего; CANCEL при потере фокуса/паузе. Pointer id стабилен на протяжении касания; action index — индекс в текущем массиве, не id. Android packed action = action | (index << 8). downTime сохраняется с первого DOWN до последнего UP/CANCEL, eventTime монотонный и переводится из микросекунд в миллисекунды при создании MotionEvent; downTime должен хранить будущий JNI sink. Давление 1 для активных точек, tool type finger; конкретный native API может иметь иной формат.

Масштабирование с letterbox: x_android=(x_vita-offset_x)*android_width/viewport_width, аналогично y; clamp в пределах viewport, учесть поворот. Здесь выводятся логические Android-like события в журнал, реальная доставка в игру не реализована. Для UI нужен отдельный режим курсора/подтверждения; touch зоны боя не заменяют все экраны меню. При смене режима отправлять CANCEL.

## Graceful fallback

Пример политики в `optional_services.h` возвращает явное состояние недоступности и никогда успех лицензии/покупки. Это внутренний контракт, не ABI реального Google SDK. Перед подключением выяснить фактическую сигнатуру, поток callbacks, таймауты и ownership. Диспетчер игры получает ровно один terminal callback на request id; повторные callbacks после timeout игнорируются. При shutdown pending-запросы отменяются, listener references освобождаются.

| Сервис | Реакция при недоступности |
|---|---|
| Analytics / crash reporting | локальный ограниченный журнал; игровой сценарий продолжается, если сервис действительно необязателен |
| Ads | not available/no fill; скрыть недоступную кнопку; не выдавать reward и не сообщать ad completed |
| Leaderboards / achievements | unsupported/offline UI, без подделки синхронизации |
| Cloud save | явный local-only режим, без удаления remote save и без заявления успешной загрузки |
| Auth | not signed in; закрыть зависящие от аккаунта функции |
| Billing / license / integrity | штатная проверка через разрешённый backend либо понятная остановка зависимого сценария; не подделывать ответ |

Не глушить ExceptionCheck глобально, не удалять произвольные вызовы, не заменять все JNI методы нулями. Сначала доказать необязательность функции по её вызывающему коду. Если offline-ветки нет, это нерешённая зависимость; fallback не является способом обойти обязательную проверку. Не возвращать фиктивные токены, receipt, entitlement, signature или успешную покупку. Не менять сохранение для разблокировки контента.

## Этапы и критерии приёмки

1. **ABI gate:** SHA-256, происхождение, неизменённая ARMv7 сборка, полный inventory. Текущий APK этот gate не проходит.
2. **Host diagnostics:** анализатор воспроизводит JSON; тест touch проверяет одновременные кнопки, удержание без повторного DOWN, освобождение первого указателя, MOVE и CANCEL.
3. **Vita diagnostics:** VPK собирается в зафиксированном SDK, очищает экран, пишет input log, выходит по START; проверить на реальном устройстве.
4. **Native load:** нет unresolved обязательных imports, корректные constructors и JNI registry; аварии содержат module base, PC и версию файла. Никаких заглушек license.
5. **Первый кадр/звук:** рабочие shaders/textures, корректная поверхность, поддерживаемый codec path, измерение памяти.
6. **Gameplay:** меню → бой → пауза → resume → смена сцены → сохранение/перезапуск. Сессия не менее 30 минут; воспроизводимость, memory high-water и frame times записаны.
7. **Offline fault injection:** отсутствие Google/ads, callback timeout, отсутствие auth; локальная игра допустима только если её допускает сама авторизованная сборка. Missing asset/import/license даёт диагностическую ошибку.

Оценка трудозатрат до прохождения ABI/JNI/runtime gates недостоверна. Не создавать впечатление готового порта наличием eboot.bin: данный target проверяет только основу платформы и управления.
