# JavaVM: owner-thread harness

Добавлена таблица JavaVM со слотами DestroyJavaVM (3), AttachCurrentThread (4), DetachCurrentThread (5), GetEnv (6) и AttachCurrentThreadAsDaemon (7). JNIEnv дополнен GetJavaVM (219); обратная ссылка возвращает тот же VM. Смещения проверяются при компиляции. Контракты и индексы взяты из [JNI Invocation API](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html).

## Поддержанный сценарий

sf_jni_vm_create создаёт ограниченный harness с одним JNIEnv, уже привязанным к создающему потоку. GetEnv принимает JNI версии 1.1, 1.2, 1.4 и 1.6. При неподдерживаемой версии возвращается -3, при отсутствии attachment — -2; выходной указатель обнуляется при отказе. Повторный attach владельца возвращает тот же адрес JNIEnv.

Detach очищает локальные ссылки и внутреннее состояние ошибки; глобальные ссылки сохраняются. Старые локальные handles остаются недействительными после reattach. Сохранённый указатель JNIEnv нельзя использовать в detached-состоянии: поддержанные callbacks проверяют guard и возвращают отказ без изменения object model. Для нового attachment допускается NULL args либо версия с NULL name/group; Java Thread/ThreadGroup не моделируются. Если поток уже attached, параметры attach игнорируются.

GetJavaVM возвращает VM только из валидного attached окружения. Для самостоятельного bridge, созданного без VM, возвращается -1 и NULL. Набор непустых слотов JNIEnv увеличился до 14; остальные ограничения JNI-BRIDGE.md сохраняются.

## Потоки и жизненный цикл

Проверка владельца использует Windows GetCurrentThreadId, POSIX pthread_equal или Vita sceKernelGetThreadId. Чужой поток получает EDETACHED от GetEnv, ERR от attach и не может использовать JNIEnv владельца. Detach чужого, не подключённого потока — no-op. Это **не многопоточная JVM**: attachment workers и общий namespace объектов ещё не реализованы. Daemon entrypoint имеет тот же ограниченный owner-only сценарий; Java threads, scheduler и shutdown wait отсутствуют.

Только владелец может уничтожать VM. DestroyJavaVM освобождает среду и помечает VM закрытой; повторный destroy возвращает ошибку. Сам VM handle сохраняется до отдельного sf_jni_vm_release, чтобы контролируемо обнаруживать повторные операции. Release автоматически закрывает ещё открытую VM. После release все указатели недействительны. После destroy нельзя обращаться к старому JNIEnv.

Вызывающий harness обязан завершить/join все потоки до destroy/release и не уничтожать среду напрямую через sf_jni_bridge_destroy. Не создавать несколько активных VM на одном потоке и не переносить ссылки между средами. OS thread ID может использоваться повторно: владелец обязан закрыть VM до завершения своего потока. Фабрики не являются JNI_CreateJavaVM и не экспортируются под этим именем.

Нет Java frames, выполнения bytecode, Java monitors или настоящих исключений: поэтому ограничения detach относительно Java stack пока не моделируются. Guard проверяет только реализованные callbacks, не делает безопасными вызовы NULL слотов и произвольные указатели. Unity/JNI_OnLoad остаются отключены.

## Проверки

Новый CTest вызывает таблицы JavaVM/JNIEnv, проверяет обратную ссылку, версии, повторный attach/detach, очистку locals, сохранение globals, неподдержанные attach args, закрытие и повторное уничтожение. Отдельный настоящий Windows/POSIX worker проверяет отказ GetEnv/attach и доступа через чужой JNIEnv. Пройдено 102 Python tests и 7 CTest targets; Vita cross-build успешен. Аппаратной проверки нет.

Дальше нужны настоящие pending exception objects и проверенные части JNI references/class semantics; для реальных Unity worker threads потребуется отдельная многопоточная модель с shared globals. Ни один из этих результатов не разрешает запуск движка автоматически.
