all: build/multi_is_perihamiltonian

MULTI_SHARED = shared/multicode_base.c\
               shared/multicode_input.c\
               shared/multicode_output.c\
               
clean:

build/multi_is_perihamiltonian: multi_is_perihamiltonian.c $(MULTI_SHARED)
	mkdir -p build
	$(CC) -o $@ -O4 $^
