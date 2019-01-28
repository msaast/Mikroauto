#pragma once
void nopeusLaskuri();
void rpmLaskuri();
void vaihtoKasky(const uint8_t suunta);
void laheta();
void vastaanota();
void lueROM();
void bensaTutkinta();
void alkuarvojenLahetys();
int ADRead(const uint8_t kanava);
void rajoitusAli();
void kytkimet();

//Arduinon m��rittele sarjaliikenteen aliohjelma.
//Ajetaan aina loop():n j�lkeen jos sarjapuskurissa on joitan.
void serialEvent2()
{
	char sarjaVali = Serial2.read();
	if (sarjaVali == 'L')
	{
		laheta();
	}
	else if (sarjaVali == 'V')
	{
		//Serial.println("Vastaant ota");
		vastaanota();
	}
	else if (sarjaVali == 'a')
	{
		alkuarvojenLahetys();
	}
}

/*Nopeuden lasku fuktio
Laskee globaaliin muuttajaan "nopeus" auton nopeuden kilometrein� tunnissa.*/
void nopeusLaskuri()
{
	nopeusPulssit = TCNT5; //Luetaan nopeuspulssit laskurista.
	TCNT5 = 0; //Nollataan pulssi laskuri.

			   //Mittauksen aika v�lit muistiin
	nopeusPulssitAikaAloitus = nopeusPulssitAikaNyt;
	nopeusPulssitAikaNyt = millisekuntit;

	double matkaVali = ((float)nopeusPulssit / pulssitPerKierrosNopeus) * pyoranKeha; //Metrej�
	double valiAika = (nopeusPulssitAikaNyt - nopeusPulssitAikaAloitus) / 1000.0; //Sekunteja

																				  //Kuljettu matka muistiin
	matka = matka + (matkaVali / 1000.0); //Kilometrej�
	trippi = trippi + matkaVali; //Metrej�

	if (nopeusIndeksi == nopeusTaulukkoKOKO)//Indeksin ymp�ripy�ritys
	{
		nopeusIndeksi = 0;
	}
	nopeusSumma = nopeusSumma - nopeusTaulukko[nopeusIndeksi];

	//lasketaan noupeus ja muutetaan m/s -> km/h
	nopeusTaulukko[nopeusIndeksi] = round((matkaVali / valiAika) * 3.6);

	nopeusSumma = nopeusSumma + nopeusTaulukko[nopeusIndeksi];

	nopeus = round(nopeusSumma / (float)nopeusTaulukkoKOKO);
	nopeusIndeksi++;
}

//Kierrosnopeus lasku funktio
//Laskee globaaliin muuttujaan "rpm" moottorin kierrosnopeuden (1/min).
void rpmLaskuri()
{
	if (kierrosIndeksi == kierrosTaulukkoKOKO)
	{
		kierrosIndeksi = 0;
	}
	rpmSumma = rpmSumma - kierrosLuku[kierrosIndeksi];

	//Pulssien v�linen aika
	uint16_t valiaika;
	if (rpmAlkuhetki > rpmLoppuhetki)
	{
		//Laskuri on vuotanut yli.
		valiaika = 0xFFFF - rpmAlkuhetki + rpmLoppuhetki;
	}
	else
	{
		valiaika = rpmLoppuhetki - rpmAlkuhetki;
	}

	//Serial.println("rpm");
	//Serial.println(valiaika);
	//Serial.println(rpmPulssitLaskenta);

	//Lasketaan kierrostaajuus 4 mikrosekuniin tikeill�.
	//(1000000 / 4 us) = (250000 / 1 tik) >>>> taajuus Hertsein�
	//Kerrotaan viel� pulssien m��r�ll� niin saadaan yhden kierroksen taajuus
	float taajuus = (250000 * rpmPulssitLaskenta) / float(valiaika);
	//Serial.println(taajuus);
	//Nollataan laskenta pulssit, ett� kierroksista saadaan nolla, jos moottori on pois p��lt�.
	rpmPulssitLaskenta = 0;

	//Sijoitetaan arvot taulukkoon, josta lasketaan keskiarvo
	//Muutetaan viel� kierrosta per minuutiksi
	kierrosLuku[kierrosIndeksi] = round(60 * taajuus);

	rpmSumma = rpmSumma + kierrosLuku[kierrosIndeksi];

	rpm = round((rpmSumma / float(kierrosTaulukkoKOKO)));
	//Serial.println(rpm);
	kierrosIndeksi++;
}

