# GMON_OUT_PREFIX=`Now`
CXX = g++
CXXFLAGS = -std=c++11 -O3 -MMD -Wall -lSDL2main -lSDL2 -fcompare-debug-second
# DEBUG = -pg -g
# SPEED = -O3
EXEC = $(shell basename $(CURDIR)).out
LIBS = 
OBJECTS = main.o drawer.o movement.o turn.o score.o bot.o
DEPENDS = ${OBJECTS:.o=.d}
${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} ${CXXFLAGS} -o test.out ${LIBS}
.PHONY: build
build: ${OBJECTS}
	${CXX} ${OBJECTS} ${CXXFLAGS} -o ${EXEC} ${LIBS}
-include ${DEPENDS}
.PHONY: clean
clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}
# gmon: gmon.out
# 	fname=gmon/gmon_$(Now).out
# 	if [-f "$fname"]; then		echo "$fname exists.";	else		mv gmon.out $fname;	fi
	
