# JNI: evidence and registration preparation

Статический отчёт jni-surface.json для Special Edition 1.0.12 содержит 63 объявления native-методов, 37 записей method_ids по выбранным файловым/framework API и результаты поиска фиксированного набора строк в 10 ARMv7 библиотеках.

В libunity.so найдены отдельные NUL-terminated строки android/content/Context, android/content/res/AssetManager, getAssets, getFilesDir, getCacheDir, getPackageCodePath, getExternalFilesDir, open и openFd. В DEX есть ссылка Context.getAssets с дескриптором ()Landroid/content/res/AssetManager;. Это поддерживает гипотезу о необходимости Java asset bridge, но не устанавливает реально исполненный путь: строки не связывают конкретный receiver с методом и не заменяют дизассемблирование или трассу.

```sh
python tools/jni_surface.py /path/to/game.apk --out reports/my-jni-surface.json
```

Скрипт читает APK без исполнения. Он использует текущие DEX helpers проекта; это инструмент для доверенного исследуемого APK, не полноценный валидатор повреждённого DEX. Публикуются объявления и разрешённые API names, а не произвольные строки из бинарников. Отсутствие строки не доказывает отсутствие динамического вызова.

## Реестр

Добавлена внутренняя операция native_register_checked. Она сравнивает class/name/descriptor с заранее заданными native declarations, затем подготавливает копию реестра. Неизвестное объявление (-4), конфликт адреса (-2), переполнение (-3), неверные аргументы (-1) оставляют исходный реестр без изменений. Успех возвращает 0; повтор того же адреса допустим. Проверяются точные строковые ключи, а не ABI функции или грамматика дескриптора.

Это **не реализация RegisterNatives в JNIEnv**. Согласно [спецификации JNI](https://docs.oracle.com/en/java/javase/26/docs/specs/jni/functions.html#registernatives), реальный вызов также работает с Java class objects и исключениями. Наш код пока не создаёт таких объектов, не реализует JNI function table/JavaVM и не вызывает зарегистрированные Android функции. Строки и указатели заимствуются у доверенного вызывающего кода; он отвечает за lifetime и синхронизацию. Реестр должен быть инициализирован нулями и изменяться через API. Не следует приводить эти function pointers к JNI сигнатурам и запускать их без отдельной проверки.

## Следующий интеграционный барьер

1. Подтвердить native registration tables и адреса UnityPlayer.initJni/nativeRender/nativePause/nativeResume в libunity, включая ARM/Thumb и calling convention.
2. Реализовать ограниченную модель классов/объектов, thread attachment, local/global references и pending exceptions. Неизвестные обязательные методы должны останавливать подготовку с диагностикой.
3. Связать Context.getAssets с контролируемым asset manager; выяснить, нужен ли openFd с offset/length, и отдельно поддержать такой путь либо явно сообщать об отсутствии.
4. Ввести surface/EGL, audio и lifecycle только после проверки зависимостей. Необязательные сервисы могут возвращать документированную недоступность; лицензирование и покупки не подменяются успешным результатом.

Локально проходят 82 Python/integration tests и 5 CTest targets, включая откат регистрации и предел ёмкости. Vita cross-build успешен. Unity не исполнялась; 451 обязательный внешний импорт остаётся прежним счётчиком незавершённой совместимости.
