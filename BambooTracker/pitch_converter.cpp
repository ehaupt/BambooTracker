#include "pitch_converter.hpp"

uint16_t PitchConverter::getPitchFM(Note note, int octave, int fine)
{
	int index = static_cast<int>(note) + fine;
	PitchConverter::calc(index, octave);
	return centTableFM_[index] | (octave << 11);
}

uint16_t PitchConverter::getPitchPSG(Note note, int octave, int fine)
{
	int index = static_cast<int>(note) + fine;
	PitchConverter::calc(index, octave);
	return centTablePSG_[index] >> octave;
}

const uint16_t PitchConverter::centTableFM_[384] = {
	0x26a, 0x26b, 0x26c, 0x26e, 0x26f, 0x270, 0x271, 0x272, 0x273, 0x274, 0x276, 0x277,
	0x278, 0x279, 0x27a, 0x27b, 0x27c, 0x27e, 0x27f, 0x280, 0x281, 0x282, 0x283, 0x284,
	0x286, 0x287, 0x288, 0x289, 0x28a, 0x28b, 0x28d, 0x28e, 0x28f, 0x290, 0x291, 0x293,
	0x294, 0x295, 0x296, 0x297, 0x299, 0x29a, 0x29b, 0x29c, 0x29d, 0x29f, 0x2a0, 0x2a1,
	0x2a2, 0x2a3, 0x2a5, 0x2a6, 0x2a7, 0x2a8, 0x2aa, 0x2ab, 0x2ac, 0x2ad, 0x2ae, 0x2b0,
	0x2b1, 0x2b2, 0x2b3, 0x2b5, 0x2b6, 0x2b7, 0x2b8, 0x2ba, 0x2bb, 0x2bc, 0x2be, 0x2bf,
	0x2c0, 0x2c1, 0x2c3, 0x2c4, 0x2c5, 0x2c6, 0x2c8, 0x2c9, 0x2ca, 0x2cc, 0x2cd, 0x2ce,
	0x2cf, 0x2d1, 0x2d2, 0x2d3, 0x2d5, 0x2d6, 0x2d7, 0x2d9, 0x2da, 0x2db, 0x2dd, 0x2de,
	0x2df, 0x2e1, 0x2e2, 0x2e3, 0x2e5, 0x2e6, 0x2e7, 0x2e9, 0x2ea, 0x2eb, 0x2ed, 0x2ee,
	0x2ef, 0x2f1, 0x2f2, 0x2f3, 0x2f5, 0x2f6, 0x2f7, 0x2f9, 0x2fa, 0x2fc, 0x2fd, 0x2fe,
	0x300, 0x301, 0x303, 0x304, 0x305, 0x307, 0x308, 0x30a, 0x30b, 0x30c, 0x30e, 0x30f,
	0x311, 0x312, 0x313, 0x315, 0x316, 0x318, 0x319, 0x31b, 0x31c, 0x31d, 0x31f, 0x320,
	0x322, 0x323, 0x325, 0x326, 0x328, 0x329, 0x32a, 0x32c, 0x32d, 0x32f, 0x330, 0x332,
	0x333, 0x335, 0x336, 0x338, 0x339, 0x33b, 0x33c, 0x33e, 0x33f, 0x341, 0x342, 0x344,
	0x345, 0x347, 0x348, 0x34a, 0x34b, 0x34d, 0x34e, 0x350, 0x351, 0x353, 0x355, 0x356,
	0x358, 0x359, 0x35b, 0x35c, 0x35e, 0x35f, 0x361, 0x362, 0x364, 0x366, 0x367, 0x369,
	0x36a, 0x36c, 0x36d, 0x36f, 0x371, 0x372, 0x374, 0x375, 0x377, 0x379, 0x37a, 0x37c,
	0x37d, 0x37f, 0x381, 0x382, 0x384, 0x386, 0x387, 0x389, 0x38a, 0x38c, 0x38e, 0x38f,
	0x391, 0x393, 0x394, 0x396, 0x398, 0x399, 0x39b, 0x39d, 0x39e, 0x3a0, 0x3a2, 0x3a3,
	0x3a5, 0x3a7, 0x3a8, 0x3aa, 0x3ac, 0x3ad, 0x3af, 0x3b1, 0x3b3, 0x3b4, 0x3b6, 0x3b8,
	0x3b9, 0x3bb, 0x3bd, 0x3bf, 0x3c0, 0x3c2, 0x3c4, 0x3c6, 0x3c7, 0x3c9, 0x3cb, 0x3cd,
	0x3ce, 0x3d0, 0x3d2, 0x3d4, 0x3d5, 0x3d7, 0x3d9, 0x3db, 0x3dd, 0x3de, 0x3e0, 0x3e2,
	0x3e4, 0x3e5, 0x3e7, 0x3e9, 0x3eb, 0x3ed, 0x3ef, 0x3f0, 0x3f2, 0x3f4, 0x3f6, 0x3f8,
	0x3f9, 0x3fb, 0x3fd, 0x3ff, 0x401, 0x403, 0x405, 0x406, 0x408, 0x40a, 0x40c, 0x40e,
	0x410, 0x412, 0x414, 0x415, 0x417, 0x419, 0x41b, 0x41d, 0x41f, 0x421, 0x423, 0x425,
	0x427, 0x428, 0x42a, 0x42c, 0x42e, 0x430, 0x432, 0x434, 0x436, 0x438, 0x43a, 0x43c,
	0x43e, 0x440, 0x442, 0x444, 0x446, 0x448, 0x44a, 0x44c, 0x44e, 0x450, 0x452, 0x454,
	0x456, 0x458, 0x45a, 0x45c, 0x45e, 0x460, 0x462, 0x464, 0x466, 0x468, 0x46a, 0x46c,
	0x46e, 0x470, 0x472, 0x474, 0x476, 0x478, 0x47a, 0x47c, 0x47e, 0x480, 0x483, 0x485,
	0x487, 0x489, 0x48b, 0x48d, 0x48f, 0x491, 0x493, 0x495, 0x498, 0x49a, 0x49c, 0x49e,
	0x4a0, 0x4a2, 0x4a4, 0x4a6, 0x4a9, 0x4ab, 0x4ad, 0x4af, 0x4b1, 0x4b3, 0x4b6, 0x4b8,
	0x4ba, 0x4bc, 0x4be, 0x4c1, 0x4c3, 0x4c5, 0x4c7, 0x4c9, 0x4cc, 0x4ce, 0x4d0, 0x4d2
};

