FLAGS = -lm
TARGET = toyvm
OBJECTS = toyvm.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(FLAGS)

install:
	cp -f $(TARGET) /usr/bin
clean:
	rm -rf $(TARGET) $(OBJECTS)
