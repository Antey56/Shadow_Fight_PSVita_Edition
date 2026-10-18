#!/usr/bin/env python3
"""Single command: APK inventory, JNI declarations, imports and evidence summary."""
import argparse,collections,json,pathlib
from analyze_apk import analyze
from deep_audit import run
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('apk',type=pathlib.Path);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    r=analyze(a.apk);a.out.mkdir(parents=True,exist_ok=True)
    (a.out/'apk-report.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
    run(a.apk,a.out)
    d=json.loads((a.out/'deep-audit.json').read_text(encoding='utf-8'))
    libs=r['libraries'];syms={s['name'] for x in libs for s in x['imports']}
    rows=['# Special Edition — фактический аудит','',f"APK: `{r['apk']['name']}`",f"SHA-256: `{r['apk']['sha256']}`",f"Размер: {r['apk']['size']} байт.",'',f"ARMv7: {r['summary']['armv7_present']}. Библиотек: {len(libs)}; уникальных импортов: {len(syms)}; native JNI declarations: {len(d['native_methods'])}.",'', '| Модуль | Размер | LOAD span | Импортов |','|---|---:|---:|---:|']
    rows += [f"| {x['path']} | {x['size']} | {x['load_span']} | {len(x['imports'])} |" for x in libs]
    rows += ['',f"Assets: {r['summary']['asset_count']} файлов, {r['summary']['asset_bytes']} байт; LOAD spans всех модулей суммарно {sum(x['load_span'] for x in libs)} байт. Это не измерение RAM.",'','## Решение','']
    if not r['summary']['armv7_present']:rows+=['NO-GO: ARMv7 отсутствует.']
    elif any(x['path'].endswith('/libunity.so') for x in libs):rows+=['ARMv7 gate пройден. Unity runtime gate НЕ пройден: необходима отдельная реализация Android/Unity platform layer. Наличие ELF32 не подтверждает работоспособность.']
    else:rows+=['ARMv7 найден; остальные этапы совместимости ещё не проверены.']
    rows+=['','Полные JNI descriptors: JNI-METHODS.md. Карта импортов: imports.csv; UNIMPLEMENTED означает, что адаптер проектом не предоставлен. Native declarations не доказывают фактическую регистрацию или вызов.','',f"Ошибок базового парсинга: {len(r['errors'])}."]
    (a.out/'SUMMARY.md').write_text('\n'.join(rows)+'\n',encoding='utf-8')
if __name__=='__main__':main()
