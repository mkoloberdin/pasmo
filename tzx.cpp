// tzx.cpp
// Revision 9-aug-2005

#include "tzx.h"

#include "pasmotypes.h"

#include "trace.h"


namespace pasmo {


void tzx::writefilehead (std::ostream & out)
{
	// TZX header.
	static const char tzxhead []= {
		'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1A, // TZX ID.
		1, 13, // TZX format version.
	};
	out.write (tzxhead, sizeof (tzxhead) );
}

void tzx::writestandardblockhead (std::ostream & out, address pause)
{
	out.put ('\x10'); // Standard speed block.
	out.put (lobyte (pause) ); // Pause after block
	out.put (hibyte (pause) ); // in milisecs.
}

void tzx::writeturboblockhead (std::ostream & out, size_t len)
{
	ASSERT (len <= 0xFFFFFF);

	out.put (0x11); // Block type.

	putword (out, 0x0380); // Pilot pulse
	putword (out, 0x01C0); // Sync first pulse
	putword (out, 0x01C0); // Sync second pulse
	putword (out, 0x01C0); // Zero bit pulse
	putword (out, 0x0380); // One bit pulse
	putword (out, 0x1000); // Pilot tone
	out.put (0x08);        // Bits used in last byte
	//putword (out, 0x03E8); // Pause after block
	putword (out, 0x0970); // Pause after block
	// Length of data
	putword (out, len & 0xFFFF);
	out.put ( (len >> 16) & 0xFF);
}

} // namespace pasmo

// End of tzx.cpp
