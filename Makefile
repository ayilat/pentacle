make:
	clang main.c manager.c bar.c $(shell pkg-config --cflags --libs x11 cairo)
