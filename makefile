# Date: 2/24/2020
# Description: makefile for smallsh

CXX = gcc
CXXFLAGS = -g #remove before submit

OBJS = pid_tDynArr.o smallsh.o 

SRCS = pid_tDynArr.c smallsh.c 

HEADERS = pid_tDynArr.h smallsh.h 


smallsh: ${OBJS} ${HEADERS}
	${CXX} ${OBJS} -o smallsh

${OBJS}: ${SRCS}
	${CXX} ${CXXFLAGS} -c $(@:.o=.c)

clean:
	rm *.o smallsh