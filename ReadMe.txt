PVR2MDL

Developed by Alexey Leusin, Novosibirsk, 2018
If you like my work and want to buy me a beer =), you can do it here: paypal.me/AlexeyLN

This is fork of my PS2 HL mdltool that is intended to convert
Dreamcast Half-Life *.MDL models with *.PVR textures to
normal format, that is recognisable by PC Half-Life.

How to use:
1) Windows explorer - drag and drop model file on pvr2mdl.exe
2) Command line\Batch:
		pvr2mdl [filename]
	Optional feature - extract textures from model (in *.BMP format):
		pvr2mdl extract [filename]

Original models would be backuped in "***-backup.mdl" files.

I found no sources that explain how twiddling works in words, so
here is my explanation:
- twiddling is appliable to square images only
- image should be stored as one continous block in memory
- to convert x and y coordinates to twiddled first you need to double
	position of every set bit inside them.
	Example: x = 10, y = 7 => x = 1010b, y = 111b
	x has set bits at positions 1 and 3 so new positions are 2 and 6:
		new_x = 1000100b = 68;
	y has set bits at 0, 1 and 2, so new positions are: 0, 2, 4.
		new_y = 10101b = 21;
- end result of twiddleing is linear pixel number that is calculated by
	applying bitwise or on new_y and double value of new_x.
	Example: let's find linear pixel number for values from
	previous section:
	x_new * 2 = 68 * 2 = 136 = 10001000b;
	Bitwise or: 10001000b | 10101b = 10011101b = 157;
	So the end result is: OriginalImage[10][7] = TwiddledImage[157];

Sources:
Got info on general structure of PVRs here:
	http://fabiensanglard.net/Mykaruga/tools/segaPVRFormat.txt
Got info on how twiddling works here:
	https://github.com/yevgeniy-logachev/spvr2png/blob/master/SegaPVRImage.c

And finally, some formal stuff:
=====================================================================
LICENSE
=====================================================================
Copyright (c) 2018, Alexey Leushin
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the copyright holders nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
=====================================================================
