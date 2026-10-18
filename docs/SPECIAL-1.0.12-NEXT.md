# Рабочая цель: Special Edition 1.0.12 (30000019)

Этот файл заменяет ARM64 Mod 3.0.3 как цель дальнейшего исследования. Результаты старого аудита сохранены отдельно для сравнения. Текущий APK — `com.nekki.shadowfight2.paid--30000019.apk`, пакет `com.nekki.shadowfight2.paid`; наличие официальной подписи пока не установлено. DRM и лицензии не изменяются.

## Что удалось установить

- Все 10 native-модулей — ELF32 EM_ARM в armeabi-v7a. Первое препятствие исходного APK устранено.
- В заголовке сериализованного `globalgamemanagers` записано **2020.3.48f1**. В бинарных строках встречается также 2018.3.0a1: простая находка строки не определяет версию движка, поэтому приоритет имеет заголовок данных.
- Unity IL2CPP metadata: magic 0xfab11baf, version 27, размер 8969044 байт. Само наличие metadata не даёт исходного C# и не заменяет код libil2cpp.so.
- Два DEX, в них 63 объявления native-методов. В отличие от предыдущего Mod APK здесь доступны обычные DEX declarations. Это не доказательство отсутствия других изменений APK.
- libunity.so импортирует EGL и Android window/looper/sensor API. Дополнительно найдены 569 строк, похожих на GL/EGL/Vulkan/audio/media API; это список кандидатов для динамического разрешения, а не 569 доказанных обязательных функций.
- Основная логика предположительно в libil2cpp.so, не в маленьком libmain.so. Последний требует dlopen/dlsym, а libunity.so зависит от libmain.so.

## JNI integration checklist по реальным объявлениям

`JNI-METHODS.md` содержит точные class/method/descriptor. Например, UnityPlayer:

| Метод | Descriptor |
|---|---|
| initJni | (Landroid/content/Context;)V |
| nativeRecreateGfxState | (ILandroid/view/Surface;)V |
| nativeRender | ()Z |
| nativeInjectEvent | (Landroid/view/InputEvent;)Z |
| nativeResume | ()V |
| nativePause | ()Z |
| nativeFocusChanged | (Z)V |

Эта таблица не задаёт безопасный порядок вызова. Нужно восстановить порядок по Java-коду и зарегистрированные native pointers. `native_registry.c` реализует строгое сопоставление class+method+descriptor и отклоняет конфликт регистрации. В регистр передаются class names без L и ;. Приведение адреса к реальной типизированной JNI функции допустимо только после проверки descriptor, ABI, env и receiver. Нельзя вызвать все методы таблицы по очереди.

TouchEvent из каркаса нельзя передать в nativeInjectEvent как указатель: там ожидается Java InputEvent/MotionEvent. Нужна реализация класса с теми методами, которые Unity запрашивает через JNI, либо доказанный нижележащий native input API. downTime, action index, pointer arrays и размеры объектов должны совпадать с контрактом. Пока sink пишет только журнал.

## Приоритет адаптеров

1. **Runtime/данные bionic:** `__sF`, `__errno`, `__stack_chk_guard`, unwind и allocation ownership. __sF — объект данных, не функция; return-0 stub недопустим.
2. **Threads:** bionic mutex/cond layout, recursive/normal mutex, thread-local storage, `pthread_cond_timedwait_relative_np`, semaphores, attach JNI на каждом потоке.
3. **Загрузчик:** управляемая таблица dlopen/dlsym, dependency modules, строгая проверка imported symbols до constructors. Ни одной неизвестной обязательной функции.
4. **Android platform:** ANativeWindow_fromSurface/acquire/release, looper lifecycle. Sensors могут быть необязательными только после проверки ветвей обработки отсутствия устройства.
5. **Graphics:** корректные EGLDisplay/Context/Surface, pbuffers, query API, eglGetProcAddress. Даже успешно разрешённые символы не гарантируют правильное поведение Unity renderer.
6. **Audio/media:** сначала установить реально используемый backend. Список строк AMediaCodec — повод для проверки, не разрешение отключить все видео/аудио функции.
7. **Опциональные услуги:** Firebase/Crashlytics отдельно от license/billing. Ошибки недоступности доставлять без фиктивного успеха и без разблокировки контента.

`imports.csv` включает все 1210 импортных записей, даже повторяющиеся между модулями. Автоматические категории помогают сортировать работу, не являются готовыми ABI-адаптерами. Нельзя автоматически связывать остальные функции с host libc только по имени.

## Проверяемый путь к первому кадру

Сначала нужна полноценная среда VitaSDK/vitaGL и устройство для теста диагностического VPK. В текущем окружении есть только vdpm/конфигурация C:/vitasdk, компилятор arm-vita-eabi и vitaGL отсутствуют. На host выполнены тесты парсера, multitouch и реестра JNI; запуск ARM-кода и графический тест на Vita не проводились.

После установки среды: зафиксировать версии зависимостей → собрать input diagnostic → проверить на устройстве → загрузить только сертифицированные модули → снять запросы JNI без угадывания классов → реализовать callbacks → первая поверхность → кадр → аудио → управление → сохранение. У каждого этапа отдельный воспроизводимый журнал и memory high-water. До этих проверок проект не может обещать загрузку меню или FPS.

Upstream Android2Vita candidate list исключает Unity из стандартного маршрута: https://github.com/Rinnegatamante/Android2Vita-Candidate-Ports-List . Это граница обычного рабочего процесса, а не математическое доказательство невозможности отдельного Unity-исследования. Проект продолжает подготовку такого исследования, не выдавая его за готовый порт.

## Повторение анализа одной командой

Из корня проекта:

```powershell
python tools/prepare_audit.py "C:\Users\vinto\Downloads\com.nekki.shadowfight2.paid--30000019.apk" --out reports/special-1.0.12
python -m unittest discover -s tests -v
```

В каталоге результата будут полный JSON, DEX/JNI JSON, таблица JNI, imports.csv и SUMMARY.md. APK и игровые assets в архив инженерного проекта не включаются. Подпись проверять отдельно Android SDK `apksigner verify --verbose --print-certs`, сравнивая сертификат с доверенной копией; сам факт наличия подписи не доказывает происхождение.
