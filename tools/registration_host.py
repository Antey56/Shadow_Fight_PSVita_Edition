"""ctypes bridge to authored host harness; never loads Android ELF code."""
import ctypes as C
class Declaration(C.Structure):
    _fields_=[('class_name',C.c_char_p),('name',C.c_char_p),('descriptor',C.c_char_p)]
class CodeRange(C.Structure):
    _fields_=[('begin',C.c_size_t),('end',C.c_size_t)]
class Method(C.Structure):
    _fields_=[('name',C.c_char_p),('signature',C.c_char_p),('fnPtr',C.c_void_p)]
def load(path):
    lib=C.CDLL(str(path.resolve()))
    for name,args,result in [
        ('sf_jni_bridge_create',[C.c_char_p,C.c_size_t],C.c_void_p),
        ('sf_jni_bridge_destroy',[C.c_void_p],None),
        ('sf_jni_bridge_configure_natives',[C.c_void_p,C.POINTER(Declaration),C.c_size_t,C.POINTER(CodeRange),C.c_size_t],C.c_int),
        ('sf_jni_bridge_registered',[C.c_void_p,C.c_char_p,C.c_char_p,C.c_char_p],C.c_size_t)]:
        f=getattr(lib,name);f.argtypes=args;f.restype=result
    return lib
def functions(env):
    p=C.c_void_p;table=C.cast(C.cast(env,C.POINTER(p))[0],C.POINTER(p))
    return {
        'find':C.CFUNCTYPE(p,p,C.c_char_p)(table[6]),
        'register':C.CFUNCTYPE(C.c_int32,p,p,C.POINTER(Method),C.c_int32)(table[215]),
        'clear':C.CFUNCTYPE(None,p)(table[17]),
        'check':C.CFUNCTYPE(C.c_uint8,p)(table[228]),
        'delete':C.CFUNCTYPE(None,p,p)(table[23])}
