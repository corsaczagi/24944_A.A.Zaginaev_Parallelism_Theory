CXX = pgc++
CXXFLAGS = -acc -Minfo=all -O3 -std=c++11
LDFLAGS = -lboost_program_options

SOURCES = heat_solver.cpp
TARGET = heat_solver

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run-cpu: $(TARGET)
	./$(TARGET) --size 512 --precision 1e-6

run-gpu: $(TARGET)
	ACC_DEVICE_TYPE=nvidia ./$(TARGET) --size 512 --precision 1e-6

run-multicore: $(TARGET)
	ACC_NUM_CORES=8 ./$(TARGET) --size 512 --precision 1e-6

profile:
	nsys profile -o heat_solver_profile --stats=true ./$(TARGET) --size 256 --iterations 50

.PHONY: all clean run-cpu run-gpu run-multicore profile