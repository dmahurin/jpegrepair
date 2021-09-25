#!/usr/bin/env python
# tool to find and print jpg segments in given file

import sys
from struct import unpack

if(len(sys.argv) < 2):
	print("usage: %s JPEGDATAFILE [MARKER]" % sys.argv[0])
	sys.exit(1)

f = open(sys.argv[1], "rb")
s = f.read()

marker_name = {
    0xd8: "SOI",
    0xe0: "APP0",
    0xe1: "APP1",
    0xdb: "DQT",
    0xc0: "SOF",
    0xc4: "DHT",
    0xda: "SOS",
    0xd9: "EOI",
    0: "ECS"
}

segments = {}
offset = 0

while True:
	f = s.find(b"\xff", offset)
	if(f < 0): break

	offset = f+2
	if(offset <= len(s)):
		m = ord(s[f+1:f+2])
		if(m == 0xda or m == 0xdb or m == 0xd8 or m == 0xd9 or m == 0xe0 or m == 0xe1 or m == 0xc0 or m == 0xc4):
			segments[f] = m

next_segment = {}
segment_size = {}

end = 0
for f in sorted(segments.keys()):
	if segments[f] == 0xd8 or segments[f] == 0xd9:
		chunklen = 0
	else:
		chunklen, = unpack(">H", s[f+2:f+4])

	segment_size[f] = chunklen + 2

	# remove segments in between
	if f < end:
		del segments[f]
		continue

	end = f+2+chunklen

	# connect with next marker, or delete if no next marker
	if segments[f] == 0xd9 and f == len(s) - 2:
		pass
	elif f+2+chunklen in segments:
		next_segment[f+2+chunklen] = f
	elif next_segment.get(f) == None:
		del(segments[f])

# connect to first segment in chain
for f in sorted(next_segment.keys()):
	if next_segment[f] in next_segment:
		x = next_segment[f]
		next_segment[f] = next_segment[x]
		del next_segment[x]

# create sythetic ECS/0 offset
for f in sorted(next_segment.keys()):
	if segments[f] == 0xd8 or segments[f] == 0xe0 or segments[f] == 0xe1: continue

	segments[f+segment_size[f]] = 0;
	segment_size[f+segment_size[f]] = 0;

# print last match of marker name or all segment offsets
if len(sys.argv) > 2:
	for f in reversed(sorted(segments.keys())):
		if marker_name[segments[f]] == sys.argv[2].upper(): break
	print(f)
else:
	for f in sorted(segments.keys()):
		print("%s %d" % (marker_name[segments[f]], f))

