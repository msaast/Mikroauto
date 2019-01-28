/*Mikroauton pääohjelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen ylös tai alas.

*/
#include <Arduino.h>
#include <EEPROM.h>
#include "AVRfunktiota.h"
#include "Pinnit.h"

//************************Aikakeskeytys ajat
//(16 MHz : 8) kello 16 bitin laskurilla = 0,5 us 
#define lyonti 0.0000005 //0,5 us
#define t10ms 0.010
#define t1ms 0.001
#define lyonnit_10ms (uint16_t)(t10ms/lyonti) //10 ms : 0,5 us = 10 ms
#define lyonnit_1ms (uint16_t)(t1ms/lyonti) //1 ms
#define rpmMittausvali 0xFFFF

#define Fmcu 16000000UL


//************************Vakioita
#define pii 3.14159 //Pii
#define kierrosTaulukkoKOKO 2
#define nopeusTaulukkoKOKO 6
#define bensaTaulukkoKOKO 20

/*
Vaihteitte tutkinta rekisterin avulla.
/Due pinnit 33-37 PC1-PC5
Mega pinnit 37-33 PC0-PC4
B10000 >> 1 = B01000 = 8
B01000 >> 1 = B00100 = 4
B00100 >> 1 = B00010 = 2
B00010 >> 1 = B00001 = 1
B00001 >> 1 = B00000 = 0
Indeksoidaan talukosta oikea vaihde.
*/
const char vaihteet[] = "N12x3xxxR";
#define lueVaihde() vaihteet[(vaihdePIN ^ vaihdeBititMaski) >> 1]

//************************Asetuksia
#define pyoranHalkaisija 0.35 //Pyörän halkaisija metreinä.
#define vetoakseliHampaat 37 //Vetoakselin ketjupyörän hammapaat
#define valiakseliHampaat 16 //Väliakselin ketjupyörän hammapaat
#define anturiHampaat 15 //Väliakselilla olevan anturipyörän hampaat

#define vetoValiValitys float(vetoakseliHampaat/valiakseliHampaat)
#define pulssitPerKierrosNopeus float(anturiHampaat * vetoValiValitys) //Montako pulssia tulee per kierros (nopeus).
#define pyoranKeha float(pyoranHalkaisija * pii)


#define vaihtoRPM  2000 //Moottorin kierrosnopeus, jonka alle ollessa saa vaihtaa vaihteen (1/min).
#define vaihtoAika uint8_t(1/t10ms) //100 * 10 ms, Vaihtopulssin kesto TODO pitää testata mikä on hyvä aika
#define vaihtoAikaLiikaaRPM uint8_t(0.03/t10ms) //Kytkin värähähtelyn huomointi aika vaan
//Rajoittimen asetukset
#define rajoitusPWMtaajuus 20 //Hz
#define rajoitusPWMduty 0.7 

//Vilkun asetukset
#define vilkahdusAika uint8_t(0.5/t10ms) //50 * 10 ms = 0,5 s => 1 hetsin taajuus 50 % dutyllä
#define vilkkumisAika uint16_t(10/t10ms) //1000 * 10 ms = 10s, Aika minkä vilkku vilkuttaa kytkimen painamisen jälkeen.

#define pakkiRajoitus 5 //Pakin nopeusrajoitus
#define minRajoitus 5
#define maxRajoitus 40
#define maxNayttoKierrokset 11 //*1000
#define punaraja 8000
#define bensapalkkiKorkeus 120
//***************

//Rajoitus PWM maskeja 
#define rajoitusPaalle 0b10000010
#define rajoitusPois 0b00000010
#define rajoitusTCCRA TCCR3A

//************************Globaalit muuttujat
//Aikalaskureita ja -lippuja
volatile uint32_t kello1YlivuotoLaskuri = 0; //Kellon 32 ylintä bitti
volatile uint32_t millisekuntit = 0;
bool lippu10ms = true;

//****Päämuuttujat
uint8_t nopeus = 0; //Auton nopeus (km/h)
uint16_t rpm = 0; //Moottorin kierrosnopeus (1/min)
uint8_t nopeusRajoitus = 20;
uint8_t bensa = 0;
double matka = 0;
double trippi = 0;
char vaihde = 'N'; //Päällä oleva vaihde
bool jarruPohjassa = false;
//****

