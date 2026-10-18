"""Host bridge for the authored C adapter only; never loads an Android .so."""
import ctypes as C
def load_backend(path):
    lib=C.CDLL(str(path.resolve()))
    signatures={
        'sf_asset_manager_create':([C.c_char_p,C.c_size_t],C.c_void_p),
        'sf_asset_manager_destroy':([C.c_void_p],None),
        'AAssetManager_open':([C.c_void_p,C.c_char_p,C.c_int],C.c_void_p),
        'AAsset_read':([C.c_void_p,C.c_void_p,C.c_size_t],C.c_int),
        'AAsset_seek':([C.c_void_p,C.c_int32,C.c_int],C.c_int32),
        'AAsset_seek64':([C.c_void_p,C.c_int64,C.c_int],C.c_int64),
        'AAsset_getLength':([C.c_void_p],C.c_int32),
        'AAsset_getLength64':([C.c_void_p],C.c_int64),
        'AAsset_getRemainingLength':([C.c_void_p],C.c_int32),
        'AAsset_getRemainingLength64':([C.c_void_p],C.c_int64),
        'AAsset_getBuffer':([C.c_void_p],C.c_void_p),
        'AAsset_isAllocated':([C.c_void_p],C.c_int),
        'AAsset_close':([C.c_void_p],None),
        'sf_asset_target_catalog':([C.c_void_p,C.c_size_t],C.c_int),
    }
    for name,(args,result) in signatures.items():
        f=getattr(lib,name);f.argtypes=args;f.restype=result
    return lib
