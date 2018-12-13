EXEC = icmp6ping
OBJS = icmp6ping.o checksum.o iface.o setup_socket.o send_echo6.o print_echo6.o

all: $(EXEC)

icmp6ping: $(OBJS)
	g++ -o $@ $(OBJS) -lpthread

endian.h: ckendian
chkendian: chkendian.o

.c.o:
	gcc -pthread -Wall -c $<

.cpp.o:
	g++ -pthread -Wall -c $<

checksum.o: checksum.c checksum.h main.h
iface.o: iface.cpp iface.h
setup_socket.o: setup_socket.cpp setup_socket.h
send_echo6.o: send_echo6.cpp send_echo6.h
print_echo6.o: print_echo6.cpp print_echo6.h main.h

clean:
	rm -f $(EXEC)
	rm -f *~ *.o $(OBJS)
