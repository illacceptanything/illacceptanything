all: clean lin32 lin64

clean:
	rm -f rae.zip

win64:
	x86_64-w64-mingw32-gcc rae.c -o rae-win64.exe

win32:
	i686-w64-mingw32-gcc rae.c -o rae-win32.exe

lin32:
	gcc -m32 rae.c -o rae-lin32

lin64:
	gcc -m64 rae.c -o rae-lin64

zipup:
	zip rae.zip *
