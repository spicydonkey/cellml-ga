CC=mpiicpc

# User-configuration
##############################################
XMLPARSER_PATH=/PATH/TO/ADVXMLPARSER/DIRECTORY	# path to the AdvXMLParser directory
CELLML_PATH=/PATH/TO/CELLML_API/DIRECTORY	# path to CellML API directory (cellml-sdk)
##############################################

CFLAGS=-I $(XMLPARSER_PATH) -I $(CELLML_PATH)/include -O
LFLAGS=-L $(XMLPARSER_PATH) -ladvxml -L $(CELLML_PATH)/lib -lcellml -lcis -Wl,-rpath=$(CELLML_PATH)/lib 

SOURCES=experiment.cpp virtexp.cpp utils.cpp cellml_observer.cpp distributor.cpp 
INCLUDES=virtexp.h utils.h GAEngine.h GAEngine.cpp cellml_observer.h distributor.h


all: experiment

experiment: $(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) $(SOURCES) -o experiment $(LFLAGS)

clean:
	rm -f experiment *~ *.o
