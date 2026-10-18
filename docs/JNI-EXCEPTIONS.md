# Pending exception objects in the restricted harness

Реализованы ExceptionOccurred (slot 15) и Throw (slot 13). Индексы, возвращаемые типы и назначение основаны на [JNI specification](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html). Всего таблица теперь содержит 16 непустых функций. Это синтетическая модель ошибок, а не полноценные java.lang.Throwable.

ExceptionOccurred возвращает новую локальную ссылку на неизменяемую запись текущей ошибки; pending state при этом не очищается. Запись содержит код ошибки и внутреннюю identity. Повторное получение создаёт отдельные ссылки с той же identity. ExceptionClear снимает pending state, но уже полученные ссылки продолжают существовать. Копирование local/global reference сохраняет payload и identity; удаление последней ссылки не снимает ожидающую ошибку.

Throw принимает только валидную ссылку на такую запись. После очистки pending state можно повторно передать сохранённую ошибку, включая глобальную ссылку, пережившую detach/reattach владельца VM. Неизвестный/удалённый handle и Context вместо throwable отклоняются. Как ограничение harness, Throw при уже ожидающей ошибке возвращает отказ и сохраняет первую ошибку; полная семантика Java replacement не реализована.

## Пределы и ошибки

ExceptionOccurred использует те же 64 слота local/global references. Если места нет, он возвращает NULL, сохраняя pending exception; ExceptionCheck остаётся true. После удаления ненужной ссылки можно повторить получение. Возвращаемые references должен удалять вызывающий код или очищать при завершении локального периода.

Новые refs через NewGlobalRef/NewLocalRef по-прежнему запрещены при pending state в этой модели: сначала получить throwable, очистить ошибку, затем копировать ссылку. DeleteLocalRef/DeleteGlobalRef доступны для cleanup при pending state. Это ограниченная политика harness; она не является универсальной реализацией всех JNI правил.

Новые ошибки получают монотонную 64-битную identity. Счётчик не переполняется: после исчерпания новые pending errors остаются видны через ExceptionCheck, но не материализуются через ExceptionOccurred. Код исключения хранится отдельно от refs и не требует malloc. Дубликаты ссылок не являются независимыми Java объектами; IsSameObject пока отсутствует.

Нет java.lang.Throwable классов/наследования, сообщений, stack traces, Java catch/unwind, ThrowNew и ExceptionDescribe. GetObjectClass для синтетического throwable не поддерживается. Слоты неподдерживаемых операций остаются NULL, capability gate обязан отклонять их до исполнения. Не выдавать эти объекты за реальные Android exceptions и не запускать Unity/JNI_OnLoad с текущим harness.

## Проверки

Добавлено 10 Python tests: получение/повторная передача, отсутствие ошибки, неверный тип/NULL/удалённая ссылка, global lifetime, сохранение первой ошибки, исчерпание ёмкости и неизменяемость payload. CTest JavaVM дополнительно проверяет передачу сохранённой глобальной ошибки после detach/reattach.

112 Python tests и 7 CTest targets проходят, Vita cross-build успешен. Аппаратной проверки и исполнения Android ELF нет. Следующий интеграционный барьер — подтвердить реальные JNI registrations Unity и реализовать требуемую семантику классов/методов, а не расширять таблицу неподтверждёнными успешными заглушками.
