CXX = g++
CXXFLAGS_RELEASE = -std=c++17 -O3 -DNDEBUG -Wall -Wextra -march=native -ffast-math
CXXFLAGS_DEBUG = -std=c++17 -O0 -g -Wall -Wextra -DDEBUG
CXXFLAGS = $(CXXFLAGS_RELEASE)

SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = tsp_optimization
TARGET_DEBUG = tsp_optimization_debug

# Archivos de cabecera para dependencias
HEADERS = point.h kd_tree.h tour_utils.h two_opt.h

.PHONY: all clean debug release test benchmark help

# Target por defecto (release)
all: release

# Build optimizado para rendimiento
release: CXXFLAGS = $(CXXFLAGS_RELEASE)
release: $(TARGET)

# Build para debugging
debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: $(TARGET_DEBUG)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)
	@echo "Build release completado: $(TARGET)"

$(TARGET_DEBUG): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET_DEBUG)
	@echo "Build debug completado: $(TARGET_DEBUG)"

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tests automatizados
test: $(TARGET)
	@echo "Ejecutando tests básicos..."
	./$(TARGET) 50 42 random
	./$(TARGET) 50 42 clustered
	@echo "Tests completados exitosamente."

# Benchmark con diferentes tamaños
benchmark: $(TARGET)
	@echo "Ejecutando benchmark completo..."
	@echo "=== Benchmark 50 puntos ==="
	./$(TARGET) 50 42 random
	@echo "=== Benchmark 100 puntos ==="
	./$(TARGET) 100 42 random
	@echo "=== Benchmark 200 puntos ==="
	./$(TARGET) 200 42 random
	@echo "=== Benchmark con clustering ==="
	./$(TARGET) 100 42 clustered

# Perfilado de rendimiento (requiere valgrind)
profile: $(TARGET_DEBUG)
	@echo "Ejecutando análisis de rendimiento..."
	valgrind --tool=callgrind ./$(TARGET_DEBUG) 100 42 random
	@echo "Resultados en callgrind.out.*"

# Análisis de memoria (requiere valgrind)
memcheck: $(TARGET_DEBUG)
	@echo "Ejecutando análisis de memoria..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_DEBUG) 50 42 random

# Limpieza
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_DEBUG)
	rm -f tsp_results.txt
	rm -f callgrind.out.*
	@echo "Archivos de build eliminados."

# Información de ayuda
help:
	@echo "Targets disponibles:"
	@echo "  all/release  - Build optimizado (por defecto)"
	@echo "  debug        - Build con información de debug"
	@echo "  test         - Ejecutar tests básicos"
	@echo "  benchmark    - Ejecutar benchmark completo"
	@echo "  profile      - Análisis de rendimiento (requiere valgrind)"
	@echo "  memcheck     - Análisis de memoria (requiere valgrind)"
	@echo "  clean        - Limpiar archivos generados"
	@echo "  help         - Mostrar esta ayuda"
	@echo ""
	@echo "Uso del programa:"
	@echo "  ./tsp_optimization [num_points] [seed] [random|clustered]"
	@echo "  Ejemplo: ./tsp_optimization 200 123 clustered"

# Instalación local (opcional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "Programa instalado en /usr/local/bin/"

# Generar documentación (requiere doxygen)
docs:
	doxygen Doxyfile
	@echo "Documentación generada en docs/" 