void vaihtoKasky(const uint8_t suunta)
{
	if (suunta == vaihtoAlas)
	{
		asetaBitti(vaihtoKaskyAlasPORT, vaihtoKaskyAlasBitti);
	}
	else if (suunta == vaihtoYlos)
	{
		asetaBitti(vaihtoKaskyYlosPORT, vaihtoKaskyYlosBitti);
	}
	vaihtoAikaLaskuri = vaihtoAika;
}

void laheta()
{
	//Serial.println("laheta");
	cli();//stop interrupts
	int matkaVali = round(matka);
	int trippiVali = round(trippi / 100) * 100; //Py�ristet��n satojen tarkkuuteeen

	Serial2.write('A');
	Serial2.write(boolLahetysTavu);//Kyll�/ei tietojen l�hetys.
	Serial2.write(nopeusRajoitus);//nopeusrajoitus
	Serial2.write(vaihde);//vaihde
	Serial2.write(rpm >> 8);//rpm
	Serial2.write(rpm | 0);//rpm
	Serial2.write(nopeus);//nopeus
	Serial2.write(matkaVali >> 8);//matakamittari
	Serial2.write(matkaVali | 0);//matakamittari
	Serial2.write(trippiVali >> 8);//trippi
	Serial2.write(trippiVali | 0);//trippi
	Serial2.write(bensa);//bensa
						 //Serial.println(boolLahetysTavu, DEC);
	sei();//allow interrupts
}

//N�yt�lt� tulevan datan vastaanotto.
void vastaanota()
{
	char otsikko;
	int param;
	while (Serial2.available() == 0) {} //Ootellaan ett� saadaan tavua
	while (Serial2.available() > 0)
	{
		otsikko = Serial2.read();
		switch (otsikko)
		{
		case 'R':
			while (Serial2.available() == 0) {} //Ootellaan ett� saadaan tavua
			param = Serial2.read();
			if (param >= minRajoitus && param <= maxRajoitus)
			{
				nopeusRajoitus = param;
			}
			break;
		case 'N':
			trippi = 0;
			break;
		default:
			break;
		}
	}
}

//Luetaan matkamittarit ja muut Romista
void lueROM()
{
	int osoite = 0;
	//double nolla = 0;
	//Matkamittari talteen
	EEPROM.get(osoite, matka);
	osoite += sizeof(double);
	//Trippimittari talteen
	EEPROM.get(osoite, trippi);
	//Nopeusrajoitus talteen
	osoite += sizeof(double);
	EEPROM.get(osoite, nopeusRajoitus);

	//Serial.print("Luku rom");
	//Serial.print("\n");
	//Serial.println(matka);
	//Serial.println(trippi);
	//Serial.println(nopeusRajoitus, DEC);

}


void bensaTutkinta()
{
	if (bensaIndeksi == bensaTaulukkoKOKO)
	{
		bensaIndeksi = 0;
	}

	bensaSumma = bensaSumma - bensaTaulukko[bensaIndeksi];

	bensaTaulukko[bensaIndeksi] = map(ADRead(bensaSensoriADC), 0, 1023, 0, bensapalkkiKorkeus);

	bensaSumma = bensaSumma + bensaTaulukko[bensaIndeksi];

	bensa = round((bensaSumma / float(bensaTaulukkoKOKO)));
	bensaIndeksi++;
}

//N�yt�lle menevien asetuksien l�hetysfunktio.
void alkuarvojenLahetys()
{
	char kuittaus = ' ';

	while (kuittaus != 'O')
	{
		Serial2.write('A');
		Serial2.write(minRajoitus);
		Serial2.write(maxRajoitus);
		Serial2.write(uint8_t(punaraja / 100.0));
		Serial2.write(maxNayttoKierrokset);

		while (Serial2.available() == 0) {}

		kuittaus = Serial2.read();
	}

}


