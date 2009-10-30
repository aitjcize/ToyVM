FLAGS = -lm
objects = toyvm.o

toyvm: $(objects)
	cc -o toyvm $(objects) $(FLAGS)

install:
	mkdir /usr/share/toyvm
	cp -rf ../examples /usr/share/toyvm
	cp -f toyvm /usr/bin
clean:
	rm -rf toyvm $(objects)
