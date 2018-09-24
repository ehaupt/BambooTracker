#include "opna_controller.hpp"
#include "pitch_converter.hpp"

#include <QDebug>

#ifdef SINC_INTERPOLATION
OPNAController::OPNAController(int clock, int rate, int duration) :
	opna_(clock, rate, duration)
#else
OPNAController::OPNAController(int clock, int rate) :
	opna_(clock, rate)
#endif
{	
	for (int ch = 0; ch < 6; ++ch) {
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::AL, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::FB, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::AR1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DR1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SR1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::RR1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SL1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::TL1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::KS1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::ML1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DT1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::AR2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DR2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SR2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::RR2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SL2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::TL2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::KS2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::ML2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DT2, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::AR3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DR3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SR3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::RR3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SL3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::TL3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::KS3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::ML3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DT3, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::AR4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DR4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SR4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::RR4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::SL4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::TL1, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::KS4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::ML4, nullptr);
		opSeqItFM_[ch].emplace(FMEnvelopeParameter::DT4, nullptr);

		isMuteFM_[ch] = false;
	}

	for (int ch = 0; ch < 3; ++ch) {
		isMuteSSG_[ch] = false;
	}

	for (int ch = 0; ch < 6; ++ch) {
		isMuteDrum_[ch] = false;
	}

	initChip();
}

/********** Reset and initialize **********/
void OPNAController::reset()
{
	opna_.reset();
	initChip();
}

void OPNAController::initChip()
{
	opna_.setRegister(0x29, 0x80);		// Init interrupt / YM2608 mode

	initFM();
	initSSG();
	initDrum();
}

/********** Forward instrument sequence **********/
void OPNAController::tickEvent(SoundSource src, int ch)
{
	switch (src) {
	case SoundSource::FM:	tickEventFM(ch);	break;
	case SoundSource::SSG:	tickEventSSG(ch);	break;
	case SoundSource::DRUM:	break;
	}
}

/********** Chip details **********/
int OPNAController::getGateCount(SoundSource src, int ch) const
{
	switch (src) {
	case SoundSource::FM:	return gateCntFM_[ch];
	case SoundSource::SSG:	return gateCntSSG_[ch];
	case SoundSource::DRUM:	return -1;
	}
}

/********** Stream samples **********/
void OPNAController::getStreamSamples(int16_t* container, size_t nSamples)
{
	opna_.mix(container, nSamples);
}

/********** Stream details **********/
int OPNAController::getRate() const
{
	return opna_.getRate();
}

int OPNAController::getDuration() const
{
	return 40;	// dummy set
}

//---------- FM ----------//
/********** Key on-off **********/
void OPNAController::keyOnFM(int ch, Note note, int octave, int pitch, bool isJam)
{
	if (isMuteFM(ch)) return;

	baseToneFM_[ch].octave = octave;
	baseToneFM_[ch].note = note;
	baseToneFM_[ch].pitch = pitch;
	realToneFM_[ch] = baseToneFM_[ch];

	setFrontFMSequences(ch);
	hasPreSetTickEventFM_[ch] = isJam;

	isKeyOnFM_[ch] = true;
}

void OPNAController::keyOffFM(int ch, bool isJam)
{
	releaseStartFMSequences(ch);
	hasPreSetTickEventFM_[ch] = isJam;

	uint8_t chdata = getFmChannelMask(ch);
	opna_.setRegister(0x28, chdata);
	isKeyOnFM_[ch] = false;
}

void OPNAController::resetFMChannelEnvelope(int ch)
{
	keyOffFM(ch);

	// Change register only
	int prev = envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR1);
	writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::RR1, 127);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR1, prev);

	prev = envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR2);
	writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::RR2, 127);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR2, prev);

	prev = envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR3);
	writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::RR3, 127);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR3, prev);

	prev = envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR4);
	writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::RR4, 127);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR4, prev);
}

/********** Set instrument **********/
void OPNAController::setInstrumentFM(int ch, std::shared_ptr<InstrumentFM> inst)
{
	if (inst == nullptr) {	// Error set ()
		if (refInstFM_[ch] != nullptr) {	// When setted instrument has been deleted
			refInstFM_[ch]->setNumber(-1);
		}
		else {
			return;
		}
	}
	else {
		refInstFM_[ch] = inst;
	}

	writeFMEnvelopeToRegistersFromInstrument(ch);
	if (isKeyOnFM_[ch] && lfoStartCntFM_[ch] == -1) writeFMLFOAllRegisters(ch);
	for (auto& p : opSeqItFM_[ch]) {
		if (refInstFM_[ch]->getOperatorSequenceNumber(p.first) == -1)
			p.second.reset();
		else
			p.second = refInstFM_[ch]->getOperatorSequenceSequenceIterator(p.first);
	}
	if (refInstFM_[ch]->getArpeggioNumber() == -1) arpItFM_[ch].reset();
	else arpItFM_[ch] = refInstFM_[ch]->getArpeggioSequenceIterator();
	if (refInstFM_[ch]->getPitchNumber() == -1) ptItFM_[ch].reset();
	else ptItFM_[ch] = refInstFM_[ch]->getPitchSequenceIterator();
	setInstrumentFMProperties(ch);

	checkLFOUsed();
}

void OPNAController::updateInstrumentFM(int instNum)
{
	for (int ch = 0; ch < 6; ++ch) {
		if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getNumber() == instNum) {
			writeFMEnvelopeToRegistersFromInstrument(ch);
			if (isKeyOnFM_[ch] && lfoStartCntFM_[ch] == -1) writeFMLFOAllRegisters(ch);
			for (auto& p : opSeqItFM_[ch]) {
				if (refInstFM_[ch]->getOperatorSequenceNumber(p.first) == -1)
					p.second.reset();
			}
			if (refInstFM_[ch]->getArpeggioNumber() == -1) arpItFM_[ch].reset();
			if (refInstFM_[ch]->getPitchNumber() == -1) ptItFM_[ch].reset();
			setInstrumentFMProperties(ch);
		}
	}

	checkLFOUsed();
}

void OPNAController::updateInstrumentFMEnvelopeParameter(int envNum, FMEnvelopeParameter param)
{
	for (int ch = 0; ch < 6; ++ch) {
		if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getEnvelopeNumber() == envNum) {
			writeFMEnveropeParameterToRegister(ch, param, refInstFM_[ch]->getEnvelopeParameter(param));
		}
	}
}

void OPNAController::setInstrumentFMOperatorEnabled(int envNum, int opNum)
{
	for (int ch = 0; ch < 6; ++ch) {
		if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getEnvelopeNumber() == envNum) {
			bool enabled = refInstFM_[ch]->getOperatorEnabled(opNum);
			envFM_[ch]->setOperatorEnabled(opNum, enabled);
			if (enabled) {
				fmOpEnables_[ch] |= (1 << opNum);
			}
			else {
				fmOpEnables_[ch] &= ~(1 << opNum);
			}
			if (isKeyOnFM_[ch]) {
				uint32_t mask = getFmChannelMask(ch);
				opna_.setRegister(0x28, (fmOpEnables_[ch] << 4) | mask);
			}
		}
	}
}

