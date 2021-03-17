/*
 *    iolib, a very basic library to capture parse IO events in C
 *    Copyright (C) 2021  Nathaniel Choe
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define INSUFFICIENT_PRIVILEGE -2
#define ALREADY_GRABBED -1

#define BUFFER_SIZE 64

int parseInputs(int fd, char (*handleInput)(struct input_event)) {
	struct input_event *buffer;
	buffer = malloc(sizeof(struct input_event) * BUFFER_SIZE);
	for (;;) {
		int read_inputs = read(fd, buffer, sizeof(struct input_event) * BUFFER_SIZE) / sizeof(struct input_event);
		for (int i = 0; i < read_inputs; i++) {
			if (handleInput(buffer[i]) != 0) {
				close(fd);
				return 0;
			}
		}
	}

	close(fd);
}

int generateFileDescriptor(char *path) {
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return INSUFFICIENT_PRIVILEGE;

	int grabStatus = ioctl(fd, EVIOCGRAB, (void*) 1);
	if (grabStatus != 0) {
		close(fd);
		return ALREADY_GRABBED;
	}

	return fd;
}

int parseInputsByPath(char *path, char (*handleInput)(struct input_event)) {
	int fd = generateFileDescriptor(path);
	if (fd < 0)
		return fd;

	return parseInputs(fd, handleInput);
}