//ADC-funktio
//Parametrina annetaan AD-kanavan numero.
//Huomiona viel�, ett� t�m� toimii vain 0-7 kanavilla.
//TODO tee muutokset, ett� kaikki 16 kanaavaa on k�yt�ss�.
int ADRead(const uint8_t kavana)
{
	ADMUX = (0b01000000 | kavana); //5V pin referenssin�
	ADCSRA = 0b11000110; //Ennabloi AD ja laita p��lle. Ei intteruptia, scaler 64
	while ((ADCSRA & 0b01000000) != 0) {} // Ootetaan muunnos loppuun
	return ADC; //Palautetaan muunnettu luku
}

void rajoitusAli()
{
	if ((nopeus > nopeusRajoitus) && (rajoitusPaalla == true))
	{
			//Serial.println("rajPe");
		rajoitusTCCRA = rajoitusPaalle; //OC3A p��lle, PIN5, PE3

	}
	else if ((nopeus > pakkiRajoitus) && (vaihde == 'R'))
	{
			//Serial.println("rajPak");
		rajoitusTCCRA = rajoitusPaalle;
	}
	else
	{
			//Serial.println("rajEi");
		rajoitusTCCRA = rajoitusPois; //OC3A pois
	}
}

//Kytkinten k�sittely
//Vilkkukytkinten toiminta ja jarru- ja rajoituskytkinten v�r�htelyn homiointi
//
void kytkimet()
{
	//Vilkut
	vilkkukytkimetTilaEdellinen = vilkkukytkimetTila;
	vilkkukytkimetTila = vilkkukytkimetPIN;
	if (((vilkkukytkimetTila & vilkkukytkinOikeaBitti) == 0) && ((vilkkukytkimetTilaEdellinen & vilkkukytkinOikeaBitti) != 0))
	{
		asetaBitti(vilkkureleOikeaPORT, vilkkureleOikeaBitti);
		vilkkuTila = vilkkuuOikea;
		vilkkumisAikaLaskuri = vilkkumisAika + 1;
		vilkahdusAikaLaskuri = vilkahdusAika + 1;
			//Serial.println("vilOi");

	}
	else if (((vilkkukytkimetTila & vilkkukytkinVasenBitti) == 0) && ((vilkkukytkimetTilaEdellinen & vilkkukytkinVasenBitti) != 0))
	{
		asetaBitti(vilkkureleVasenPORT, vilkkureleVasenBitti);
		vilkkuTila = vilkkuuVasen;
		vilkkumisAikaLaskuri = vilkkumisAika + 1;
		vilkahdusAikaLaskuri = vilkahdusAika + 1;
			//Serial.println("vilVa");

	}

	//Jarrukytkin
	if (jarruKytkinVarahtelyLaskin > 0)
	{
		jarruKytkinVarahtelyLaskin--;
		if (jarruKytkinVarahtelyLaskin == 0)
		{
			//Pinni on yl�s vedetty niin pit�� k��nt�� sen tila, ett� t�m� toimii niin kun olen ajatellu sen alunpitt�in.
			jarruPohjassa = lueBitti(jarruKytkinPIN, jarruKytkinBitti) ^ true;
			kirjoitaBitti(boolLahetysTavu, jarruPojasssaBitti, jarruPohjassa); ///Kirjoitetaan l�hetystavuun jarrun tila.

			asetaBitti(PCIFR, jarruKytkinPCINTbitti);//Nollataan keskeytyslippu
			asetaBitti(PCICR, jarruKytkinPCINTbitti);//Sallitaan keskeytys
		}
	}

	//Rajoituskytkin
	if (rajoitusKytkinVarahtelyLaski > 0)
	{
		rajoitusKytkinVarahtelyLaski--;
		if (rajoitusKytkinVarahtelyLaski == 0)
		{
			rajoitusPaalla = lueBitti(rajoitusKytkinPIN, rajoitusKytkinBitti); //Mik� on rajoituskykimen tila
			kirjoitaBitti(boolLahetysTavu, rajoitusPaallaBitti, rajoitusPaalla);

			asetaBitti(PCIFR, rajoitusKytkinPCINTbitti);//Nollataan keskeytyslippu
			asetaBitti(PCICR, rajoitusKytkinPCINTbitti);//Sallitaan keskeytys
		}
	}

}