void OPNAController::updateInstrumentFMLFOParameter(int lfoNum, FMLFOParameter param)
{
	for (int ch = 0; ch < 6; ++ch) {
		if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getLFONumber() == lfoNum) {
			writeFMLFORegister(ch, param);
		}
	}
}

/********** Set volume **********/
void OPNAController::setVolumeFM(int ch, int volume)
{
	volFM_[ch] = volume;

	if (isKeyOnFM(ch) && refInstFM_[ch] != nullptr) {	// Change TL
		writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::TL1,
										   refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL1));
		writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::TL2,
										   refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL2));
		writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::TL3,
										   refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL3));
		writeFMEnveropeParameterToRegister(ch, FMEnvelopeParameter::TL4,
										   refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL4));
	}
}

/********** Set pan **********/
void OPNAController::setPanFM(int ch, int value)
{
	panFM_[ch] = value;

	uint32_t bch = getFMChannelOffset(ch);	// Bank and channel offset
	uint8_t data = value << 6;
	if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getLFONumber() != -1) {
		data |= (refInstFM_[ch]->getLFOParameter(FMLFOParameter::AMS) << 4);
		data |= refInstFM_[ch]->getLFOParameter(FMLFOParameter::PMS);
	}
	opna_.setRegister(0xb4 + bch, data);
}

/********** Mute **********/
void OPNAController::setMuteFMState(int ch, bool isMute)
{
	isMuteFM_[ch] = isMute;

	if (isMute) resetFMChannelEnvelope(ch);
}

bool OPNAController::isMuteFM(int ch)
{
	return isMuteFM_[ch];
}

/********** Chip details **********/
bool OPNAController::isKeyOnFM(int ch) const
{
	return isKeyOnFM_[ch];
}

bool OPNAController::enableFMEnvelopeReset(int ch) const
{
	return (envFM_[ch] == nullptr)
			? true
			: enableEnvResetFM_[ch];
}

ToneDetail OPNAController::getFMTone(int ch) const
{
	return baseToneFM_[ch];
}

/***********************************/
void OPNAController::initFM()
{
	lfoFreq_ = -1;

	for (int ch = 0; ch < 6; ++ch) {
		// Init operators key off
		fmOpEnables_[ch] = 0xf;
		isKeyOnFM_[ch] = false;

		// Init envelope
		envFM_[ch] = std::make_unique<EnvelopeFM>(-1);
		refInstFM_[ch].reset();

		baseToneFM_[ch].octave = -1;	// Init key on note data
		realToneFM_[ch].octave = -1;
		volFM_[ch] = 0;	// Init volume
		gateCntFM_[ch] = 0;
		enableEnvResetFM_[ch] = true;
		lfoStartCntFM_[ch] = -1;

		// Init sequence
		hasPreSetTickEventFM_[ch] = false;
		for (auto& p : opSeqItFM_[ch]) {
			p.second.reset();
		}
		arpItFM_[ch].reset();
		ptItFM_[ch].reset();
		needToneSetFM_[ch] = false;

		// Init pan
		uint32_t bch = getFMChannelOffset(ch);
		panFM_[ch] = 3;
		opna_.setRegister(0xb4 + bch, 0xc0);
	}
}

uint32_t OPNAController::getFmChannelMask(int ch)
{
	// UNDONE: change channel type by Effect mode
	switch (ch) {
	case 0: return 0x00;
	case 1: return 0x01;
	case 2: return 0x02;
	case 3: return 0x04;
	case 4: return 0x05;
	case 5: return 0x06;
	default: return 0;
	}
}

uint32_t OPNAController::getFMChannelOffset(int ch)
{
	switch (ch) {
	case 0:
	case 1:
	case 2:
		return ch;
	case 3:
	case 4:
	case 5:
		return 0x100 + ch % 3;
	default:
		return 0;
	}
}

void OPNAController::writeFMEnvelopeToRegistersFromInstrument(int ch)
{
	uint32_t bch = getFMChannelOffset(ch);	// Bank and channel offset
	uint8_t data1, data2;
	int al;

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::FB);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::FB, data1);
	data1 <<= 3;
	al = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::AL);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::AL, al);
	data1 += al;
	opna_.setRegister(0xb0 + bch, data1);

	uint32_t offset = bch;	// Operator 1

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DT1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DT1, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::ML1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::ML1, data2);
	data1 |= data2;
	opna_.setRegister(0x30 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL1);
	if (isCareer(0, al)) data1 = calculateTL(ch, data1);	// Adjust volume
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::TL1, data1);
	opna_.setRegister(0x40 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::KS1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::KS1, data1);
	data1 <<= 6;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::AR1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::AR1, data2);
	data1 |= data2;
	opna_.setRegister(0x50 + offset, data1);

	data1 = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM1);
	data1 <<= 7;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DR1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DR1, data2);
	data1 |= data2;
	opna_.setRegister(0x60 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SR1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SR1, data2);
	opna_.setRegister(0x70 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SL1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SL1, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::RR1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR1, data2);
	data1 |= data2;
	opna_.setRegister(0x80 + offset, data1);

	int tmp = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SSGEG1);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SSGEG1, tmp);
	data1 = judgeSSEGRegisterValue(tmp);
	opna_.setRegister(0x90 + offset, data1);

	offset = bch + 8;	// Operator 2

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DT2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DT2, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::ML2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::ML2, data2);
	data1 |= data2;
	opna_.setRegister(0x30 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL2);
	if (isCareer(1, al)) data1 = calculateTL(ch, data1);	// Adjust volume
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::TL2, data1);
	opna_.setRegister(0x40 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::KS2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::KS2, data1);
	data1 <<= 6;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::AR2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::AR2, data2);
	data1 |= data2;
	opna_.setRegister(0x50 + offset, data1);

	data1 = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM2);
	data1 <<= 7;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DR2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DR2, data2);
	data1 |= data2;
	opna_.setRegister(0x60 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SR2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SR2, data2);
	opna_.setRegister(0x70 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SL2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SL2, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::RR2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR2, data2);
	data1 |= data2;
	opna_.setRegister(0x80 + offset, data1);

	tmp = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SSGEG2);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SSGEG2, tmp);
	data1 = judgeSSEGRegisterValue(tmp);
	opna_.setRegister(0x90 + offset, data1);

	offset = bch + 4;	// Operator 3

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DT3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DT3, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::ML3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::ML3, data2);
	data1 |= data2;
	opna_.setRegister(0x30 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL3);
	if (isCareer(3, al)) data1 = calculateTL(ch, data1);	// Adjust volume
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::TL3, data1);
	opna_.setRegister(0x40 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::KS3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::KS3, data1);
	data1 <<= 6;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::AR3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::AR3, data2);
	data1 |= data2;
	opna_.setRegister(0x50 + offset, data1);

	data1 = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM3);
	data1 <<= 7;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DR3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DR3, data2);
	data1 |= data2;
	opna_.setRegister(0x60 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SR3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SR3, data2);
	opna_.setRegister(0x70 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SL3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SL3, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::RR3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR3, data2);
	data1 |= data2;
	opna_.setRegister(0x80 + offset, data1);

	tmp = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SSGEG3);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SSGEG3, tmp);
	data1 = judgeSSEGRegisterValue(tmp);
	opna_.setRegister(0x90 + offset, data1);

	offset = bch + 12;	// Operator 4

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DT4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DT4, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::ML4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::ML4, data2);
	data1 |= data2;
	opna_.setRegister(0x30 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::TL4);
	data1 = calculateTL(ch, data1);	// Adjust volume
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::TL4, data1);
	opna_.setRegister(0x40 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::KS4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::KS4, data1);
	data1 <<= 6;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::AR4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::AR4, data2);
	data1 |= data2;
	opna_.setRegister(0x50 + offset, data1);

	data1 = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM4);
	data1 <<= 7;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::DR4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::DR4, data2);
	data1 |= data2;
	opna_.setRegister(0x60 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SR4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SR4, data2);
	opna_.setRegister(0x70 + offset, data1);

	data1 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SL4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SL4, data1);
	data1 <<= 4;
	data2 = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::RR4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::RR4, data2);
	data1 |= data2;
	opna_.setRegister(0x80 + offset, data1);

	tmp = refInstFM_[ch]->getEnvelopeParameter(FMEnvelopeParameter::SSGEG4);
	envFM_[ch]->setParameterValue(FMEnvelopeParameter::SSGEG4, tmp);
	data1 = judgeSSEGRegisterValue(tmp);
	opna_.setRegister(0x90 + offset, data1);
}

