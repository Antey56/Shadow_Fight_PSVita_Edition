"""Allowlisted public view: no arbitrary APK strings, metadata or DEX strings."""
def public_report(r):
    libkeys={'path','size','bits','machine','type','flags','needed','imports','jni_exports','relocations','load_segments','load_span'}
    manifest=[]
    safe={'manifest':{'package','versionCode','versionName'},'uses-sdk':{'minSdkVersion','targetSdkVersion'},'uses-permission':{'name'},'uses-feature':{'name','glEsVersion','required'},'application':{'name'}}
    for tag in r['manifest']:
        if tag['tag'] in safe:
            attrs={k.split('}')[-1]:v for k,v in tag['attributes'].items() if k.split('}')[-1] in safe[tag['tag']]}
            manifest.append({'tag':tag['tag'],'attributes':attrs})
    return {'publication_view':'allowlisted technical summary; full report kept locally','apk':r['apk'],'summary':r['summary'],'manifest':manifest,'libraries':[{k:v for k,v in lib.items() if k in libkeys} for lib in r['libraries']],'dex':[{'path':x['path'],'string_count':x['string_count']} for x in r['dex']],'omitted':['arbitrary ELF strings','DEX strings','manifest metadata and tokens','full export names','full asset paths'],'analysis_error_count':len(r['errors'])}