//Kierrosluvun laskenta
uint8_t rpmPulssit = 0;
uint8_t rpmPulssitLaskenta = 0;
uint16_t rpmAlkuhetki = 0;
uint16_t rpmLoppuhetki = 0;
uint8_t rpmMittaa = 0;
bool rpmLaske = false;
uint16_t kierrosLuku[kierrosTaulukkoKOKO] = { 0 }; //Taulukko, johon tallennetaan kierroslukuja, että voidaan laskea keskiarvoja.
uint8_t kierrosIndeksi = 0;
double rpmSumma = 0;

//Nopeuden laskenta
volatile uint16_t nopeusPulssit = 0; //Muuttuja johon lasketaan pyörältä tulleet pulssit.
uint32_t nopeusPulssitAikaNyt = 0; //Aika jolloin aloitettiin laskemaan nopeus pulsseja.
uint32_t nopeusPulssitAikaAloitus = 0;
#define nopeudenLaskentaAika uint8_t(0.2/t10ms) //20 * 10 ms = 200 ms
uint8_t nopeudenLaskentaAikaLaskuri = nopeudenLaskentaAika;
double nopeusSumma = 0;
uint8_t nopeusTaulukko[nopeusTaulukkoKOKO] = { 0 };
uint8_t nopeusIndeksi = 0;

//Polttoaineen mittaaminen
#define bensaMittausAika uint16_t(3/t10ms) //300 * 10ms = 3s
uint16_t bensaMittausAikaLaskuri = bensaMittausAika;
double bensaSumma = 0;
uint8_t bensaTaulukko[bensaTaulukkoKOKO] = { 0 };
uint8_t bensaIndeksi = 0;

//Jarrukytkin
#define jarruKytkinVarahtelyAika 5 //4 * 10 ms, Kytkinvärähtely jäähy
uint8_t jarruKytkinVarahtelyLaskin = 0;

//Rajoituskytkin
#define rajoitusKytkinVarahtelyAika 5 //4 * 10 ms, Kytkinvärähtely jäähy
uint8_t rajoitusKytkinVarahtelyLaski = 0;

bool rajoitusPaalla = false; //Kierrosten rajoittimen muuttuja
bool liikaaKierroksia = false;

//Vaihteen vaihto
#define vaihtoKytkinTilaKeski 0
#define vaihtoKytkinTilaAlas 1
#define vaihtoKytkinTilaYlos 2
#define vaihtoAlas 1
#define vaihtoYlos 2
uint8_t vaihtoKytkinTila = 0; //0 = keskellä, 1 = alas vaihto, 2 = ylös vaihto
uint16_t vaihtoAikaLaskuri = 0;

//Vilkut
#define eiVilku 0
#define vilkkuuOikea vilkkukytkinOikeaBitti
#define vilkkuuVasen vilkkukytkinVasenBitti
uint16_t vilkkumisAikaLaskuri = 0;
uint8_t vilkahdusAikaLaskuri = 0;
uint8_t vilkkuTila = 0;
uint8_t vilkkukytkimetTila = 0;
uint8_t vilkkukytkimetTilaEdellinen = 0;

/*
B00000001 = Rajoitus päällä/pois
B00000010 = Liikaa kierroksia vaihtoon
B00000100 = Jarru pohjassa
B00001000 =
B00010000 =
B00100000 =
B01000000 =
B10000000 =							*/
#define rajoitusPaallaBitti 0b1
#define liikaaRPMbitti 0b10
#define jarruPojasssaBitti 0b100
uint8_t boolLahetysTavu = 0; //Tavu millä voidaan lähettää 8 bool bittiä.

#include "Keskeytykset.h"

#include "Aliohjelmat.h"