void OPNAController::writeFMEnveropeParameterToRegister(int ch, FMEnvelopeParameter param, int value)
{
	uint32_t bch = getFMChannelOffset(ch);	// Bank and channel offset
	uint8_t data;
	int tmp;

	envFM_[ch]->setParameterValue(param, value);

	switch (param) {
	case FMEnvelopeParameter::AL:
	case FMEnvelopeParameter::FB:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::FB) << 3;
		data += envFM_[ch]->getParameterValue(FMEnvelopeParameter::AL);
		opna_.setRegister(0xb0 + bch, data);
		break;
	case FMEnvelopeParameter::DT1:
	case FMEnvelopeParameter::ML1:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::DT1) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::ML1);
		opna_.setRegister(0x30 + bch, data);
		break;
	case FMEnvelopeParameter::TL1:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::TL1);
		// Adjust volume
		if (isCareer(0, envFM_[ch]->getParameterValue(FMEnvelopeParameter::AL))) {
			data = calculateTL(ch, data);
			envFM_[ch]->setParameterValue(param, data);	// Update
		}
		opna_.setRegister(0x40 + bch, data);
		break;
	case FMEnvelopeParameter::KS1:
	case FMEnvelopeParameter::AR1:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::KS1) << 6;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::AR1);
		opna_.setRegister(0x50 + bch, data);
		break;
	case FMEnvelopeParameter::DR1:
		data = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM1);
		data <<= 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR1);
		opna_.setRegister(0x60 + bch, data);
		break;
	case FMEnvelopeParameter::SR1:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SR1);
		opna_.setRegister(0x70 + bch, data);
		break;
	case FMEnvelopeParameter::SL1:
	case FMEnvelopeParameter::RR1:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SL1) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR1);
		opna_.setRegister(0x80 + bch, data);
		break;
	case::FMEnvelopeParameter::SSGEG1:
		tmp = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SSGEG1);
		data = (tmp == -1) ? 0 : (0x08 + tmp);
		opna_.setRegister(0x90 + bch, data);
		break;
	case FMEnvelopeParameter::DT2:
	case FMEnvelopeParameter::ML2:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::DT2) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::ML2);
		opna_.setRegister(0x30 + bch + 8, data);
		break;
	case FMEnvelopeParameter::TL2:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::TL2);
		// Adjust volume
		if (isCareer(1, envFM_[ch]->getParameterValue(FMEnvelopeParameter::AL))) {
			data = calculateTL(ch, data);
			envFM_[ch]->setParameterValue(param, data);	// Update
		}
		opna_.setRegister(0x40 + bch + 8, data);
		break;
	case FMEnvelopeParameter::KS2:
	case FMEnvelopeParameter::AR2:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::KS2) << 6;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::AR2);
		opna_.setRegister(0x50 + bch + 8, data);
		break;
	case FMEnvelopeParameter::DR2:
		data = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM2);
		data <<= 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR2);
		opna_.setRegister(0x60 + bch + 8, data);
		break;
	case FMEnvelopeParameter::SR2:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SR2);
		opna_.setRegister(0x70 + bch + 8, data);
		break;
	case FMEnvelopeParameter::SL2:
	case FMEnvelopeParameter::RR2:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SL2) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR2);
		opna_.setRegister(0x80 + bch + 8, data);
		break;
	case FMEnvelopeParameter::SSGEG2:
		tmp = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SSGEG2);
		data = (tmp == -1) ? 0 : (0x08 + tmp);
		opna_.setRegister(0x90 + bch + 8, data);
		break;
	case FMEnvelopeParameter::DT3:
	case FMEnvelopeParameter::ML3:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::DT3) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::ML3);
		opna_.setRegister(0x30 + bch + 4, data);
		break;
	case FMEnvelopeParameter::TL3:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::TL3);
		// Adjust volume
		if (isCareer(2, envFM_[ch]->getParameterValue(FMEnvelopeParameter::AL))) {
			data = calculateTL(ch, data);
			envFM_[ch]->setParameterValue(param, data);	// Update
		}
		opna_.setRegister(0x40 + bch + 4, data);
		break;
	case FMEnvelopeParameter::KS3:
	case FMEnvelopeParameter::AR3:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::KS3) << 6;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::AR3);
		opna_.setRegister(0x50 + bch + 4, data);
		break;
	case FMEnvelopeParameter::DR3:
		data = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM3);
		data <<= 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR3);
		opna_.setRegister(0x60 + bch + 4, data);
		break;
	case FMEnvelopeParameter::SR3:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SR3);
		opna_.setRegister(0x70 + bch + 4, data);
		break;
	case FMEnvelopeParameter::SL3:
	case FMEnvelopeParameter::RR3:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SL3) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR3);
		opna_.setRegister(0x80 + bch + 4, data);
		break;
	case FMEnvelopeParameter::SSGEG3:
		tmp = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SSGEG3);
		data = (tmp == -1) ? 0 : (0x08 + tmp);
		opna_.setRegister(0x90 + bch + 4, data);
		break;
	case FMEnvelopeParameter::DT4:
	case FMEnvelopeParameter::ML4:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::DT4) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::ML4);
		opna_.setRegister(0x30 + bch + 12, data);
		break;
	case FMEnvelopeParameter::TL4:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::TL4);
		// Adjust volume
		data = calculateTL(ch, data);
		envFM_[ch]->setParameterValue(param, data);	// Update
		opna_.setRegister(0x40 + bch + 12, data);
		break;
	case FMEnvelopeParameter::KS4:
	case FMEnvelopeParameter::AR4:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::KS4) << 6;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::AR4);
		opna_.setRegister(0x50 + bch + 12, data);
		break;
	case FMEnvelopeParameter::DR4:
		data = (refInstFM_[ch]->getLFONumber() == -1) ? 0 : refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM4);
		data <<= 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR4);
		opna_.setRegister(0x60 + bch + 12, data);
		break;
	case FMEnvelopeParameter::SR4:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SR4);
		opna_.setRegister(0x70 + bch + 12, data);
		break;
	case FMEnvelopeParameter::SL4:
	case FMEnvelopeParameter::RR4:
		data = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SL4) << 4;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::RR4);
		opna_.setRegister(0x80 + bch + 12, data);
		break;
	case FMEnvelopeParameter::SSGEG4:
		tmp = envFM_[ch]->getParameterValue(FMEnvelopeParameter::SSGEG4);
		data = judgeSSEGRegisterValue(tmp);
		opna_.setRegister(0x90 + bch + 12, data);
		break;
	}
}

