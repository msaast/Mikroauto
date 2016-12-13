#pragma once
#include "XYpaikka.h"
#include "stdint.h"

class Suorakaide
{
public:
	Suorakaide();
	Suorakaide(XYpaikka keskipiste, uint16_t leveys, uint16_t korkeus);
	~Suorakaide();

	XYpaikka annaKeskipiste() const;
	uint16_t keskiX() const;
	uint16_t keskiY() const;
	uint16_t ylaY() const;
	uint16_t alaY() const;
	uint16_t vasenX() const;
	uint16_t oikeaX() const;

	uint16_t leveys() const;
	uint16_t korkeus() const;
	uint16_t leveysR() const;
	uint16_t korkeusR() const;

	void asetaKeskipiste(XYpaikka keskipiste);
	void asetaKpX(uint16_t KpX);
	void asetaKpY(uint16_t KpY);

	void asetaLeveys(uint16_t leveys);
	void asetaKorkeus(uint16_t korkeus);

	int onkoSisalla(XYpaikka tarkasta);

private:
	XYpaikka keskipiste_;
	uint16_t leveysX_;
	uint16_t korkeusY_;
};

