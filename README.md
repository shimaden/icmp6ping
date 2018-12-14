# icmp6ping
Learning ICMPv6

## Description
Send ICMP6_ECHO_REQUEST.

## To do
Unify the coding style. Since I began with writing in C seeing tiny examples on the Internet and pieces of source code of some open source software, was frustrated by wiring low level operation in C and decided to change the programming language to C++, some kinds of coding styles are mixed in this program.

Confirm the checksums of received ICMP6_ECHO_REPLY packets.

Assign an appropriate unicast address to the destination address in an ethernet frame header. Currently, the destination addresses of all ethernet headers are assigned the broadcast address FF:FF:FF:FF:FF:FF.

## Author
Shimaden
