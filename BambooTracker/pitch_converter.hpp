#pragma once

#include <cstdint>
#include "misc.hpp"

class PitchConverter
{
public:
	static uint16_t getPitchFM(Note note, int octave, int fine);
	static uint16_t getPitchPSG(Note note, int octave, int fine);

private:
	static const uint16_t centTableFM_[384];
	static const uint16_t centTablePSG_[384];

	PitchConverter();
	static void calc(int& index, int& octave);
};