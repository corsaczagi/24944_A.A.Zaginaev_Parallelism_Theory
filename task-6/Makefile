CXX = pgc++
CXXFLAGS = -acc -Minfo=all -O3 -std=c++11 -ta=tesla:managed
LDFLAGS = 

TARGET = heat_solver
SOURCES = heat_solver.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run-cpu:
	./$(TARGET) --size 512 --precision 1e-6 --cpu

run-gpu:
	./$(TARGET) --size 512 --precision 1e-6

run-small:
	./$(TARGET) --size 128 --precision 1e-6

print:
	./$(TARGET) --size 10 --print

bench:
	for size in 128 256 512 1024; do \
		echo "=== Size: $$size ==="; \
		echo -n "CPU: "; \
		./$(TARGET) --size $$size --precision 1e-6 --cpu 2>/dev/null | grep "Time:"; \
		echo -n "GPU: "; \
		./$(TARGET) --size $$size --precision 1e-6 2>/dev/null | grep "Time:"; \
		echo ""; \
	done

profile:
	nsys profile -o heat_profile --stats=true ./$(TARGET) --size 256 --iterations 50

.PHONY: all clean run-cpu run-gpu run-small print bench profile