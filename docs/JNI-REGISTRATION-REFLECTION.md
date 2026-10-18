# Registration, strings and the first reflection path

Текущий bridge реализует 26 непустых JNI slots. Добавлены RegisterNatives (215), GetStaticMethodID/CallStaticObjectMethod/A/V (113–116), GetStringLength (164), NewStringUTF/GetStringUTFLength/GetStringUTFChars/ReleaseStringUTFChars (167–170). Размещение слотов, jvalue и native method records проверяется при компиляции на host и Vita. Официальные контракты: [JNI functions](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html).

## Регистрация

sf_jni_bridge_configure_natives принимает immutable policy: NativeDeclaration массив и до 16 разрешённых диапазонов ARM32 code addresses. Policy заимствуется и обязана жить до уничтожения окружения. Конфигурация однократная. Классы Unity/Android Game SDK разрешаются только при наличии их в policy; произвольные FindClass не сообщают об успехе.

RegisterNatives принимает настоящий по форме JNINativeMethod массив: name, signature, fnPtr. Проверяется точное совпадение class/name/signature с декларациями, диапазон адреса, Thumb bit и ARM alignment. Строки копируются в owned catalog на 128 записей: name/class до 127 байт, signature до 511. Это отдельный целочисленный каталог адресов, не старый NativeRegistry с C function pointers.

Каждая регистрация транзакционная: неизвестный метод, конфликт адреса, нехватка ёмкости или адрес вне разрешённых диапазонов откатывают весь batch и выставляют внутреннюю pending error. Повтор того же адреса идемпотентен. Временная копия каталога выделяется в heap, а не на небольшом ARM stack. Размер таблицы methods — от 1 до 128; нулевые/отрицательные значения отклоняются.

Диапазоны задаёт доверенный loader/harness. Эта проверка не доказывает, что память отображена как executable, что сигнатура функции соответствует фактическому коду или что конструкторы готовы. Читаемость указателей массива/строк — предусловие доверенного caller: произвольные guest pointers пока не валидируются memory map. RegisterNatives сам ничего не вызывает.

verify_native_registration.py воспроизводит все **28 подтверждённых записей** реального Unity profile через слот 215 в host C harness. Адреса остаются link-time VA числами, Android ELF не загружается в исполняемую память. Три class groups и каждый результат lookup совпали. Это метаданные регистрации, а не выполненный JNI_OnLoad.

```sh
python tools/verify_native_registration.py local-input/1.0.12/libunity.so --profile reports/special-1.0.12/unity-bootstrap.json --host-library build-host/jni_objects_host.so --out reports/special-1.0.12/native-registration-verification.json
```

На Windows используется jni_objects_host.dll. Загружается только наша host library, не libunity.so.

## Строки

NewStringUTF валидирует modified UTF-8: одно-, двух- и трёхбайтовые кодировки UTF-16 code units, включая C0 80 для NUL и отдельные surrogate units. Четырёхбайтовый обычный UTF-8, overlong encodings и усечённые sequences отвергаются. GetStringLength возвращает количество UTF-16 units, GetStringUTFLength — количество байт modified UTF-8, без терминатора. Максимум одной строки — 64 KiB закодированных данных. Больше возвращает внутреннюю capacity error.

Строки неизменяемы, local/global copies разделяют refcounted backing object. GetStringUTFChars создаёт самостоятельную копию и выставляет isCopy=true; одновременно допускается 16 outstanding leases. Release проверяет указатель и identity строки, допускается при pending error. Удаление/копирование ссылок не изменяет содержимое других refs. Если все ссылки удалены, хранить borrowed object view нельзя; JNI caller обязан сохранить валидную ссылку до Release.

При detach explicit local cleanup также освобождает все outstanding UTF leases. После этого старые char pointers недействительны. До detach caller обязан завершить их использование. VM destruction освобождает оставшиеся leases, строки и каталог. Общая модель refs всё ещё ограничена 64 слотами, один поток-владелец; shared globals между worker envs отсутствуют.

## Reflection

GetObjectClass для class object теперь возвращает java/lang/Class. Class.getClassLoader() возвращает синтетический loader. Метод Class.forName(String,boolean,ClassLoader) доступен в static A/V/обычном вариантах и поддерживает только разрешённые синтетические классы по ASCII dotted names. Для app classes нужен наш loader; NULL допускается только для известных bootstrap/framework classes.

initialize=true **отклоняется**: Java initializers/bytecode не реализованы и успешный ответ был бы неверен. Неизвестные classes, массивные имена, неправильный loader/receiver и сигнатуры дают отказ. Нельзя считать forName универсальным class loading. Строки поддерживают Unicode, но class name lookup намеренно ограничен подтверждённым ASCII namespace. Constructor, GetFieldID, primitive/object arrays, instance creation и остальная reflection пока отсутствуют.

## Проверки и граница запуска

161 Python/integration tests и 8 CTest targets проходят; Vita cross-build без предупреждений успешен. Проверены 28 реальные metadata bindings, атомарность/ошибки регистрации, lifetime copied strings, Unicode lengths, malformed encodings, char leases, reflection receivers и static A/V/varargs calls. Диагностический VPK 0.5 дополнительно вызывает string/forName цепочку в Vita-коде, но аппаратный результат ещё не получен.

Числовое отсутствие RegisterNatives/GetStaticMethodID slots устранено; полнота Unity bootstrap не доказана. FatalError остаётся неподдержанным failure slot. Конструкторы/JNI_OnLoad/игровой код отключены: реальная Java initialization, worker JNI, EGL/audio, bionic structs и executable loader остаются незавершёнными. Счётчик сертифицированных внешних импортов не изменён.
