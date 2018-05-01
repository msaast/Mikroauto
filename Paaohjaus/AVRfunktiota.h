#pragma once

#define ULOS_0 0
#define ULOS_1 1
#define KELLUU 2
#define YLOSVETO 3

#define nollaaBitti(tavu, bitti) ((tavu) &= (~bitti))
#define asetaBitti(tavu, bitti) ((tavu) |= (bitti))
#define lueBitti(tavu, bitti) (((tavu) & (bitti)) ? (1) : (0))
#define kirjoitaBitti(tavu, bitti, tila) (tila ? asetaBitti(tavu, bitti) : nollaaBitti(tavu, bitti))

//Pinnin tominnon asetusfunktio
//
void pinninToiminto(const uint8_t bitti, volatile uint8_t* DDR, volatile uint8_t* PORT, const uint8_t toiminta)
{
	if (toiminta == ULOS_0)
	{
		*DDR = *DDR | bitti;
		*PORT = *PORT & (~bitti);
	}
	else if (toiminta == YLOSVETO)
	{
		*DDR = *DDR & (~bitti);
		*PORT = *PORT | bitti;
	}
	else if (toiminta == KELLUU)
	{
		*DDR = *DDR & (~bitti);
		*PORT = *PORT & (~bitti);
	}
	else if (toiminta == ULOS_1)
	{
		*DDR = *DDR | bitti;
		*PORT = *PORT | bitti;
	}
}