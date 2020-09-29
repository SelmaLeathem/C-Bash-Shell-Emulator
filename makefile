# Name: Selma Leathem
# Date: 2/24/2020
# Description: makefile for bashShell

CXX = gcc
CXXFLAGS = -g #remove before submit

OBJS = pid_tDynArr.o bashShell.o 

SRCS = pid_tDynArr.c bashShell.c 

HEADERS = pid_tDynArr.h bashShell.h 


bashShell: ${OBJS} ${HEADERS}
	${CXX} ${OBJS} -o bashShell

${OBJS}: ${SRCS}
	${CXX} ${CXXFLAGS} -c $(@:.o=.c)

clean:
	rm *.o bashShell