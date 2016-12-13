#include "Suorakaide.h"

Suorakaide::Suorakaide()
{
}

Suorakaide::Suorakaide(XYpaikka keskipiste, uint16_t leveys, uint16_t korkeus) :
	keskipiste_(keskipiste), leveysX_(leveys), korkeusY_(korkeus)
{
}


Suorakaide::~Suorakaide()
{
}

XYpaikka Suorakaide::annaKeskipiste() const
{
	return keskipiste_;
}


uint16_t Suorakaide::ylaY() const
{
	return keskipiste_.Y - korkeusY_ / 2;
}

uint16_t Suorakaide::alaY() const
{
	return keskipiste_.Y + korkeusY_ / 2;
}

uint16_t Suorakaide::keskiX() const
{
	return keskipiste_.X;
}

uint16_t Suorakaide::keskiY() const
{
	return keskipiste_.Y;
}

uint16_t Suorakaide::vasenX() const
{
	return keskipiste_.X - leveysX_ / 2;
}

uint16_t Suorakaide::oikeaX() const
{
	return keskipiste_.X + leveysX_ / 2;
}

uint16_t Suorakaide::leveys() const
{
	return leveysX_;
}

uint16_t Suorakaide::korkeus() const
{
	return korkeusY_;
}

uint16_t Suorakaide::leveysR() const
{
	return leveysX_ / 2;
}

uint16_t Suorakaide::korkeusR() const
{
	return korkeusY_ / 2;
}

void Suorakaide::asetaKeskipiste(XYpaikka keskipiste)
{
	keskipiste_.X = keskipiste.X;
	keskipiste_.Y = keskipiste.Y;
}

void Suorakaide::asetaKpX(uint16_t KpX)
{
	keskipiste_.X = KpX;
}

void Suorakaide::asetaKpY(uint16_t KpY)
{
	keskipiste_.Y = KpY;
}

void Suorakaide::asetaLeveys(uint16_t leveys)
{
	leveysX_ = leveys;
}

void Suorakaide::asetaKorkeus(uint16_t korkeus)
{
	korkeusY_ = korkeus;
}

int Suorakaide::onkoSisalla(XYpaikka tarkasta)
{
	//xKord >= 200 && xKord <= 470 && yKord >= 160 && yKord <= 310
	//if (tarkasta.X >= keski.X - xR && tarkasta.X <= keski.X + xR && tarkasta.Y >= keski.Y - yR && tarkasta.Y <= keski.Y + yR)
	if (tarkasta.X >= vasenX() && tarkasta.X <= oikeaX() && tarkasta.Y >= ylaY() && tarkasta.Y <= alaY())
	{
		return 0;
	}
	return 1;
}
