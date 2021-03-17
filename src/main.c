/*
 * Gameplayer, a C program to allow for gaming with multiple keyboards
 * Copyright (C) 2021  Nathaniel Choe
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU general public license as published by * the Free Software Foundation, either version 3 of the License, or * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * */

#define KEYCODE_COUNT 16384
//There are 16384 possible keycodes because the input_event struct returns the code of the input in an unsigned short. The actual mappings, however, are stored in an int because I need extra controls.
#define SETTING_VALUE -1
#define FALSE 0
#define TRUE 1
#include <linux/uinput.h>
#include <string.h>
#include <stdio.h>
#include "iolib.h"

int mapping[KEYCODE_COUNT];
int quitKey;
int currentlyMappedCharacter;
int uinputfd;
char currentlyMapping;
//the handleInput function has to be stateless, so all states have to be stored in an even greater scope.
struct input_event reporter;

char handleInput(struct input_event event) {
	if (event.type != 1)
		return 0;
	//Now it's just keyboard events.

	if (quitKey == event.code && event.value == 1) {
		fprintf(stderr, "Exiting program\n");
		return 1;
		//handleInput always returns 0, and if handleInput returns anything other than 0, the program ends.
	}

	if (currentlyMapping) {
		if (event.value != 1)
			return 0;
		if (quitKey == SETTING_VALUE) {
			quitKey = event.code;
			printf("Start binding your keys now. Press the key to be bound, and then press the key to bind that to.\n");
			return 0;
		}

		if (currentlyMappedCharacter == SETTING_VALUE) {
			currentlyMappedCharacter = event.code;
			return 0;
		}
		if (mapping[currentlyMappedCharacter] != event.code) {
			mapping[currentlyMappedCharacter] = event.code;
			printf("You just bound %d to %hu. Bind an already existing binding to end the binding process.\n", currentlyMappedCharacter, event.code);
			currentlyMappedCharacter = SETTING_VALUE;
			return 0;
		}
		printf("This binding has already been set, ending the binding process.\n");
		currentlyMapping = FALSE;

		for (int i = 0; i < KEYCODE_COUNT; i++)
			if (mapping[i] != i)
				ioctl(uinputfd, UI_SET_KEYBIT, mapping[i]);
		sleep(1);
		struct uinput_setup usetup;
		usetup.id.bustype = BUS_USB;
		usetup.id.vendor = 0x2048;
		usetup.id.product = 0x1337;
		strcpy(usetup.name, "Definitely a real keyboard connected by USB.");
		ioctl(uinputfd, UI_DEV_SETUP, &usetup);
		ioctl(uinputfd, UI_DEV_CREATE);
		printf("Starting the listening process.\n");
		return 0;
	}

	event.code = mapping[event.code];
	write(uinputfd, &event, sizeof(event));
	write(uinputfd, &reporter, sizeof(reporter));

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Format: (sudo/doas) gameplayer [Event path]\n");
		return 1;
	}

	int fd = generateFileDescriptor(argv[1]);
	switch (fd) {
		case INSUFFICIENT_PRIVILEGE:
			printf("You don't have sufficient privileges for this operation. Try running as root?\n");
			exit(1);
		case ALREADY_GRABBED:
			printf("That keyboard is already being grabbed by some other program. Maybe you accidentally ran this program on the same keyboard twice?\n");
			exit(1);
	}

	for (int i = 0; i < KEYCODE_COUNT; i++)
		mapping[i] = i;
	//The default mappings are just the normal keymaps.
	quitKey = SETTING_VALUE;
	currentlyMappedCharacter = SETTING_VALUE;
	currentlyMapping = TRUE;
	uinputfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (uinputfd < 0) {
		printf("You don't have sufficient privileges for this operation. Try running as root?\n");
		exit(1);
	}
	ioctl(uinputfd, UI_SET_EVBIT, EV_KEY);
	reporter.type = EV_SYN;
	reporter.code = SYN_REPORT;
	reporter.value = 0;

	char (*functionPointer)(struct input_event);
	functionPointer = handleInput;

	printf("Please type the key you'll use to quit the program.\n");
	parseInputs(fd, functionPointer);
	sleep(1);
	ioctl(uinputfd, UI_DEV_DESTROY);
	exit(0);
}