void setup()
{
	//************************Digi ulostulot
	//Virtalukon ohitus asetukset ja FETti laitetaan johtavaan tilaan saman tien.
	pinninToiminto(virtalukonOhistusBitti, &virtalukonOhistusDDR, &virtalukonOhistusPORT, ULOS_1);

	//Vilkkureleet
	pinninToiminto(vilkkureleVasenBitti, &vilkkureleVasenDDR, &vilkkureleVasenPORT, ULOS_0);
	pinninToiminto(vilkkureleOikeaBitti, &vilkkureleOikeaDDR, &vilkkureleOikeaPORT, ULOS_0);

	//Vaiheteen vaihto
	pinninToiminto(vaihtoKaskyAlasBitti, &vaihtoKaskyAlasPORT, &vaihtoKaskyAlasPORT, ULOS_0);
	pinninToiminto(vaihtoKaskyYlosBitti, &vaihtoKaskyYlosPORT, &vaihtoKaskyYlosPORT, ULOS_0);

	//Pakkipiippi
	pinninToiminto(pakkipiipppiBitti, &pakkipiipppiDDR, &pakkipiipppiPORT, ULOS_0);

	//**********************UART asetukset
	Serial.begin(57600);
	Serial2.begin(115200);

	//**********************Kellot/laskuri
	//**Kello0
	TIMSK0 = 0; //Laitetaan Arduino-kirjaston millis() pois päältä.

	//**Kello1
	//Aikakeskeytys
	//Tik 0,5 us, 0,5 us * 2^16 = 32,768 ms
	OCR1A = lyonnit_1ms;
	OCR1B = lyonnit_10ms;
	TCCR1A = 0b01010000; //Toggle A ja B, normaali laskri
	TCCR1B = 0b00000010; //Jakaja 8, katto 0xFFFF
	TIMSK1 = 0b00000111; //A ja B vertailu- ja ylivuotokesketys päällä

	//**Kello3
	//PWM 
	//WGM 1110, CS 101
	pinninToiminto(rajoitusSSRBitti, &rajoitusSSRDDR, &rajoitusSSRPORT, ULOS_0);
	//Fast PWM 10-bit, OCR nollaa, katto ICR, asettaa pohjalla
	TCCR3A = rajoitusPois; //0b10000010 OCR3A = päällä
	TCCR3B = 0b00011101;//Esijakaja 1024
	#define rajoitusPWMICR round(Fmcu/(1024 * rajoitusPWMtaajuus) - 1)
	#define rajoitusPWMOCR round(rajoitusPWMICR * rajoitusPWMduty)
	ICR3 = rajoitusPWMICR;
	rajoitusSSROCR = rajoitusPWMOCR;

	//**kello4
	//Kierroslukusignaalin aikakaappus
	//Tik 4 us, 4 us * 2^16 = 262,144 ms
	pinninToiminto(rpmSisaanBitti, &rpmSisaanDDR, &rpmSisaanPORT, KELLUU);
	TCCR4A = 0;//Ulostulos irti, normaali kellon laskenta
	TCCR4B = 0b01000011; //Esijakaja 64, katto 0xFFFF
	TIMSK4 = 0b00100000; //Kaappauskeskeytys

	//**kello5
	//Ulkoisensignaalin laskuri
	//Normaali laskuri, katto 16-bit max
	pinninToiminto(nopeuspulssiBitti, &nopeuspulssiDDR, &nopeuspulssiPORT, KELLUU);
	TCCR5A = 0;
	TCCR5B = (1 << CS52) | (1 << CS51) | (1 << CS50); //0b00000111
	TCCR5C = 0;

	//************************Ulkoiset interuptit
	//INT2
	pinninToiminto(virratPoisBitti, &virratPoisDDR, &virratPoisPORT, YLOSVETO);
	//INT3
	pinninToiminto(vaihtoKytkinAlasBitti, &vaihtoKytkinAlasDDR, &vaihtoKytkinAlasPORT, YLOSVETO);
	//INT4
	pinninToiminto(vaihtoKytkinYlosBitti, &vaihtoKytkinYlosDDR, &vaihtoKytkinYlosPORT, YLOSVETO);

	EICRA = 0b10110000; //Keskeytys 2 nousevalla reunalla ja 3 laskevalla reunalla
	EICRB = 0b10;//Keskeytys 4 laskevalla reunalla
	EIMSK = 0b00011100; //Interuptimaski
	//EIMSK = 0b0011000; //ROMin kirjotus poissa

	//PCINT0 tilanvaihto keskeytys
	pinninToiminto(jarruKytkinBitti, &jarruKytkinDDR, &jarruKytkinPORT, KELLUU);
	//PCINT9 tilanvaihto keskeytys
	pinninToiminto(rajoitusKytkinBitti, &rajoitusKytkinDDR, &rajoitusKytkinPORT, YLOSVETO);

	PCICR = 0b11; //0 ja 1 tilamuutos joukot päällä
	PCMSK0 = 0b1; //0 joukosta PCINT0 päällä
	PCMSK1 = 0b10; //1 joukosta PCINT9 päällä

	//**********************Digi sisään menot
	//Vaihteet, kaikki kerralla
	pinninToiminto(vaihdeBititMaski, &vaihdeDDR, &vaihdePORT, YLOSVETO);
	
	//Vilkkukytkimet
	pinninToiminto(vilkkukytkinOikeaBitti, &vilkkukytkinOikeaDDR, &vilkkukytkinOikeaPORT, YLOSVETO);
	pinninToiminto(vilkkukytkinVasenBitti, &vilkkukytkinVasenDDR, &vilkkukytkinVasenPORT, YLOSVETO);

	//**********************Analogi sisään menot
	//Otetaan digi asiat analogipinneistä pois päältä.
	//Bensamittarin kohopotikka
	asetaBitti(bensaSensoriDIDR, bensaSensoriADCD);

	//Jänniteen jako akun jännitteelle
	asetaBitti(akkuJanniteDIDR, akkuJanniteADCD);

	//**********************Ennen pääohjelmaan alkua tehtävät hommat
	//Vaihdetaan näytön kanssa tarvittavat alkutiedot.
	alkuarvojenLahetys();

	//Mikä on rajoituskykimen ja jarrykytkimen tila, kun laitettaan virrat päällle?
	rajoitusPaalla = lueBitti(rajoitusKytkinPIN, rajoitusKytkinBitti);
	kirjoitaBitti(boolLahetysTavu, rajoitusPaallaBitti, rajoitusPaalla);



	jarruPohjassa = lueBitti(jarruKytkinPIN, jarruKytkinBitti) ^ true;
	kirjoitaBitti(boolLahetysTavu, jarruPojasssaBitti, jarruPohjassa);

	//Mikä vaihde on päällä?
	vaihde = lueVaihde();

	//Luetaan EEPROMista tarvittavat tiedot
	lueROM();

	//Täytetään bensa taulukko
	for (uint8_t i = 0; i < bensaTaulukkoKOKO; i++)
	{
		bensaTutkinta();
	}
	Serial.println("bensa alussa");
	Serial.println(bensa);
	//matka = 32413.3;
	//trippi = 113121.2;
}