void OPNAController::writeFMLFOAllRegisters(int ch)
{
	if (refInstFM_[ch]->getLFONumber() == -1 || lfoStartCntFM_[ch] > 0) {	// Clear data
		uint32_t bch = getFMChannelOffset(ch);	// Bank and channel offset
		opna_.setRegister(0xb4 + bch, panFM_[ch] << 6);
		opna_.setRegister(0x60 + bch, envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR1));
		opna_.setRegister(0x60 + bch + 8, envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR2));
		opna_.setRegister(0x60 + bch + 4, envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR3));
		opna_.setRegister(0x60 + bch + 12, envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR4));
	}
	else {
		writeFMLFORegister(ch, FMLFOParameter::FREQ);
		writeFMLFORegister(ch, FMLFOParameter::PMS);
		writeFMLFORegister(ch, FMLFOParameter::AMS);
		writeFMLFORegister(ch, FMLFOParameter::AM1);
		writeFMLFORegister(ch, FMLFOParameter::AM2);
		writeFMLFORegister(ch, FMLFOParameter::AM3);
		writeFMLFORegister(ch, FMLFOParameter::AM4);
		lfoStartCntFM_[ch] = -1;
	}
}

void OPNAController::writeFMLFORegister(int ch, FMLFOParameter param)
{
	uint32_t bch = getFMChannelOffset(ch);	// Bank and channel offset
	uint8_t data;

	switch (param) {
	case FMLFOParameter::FREQ:
		lfoFreq_ = refInstFM_[ch]->getLFOParameter(FMLFOParameter::FREQ);
		opna_.setRegister(0x22, lfoFreq_ | (1 << 3));
		break;
	case FMLFOParameter::PMS:
	case FMLFOParameter::AMS:
		data = panFM_[ch] << 6;
		data |= (refInstFM_[ch]->getLFOParameter(FMLFOParameter::AMS) << 4);
		data |= refInstFM_[ch]->getLFOParameter(FMLFOParameter::PMS);
		opna_.setRegister(0xb4 + bch, data);
		break;
	case FMLFOParameter::AM1:
		data = refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM1) << 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR1);
		opna_.setRegister(0x60 + bch, data);
		break;
	case FMLFOParameter::AM2:
		data = refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM2) << 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR2);
		opna_.setRegister(0x60 + bch + 8, data);
		break;
	case FMLFOParameter::AM3:
		data = refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM3) << 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR3);
		opna_.setRegister(0x60 + bch + 4, data);
		break;
	case FMLFOParameter::AM4:
		data = refInstFM_[ch]->getLFOParameter(FMLFOParameter::AM4) << 7;
		data |= envFM_[ch]->getParameterValue(FMEnvelopeParameter::DR4);
		opna_.setRegister(0x60 + bch + 12, data);
		break;
	default:
		break;
	}
}

void OPNAController::checkLFOUsed()
{
	for (int ch = 0; ch < 6; ++ch) {
		if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getLFONumber() != -1) return;
	}

	if (lfoFreq_ != -1) {
		lfoFreq_ = -1;
		opna_.setRegister(0x22, 0);	// LFO off
	}
}

void OPNAController::setFrontFMSequences(int ch)
{
	if (refInstFM_[ch] != nullptr && refInstFM_[ch]->getLFONumber() != -1) {
		lfoStartCntFM_[ch] = refInstFM_[ch]->getLFOParameter(FMLFOParameter::COUNT);
		writeFMLFOAllRegisters(ch);
	}
	else {
		lfoStartCntFM_[ch] = -1;
	}

	checkOperatorSequenceFM(ch, 1);

	if (arpItFM_[ch]) checkRealToneFMByArpeggio(ch, arpItFM_[ch]->front());

	if (ptItFM_[ch]) checkRealToneFMByPitch(ch, ptItFM_[ch]->front());

	writePitchFM(ch);
}

void OPNAController::releaseStartFMSequences(int ch)
{
	if (lfoStartCntFM_[ch] > 0) {
		--lfoStartCntFM_[ch];
		writeFMLFOAllRegisters(ch);
	}

	checkOperatorSequenceFM(ch, 2);

	if (arpItFM_[ch]) checkRealToneFMByArpeggio(ch, arpItFM_[ch]->next(true));

	if (ptItFM_[ch]) checkRealToneFMByPitch(ch, ptItFM_[ch]->next(true));

	if (needToneSetFM_[ch]) writePitchFM(ch);
}

void OPNAController::tickEventFM(int ch)
{
	if (hasPreSetTickEventFM_[ch]) {
		hasPreSetTickEventFM_[ch] = false;
	}
	else {
		if (lfoStartCntFM_[ch] > 0) {
			--lfoStartCntFM_[ch];
			writeFMLFOAllRegisters(ch);
		}

		checkOperatorSequenceFM(ch, 0);

		if (arpItFM_[ch]) checkRealToneFMByArpeggio(ch, arpItFM_[ch]->next());

		if (ptItFM_[ch]) checkRealToneFMByPitch(ch, ptItFM_[ch]->next());

		if (needToneSetFM_[ch]) writePitchFM(ch);
	}
}

void OPNAController::checkOperatorSequenceFM(int ch, int type)
{
	for (auto& p : opSeqItFM_[ch]) {
		if (p.second != nullptr) {
			int t;
			switch (type) {
			case 0:	t = p.second->next();		break;
			case 1:	t = p.second->front();		break;
			case 2:	t = p.second->next(true);	break;
			}
			if (t != -1) {
				int d = p.second->getCommandType();
				if (d != envFM_[ch]->getParameterValue(p.first)) {
					writeFMEnveropeParameterToRegister(ch, p.first, d);
				}
			}
		}
	}
}

