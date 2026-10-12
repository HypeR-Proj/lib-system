/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

static int require(int condition)
{
	return condition ? 0 : 1;
}

int main(void)
{
	unsigned char source[] = {1, 2, 3, 4, 5};
	unsigned char destination[5] = {0};
	unsigned char overlap[] = {1, 2, 3, 4, 5};

	if (require(memcpy(destination, source, sizeof(source)) == destination) ||
	    require(memcmp(destination, source, sizeof(source)) == 0) ||
	    require(memmove(overlap + 1, overlap, 4) == overlap + 1) ||
	    require(overlap[0] == 1 && overlap[1] == 1 && overlap[4] == 4) ||
	    require(memset(destination, 0xa5, sizeof(destination)) == destination) ||
	    require(destination[0] == 0xa5 && destination[4] == 0xa5) ||
	    require(strlen("HypeR") == 5)) {
		return 1;
	}
	return 0;
}
