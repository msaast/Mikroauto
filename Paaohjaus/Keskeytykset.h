#pragma once
//********************Keskeytyspalvelut****************************//

//Kello1, ylivuotokeskeytys
//32 bit + 16 bit muuttuja 0,5 us askeleella = noin 1600 d = 4,5 a
//RPM laskenta v‰li on 32,768 ms
ISR(TIMER1_OVF_vect)
{
	kello1YlivuotoLaskuri++;

	//Jos moottori on sammunut, rpmMittaa kasvaa suureksi, kaapauskeskeytys ei nollaa sit‰.
	//T‰ll‰in pit‰‰ k‰yd‰ laskemasssa kierrosluku nollaksi.
	rpmMittaa++;
}

//Kello1, A-vertailukeskeytys
//1 ms, 1 ms * 2^32 = noin 4,9 d
ISR(TIMER1_COMPA_vect)
{
	OCR1A = OCR1A + lyonnit_1ms;
	millisekuntit++;
}
//Kello1, B-vertailukeskeytys
//10 ms, hitaiden kytkimien ja sensoreiden luku
ISR(TIMER1_COMPB_vect)
{
	OCR1B = OCR1B + lyonnit_10ms;
	lippu10ms = true;
}

//Kello4, aikakaappauskeskeytys
//Moottorin kierrosluku lasketaan nelj‰n mikrosekuntin tarkkuudella.
//Kaappauskeskeytyksess‰ lasketaan kierrospulssit noin 30-60 millisekuntin ajalta.
//Lasku aikav‰li tulee kello1:lt‰
ISR(TIMER4_CAPT_vect)
{
	rpmPulssit++;

	if (rpmMittaa > 0)
	{
		rpmAlkuhetki = rpmLoppuhetki;
		rpmLoppuhetki = ICR4;
		rpmPulssitLaskenta = rpmPulssit;

			//Serial.println("kaappaus laskenta");
			//Serial.println(rpmPulssit);

		rpmPulssit = 0;
		rpmLaske = true;
		rpmMittaa = 0;

	}
}

ISR(ADC_vect)
{

}

/* Kirjoitettaan tarvittavat tiedot EEPROMiin talteen, kun virrat katkaistaan.
*/
ISR(INT2_vect)
{

	uint16_t viive = 0xFFFF; //÷÷h vaikka 3000 *  kellojaksoa noin 350 ms.
	//Tehd‰‰n pieni viive ennekuin sammutetaan systeemit, jos meill‰ on jotain v‰r‰htely‰.
	while (viive != 0)
	{
		__asm__ volatile ("nop");
		viive--;
	}
	//Sitten katotaan onko meill‰ viel‰ virtalukko sammuksissa.
	if (lueBitti(virratPoisPIN, virratPoisBitti) == 1)
	{
		int osoite = 0;
		//Matkamittari talteen
		//double nolla = 0;
		EEPROM.put(osoite, matka);
		osoite += sizeof(double);
		//Trippimittari talteen
		EEPROM.put(osoite, trippi);
		//Nopeusrajoitus talteen
		osoite += sizeof(double);
		EEPROM.put(osoite, nopeusRajoitus);


		//Serial.print("kirjoita rom");
		//Serial.print("\n");
		//Serial.println(nopeusRajoitus, DEC);


		//Laitetaan virtalukon ohitus pois p‰‰lt‰
		nollaaBitti(virtalukonOhistusPORT, virtalukonOhistusBitti);

		//Otetaan ett‰ virrat katkee
		while (true)
		{
		}
	}

	asetaBitti(EIFR, virratPoisINT);
}

//Vaihto alas
ISR(INT3_vect)
{
	//Estet‰‰n kytkinten keskeytykset kunnes vaihde on vaihdettu.
	nollaaBitti(EIMSK, vaihtoKytkinINTmaski);
	vaihtoKytkinTila = vaihtoKytkinTilaAlas;
	//Serial.println("alas vaihto");
}

//Vaihto ylˆs
ISR(INT4_vect)
{
	//Estet‰‰n kytkinten keskeytykset kunnes vaihde on vaihdettu.
	nollaaBitti(EIMSK, vaihtoKytkinINTmaski);
	vaihtoKytkinTila = vaihtoKytkinTilaYlos;
	//Serial.println("ylos vaihto");
}

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois p‰‰lt‰ poistetaan rajoitus.*/
ISR(PCINT0_vect)
{
	nollaaBitti(PCICR, rajoitusKytkinPCINTbitti);//Estet‰‰n keskeytykset
	rajoitusKytkinVarahtelyLaski = rajoitusKytkinVarahtelyAika;
	//Serial.println("rkyt");
}

//Jarrun tilan tutkinta interrupti
ISR(PCINT1_vect)
{
	nollaaBitti(PCICR, jarruKytkinPCINTbitti);//Estet‰‰n keskeytykset
	jarruKytkinVarahtelyLaskin = jarruKytkinVarahtelyAika;
	//Serial.println("jkyt");
}

//*******************************************************