void OPNAController::checkRealToneFMByArpeggio(int ch, int seqPos)
{
	if (seqPos == -1) return;

	switch (arpItFM_[ch]->getSequenceType()) {
	case 0:	// Absolute
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(
										octaveAndNoteToNoteNumber(baseToneFM_[ch].octave, baseToneFM_[ch].note)
										+ arpItFM_[ch]->getCommandType() - 48);
		realToneFM_[ch].octave = pair.first;
		realToneFM_[ch].note = pair.second;
		break;
	}
	case 1:	// Fix
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(arpItFM_[ch]->getCommandType());
		realToneFM_[ch].octave = pair.first;
		realToneFM_[ch].note = pair.second;
		break;
	}
	case 2:	// Relative
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(
										octaveAndNoteToNoteNumber(realToneFM_[ch].octave, realToneFM_[ch].note)
										+ arpItFM_[ch]->getCommandType() - 48);
		realToneFM_[ch].octave = pair.first;
		realToneFM_[ch].note = pair.second;
		break;
	}
	}

	needToneSetFM_[ch] = true;
}

void OPNAController::checkRealToneFMByPitch(int ch, int seqPos)
{
	if (seqPos == -1) return;

	switch (ptItFM_[ch]->getSequenceType()) {
	case 0:	// Absolute
		realToneFM_[ch].pitch = ptItFM_[ch]->getCommandType() - 127;
		break;
	case 1:	// Relative
		realToneFM_[ch].pitch += (ptItFM_[ch]->getCommandType() - 127);
		break;
	}

	needToneSetFM_[ch] = true;
}

void OPNAController::writePitchFM(int ch)
{
	uint16_t p = PitchConverter::getPitchFM(realToneFM_[ch].note,
											realToneFM_[ch].octave,
											realToneFM_[ch].pitch);
	uint32_t offset = getFMChannelOffset(ch);
	opna_.setRegister(0xa4 + offset, p >> 8);
	opna_.setRegister(0xa0 + offset, p & 0x00ff);
	uint32_t chdata = getFmChannelMask(ch);
	opna_.setRegister(0x28, (fmOpEnables_[ch] << 4) | chdata);

	needToneSetFM_[ch] = false;
}

void OPNAController::setInstrumentFMProperties(int ch)
{
	gateCntFM_[ch] = refInstFM_[ch]->getGateCount();
	enableEnvResetFM_[ch] = refInstFM_[ch]->getEnvelopeResetEnabled();
}

bool OPNAController::isCareer(int op, int al)
{
	switch (op) {
	case 0:
		return (al == 7);
	case 1:
		switch (al) {
		case 4:
		case 5:
		case 6:
		case 7:
			return true;
		default:
			return false;
		}
	case 2:
		switch (al) {
		case 5:
		case 6:
		case 7:
			return true;
		default:
			return false;
		}
	case 3:
		return true;
	default:
		return false;
	}
}

//---------- SSG ----------//
/********** Key on-off **********/
void OPNAController::keyOnSSG(int ch, Note note, int octave, int pitch, bool isJam)
{
	if (isMuteSSG(ch)) return;

	baseToneSSG_[ch].octave = octave;
	baseToneSSG_[ch].note = note;
	baseToneSSG_[ch].pitch = pitch;
	realToneSSG_[ch] = baseToneSSG_[ch];

	setFrontSSGSequences(ch);

	hasPreSetTickEventSSG_[ch] = isJam;
	isKeyOnSSG_[ch] = true;
}

void OPNAController::keyOffSSG(int ch, bool isJam)
{
	releaseStartSSGSequences(ch);
	hasPreSetTickEventSSG_[ch] = isJam;
	isKeyOnSSG_[ch] = false;
}

/********** Set instrument **********/
void OPNAController::setInstrumentSSG(int ch, std::shared_ptr<InstrumentSSG> inst)
{
	if (inst == nullptr) {	// Error set ()
		if (refInstSSG_[ch] != nullptr) {
			refInstSSG_[ch]->setNumber(-1);
		}
		else {
			return;
		}
	}
	else {
		refInstSSG_[ch] = inst;
	}

	if (refInstSSG_[ch]->getWaveFormNumber() == -1) wfItSSG_[ch].reset();
	else wfItSSG_[ch] = refInstSSG_[ch]->getWaveFormSequenceIterator();
	if (refInstSSG_[ch]->getToneNoiseNumber() == -1) tnItSSG_[ch].reset();
	else tnItSSG_[ch] = refInstSSG_[ch]->getToneNoiseSequenceIterator();
	if (refInstSSG_[ch]->getEnvelopeNumber() == -1) envItSSG_[ch].reset();
	else envItSSG_[ch] = refInstSSG_[ch]->getEnvelopeSequenceIterator();
	if (refInstSSG_[ch]->getArpeggioNumber() == -1) arpItSSG_[ch].reset();
	else arpItSSG_[ch] = refInstSSG_[ch]->getArpeggioSequenceIterator();
	if (refInstSSG_[ch]->getPitchNumber() == -1) ptItSSG_[ch].reset();
	else ptItSSG_[ch] = refInstSSG_[ch]->getPitchSequenceIterator();
	setInstrumentSSGProperties(ch);
}

void OPNAController::updateInstrumentSSG(int instNum)
{
	for (int ch = 0; ch < 3; ++ch) {
		if (refInstSSG_[ch] != nullptr && refInstSSG_[ch]->getNumber() == instNum) {
			if (refInstSSG_[ch]->getWaveFormNumber() == -1) wfItSSG_[ch].reset();
			if (refInstSSG_[ch]->getToneNoiseNumber() == -1) tnItSSG_[ch].reset();
			if (refInstSSG_[ch]->getEnvelopeNumber() == -1) envItSSG_[ch].reset();
			if (refInstSSG_[ch]->getArpeggioNumber() == -1) arpItSSG_[ch].reset();
			if (refInstSSG_[ch]->getPitchNumber() == -1) ptItSSG_[ch].reset();
			setInstrumentSSGProperties(ch);
		}
	}
}

/********** Set volume **********/
void OPNAController::setVolumeSSG(int ch, int volume)
{
	if (volume > 0xf) return;	// Out of range

	baseVolSSG_[ch] = volume;

	if (isKeyOnSSG(ch)) setRealVolumeSSG(ch);
}

void OPNAController::setRealVolumeSSG(int ch)
{
	if (isBuzzEffSSG_[ch] || isHardEnvSSG_[ch]) return;

	int volume = baseVolSSG_[ch];
	if (envItSSG_[ch]) {
		int type = envItSSG_[ch]->getCommandType();
		if (0 <= type && type < 16) {
			volume = volume - (15 - type);
			if (volume < 0) volume = 0;
		}
	}
	opna_.setRegister(0x08 + ch, volume);
	needEnvSetSSG_[ch] = false;
}

/********** Mute **********/
void OPNAController::setMuteSSGState(int ch, bool isMute)
{
	isMuteSSG_[ch] = isMute;

	if (isMute) keyOffSSG(ch);
}

bool OPNAController::isMuteSSG(int ch)
{
	return isMuteSSG_[ch];
}

/********** Chip details **********/
bool OPNAController::isKeyOnSSG(int ch) const
{
	return isKeyOnSSG_[ch];
}

