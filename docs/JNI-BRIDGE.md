# Restricted JNIEnv table harness

Добавлен jni_bridge.c: таблица формы JNI 1.6 на 233 слота, поддерживающая только перечисленные ниже операции. Раскладка и индексы сверены со [спецификацией JNI](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html). Это исследовательский harness; запуск Unity с этой таблицей остаётся запрещён текущим runtime gate.

| Slot | Operation | Ограничение |
|---|---|---|
| 4 | GetVersion | 0x10006 обозначает выбранную раскладку; не полноту реализации |
| 6 | FindClass | Только android/content/Context и android/content/res/AssetManager |
| 17 | ExceptionClear | Очистка внутренней ошибки, без Java Throwable |
| 21–23 | NewGlobalRef, DeleteGlobalRef, DeleteLocalRef | Ссылки внутренней модели |
| 25 | NewLocalRef | Копия ссылки |
| 31 | GetObjectClass | Только объекты Context/AssetManager, не class objects |
| 33 | GetMethodID | Только Context.getAssets: ()Landroid/content/res/AssetManager; |
| 34–36 | CallObjectMethod, V, A | Только указанный метод без аргументов |
| 228 | ExceptionCheck | Состояние внутренней ошибки |

JNIEnv в C имеет дополнительный уровень указателя. sf_jni_bridge_create возвращает адрес указателя на таблицу; тесты вызывают (*env)->GetMethodID и (*env)->CallObjectMethodA. Индексы ключевых функций, размер таблицы и смещения проверяются compile-time assertions на host и Vita ARM. Callback-функции используют базовый ARM AAPCS через SF_ANDROID_ABI.

Ссылки на объекты кодируются внутренними 32-битными handles через uintptr_t; на host они не являются разыменовываемыми адресами. Method ID — непрозрачный стабильный token, проверяется по точному значению; произвольный token не вызывается как функция. Это ограничение нашей реализации, а не универсальная реализация Java object layout.

## Неподдерживаемые операции

Все остальные слоты NULL, а не ошибочно типизированные универсальные stubs. Их вызывать нельзя: косвенный вызов NULL приведёт к сбою. sf_jni_bridge_require проверяет явный список требуемых номеров до исполнения и возвращает отказ, если хотя бы один не поддержан. Этот список обязан предоставить доверенный harness; проверка не обнаруживает все будущие обращения движка автоматически и не доказывает совместимость методов или Java semantics.

Неизвестный класс, метод, сигнатура или receiver в поддерживаемых операциях возвращают NULL и сохраняют первую внутреннюю ошибку. ExceptionOccurred/ThrowNew, настоящие классы исключений и unwind Java пока отсутствуют. JavaVM, AttachCurrentThread, GetJavaVM, строки/массивы, IsSameObject/IsInstanceOf, поля и RegisterNatives через JNI ещё не реализованы. Нельзя выдавать этот harness за полноценный JNI runtime, даже если GetVersion возвращает 1.6.

Объектная модель остаётся однопоточной, ограниченной 64 ссылками и одной средой. Передавать handles между средами нельзя. sf_jni_bridge_assets — внутренняя проверенная конверсия AssetManager в заимствованный указатель backend; настоящий экспорт AAssetManager_fromJava пока не публикуется. Указатель действителен только до уничтожения среды. Данные и файловые ограничения описаны в ANDROID-ASSETS.md.

## Проверки

Новый CTest выполняет реальные косвенные вызовы вариантов обычный/A/V и проверяет ссылки, receiver и capability gate. Восемь Python integration tests дополнительно читают файл через таблицу, сверяют набор непустых слотов, проверяют ошибки, удалённые ссылки и поддельный method ID. Всего: 102 Python tests, 6 CTest targets, успешный Vita cross-build. Android ELF и Unity не исполнялись, аппаратной проверки нет. Vita main пока не вызывает bridge.

Следующий необходимый шаг — JavaVM/thread contract и модель настоящих pending exceptions, затем расширение таблицы строго по подтверждённым обращениям Unity. До этого JNI_OnLoad и конструкторы Android-библиотек не запускаются. Счётчик 451 обязательного внешнего импорта не уменьшен.