void loop()
{
	
	if (rpmLaske == true || rpmMittaa > 2)
	{
		rpmLaske = false;
		rpmMittaa = 0;
		rpmLaskuri();
	}

	//10 ms:n ja sen kerrannaisten välein tapahtuvat asiat
	if (lippu10ms == true)
	{
		lippu10ms = false;

		kytkimet();
		//
		if (vaihtoAikaLaskuri > 0) //Onko vaihtoo vaihto vielä meneillään
		{
			//Serial.println("vaihedaan");

			vaihtoAikaLaskuri--;
			if (vaihtoAikaLaskuri == 0)
			{
				//Vaihteen vaihto pois päältä
				nollaaBitti(vaihtoKaskyAlasPORT, vaihtoKaskyAlasBitti);
				nollaaBitti(vaihtoKaskyYlosPORT, vaihtoKaskyYlosBitti);

				//Nollataan keskeytysliput värähtelyistä.
				asetaBitti(EIFR, vaihtoKytkinINTmaski);
				//Sallitaan taas vaihtokytkimien keskeytykset.
				asetaBitti(EIMSK, vaihtoKytkinINTmaski);

				//Tutkitaan mihinkä vaihteeseen vaihdettiin.
				vaihde = lueVaihde();
			}
		}

		//
		nopeudenLaskentaAikaLaskuri--;
		if (nopeudenLaskentaAikaLaskuri == 0)
		{
			nopeudenLaskentaAikaLaskuri = nopeudenLaskentaAika;
			nopeusLaskuri();

			//Kun uusi nopeus on laskettu, on syytä tarkastaa pitääkö sitä rajoittaa.
			//Muulloinhan sitä ei taida tarvita tutkia.
			rajoitusAli();
		}

		//
		if (vilkkumisAikaLaskuri > 0)
		{
				//Serial.println(vilkkumisAikaLaskuri);
			vilkkumisAikaLaskuri--;
			if (vilkkumisAikaLaskuri == 0)
			{
				nollaaBitti(vilkkureleePORT, vilkkureleetMaski);
			}
			else
			{
				vilkahdusAikaLaskuri--;
				if (vilkahdusAikaLaskuri == 0)
				{
					vilkahdusAikaLaskuri = vilkahdusAika;
					if (vilkkuTila == vilkkuuOikea)
					{
						kaannaBitti(vilkkureleOikeaPORT, vilkkureleOikeaBitti);
					}
					else if (vilkkuTila == vilkkuuVasen)
					{
						kaannaBitti(vilkkureleVasenPORT, vilkkureleVasenBitti);
					}
				}
			}

		}
		
		//
		bensaMittausAikaLaskuri--;
		if (bensaMittausAikaLaskuri == 0)
		{
			bensaMittausAikaLaskuri = bensaMittausAika;
			bensaTutkinta();
				//Serial.println("bensa");
				//Serial.println(bensa);
			//Katotaan mikä vaihde no päällä vaikka täälläkin, jos se sattuu jotenkin vaihtumaan itestään.
			vaihde = lueVaihde();
		}

	}


	//Vaihteen vaihtoon mennään, jos jompaa kumpaa vaihto nappia on painettu .
	if (vaihtoKytkinTila > vaihtoKytkinTilaKeski)//Aika rajoitus
	{
		//TODO  Mieti vaihteen vaihto rajoitukset kunnolla. Nopeuden ja vaihteen suhteen tai joitan.
		//Mietin yhden kerran ja tulin siihen tulokseen, että ei välttämättä tarvii.
		//Kovassa vauhdissa liian pienelle vahtaminen lienee suurin ongelma, mutta se nyt ei ole kovin vaarallista.

		if (rpm < vaihtoRPM) //Päästään vaihtamaan jos moottorin kierrosnopeus on tietyn rajan alle.
		{
			nollaaBitti(boolLahetysTavu, liikaaRPMbitti);

			switch (vaihde)
			{
				case 'R': //Pakilta annetaan vaihtaa vain ylös päin.
					if (vaihtoKytkinTila == vaihtoKytkinTilaYlos)
					{
						vaihtoKasky(vaihtoYlos);
					}
					break;
				case 'N': //Vappaalta voi vaihtaa kumpaankin suuntaa, mutta jarru pitää olla painettuna.

					if ((vaihtoKytkinTila == vaihtoKytkinTilaAlas) && (jarruPohjassa == true))
					{
						vaihtoKasky(vaihtoAlas);
					}
					else if ((vaihtoKytkinTila == vaihtoKytkinTilaYlos) && (jarruPohjassa == true))
					{
						vaihtoKasky(vaihtoYlos);
					}
					break;
				case '1': //Ykköseltä voi vaihtaa kumpaankin suuntaa.
					if (vaihtoKytkinTila == vaihtoKytkinTilaAlas)
					{
						vaihtoKasky(vaihtoAlas);
					}
					else if (vaihtoKytkinTila == vaihtoKytkinTilaYlos)
					{
						vaihtoKasky(vaihtoYlos);
					}
					break;
				case '2': //Kakkoselta voi vaihtaa kumpaankin suuntaa.
					if (vaihtoKytkinTila == vaihtoKytkinTilaAlas)
					{
						vaihtoKasky(vaihtoAlas);
					}
					else if (vaihtoKytkinTila == vaihtoKytkinTilaYlos)
					{
						vaihtoKasky(vaihtoYlos);
					}
					break;
				case '3': //Kolmeoselta voi vaihtaa vain alas päin.
					if (vaihtoKytkinTila == vaihtoKytkinTilaAlas)
					{
						vaihtoKasky(vaihtoAlas);
					}
					break;
				default:
					break;
			}
		}
		else
		{
			asetaBitti(boolLahetysTavu, liikaaRPMbitti); //Kirjoitettaan lähetystavuun, että liikaa kierroksia vaihteen vaihtoon.
				//Kaasu ylös!!
			vaihtoAikaLaskuri = vaihtoAikaLiikaaRPM;
		}
		vaihtoKytkinTila = vaihtoKytkinTilaKeski;
	}
}