ToneDetail OPNAController::getSSGTone(int ch) const
{
	return baseToneSSG_[ch];
}

/***********************************/
void OPNAController::initSSG()
{
	mixerSSG_ = 0xff;
	opna_.setRegister(0x07, mixerSSG_);	// SSG mix

	for (int ch = 0; ch < 3; ++ch) {
		isKeyOnSSG_[ch] = false;

		refInstSSG_[ch].reset();	// Init envelope
		baseToneSSG_[ch].octave = -1;	// Init key on note data
		realToneSSG_[ch].octave = -1;
		tnSSG_[ch] = { false, false, -1 };
		baseVolSSG_[ch] = 0xf;	// Init volume
		isHardEnvSSG_[ch] = false;
		isBuzzEffSSG_[ch] = false;

		// Init sequence
		hasPreSetTickEventSSG_[ch] = false;
		wfItSSG_[ch].reset();
		wfSSG_[ch] = { -1, -1 };
		envItSSG_[ch].reset();
		envSSG_[ch] = { -1, -1 };
		tnItSSG_[ch].reset();
		arpItSSG_[ch].reset();
		ptItSSG_[ch].reset();
		needEnvSetSSG_[ch] = false;
		needMixSetSSG_[ch] = false;
		needToneSetSSG_[ch] = false;

		gateCntSSG_[ch] = 0;
	}
}

void OPNAController::setFrontSSGSequences(int ch)
{
	if (wfItSSG_[ch]) writeWaveFormSSGToRegister(ch, wfItSSG_[ch]->front());
	else writeSquareWaveForm(ch);

	if (envItSSG_[ch]) writeEnvelopeSSGToRegister(ch, envItSSG_[ch]->front());
	else setRealVolumeSSG(ch);

	if (tnItSSG_[ch]) writeToneNoiseSSGToRegister(ch, tnItSSG_[ch]->front());
	else if (needMixSetSSG_[ch]) writeToneNoiseSSGToRegisterNoReference(ch);

	if (arpItSSG_[ch]) checkRealToneSSGByArpeggio(ch, arpItSSG_[ch]->front());

	if (ptItSSG_[ch]) checkRealToneSSGByPitch(ch, ptItSSG_[ch]->front());

	writePitchSSG(ch);
}

void OPNAController::releaseStartSSGSequences(int ch)
{
	if (wfItSSG_[ch]) writeWaveFormSSGToRegister(ch, wfItSSG_[ch]->next(true));

	if (envItSSG_[ch]) {
		int pos = envItSSG_[ch]->next(true);
		if (pos == -1) {
			opna_.setRegister(0x08 + ch, 0);
			isHardEnvSSG_[ch] = false;
		}
		else writeEnvelopeSSGToRegister(ch, pos);
	}
	else {
		if (!hasPreSetTickEventSSG_[ch]) {
			opna_.setRegister(0x08 + ch, 0);
			isHardEnvSSG_[ch] = false;
		}
	}

	if (tnItSSG_[ch]) writeToneNoiseSSGToRegister(ch, tnItSSG_[ch]->next(true));
	else if (needMixSetSSG_[ch]) writeToneNoiseSSGToRegisterNoReference(ch);

	if (arpItSSG_[ch]) checkRealToneSSGByArpeggio(ch, arpItSSG_[ch]->next(true));

	if (ptItSSG_[ch]) checkRealToneSSGByPitch(ch, ptItSSG_[ch]->next(true));

	if (needToneSetSSG_[ch]) writePitchSSG(ch);
}

void OPNAController::tickEventSSG(int ch)
{
	if (hasPreSetTickEventSSG_[ch]) {
		hasPreSetTickEventSSG_[ch] = false;
	}
	else {
		if (wfItSSG_[ch]) writeWaveFormSSGToRegister(ch, wfItSSG_[ch]->next());
		if (envItSSG_[ch]) {
			writeEnvelopeSSGToRegister(ch, envItSSG_[ch]->next());
		}
		else if (needToneSetSSG_[ch]) {
			setRealVolumeSSG(ch);
		}
		if (tnItSSG_[ch]) writeToneNoiseSSGToRegister(ch, tnItSSG_[ch]->next());
		else if (needMixSetSSG_[ch]) writeToneNoiseSSGToRegisterNoReference(ch);

		if (arpItSSG_[ch]) checkRealToneSSGByArpeggio(ch, arpItSSG_[ch]->next());

		if (ptItSSG_[ch]) checkRealToneSSGByPitch(ch, ptItSSG_[ch]->next());

		if (needToneSetSSG_[ch]) writePitchSSG(ch);
	}
}

