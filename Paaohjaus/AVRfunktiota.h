#pragma once

#define ULOS_0 0
#define ULOS_1 1
#define KELLUU 2
#define YLOSVETO 3


//Pinnin tominnon asetusfunktio
//
void pinninToiminto(uint8_t bitti, uint8_t DDR, uint8_t PORT, uint8_t toiminta)
{
	if (toiminta == ULOS_0)
	{
		DDR = DDR | bitti;
		PORT = PORT & (~bitti);
	}
	else if (toiminta == YLOSVETO)
	{
		DDR = DDR & (~bitti);
		PORT = PORT | bitti;
	}
	else if (toiminta == KELLUU)
	{
		DDR = DDR & (~bitti);
		PORT = PORT & (~bitti);
	}
	else if (toiminta == ULOS_1)
	{
		DDR = DDR | bitti;
		PORT = PORT | bitti;
	}
}