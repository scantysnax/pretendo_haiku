
#ifndef MAPPER240_20080314_H_
#define MAPPER240_20080314_H_

#include "Mapper.h"

class Mapper240 : public Mapper {
public:
	Mapper240();

public:
	virtual std::string name() const;

public:
	virtual void write_4(uint16_t address, uint8_t value);
	virtual void write_5(uint16_t address, uint8_t value);

private:
	void write_handler(uint16_t address, uint8_t value);
};

#endif