void OPNAController::writeWaveFormSSGToRegister(int ch, int seqPos)
{
	if (seqPos == -1) return;

	switch (wfItSSG_[ch]->getCommandType()) {
	case 0:	// Square
	{
		writeSquareWaveForm(ch);
		return;
	}
	case 1:	// Triangle
	{
		if (wfSSG_[ch].type == 1 && isKeyOnSSG_[ch]) return;

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 3:
		case 4:
			needMixSetSSG_[ch] = true;
			break;
		default:
			break;
		}

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 2:
		case 4:
			opna_.setRegister(0x0d, 0x0e);
			break;
		default:
			if (!isKeyOnSSG_[ch]) opna_.setRegister(0x0d, 0x0e);	// First key on
			break;
		}

		if (isHardEnvSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			isHardEnvSSG_[ch] = false;
		}
		else if (!isBuzzEffSSG_[ch] || !isKeyOnSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			opna_.setRegister(0x08 + ch, 0x10);
		}

		if (envSSG_[ch].type == 0) envSSG_[ch] = { -1, -1 };

		needEnvSetSSG_[ch] = false;
		needToneSetSSG_[ch] = true;
		wfSSG_[ch] = { 1, -1 };
		return;
	}
	case 2:	// Saw
	{
		if (wfSSG_[ch].type == 2 && isKeyOnSSG_[ch]) return;

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 3:
		case 4:
			needMixSetSSG_[ch] = true;
			break;
		default:
			break;
		}

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 1:
		case 4:
			opna_.setRegister(0x0d, 0x0c);
			break;
		default:
			if (!isKeyOnSSG_[ch]) opna_.setRegister(0x0d, 0x0c);	// First key on
			break;
		}

		if (isHardEnvSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			isHardEnvSSG_[ch] = false;
		}
		else if (!isBuzzEffSSG_[ch] || !isKeyOnSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			opna_.setRegister(0x08 + ch, 0x10);
		}

		if (envSSG_[ch].type == 0) envSSG_[ch] = { -1, -1 };

		needEnvSetSSG_[ch] = false;
		needToneSetSSG_[ch] = true;
		wfSSG_[ch] = { 2, -1 };
		return;
	}
	case 3:	// Triangle with square
	{
		int data = wfItSSG_[ch]->getCommandData();
		if (wfSSG_[ch].type == 3 && wfSSG_[ch].data == data && isKeyOnSSG_[ch]) return;

		switch (wfSSG_[ch].type) {
		case -1:
		case 1:
		case 2:
			needMixSetSSG_[ch] = true;
			break;
		default:
			break;
		}

		if (wfSSG_[ch].data != data) {
			uint16_t pitch = PitchConverter::getPitchSSGSquare(data);
			uint8_t offset = ch << 1;
			opna_.setRegister(0x00 + offset, pitch & 0xff);
			opna_.setRegister(0x01 + offset, pitch >> 8);
		}

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 2:
		case 4:
			opna_.setRegister(0x0d, 0x0e);
			break;
		default:
			if (!isKeyOnSSG_[ch]) opna_.setRegister(0x0d, 0x0e);	// First key on
			break;
		}

		if (isHardEnvSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			isHardEnvSSG_[ch] = false;
		}
		else if (!isBuzzEffSSG_[ch] || !isKeyOnSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			opna_.setRegister(0x08 + ch, 0x10);
		}

		if (envSSG_[ch].type == 0) envSSG_[ch] = { -1, -1 };

		needEnvSetSSG_[ch] = false;
		needToneSetSSG_[ch] = true;
		wfSSG_[ch] = { 3, data };
		return;
	}
	case 4:	// Saw with square
	{
		int data = wfItSSG_[ch]->getCommandData();
		if (wfSSG_[ch].type == 4 && wfSSG_[ch].data == data && isKeyOnSSG_[ch]) return;

		switch (wfSSG_[ch].type) {
		case -1:
		case 1:
		case 2:
			needMixSetSSG_[ch] = true;
			break;
		default:
			break;
		}

		if (wfSSG_[ch].data != data) {
			uint16_t pitch = PitchConverter::getPitchSSGSquare(data);
			uint8_t offset = ch << 1;
			opna_.setRegister(0x00 + offset, pitch & 0xff);
			opna_.setRegister(0x01 + offset, pitch >> 8);
		}

		switch (wfSSG_[ch].type) {
		case -1:
		case 0:
		case 1:
		case 3:
			opna_.setRegister(0x0d, 0x0c);
			break;
		default:
			if (!isKeyOnSSG_[ch]) opna_.setRegister(0x0d, 0x0c);	// First key on
			break;
		}

		if (isHardEnvSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			isHardEnvSSG_[ch] = false;
		}
		else if (!isBuzzEffSSG_[ch] || !isKeyOnSSG_[ch]) {
			isBuzzEffSSG_[ch] = true;
			opna_.setRegister(0x08 + ch, 0x10);
		}

		if (envSSG_[ch].type == 0) envSSG_[ch] = { -1, -1 };

		needEnvSetSSG_[ch] = false;
		needToneSetSSG_[ch] = true;
		wfSSG_[ch] = { 4, data };
		return;
	}
	}
}

void OPNAController::writeSquareWaveForm(int ch)
{
	if (wfSSG_[ch].type == 0) {
		if (!isKeyOnSSG_[ch]) {
			needEnvSetSSG_[ch] = true;
			needToneSetSSG_[ch] = true;
		}
		return;
	}

	switch (wfSSG_[ch].type) {
	case 3:
	case 4:
		break;
	default:
	{
		needMixSetSSG_[ch] = true;
		break;
	}
	}

	if (isBuzzEffSSG_[ch]) isBuzzEffSSG_[ch] = false;

	needEnvSetSSG_[ch] = true;
	needToneSetSSG_[ch] = true;
	wfSSG_[ch] = { 0, -1 };
}

void OPNAController::writeToneNoiseSSGToRegister(int ch, int seqPos)
{
	if (seqPos == -1) {
		if (needMixSetSSG_[ch]) writeToneNoiseSSGToRegisterNoReference(ch);
		return;
	}

	int type = tnItSSG_[ch]->getCommandType();
	if (type == 0) {	// tone
		if (tnSSG_[ch].isTone_) {
			if (tnSSG_[ch].isNoise_) {
				mixerSSG_ |= (1 << (ch + 3));
				switch (wfSSG_[ch].type) {
				case 1:
				case 2:
					mixerSSG_ |= (1 << ch);
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { false, false, -1 };
					break;
				default:
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { true, false, -1 };
					break;
				}
			}
			else {
				switch (wfSSG_[ch].type) {
				case 1:
				case 2:
					mixerSSG_ |= (1 << ch);
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { false, false, -1 };
					break;
				default:
					break;
				}
			}
		}
		else {
			if (tnSSG_[ch].isNoise_) {
				mixerSSG_ |= (1 << (ch + 3));
				switch (wfSSG_[ch].type) {
				case 1:
				case 2:
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { false, false, -1 };
					break;
				default:
					mixerSSG_ &= ~(1 << ch);
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { true, false, -1 };
					break;
				}
			}
			else {
				switch (wfSSG_[ch].type) {
				case 1:
				case 2:
					break;
				default:
					mixerSSG_ &= ~(1 << ch);
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch] = { true, false, -1 };
					break;
				}
			}
		}
	}
	else {
		if (type > 32) {	// Tone&Noise
			if (tnSSG_[ch].isNoise_) {
				if (tnSSG_[ch].isTone_) {
					switch (wfSSG_[ch].type) {
					case 1:
					case 2:
						mixerSSG_ |= (1 << ch);
						tnSSG_[ch].isTone_ = false;
						opna_.setRegister(0x07, mixerSSG_);
						break;
					default:
						break;
					}
				}
				else {
					switch (wfSSG_[ch].type) {
					case 1:
					case 2:
						break;
					default:
						mixerSSG_ &= ~(1 << ch);
						tnSSG_[ch].isTone_ = true;
						opna_.setRegister(0x07, mixerSSG_);
						break;
					}
				}
			}
			else {
				if (tnSSG_[ch].isTone_) {
					switch (wfSSG_[ch].type) {
					case 1:
					case 2:
						mixerSSG_ |= (1 << ch);
						tnSSG_[ch].isTone_ = false;
						break;
					default:
						break;
					}
				}
				else {
					switch (wfSSG_[ch].type) {
					case 1:
					case 2:
						break;
					default:
						mixerSSG_ &= ~(1 << ch);
						tnSSG_[ch].isTone_ = true;
						break;
					}
				}
				mixerSSG_ &= ~(1 << (ch + 3));
				opna_.setRegister(0x07, mixerSSG_);
				tnSSG_[ch].isNoise_ = true;
			}

			if (!tnSSG_[ch].isTone_ || !tnSSG_[ch].isNoise_) {
				mixerSSG_ &= ~(0x1001 << ch);
				opna_.setRegister(0x07, mixerSSG_);
				tnSSG_[ch].isTone_ = true;
				tnSSG_[ch].isNoise_ = true;
			}


			int p = type - 33;
			if (tnSSG_[ch].noisePeriod_ != p) {
				opna_.setRegister(0x06, p);
				tnSSG_->noisePeriod_ = p;
			}
		}
		else {	// Noise
			if (tnSSG_[ch].isNoise_) {
				if (tnSSG_[ch].isTone_) {
					mixerSSG_ |= (1 << ch);
					opna_.setRegister(0x07, mixerSSG_);
					tnSSG_[ch].isTone_ = false;
				}
			}
			else {
				if (tnSSG_[ch].isTone_) {
					mixerSSG_ |= (1 << ch);
					tnSSG_[ch].isTone_ = false;
				}
				mixerSSG_ &= ~(1 << (ch + 3));
				opna_.setRegister(0x07, mixerSSG_);
				tnSSG_[ch].isNoise_ = true;
			}

			int p = type - 1;
			if (tnSSG_[ch].noisePeriod_ != p) {
				opna_.setRegister(0x06, p);
				tnSSG_->noisePeriod_ = p;
			}
		}
	}

	needMixSetSSG_[ch] = false;
}

