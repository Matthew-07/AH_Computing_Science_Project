#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

// Client -> Coordinator commands
#define JOIN_QUEUE		00001
#define LEAVE_QUEUE		00002
#define SWITCH_ACCOUNT	00003

// Coordinator -> Client
#define LEFT_QUEUE		10001
#define GAME_FOUND		10002

// Coordinator -> Server commands
#define START_GAME		20001

// Server -> Coordinator
#define GAME_FINISHED 	30001




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
