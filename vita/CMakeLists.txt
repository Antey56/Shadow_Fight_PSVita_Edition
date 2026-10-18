cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
endif()
project(sf2_vita_diagnostic C CXX ASM)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_STANDARD 99)
find_program(SF2_HOST_PYTHON NAMES python3 python REQUIRED NO_CMAKE_FIND_ROOT_PATH)
set(LIVEAREA_DIR "${CMAKE_CURRENT_BINARY_DIR}/livearea_assets")
set(LIVEAREA_FILES "${LIVEAREA_DIR}/icon0.png" "${LIVEAREA_DIR}/livearea/contents/bg.png"
  "${LIVEAREA_DIR}/livearea/contents/startup.png" "${LIVEAREA_DIR}/livearea/contents/template.xml")
add_custom_command(OUTPUT ${LIVEAREA_FILES}
  COMMAND "${SF2_HOST_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/prepare_livearea.py"
    --source "${CMAKE_CURRENT_SOURCE_DIR}/sce_sys" --out "${LIVEAREA_DIR}"
  DEPENDS ../tools/prepare_livearea.py sce_sys/icon0.png sce_sys/livearea/contents/bg.png
    sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/template.xml VERBATIM)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/arm_smoke_image.h"
  COMMAND "${CMAKE_C_COMPILER}" -shared -nostdlib -fPIC -fvisibility=hidden -mthumb -march=armv7-a
    -mfloat-abi=softfp -mfpu=neon -O1 -Wl,--hash-style=sysv -Wl,-soname,sf2-arm-smoke.so
    -Wl,-z,max-page-size=4096 -o "${CMAKE_CURRENT_BINARY_DIR}/arm-smoke.so"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/arm_smoke_fixture.c"
  COMMAND "${SF2_HOST_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/embed_arm_smoke.py"
    "${CMAKE_CURRENT_BINARY_DIR}/arm-smoke.so" "${CMAKE_CURRENT_BINARY_DIR}/arm_smoke_image.h"
  DEPENDS ../tests/arm_smoke_fixture.c ../tools/embed_arm_smoke.py ../tools/elf_dynamic.py VERBATIM)
add_executable(sf2_diagnostic src/main.c src/diagnostic_ui.c src/touch.c src/loader_plan.c src/elf_preflight.c src/native_registry.c src/arm_reloc.c src/arm_dynamic.c src/arm_link.c src/arm_scope.c src/android_log_adapter.c src/android_asset_adapter.c src/jni_objects.c src/jni_bridge.c src/jni_vm.c src/bionic_memory.c src/jni_registration.c src/aeabi_memory.c src/aeabi_memory.S src/bionic_once.c src/libmain_runtime.c src/arm_exec.c src/arm_symbols.c src/arm_vm_probe.c "${CMAKE_CURRENT_BINARY_DIR}/arm_smoke_image.h")
target_include_directories(sf2_diagnostic PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(sf2_diagnostic PRIVATE -Wall -Wextra)
target_compile_definitions(sf2_diagnostic PRIVATE SF2_VITA_TARGET=1)
set_target_properties(sf2_diagnostic PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(sf2_diagnostic vitaGL vitashark SceShaccCgExt mathneon SceShaccCg_stub taihen_stub
  SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub SceSysmodule_stub
  SceCommonDialog_stub SceKernelDmacMgr_stub SceIofilemgr_stub SceSysmem_stub m)
vita_create_self(eboot.bin sf2_diagnostic UNSAFE)
vita_create_vpk(sf2-diagnostic.vpk SF2AUD001 eboot.bin VERSION 00.09 NAME "SF2 Input Diagnostic"
  FILE "${LIVEAREA_DIR}/icon0.png" sce_sys/icon0.png
  FILE "${LIVEAREA_DIR}/livearea/contents/bg.png" sce_sys/livearea/contents/bg.png
  FILE "${LIVEAREA_DIR}/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  FILE "${LIVEAREA_DIR}/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml)
add_custom_target(validate_diagnostic_vpk ALL
  COMMAND "${SF2_HOST_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/validate_vpk.py"
    "${CMAKE_CURRENT_BINARY_DIR}/sf2-diagnostic.vpk" --version 00.09
    --out "${CMAKE_CURRENT_BINARY_DIR}/vpk-validation.json"
  DEPENDS sf2-diagnostic.vpk-vpk VERBATIM)

# Explicit local input, OFF by default. Generated game bytes are never committed.
set(SF2_LIBMAIN_PROBE_INPUT "" CACHE FILEPATH "Local exact-hash libmain.so for JNI_OnLoad-only hardware probe")
if(SF2_LIBMAIN_PROBE_INPUT)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libmain_probe_image.h"
    COMMAND "${SF2_HOST_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/embed_libmain_probe.py"
      "${SF2_LIBMAIN_PROBE_INPUT}" "${CMAKE_CURRENT_BINARY_DIR}/libmain_probe_image.h"
    DEPENDS "${SF2_LIBMAIN_PROBE_INPUT}" ../tools/embed_libmain_probe.py ../tools/audit_libmain_bootstrap.py VERBATIM)
  target_sources(sf2_diagnostic PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libmain_probe_image.h")
  target_compile_definitions(sf2_diagnostic PRIVATE SF2_LIBMAIN_ENTRY_PROBE=1)
endif()
