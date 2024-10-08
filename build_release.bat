@echo off
if not exist build (
	mkdir build
)
if not exist "build\release" (
	mkdir build\release
)


pushd build
pushd release

clang -mamx-int8 -g -o cgame.exe ../build.c -O0 -std=c11 -D_CRT_SECURE_NO_WARNINGS -Wextra -Wno-incompatible-library-redeclaration -Wno-sign-compare -Wno-unused-parameter -Wno-builtin-requires-header -lkernel32 -lgdi32 -luser32 -lwinmm -ld3d11 -ldxguid -ld3dcompiler -lshlwapi -lole32 -loleaut32 -lcomdlg32 -lcomctl32 -lavrt -lksuser -ldbghelp -lshcore -femit-all-decls

popd
popd