const uint16_t PitchConverter::centTablePSG_[384] = {
	0xee8, 0xee1, 0xeda, 0xed4, 0xecd, 0xec6, 0xebf, 0xeb8, 0xeb1, 0xeab, 0xea4, 0xe9d,
	0xe96, 0xe90, 0xe89, 0xe82, 0xe7c, 0xe75, 0xe6e, 0xe67, 0xe61, 0xe5a, 0xe54, 0xe4d,
	0xe46, 0xe40, 0xe39, 0xe33, 0xe2c, 0xe26, 0xe1f, 0xe18, 0xe12, 0xe0b, 0xe05, 0xdff,
	0xdf8, 0xdf2, 0xdeb, 0xde5, 0xdde, 0xdd8, 0xdd2, 0xdcb, 0xdc5, 0xdbe, 0xdb8, 0xdb2,
	0xdab, 0xda5, 0xd9f, 0xd99, 0xd92, 0xd8c, 0xd86, 0xd7f, 0xd79, 0xd73, 0xd6d, 0xd67,
	0xd60, 0xd5a, 0xd54, 0xd4e, 0xd48, 0xd42, 0xd3c, 0xd35, 0xd2f, 0xd29, 0xd23, 0xd1d,
	0xd17, 0xd11, 0xd0b, 0xd05, 0xcff, 0xcf9, 0xcf3, 0xced, 0xce7, 0xce1, 0xcdb, 0xcd5,
	0xccf, 0xcc9, 0xcc3, 0xcbe, 0xcb8, 0xcb2, 0xcac, 0xca6, 0xca0, 0xc9a, 0xc95, 0xc8f,
	0xc89, 0xc83, 0xc7d, 0xc78, 0xc72, 0xc6c, 0xc66, 0xc61, 0xc5b, 0xc55, 0xc50, 0xc4a,
	0xc44, 0xc3f, 0xc39, 0xc33, 0xc2e, 0xc28, 0xc22, 0xc1d, 0xc17, 0xc12, 0xc0c, 0xc06,
	0xc01, 0xbfb, 0xbf6, 0xbf0, 0xbeb, 0xbe5, 0xbe0, 0xbda, 0xbd5, 0xbcf, 0xbca, 0xbc5,
	0xbbf, 0xbba, 0xbb4, 0xbaf, 0xba9, 0xba4, 0xb9f, 0xb99, 0xb94, 0xb8f, 0xb89, 0xb84,
	0xb7f, 0xb79, 0xb74, 0xb6f, 0xb69, 0xb64, 0xb5f, 0xb5a, 0xb54, 0xb4f, 0xb4a, 0xb45,
	0xb40, 0xb3a, 0xb35, 0xb30, 0xb2b, 0xb26, 0xb21, 0xb1b, 0xb16, 0xb11, 0xb0c, 0xb07,
	0xb02, 0xafd, 0xaf8, 0xaf3, 0xaee, 0xae9, 0xae4, 0xadf, 0xad9, 0xad4, 0xacf, 0xaca,
	0xac6, 0xac1, 0xabc, 0xab7, 0xab2, 0xaad, 0xaa8, 0xaa3, 0xa9e, 0xa99, 0xa94, 0xa8f,
	0xa8a, 0xa86, 0xa81, 0xa7c, 0xa77, 0xa72, 0xa6d, 0xa69, 0xa64, 0xa5f, 0xa5a, 0xa55,
	0xa51, 0xa4c, 0xa47, 0xa42, 0xa3e, 0xa39, 0xa34, 0xa2f, 0xa2b, 0xa26, 0xa21, 0xa1d,
	0xa18, 0xa13, 0xa0f, 0xa0a, 0xa05, 0xa01, 0x9fc, 0x9f8, 0x9f3, 0x9ee, 0x9ea, 0x9e5,
	0x9e1, 0x9dc, 0x9d8, 0x9d3, 0x9ce, 0x9ca, 0x9c5, 0x9c1, 0x9bc, 0x9b8, 0x9b3, 0x9af,
	0x9aa, 0x9a6, 0x9a2, 0x99d, 0x999, 0x994, 0x990, 0x98b, 0x987, 0x983, 0x97e, 0x97a,
	0x975, 0x971, 0x96d, 0x968, 0x964, 0x960, 0x95b, 0x957, 0x953, 0x94e, 0x94a, 0x946,
	0x942, 0x93d, 0x939, 0x935, 0x931, 0x92c, 0x928, 0x924, 0x920, 0x91b, 0x917, 0x913,
	0x90f, 0x90b, 0x906, 0x902, 0x8fe, 0x8fa, 0x8f6, 0x8f2, 0x8ee, 0x8e9, 0x8e5, 0x8e1,
	0x8dd, 0x8d9, 0x8d5, 0x8d1, 0x8cd, 0x8c9, 0x8c5, 0x8c1, 0x8bd, 0x8b9, 0x8b4, 0x8b0,
	0x8ac, 0x8a8, 0x8a4, 0x8a0, 0x89c, 0x899, 0x895, 0x891, 0x88d, 0x889, 0x885, 0x881,
	0x87d, 0x879, 0x875, 0x871, 0x86d, 0x869, 0x865, 0x862, 0x85e, 0x85a, 0x856, 0x852,
	0x84e, 0x84a, 0x847, 0x843, 0x83f, 0x83b, 0x837, 0x834, 0x830, 0x82c, 0x828, 0x825,
	0x821, 0x81d, 0x819, 0x816, 0x812, 0x80e, 0x80a, 0x807, 0x803, 0x7ff, 0x7fc, 0x7f8,
	0x7f4, 0x7f1, 0x7ed, 0x7e9, 0x7e6, 0x7e2, 0x7de, 0x7db, 0x7d7, 0x7d3, 0x7d0, 0x7cc,
	0x7c9, 0x7c5, 0x7c1, 0x7be, 0x7ba, 0x7b7, 0x7b3, 0x7b0, 0x7ac, 0x7a8, 0x7a5, 0x7a1,
	0x79e, 0x79a, 0x797, 0x793, 0x790, 0x78c, 0x789, 0x785, 0x782, 0x77e, 0x77b, 0x778
};

PitchConverter::PitchConverter()
{
}

void PitchConverter::calc(int &index, int &octave)
{
	if (index < 0) {
		octave = octave + index / 384 - 1;
		index = 384 + index % 384;
	}
	else if (384 <= index) {
		octave = octave + index / 384;
		index = index % 384;
	}

	if (octave < 0) {
		octave = 0;
		index = 0;
	}
	else if (octave > 8) {
		octave = 7;
		index = 383;
	}
}