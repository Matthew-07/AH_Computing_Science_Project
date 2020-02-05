#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

bool inline sendData(SOCKET& s, char* buff, int dataLength)
{
	while (dataLength > 0) {
		int res = send(s, buff, dataLength, 0);
		if (res > 0) {
			dataLength -= res;
			buff += res;
		}
		else {
			// An error occured!
			return false;
		}
	}
	return true;
}

bool inline recieveData(SOCKET& s, char* buff, int dataLength)
{
	while (dataLength > 0) {
		int res = recv(s, buff, dataLength, 0);
		if (res > 0) {
			dataLength -= res;
			buff += res;
		}
		else {
			// An error occured!
			return false;
		}
	}
	return true;
}