CXX = g++
CXXFLAGS = -g -MMD -Wall -lSDL2main -lSDL2
EXEC = $(shell basename $(CURDIR)).out
LIBS = 
OBJECTS = main.o
DEPENDS = ${OBJECTS:.o=.d}
${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} ${CXXFLAGS} -o ${EXEC} ${LIBS}
-include ${DEPENDS}
.PHONY: clean
clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}

