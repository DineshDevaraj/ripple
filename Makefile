
GPP = g++

LIB = -lpthread

INC = ripple.h \
      soketpp.h
      
SRV = demo.cpp \
      ripple.cpp \
      soketpp.cpp

CLN = client.cpp \
      soketpp.cpp

all : ripple.x client.x

ripple.x : $(INC) $(SRV)
	$(GPP) $(FLG) $(SRV) $(LIB) -o $@

client.x : $(INC) $(CLN)
	$(GPP) $(FLG) $(CLN) $(LIB) -o $@

clean :
	rm *.x
