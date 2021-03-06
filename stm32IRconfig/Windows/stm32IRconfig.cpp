/**********************************************************************************************************
	stm32config: configure and monitor STM32IR

	Copyright (C) 2014-2015 Jörg Riechardt

	based on work by Alan Ott
	Copyright 2010  Alan Ott

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

***********************************************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"
#include <stdint.h>
#include <inttypes.h>

// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

hid_device *handle;
unsigned char inBuf[17];
unsigned char outBuf[17];

static bool open_stm32() {
	// Open the device using the VID, PID.
	handle = hid_open(0x1209, 0x4444, NULL);
	if (!handle) {
		printf("error opening stm32 device\n");
		return false;
	}
	printf("opened stm32 device\n");
	return true;
}

static void read_stm32() {
	int i;
	int retVal;
	retVal = hid_read(handle, inBuf, sizeof(inBuf));
	if (retVal < 0) {
		printf("read error\n");
	} else {
		printf("read %d bytes:\n\t", retVal);
		for (i = 0; i < retVal; i++)
			printf("%02hx ", inBuf[i]);
		puts("\n");
	}
} 

static void write_stm32() {
	int i;
	int retVal;
	retVal = hid_write(handle, outBuf, sizeof(outBuf));
	if (retVal < 0) {
		printf("write error: %ls\n", hid_error(handle));
	} else {
		printf("written %d bytes:\n\t", retVal);
		for (i = 0; i < retVal; i++)
			printf("%02hx ", outBuf[i]);
		puts("\n");
	}
}

void write_and_check() {
	write_stm32();
	#ifdef WIN32
	Sleep(2);
	#else
	usleep(2000);
	#endif
	read_stm32();
	while (inBuf[0] == 0x01)
		read_stm32();
	if (inBuf[1] == 0x01) { // STAT_SUCCESS
		puts("*****************************OK********************************\n");
	} else {
		puts("***************************ERROR*******************************\n");
	}
}

int main(int argc, char* argv[])
{
	uint64_t i;
	char c, d;
	uint8_t s, m, k, l, idx;
	int retValm, jump_to_firmware;
	jump_to_firmware = 0;

#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif

	if (hid_init())
		return -1;

	open_stm32();

	outBuf[0] = 0x03; // Report ID
	outBuf[1] = 0x00; // STAT_CMD

cont:	printf("program eeprom: wakeups and macros (p)\nprogram eeprom: wakeups and macros with remote control(P)\nget eeprom (wakeups, macros and capabilities) (g)\nreset (wakeups, macros and alarm) (r)\nset alarm (s)\nget alarm (a)\nsend IR (i)\nreboot (b)\nmonitor until ^C (m)\nexit (x)\n");
	scanf("%s", &c);

	switch (c) {

	case 'p':
prog:		printf("set wakeup(w)\nset macro(m)\n");
		scanf("%s", &d);
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x01; // ACC_SET
		switch (d) {
		case 'w':
			printf("enter slot number (starting with 0)\n");
			scanf("%u", &s);
			outBuf[idx++] = 0x05; // CMD_WAKE
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'm':
			printf("enter macro number (starting with 0)\n");
			scanf("%u", &m);
			outBuf[idx++] = 0x04; // CMD_MACRO
			outBuf[idx++] = m;    // (m+1)-th macro
			printf("enter slot number, 0 for trigger\n");
			scanf("%u", &s);
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		default:
			goto prog;
		}
		printf("enter IRData (protocoladdresscommandflag)\n");
		scanf("%I64x", &i);
		outBuf[idx++] = (i>>40) & 0xFF;
		outBuf[idx++] = (i>>24) & 0xFF;
		outBuf[idx++] = (i>>32) & 0xFF;
		outBuf[idx++] = (i>>8) & 0xFF;
		outBuf[idx++] = (i>>16) & 0xFF;
		outBuf[idx++] = i & 0xFF;
		write_and_check();
		break;

	case 'P':
Prog:		printf("set wakeup with remote control(w)\nset macro with remote control(m)\n");
		scanf("%s", &d);
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x01; // ACC_SET
		switch (d) {
		case 'w':
			printf("enter slot number (starting with 0)\n");
			scanf("%u", &s);
			outBuf[idx++] = 0x05; // CMD_WAKE
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'm':
			printf("enter macro number (starting with 0)\n");
			scanf("%u", &m);
			outBuf[idx++] = 0x04; // CMD_MACRO
			outBuf[idx++] = m;    // (m+1)-th macro
			printf("enter slot number, 0 for trigger\n");
			scanf("%u", &s);
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		default:
			goto Prog;
		}
		printf("enter IRData by pressing a button on the remote control\n");
		read_stm32();
		/* it is necessary, to have *all* IR codes received, *before* calling 
		* write_and_check(), in order to avoid, that these disturb later! */
		#ifdef WIN32
		Sleep(500); 
		#else
		usleep(500000);
		#endif
		outBuf[idx++] = inBuf[1];
		outBuf[idx++] = inBuf[2];
		outBuf[idx++] = inBuf[3];
		outBuf[idx++] = inBuf[4];
		outBuf[idx++] = inBuf[5];
		outBuf[idx++] = inBuf[6];
		write_and_check();
		break;

	case 'g':
