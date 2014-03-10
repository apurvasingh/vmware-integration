#
# Makefile for vmhalo program
#
OBJS=vmhalo.o vixconnection.o vmhconfig.o

vmhalo: $(OBJS)
	g++ -g -o vmhalo $(OBJS) -lvixAllProducts -ldl -lpthread
	cp vmhalo ../bin

vmhalo.o: vmhalo.cpp vixconnection.hpp vmhconfig.hpp
	g++ -g -c -I/usr/include/vmware-vix/ vmhalo.cpp

vmhconfig.o: vmhconfig.cpp vmhconfig.hpp
	g++ -g -c -I/usr/include/vmware-vix/ vmhconfig.cpp

vixconnection.o: vixconnection.cpp vixconnection.hpp
	g++ -g -c -I/usr/include/vmware-vix/ vixconnection.cpp

clean:
	rm -f vmhalo $(OBJS)
