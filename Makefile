CXX = g++-5
CXXFLAGS = -std=c++14 -Werror=vla
EXEC = main
OBJECTS = asm.o scanner.o
DEPENDS = ${OBJECTS.o: .d}

${EXEC}: ${OBJECTS}
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC}
-include ${DEPENDS}

.PHONY: clean

clean:
	rm ${OBJECTS} ${EXEC} ${DEPENDS}
