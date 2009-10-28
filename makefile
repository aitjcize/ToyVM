CFLAGS = -lm
objects = toyvm.o

toyvm: $(objects)
	cc -o toyvm $(objects) $(CFLAGS)

clean:
	rm -rf toyvm $(objects)