get:		printf("get wakeup(w)\nget macro slot(m)\nget caps(c)\n");
		scanf("%s", &d);
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x00; // ACC_GET
		switch (d) {
		case 'w':
			printf("enter slot number (starting with 0)\n");
			scanf("%u", &s);
			outBuf[idx++] = 0x05; // CMD_WAKE
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'm':
			printf("enter macro number (starting with 0)\n");
			scanf("%u", &m);
			outBuf[idx++] = 0x04; // CMD_MACRO
			outBuf[idx++] = m;    // (m+1)-th macro
			printf("enter slot number, 0 for trigger\n");
			scanf("%u", &s);
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'c':
			outBuf[idx++] = 0x01; // CMD_CAPS
			for (l = 0; l < 20; l++) { // for safety stop after 20 loops
				outBuf[idx] = l;
				write_stm32();
				#ifdef WIN32
				Sleep(2);
				#else
				usleep(2000);
				#endif
				read_stm32();
				while (inBuf[0] == 0x01)
					read_stm32();
				if (!l) { // first query for slots and depth
					printf("macro_slots: %u\n", inBuf[4]);
					printf("macro_depth: %u\n", inBuf[5]);
					printf("wakeup_slots: %u\n", inBuf[6]);
				} else {
					if(!jump_to_firmware) { // queries for supported_protocols
						printf("protocols: ");
						for (k = 4; k < 17; k++) {
							if (!inBuf[k]) {
								printf("\n\n");
								jump_to_firmware = 1;
								goto again;
							}
							printf("%u ", inBuf[k]);
						}
					} else { // queries for firmware
						printf("firmware: ");
						for (k = 4; k < 17; k++) {
							if (!inBuf[k]) {
								printf("\n\n");
								goto out;
							}
							printf("%c", inBuf[k]);
						}
					}
				}
				printf("\n\n");
again:			;
			}
			break;
		default:
			goto get;
		}
		write_and_check();
out:		break;

	case 'r':
reset:		printf("reset wakeup(w)\nreset macro slot(m)\nreset alarm(a)\n");
		scanf("%s", &d);
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x02; // ACC_RESET
		switch (d) {
		case 'w':
			printf("enter slot number (starting with 0)\n");
			scanf("%u", &s);
			outBuf[idx++] = 0x05; // CMD_WAKE
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'm':
			printf("enter macro number (starting with 0)\n");
			scanf("%u", &m);
			outBuf[idx++] = 0x04; // CMD_MACRO
			outBuf[idx++] = m;    // (m+1)-th macro
			printf("enter slot number, 0 for trigger\n");
			scanf("%u", &s);
			outBuf[idx++] = s;    // (s+1)-th slot
			break;
		case 'a':
			outBuf[idx++] = 0x03; // CMD_ALARM
			break;
		default:
			goto reset;
		}
		write_and_check();
		break;

	case 's':
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x01; // ACC_SET
		outBuf[idx++] = 0x03; // CMD_ALARM
		printf("enter alarm\n");
		scanf("%I64x", &i);
		memcpy(&outBuf[idx++], &i, 4);
		write_and_check();
		break;

	case 'a':
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x00; // ACC_GET
		outBuf[idx++] = 0x03; // CMD_ALARM
		write_and_check();
		break;

	case 'i':
		printf("enter IRData (protocoladdresscommandflag)\n");
		scanf("%I64x", &i);
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x01; // ACC_SET
		outBuf[idx++] = 0x00; // CMD_EMIT
		outBuf[idx++] = (i>>40) & 0xFF;
		outBuf[idx++] = (i>>24) & 0xFF;
		outBuf[idx++] = (i>>32) & 0xFF;
		outBuf[idx++] = (i>>8) & 0xFF;
		outBuf[idx++] = (i>>16) & 0xFF;
		outBuf[idx++] = i & 0xFF;
		write_and_check();
		break;

	case 'b':
		memset(&outBuf[2], 0, 15);
		idx = 2;
		outBuf[idx++] = 0x01; // ACC_SET
		outBuf[idx++] = 0x06; // CMD_REBOOT
		write_and_check();
		#ifdef WIN32
		Sleep(2500);
		#else
		usleep(2500000);
		#endif
		hid_close(handle);
		open_stm32();
		break;

	case 'm':
		goto monit;
		break;

	case 'x':
		goto exit;
		break;

	default:
		goto cont;
	}

	goto cont;

monit:	while(true) {
		retValm = hid_read(handle, inBuf, sizeof(inBuf));
		if (retValm >= 0) {
			printf("read %d bytes:\n\t", retValm);
			for (l = 0; l < retValm; l++)
				printf("%02hx ", inBuf[l]);
			printf("\n");
			printf("converted to protocoladdresscommandflag:\n\t");
			printf("%02hx%02hx%02hx%02hx%02hx%02hx", inBuf[1],inBuf[3],inBuf[2],inBuf[5],inBuf[4],inBuf[6]);
			printf("\n\n");
		}
	}
	
exit:	hid_close(handle);

	/* Free static HIDAPI objects. */
	hid_exit();

#ifdef WIN32
	Sleep(2);
#endif

	return 0;
}
