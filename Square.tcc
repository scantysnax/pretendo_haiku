
#ifndef SQUARE_20130206_TCC_
#define SQUARE_20130206_TCC_

//------------------------------------------------------------------------------
// Name: Square
//------------------------------------------------------------------------------
template <int Channel>
Square<Channel>::Square() : sweep_(this), timer_reload_(0), duty_(0), 
		sequence_index_(0), enabled_(false) {

}

//------------------------------------------------------------------------------
// Name: ~Square
//------------------------------------------------------------------------------
template <int Channel>
Square<Channel>::~Square() {
}

//------------------------------------------------------------------------------
// Name: enable
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::enable() {
	enabled_ = true;
}

//------------------------------------------------------------------------------
// Name: disable
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::disable() {
	enabled_ = false;
	length_counter_.clear();
}

//------------------------------------------------------------------------------
// Name: write_reg0
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::write_reg0(uint8_t value) {

	duty_ = (value >> 6) & 0x03;

	if(value & 0x20) {
		length_counter_.halt();
	} else {
		length_counter_.resume();
	}

	envelope_.set_control(value);
}

//------------------------------------------------------------------------------
// Name: write_reg1
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::write_reg1(uint8_t value) {
	sweep_.set_control(value);
}

//------------------------------------------------------------------------------
// Name: write_reg2
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::write_reg2(uint8_t value) {

	timer_reload_ = (timer_reload_ & 0xff00) | value;
	timer_.set_frequency((timer_reload_ + 1) * 2);
	sweep_.set_pulse_period(timer_reload_);
}

//------------------------------------------------------------------------------
// Name: write_reg3
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::write_reg3(uint8_t value) {

	timer_reload_ = (timer_reload_ & 0x00ff) | ((value & 0x07) << 8);
	timer_.set_frequency((timer_reload_ + 1) * 2);
	sweep_.set_pulse_period(timer_reload_);

	if(enabled_) {
		length_counter_.load((value >> 3) & 0x1f);
	}

	sequence_index_ = 0;
	envelope_.start();
}

//------------------------------------------------------------------------------
// Name: tick
//------------------------------------------------------------------------------
template <int Channel>
void Square<Channel>::tick() {
	if(timer_.tick()) {
		sequence_index_ = (sequence_index_ + 1) % 8;
	}
}

//------------------------------------------------------------------------------
// Name: enabled
//------------------------------------------------------------------------------
template <int Channel>
bool Square<Channel>::enabled() const {
	return enabled_;
}

//------------------------------------------------------------------------------
// Name: length_counter
//------------------------------------------------------------------------------
template <int Channel>
LengthCounter &Square<Channel>::length_counter() {
	return length_counter_;
}

//------------------------------------------------------------------------------
// Name: output
//------------------------------------------------------------------------------
template <int Channel>
uint8_t Square<Channel>::output() const {

	static const uint8_t sequence[4][8] = {
		{ 0,1,0,0,0,0,0,0 },
		{ 0,1,1,0,0,0,0,0 },
		{ 0,1,1,1,1,0,0,0 },
		{ 1,0,0,1,1,1,1,1 }
	};

	if((timer_.frequency() - 1) < 8) {
		return 0;
	} else if(sequence[duty_][sequence_index_] == 0) {
		return 0;
	} else if(sweep_.silenced()) {
		return 0;
	} else if(length_counter_.value() == 0) {
		return 0;
	} else {
		return envelope_.volume();
	}
}

//------------------------------------------------------------------------------
// Name: envelope
//------------------------------------------------------------------------------
template <int Channel>
Envelope &Square<Channel>::envelope() {
	return envelope_;
}

//------------------------------------------------------------------------------
// Name: sweep
//------------------------------------------------------------------------------
template <int Channel>
Sweep<Channel> &Square<Channel>::sweep() {
	return sweep_;
}

#endif