void OPNAController::writeToneNoiseSSGToRegisterNoReference(int ch)
{
	switch (wfSSG_[ch].type) {
	case 0:
	case 3:
	case 4:
		mixerSSG_ &= ~(1 << ch);
		tnSSG_[ch].isTone_ = true;
		break;
	case 1:
	case 2:
		mixerSSG_ |= (1 << ch);
		tnSSG_[ch].isNoise_ = false;
		break;
	}
	opna_.setRegister(0x07, mixerSSG_);

	needMixSetSSG_[ch] = false;
}

void OPNAController::writeEnvelopeSSGToRegister(int ch, int seqPos)
{
	if (seqPos == -1 || isBuzzEffSSG_[ch]) return;

	int type = envItSSG_[ch]->getCommandType();
	if (type < 16) {	// Software envelope
		isHardEnvSSG_[ch] = false;
		envSSG_[ch] = { type, -1 };
		setRealVolumeSSG(ch);
	}
	else {	// Hardware envelope
		unsigned int data = envItSSG_[ch]->getCommandData();
		if (envSSG_[ch].data != data) {
			opna_.setRegister(0x0b, 0x00ff & data);
			opna_.setRegister(0x0c, data >> 8);
			envSSG_[ch].data = data;
		}
		if (envSSG_[ch].type != type || !isKeyOnSSG_[ch]) {
			opna_.setRegister(0x0d, type - 16 + 8);
			envSSG_[ch].type = type;
		}
		if (!isHardEnvSSG_[ch]) {
			opna_.setRegister(0x08 + ch, 0x10);
			isHardEnvSSG_[ch] = true;
		}
	}

	needEnvSetSSG_[ch] = false;
}

void OPNAController::checkRealToneSSGByArpeggio(int ch, int seqPos)
{
	if (seqPos == -1) return;

	switch (arpItSSG_[ch]->getSequenceType()) {
	case 0:	// Absolute
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(
										octaveAndNoteToNoteNumber(baseToneSSG_[ch].octave, baseToneSSG_[ch].note)
										+ arpItSSG_[ch]->getCommandType() - 48);
		realToneSSG_[ch].octave = pair.first;
		realToneSSG_[ch].note = pair.second;
		break;
	}
	case 1:	// Fix
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(arpItSSG_[ch]->getCommandType());
		realToneSSG_[ch].octave = pair.first;
		realToneSSG_[ch].note = pair.second;
		break;
	}
	case 2:	// Relative
	{
		std::pair<int, Note> pair = noteNumberToOctaveAndNote(
										octaveAndNoteToNoteNumber(realToneSSG_[ch].octave, realToneSSG_[ch].note)
										+ arpItSSG_[ch]->getCommandType() - 48);
		realToneSSG_[ch].octave = pair.first;
		realToneSSG_[ch].note = pair.second;
		break;
	}
	}

	needToneSetSSG_[ch] = true;
}

void OPNAController::checkRealToneSSGByPitch(int ch, int seqPos)
{
	if (seqPos == -1) return;

	switch (ptItSSG_[ch]->getSequenceType()) {
	case 0:	// Absolute
		realToneSSG_[ch].pitch = ptItSSG_[ch]->getCommandType() - 127;
		break;
	case 1:	// Relative
		realToneSSG_[ch].pitch += (ptItSSG_[ch]->getCommandType() - 127);
		break;
	}

	needToneSetSSG_[ch] = true;
}

void OPNAController::writePitchSSG(int ch)
{
	switch (wfSSG_[ch].type) {
	case 0:	// Square
	{
		uint16_t pitch = PitchConverter::getPitchSSGSquare(realToneSSG_[ch].note,
													 realToneSSG_[ch].octave,
													 realToneSSG_[ch].pitch);
		uint8_t offset = ch << 1;
		opna_.setRegister(0x00 + offset, pitch & 0xff);
		opna_.setRegister(0x01 + offset, pitch >> 8);
		break;
	}
	case 1:	// Triangle
	case 3:
	{
		uint16_t pitch = PitchConverter::getPitchSSGTriangle(realToneSSG_[ch].note,
															 realToneSSG_[ch].octave,
															 realToneSSG_[ch].pitch);
		opna_.setRegister(0x0b, pitch & 0x00ff);
		opna_.setRegister(0x0c, pitch >> 8);
		break;
	}
	case 2:	// Saw
	case 4:
	{
		uint16_t pitch = PitchConverter::getPitchSSGSaw(realToneSSG_[ch].note,
															 realToneSSG_[ch].octave,
															 realToneSSG_[ch].pitch);
		opna_.setRegister(0x0b, pitch & 0x00ff);
		opna_.setRegister(0x0c, pitch >> 8);
		break;
	}
	}

	needToneSetSSG_[ch] = false;
}

void OPNAController::setInstrumentSSGProperties(int ch)
{
	gateCntSSG_[ch] = refInstSSG_[ch]->getGateCount();
}

//---------- Drum ----------//
/********** Key on-off **********/
void OPNAController::keyOnDrum(int ch)
{
	opna_.setRegister(0x10, 1 << ch);
}

void OPNAController::keyOffDrum(int ch)
{
	opna_.setRegister(0x10, 0x80 | (1 << ch));
}

/********** Set volume **********/
void OPNAController::setVolumeDrum(int ch, int volume)
{
	if (volume > 0x1f) return;	// Out of range

	volDrum_[ch] = volume;
	opna_.setRegister(0x18 + ch, (panDrum_[ch] << 6) | volume);
}

/********** Set pan **********/
void OPNAController::setPanDrum(int ch, int value)
{
	panDrum_[ch] = value;
	opna_.setRegister(0x18 + ch, (value << 6) | volDrum_[ch]);
}

/********** Mute **********/
void OPNAController::setMuteDrumState(int ch, bool isMute)
{
	isMuteDrum_[ch] = isMute;

	if (isMute) keyOffDrum(ch);
}

bool OPNAController::isMuteDrum(int ch)
{
	return isMuteDrum_[ch];
}

/***********************************/
void OPNAController::initDrum()
{
	opna_.setRegister(0x11, 0x3f);		// Drum total volume

	for (int ch = 0; ch < 6; ++ch) {
		volDrum_[ch] = 0x1f;	// Init volume

		// Init pan
		panDrum_[ch] = 3;
		opna_.setRegister(0x18 + ch, 0xdf);
	}
}
