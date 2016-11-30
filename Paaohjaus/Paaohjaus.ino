/*Mikroauton p��ojelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen yl�s tai alas.

*/

#include <Arduino.h>
#include <EEPROM.h>

//Funktioiden otsikot
void nopeusLaskuri();
void rpmLaskuri();
void vaihtoKasky(int kaskyPin);
void laheta();
void vastaanota();

//Pinnien paikat
//interruptit

const int vastaanotaPin = 18; //
const int lahetaPin = 19;
const int virratPoisPin = 20;
const int rajoituksenKytkentaPin = 51; //Kierrostenrajoittimen avainkytkimen pinni

									   //PWM-pinnit (HUOM! Taajuuksia on vaihdettu,joten PWMm�� on ohjattu suoraan rekisteri� manipuloimalla.)
const int ylosVaihtoKaskyPin = 9; //K�skys moottorille vaihtaa yl�s. kello2 OC2B 
const int alasVaihtoKaskyPin = 10; //K�skys moottorille vaihtaa alas. kello2 OC2A 
const int rajoitusPWMpin = 5; //kello3
const int vilkkuOikeaPWMpin = 6; //kello4 duty=OCR4A
const int vilkkuVasenPWMpin = 7; //kello4 duty=OCR4B

const int servoajuriKytkentaPin = 8; //Servon kytkent� pinni
const int bensaSensoriPin = 0; //Bensa sensori potikka (A0)

								//Kytkimet
const int vaihtoKytkinAlasPin = 22; //Ajajan vaihtokytkin alas
const int vaihtoKytkinYlosPin = 23; //Ajajan vaihtokytkin yl�s
const int jarruKytkinPin = 24;
const int vilkkuOikeaKytkinPin = 25;
const int vilkkuVasenKytkinPin = 26;

//Valot
const int jarruvaloPin = 46;

//Vaihteiden pinnit:
const int vapaaPin = 37; //N Mega
const int ykkonenPin = 36; //1
const int kakkonenPin = 35; //2
const int kolmonenPin = 34; //3
const int pakkiPin = 33; //R
						 //Vakioita
const float pii = 3.14159; //Pii
const uint8_t kierrosTaulukkoKOKO = 6;
const uint8_t nopeusTaulukkoKOKO = 6;
const uint8_t bensaTaulukkoKOKO = 20;

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

//Asetuksia
const float pyoraHalkaisija = 0.3; //Py�r�n halkaisija metrein�.
const uint16_t vaihtoRpm = 1800; //Moottorin kierrosnopeus, jonka alle ollessa saa vaihtaa vaihteen (1/min).
const uint16_t pulssitPerKierrosNopeus = 200; //Montako pulssia tulee per kierros (nopeus).
const uint16_t vaihtoNappiAika = 2000; //Aika mink� vaihto napin on oltava ylh��ll� ett� voidaan uudestaa koittaa vaihtaan vaihetta.
const uint16_t vaihtoAika = 1000; //Vaihtopulssin kesto
const uint16_t nopeudenLaskentaAika = 200;
const uint16_t bensaMittausAika = 3000; //3s
const uint16_t rajoitusAika = 50;
const uint16_t vilkkuNopeus = 20000; //Alustavasti hyv� taajuus
const uint16_t pakkiRajoitus = 5; //Pakin nopeusrajoitus
const uint8_t minRajoitus = 5;
const uint8_t maxRajoitus = 30;
const uint8_t maxNayttoKierrokset = 11; //*1000
const uint16_t punaraja = 8000;
const uint8_t bensapalkkiKorkeus = 120;

//Globaalit muuttujat
volatile uint16_t nopeusPulssit = 0; //Muuttuja johon lasketaan py�r�lt� tulleet pulssit.
volatile uint16_t rpmMuunnnos = 0;
bool laskeNopeus = true;
bool laskeKierrokset = true;
bool laskeBensa = true;

bool kuittaus = true; //Vaihteen vaihtajan kuittas muuttuja
bool rajoitus = false; //Kierrosten rajoittimen muuttuja
bool vaihtoNappiYlhaalla = true; //
bool liikaaKierroksia = false;
unsigned long vaihdetaanVanhaAika = 0;
unsigned long vaihtoNappiVanhaAika = 0; //
unsigned long nopeusPulssitAika = 0; //Aika jolloin aloitettiin laskemaan nopeus pulsseja.
volatile uint16_t nopeudenLaskentaAikaLaskuri = 0;
volatile uint16_t bensaMittausAikaLaskuri = 0;
int vaihdeKytkinYlos = HIGH; //Vaihteen vaihto kytkin yl�s tila
int vaihdeKytkinAlas = HIGH; //Vaihteen vaihto kytkin alas tila
uint8_t nopeus = 0; //Auton nopeus (km/h)
uint16_t rpm = 0; //Moottorin kierrosnopeus (1/min)
uint8_t nopeusRajoitus = 20;
uint8_t bensa = 0;
double matka = 0;
double trippi = 0;
char vaihde = 'N'; //P��ll� oleva vaihde
bool jarruvalo = HIGH;

/*
B00000001 = Rajoitus p��ll�/pois
B00000010 = Liikaa kierroksia vaihtoon
B00000100 = Jarru pohjassa
B00001000 =
B00010000 =
B00100000 =
B01000000 =
B10000000 =							*/
uint8_t boolLahetysTavu = 0;

uint16_t kierrosLuku[kierrosTaulukkoKOKO] = { 0 }; //Taulukko, johon tallennetaan kierroslukuja, ett� voidaan laskea keskiavaja.
uint8_t kierrosIndeksi = 0;
uint8_t nopeusTaulukko[nopeusTaulukkoKOKO] = { 0 };
uint8_t nopeusIndeksi = 0;
uint8_t bensaTaulukko[bensaTaulukkoKOKO] = { 0 };

double nopeusSumma = 0;
double rpmSumma = 0;

void setup()
{
	Serial.begin(57600);
	Serial2.begin(9600);

	alkuarvojenLahetys();

	//Jos rajoitus kytkin on p��ll�, kun autoon kytket��n virrat, niin laitetaan rajoitus p��lle.
	pinMode(rajoituksenKytkentaPin, INPUT);
	if (digitalRead(rajoituksenKytkentaPin) == HIGH)
	{
		rajoitus = true;
	}

	//bitSet(rekisteri, bitti);
	//PWM
	pinMode(ylosVaihtoKaskyPin, OUTPUT); //Vaiteen vaihto k�sky yl�s
	pinMode(alasVaihtoKaskyPin, OUTPUT); //Vaiteen vaihto alas yl�s
	pinMode(rajoitusPWMpin, OUTPUT); //Nopeusrajoitus
	pinMode(vilkkuOikeaPWMpin, OUTPUT); //Ulostulo oikelle vilkkureleelle
	pinMode(vilkkuVasenPWMpin, OUTPUT); //Ulostulo vasemmalle vilkkureleelle
	analogWrite(ylosVaihtoKaskyPin, 1);
	analogWrite(alasVaihtoKaskyPin, 1);
	analogWrite(rajoitusPWMpin, 1);
	analogWrite(vilkkuOikeaPWMpin, 1);
	analogWrite(vilkkuVasenPWMpin, 1);

	
	//Taajuudet
	//kello3 30Hz PIN5
	TCCR3B = (TCCR3B & B11111000) | B00000101;
	OCR3A = 0;
	//TIMSK3 |= (1 << TOIE3);

	//kello4 0,8Hz A PIN6 ja B PIN7
	TCCR4B = (TCCR4B & B11100000) | B00011101;
	TCCR4A = (TCCR4A & B11111100) | B00000010;
	ICR4 = vilkkuNopeus;
	OCR4A = 0;
	OCR4B = 0;
	//TIMSK4 |= (1 << TOIE4);
	

	//kello2 60kHz B PIN9  ja A PIN10
	//Fast PWM, nollaa OCR, asetaa pojalla.  0b10100011;
	TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	/*
	TCCR2A = 0; //TODO p��t� joskus mit� haluat k�ytt��.
	bitSet(TCCR2A, COM2A1);
	bitSet(TCCR2A, COM2B1);
	bitSet(TCCR2A, WGM21);
	bitSet(TCCR2A, WGM20);
	*/
	// Esijakaja 1.  0b00000001;
	TCCR2B = (1 << CS20); 
	//bitSet(TCCR2B, CS20);
	OCR2A = 125; 
	OCR2B = 12;


	//kello1 aikakeskeytykset
	TCCR1A = 0;     // set entire TCCR1A register to 0
	TCCR1B = 0;     // same for TCCR1B
	TCNT1 = 0;
	// set compare match register to desired timer count:
	OCR1A = 16; //kierros ja muut softalaskurit, 1ms
	// CTC kattona maksimi
	TCCR1B |= (1 << WGM12);
	// Set CS10 and CS12 bits for 1024 prescaler:
	TCCR1B |= (1 << CS10) | (1 << CS12);
	//Aika keskeytykset p��lle. A, B ja C-kanava
	TIMSK1 |= (1 << OCIE1A);

	//kello5 ulkoinen laskuri
	TCCR5A = 0;
	TCCR5B = (1 << CS52) | (1 << CS51) | (1 << CS50); //0b00000111
	TCCR5C = 0;


	//Ulkoiset interuptit
	//attachInterrupt(digitalPinToInterrupt(lahetaPin), laheta, RISING);
	//attachInterrupt(digitalPinToInterrupt(vastaanotaPin), vastaanota, FALLING);
	//attachInterrupt(digitalPinToInterrupt(virratPoisPin), kirjoitaROM, FALLING);
	pinMode(vaihtoKytkinAlasPin, INPUT_PULLUP); //Vaihde yl�s kytkin
	pinMode(vaihtoKytkinYlosPin, INPUT_PULLUP); //Vaihde alas kytkin
	pinMode(jarruKytkinPin, INPUT_PULLUP); //Jaaruvalon kytkin
	pinMode(vilkkuOikeaKytkinPin, INPUT_PULLUP); //Vilkku oikealle kytkin
	pinMode(vilkkuVasenKytkinPin, INPUT_PULLUP); //Villku vasemmalle kytkin

	attachInterrupt(digitalPinToInterrupt(rajoituksenKytkentaPin), rajoitinKytkinInterrupt, CHANGE); //TODO Siirr� pinchange pinniin

	//Vaihteet INPUT PULLUP
	DDRC = 0b000000;
	PORTC = 0b11111;
	/*
	pinMode(vapaaPin, INPUT_PULLUP); //N
	pinMode(ykkonenPin, INPUT_PULLUP); //1
	pinMode(kakkonenPin, INPUT_PULLUP); //2
	pinMode(kolmonenPin, INPUT_PULLUP); //3
	pinMode(pakkiPin, INPUT_PULLUP); //R
	*/

	pinMode(jarruvaloPin, OUTPUT); //Ulostulo jarruvalon releelle
	pinMode(servoajuriKytkentaPin, OUTPUT); //Servon kytkent�

	DIDR0 = 0b11; //Anologi p��lle A0, A1

	lueROM();

	bensaTutkinta();
	matka = 324.3;
	trippi = 1121.2;

}

void loop()
{
	//TODO Mieti tekisk� kolme eri taajuista aikakeskeytyst� joille jakais fiksusti tutkittavat asiat.
	//Aikakeskeytys liputtaa.
	if (nopeudenLaskentaAikaLaskuri >= nopeudenLaskentaAika)
	{
		nopeudenLaskentaAikaLaskuri = 0;
		nopeusLaskuri();
		valot();
		//vaihde = vaihteet[(PIOC->PIO_PDSR ^ B11111) >> 1]; //Due
		vaihde = vaihteet[(PINC ^ B11111) >> 1]; //Mega
	}
	//Aikakeskeytus liputtaa.
	if (laskeKierrokset == true)
	{
		laskeKierrokset = false;
		rpmLaskuri();
	}

	if (bensaMittausAikaLaskuri >= bensaMittausAika)
	{
		bensaMittausAikaLaskuri = 0;
		bensaTutkinta();
	}

	//Jos nopeusrajoitus on p��ll� rajoitetaan nopeutta.
	if ((rajoitus == true && nopeus > nopeusRajoitus) || (vaihde == 'R'))
	{
		if (vaihde == 'R') //Aina jos ollaan pakilla rajoitetetaan nopeutta
		{
			if (nopeus > pakkiRajoitus)
			{
				OCR3A = rajoitusAika;
			}
			else
			{
				OCR3A = 0; //5 PWM duty %
			}
		}
		else
		{
			OCR3A = rajoitusAika; //5 PWM duty %
		}

		//analogWrite(rajoitusPWM, rajoitusAika);
	}
	else
	{
		OCR3A = 0; //5 PWM duty %
				   //analogWrite(rajoitusPWM, 0);
	}

	//Tutkitaan onko vaihtokytkin ollut tarpeeksi kauan ylh��ll�
	if (millis() - vaihtoNappiVanhaAika > vaihtoNappiAika)
	{
		vaihtoNappiYlhaalla = true;
	}

	//Tutkitaan kuinka kauan on vaihteenvaihtomoottorille annettu PWMm��.
	if (millis() - vaihdetaanVanhaAika > vaihtoAika)
	{
		//analogWrite(alasVaihtoKaskyPin, 0);
		//analogWrite(ylosVaihtoKaskyPin, 0);
		OCR4A = 0; //PIN6 PWM duty %
		OCR4B = 0; //PIN7 PWM duty %
		digitalWrite(servoajuriKytkentaPin, LOW);
		kuittaus = true;
	}

	vaihdeKytkinAlas = digitalRead(vaihtoKytkinAlasPin);
	vaihdeKytkinYlos = digitalRead(vaihtoKytkinYlosPin);
	//Vaihdetaan bitwise tutkintaan, koska ne on nopeampia.
	//vaihdeKytkinAlas = PINB ^ B1; //
	//vaihdeKytkinYlos = (PINB ^ B10) >> 1; //


	//Vaihteen vaihtoon menn��, jos jompaa kumpaa vaihto nappia on painettu .
	if (vaihdeKytkinAlas == LOW || vaihdeKytkinYlos == LOW)//Aika rajoitus
	{

		vaihtoNappiVanhaAika = millis();

		if (kuittaus == true && vaihtoNappiYlhaalla == true)
		{
			vaihtoNappiYlhaalla = false;

			//TODO  Mieti vaihteen vaihto rajoitukset kunnolla. Nopeuden ja vaihteen suhteen tai joitan.
			if (rpm < vaihtoRpm) //P��st��n vaihtamaan jos moottorin kierrosnopeus on tietyn rajan alle.
			{
				kuittaus = false; //Kirjotetaan vaihteen vaihto kuittas ep�todeksi, koska ruvetaan vaihtamaan vaihdetta.
				bitWrite(boolLahetysTavu, 1, 0);
				switch (vaihde)
				{
				case 'R': //Pakilta annetaan vaihtaa vain yl�s p�in.
					if (vaihdeKytkinYlos == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case 'N': //Vappaalta voi vaihtaa kumpaankin suuntaa, mutta jarru pit�� olla painettuna.

					if (vaihdeKytkinAlas == LOW && jarruvalo == LOW)
					{
						vaihtoKasky(alasVaihtoKaskyPin);
					}
					else if (vaihdeKytkinYlos == LOW && jarruvalo == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case '1': //Ykk�selt� voi vaihtaa kumpaankin suuntaa.
					if (vaihdeKytkinAlas == LOW)
					{
						vaihtoKasky(alasVaihtoKaskyPin);
					}
					else if (vaihdeKytkinYlos == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case '2': //Kakkoselta voi vaihtaa kumpaankin suuntaa.
					if (vaihdeKytkinAlas == LOW)
					{
						vaihtoKasky(alasVaihtoKaskyPin);
					}
					else if (vaihdeKytkinYlos == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case '3': //Kolmeoselta voi vaihtaa vain alas p�in.
					if (vaihdeKytkinAlas == LOW)
					{
						vaihtoKasky(alasVaihtoKaskyPin);
					}
				default:
					break;
				}
			}
			else
			{
				bitWrite(boolLahetysTavu, 1, 1); //Kirjoitettaan l�hetystavuun, ett� liikaa kierroksia vaihteen vaihtoon.
												 //Kaasu yl�s!!
			}
		}

		vaihtoNappiYlhaalla = false;
	}

}

//Aikakeskeytys noin 1ms
ISR(TIMER1_COMPA_vect)
{
	rpmADC();
	nopeudenLaskentaAikaLaskuri++;
	bensaMittausAikaLaskuri++;

	//Serial.println(TCNT5);
}

ISR(ADC_vect)
{
	rpmMuunnnos = ADC;
	laskeKierrokset = true;
}

char sarjaVali;
void serialEvent2()
{
	sarjaVali = Serial2.read();
	if (sarjaVali == 'L')
	{
		laheta();
	}
	else if (sarjaVali == 'V')
	{
		Serial.println("Vastaant ota");
		vastaanota();
	}
	else if (sarjaVali == 'a')
	{
		alkuarvojenLahetys();
	}
}

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois p��lt� poistetaan rajoitus.*/
void rajoitinKytkinInterrupt()
{
	/*
	if (rajoitus == false)
	{
	rajoitus = true;
	}
	else
	{
	rajoitus = false;
	}
	*/

	if (digitalRead(rajoituksenKytkentaPin) == HIGH)
	{
		rajoitus = true;
		bitWrite(boolLahetysTavu, 0, 1);
	}
	else
	{
		rajoitus = false;
		bitWrite(boolLahetysTavu, 0, 0);
	}
	/*
	Serial.print("Onko rajoitus paalla: ");
	Serial.println(rajoitus, DEC);
	*/
}

/*Nopeuden lasku fuktio
Laskee globaaliin muuttajaan "nopeus" auton nopeuden kilometrein� tunnissa. */
void nopeusLaskuri()
{
	nopeusPulssit = TCNT5;

	double matkaVali = ((float)nopeusPulssit / (float)pulssitPerKierrosNopeus) * pii * pyoraHalkaisija; //Metrej�
	double valiAika = (millis() - nopeusPulssitAika) / 1000.0;

	matka = matka + (matkaVali / 1000); //Kilometrej�
	trippi = trippi + matkaVali; //Metrej�

								 /*
								 Serial.print("pulssit: ");
								 Serial.println(nopeusPulssit, DEC);
								 Serial.print("aika: ");
								 Serial.println(millis() - nopeusPulssitAika, DEC);
								 Serial.print("Vali: ");
								 Serial.println(round(matkaVali), DEC);
								 Serial.print("Nopeus: ");
								 Serial.println((round(matkaVali / valiAika)* 3.6), DEC);
								 */

	if (nopeusIndeksi == nopeusTaulukkoKOKO)//Indeksin ymp�ripy�ritys
	{
		nopeusIndeksi = 0;
	}
	nopeusSumma = nopeusSumma - nopeusTaulukko[nopeusIndeksi];

	//Lasketaan auton nopeus (km/h). v=n*d*pii	n=1/t

	nopeusTaulukko[nopeusIndeksi] = round(matkaVali / valiAika * 3.6);

	nopeusPulssitAika = millis(); //Laitetetaan aika muistiin.
	TCNT5 = 0; //Nollataan pulssi laskuri.

	nopeusSumma = nopeusSumma + nopeusTaulukko[nopeusIndeksi];

	/*
	for (int i = 0; i < nopeusTaulukkoKOKO; i++)//Keskiarvonlaskeminen taulukosta
	{
	nopeusSumma = nopeusSumma + nopeusTaulukko[i];
	}
	*/

	nopeus = round(nopeusSumma / (float)nopeusTaulukkoKOKO);

	nopeusIndeksi++;
}

//Kierrosnopeus lasku funktio
//Laskee globaaliin muuttujaan "RPM" moottorin kierrosnopeuden (1/min).
void rpmLaskuri()
{
	if (kierrosIndeksi == kierrosTaulukkoKOKO)
	{
		kierrosIndeksi = 0;
	}

	rpmSumma = rpmSumma - kierrosLuku[kierrosIndeksi];

	//Sijoitetaan arvot taulukkoon, josta lasketaan keskiarvo
	//Lasketaan kierrosnopeus (1/min). n=1/t
	kierrosLuku[kierrosIndeksi] = rpmMuunnnos;
	
	rpmSumma = rpmSumma + kierrosLuku[kierrosIndeksi];

	//TODO tee oikea muunnos
	rpm = round((rpmSumma / float(kierrosTaulukkoKOKO)) * 10);
	//Serial.println(rpm);
	kierrosIndeksi++;
}

/*Vaihteen vaihto k�sky fuktio. (Huom toimii nyt vain megalla pinneill� 6 ja 7, taajuuksia vaihdettu)
Funktioon sy�tet��n pinni, mit� halutaan k�ytt�� ylh��ll�. Eli pinni� mik� vastaa suuntaa mihin haluttan vaihtaa.
Vaihteen vaihtaja suorittaa yl�s- tai alasvaihto funktion riipuen siit� kumpaan pinniin saa k�skyn.*/

void vaihtoKasky(int kaskyPin)
{
	//analogWrite(kaskyPin, 250);

	digitalWrite(servoajuriKytkentaPin, HIGH);
	if (kaskyPin == 6)
	{
		//OCR4A = 195; //6 PWM duty %, 98%=195
		OCR2A = 250;
	}
	else
	{
		//OCR4B = 195; //7 PWM duty %, 98%=195
		OCR2B = 250;
	}
	vaihdetaanVanhaAika = millis();
}

void laheta()
{
	/*
	char rajoitusPaalla;
	if (rajoitus == false)
	{
	//rajoitusPaalla = 'E';
	}
	else
	{
	rajoitusPaalla = 'K';
	}
	*/
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

void vastaanota()
{
	char otsikko;
	int param;
	while (Serial2.available() == 0){} //Ootellaan ett� saadaan tavua
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
		Serial.print(" otsikko: ");
		Serial.print(otsikko);
		Serial.print(" data: ");
		Serial.println(param, DEC);
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
	/*
	Serial.print("Luku rom");
	Serial.print("\n");
	Serial.println(nopeusRajoitus, DEC);
	*/
}

/* Kirjoitettaan tarvittavat tiedot EEPROMiin talteen, kun virrat katkaistaan.
Tutki millainen konkka tarvitaan pit�m��n virrat p��ll� tarpeekksi kauvan.
*/
void kirjoitaROM()
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

	/*
	Serial.print("kirjoita rom");
	Serial.print("\n");
	Serial.println(nopeusRajoitus, DEC);
	*/
	//Otetaan ett� virrat katkee
	while (true)
	{
	}
}

void valot()
{
	//TODO v��r�t rekisterit
	//Vilkku oikea
	if (digitalRead(vilkkuOikeaKytkinPin) == LOW)
	{
		OCR5C = vilkkuNopeus / 2; //PIN44 PWM duty %
	}
	else
	{
		OCR5C = 0; //PIN44 PWM duty %
	}
	//Vilkku vasen
	if (digitalRead(vilkkuVasenKytkinPin) == LOW)
	{
		OCR5B = vilkkuNopeus / 2; //PIN45 PWM duty %
	}
	else
	{
		OCR5B = 0; //PIN44 PWM duty %
	}
	//Jarruvalo
	jarruvalo = digitalRead(jarruKytkinPin);
	if (jarruvalo == LOW)
	{
		digitalWrite(jarruvaloPin, HIGH);
		bitWrite(boolLahetysTavu, 2, 1); ///Kirjoitetaan l�hetystavuun, ett� jarru on painettuna.
	}
	else
	{
		digitalWrite(jarruvaloPin, LOW);
		bitWrite(boolLahetysTavu, 2, 0); ///Kirjoitetaan l�hetystavuun, ett� jarru on ylh��ll�.
	}
}

void bensaTutkinta()
{
	bensa = map(ADRead(bensaSensoriPin), 0, 1023, 0, bensapalkkiKorkeus);
	
	/*
	if (nopeus == 0)
	{
		bensa = bensaVali;
	}
	else
	{
		if (bensaVali > bensa + 1)
		{

		}
	}
	*/
}

void alkuarvojenLahetys()
{
	char kuittaus = ' ';

	while (kuittaus != 'O')
	{
		Serial2.write('A');
		Serial2.write(minRajoitus);
		Serial2.write(maxRajoitus);
		Serial2.write(uint8_t(punaraja / 100));
		Serial2.write(maxNayttoKierrokset);

		while (Serial2.available() == 0){}

		kuittaus = Serial2.read();
	}

}

void rpmADC()
{
	ADMUX = (0b01000000 | 1); //5V pin referenssin� ja luku 1
	ADCSRA = 0b11001110; //Ennabloi AD, ajoon ja interupti. Prescailer 64	
}

int ADRead(uint8_t pin)
{
	ADMUX = (0b01000000 | pin); //5V pin referenssin�
	ADCSRA = 0b11000110; //Ennabloi AD ja laita p��lle. Ei intteruptia, scaler 64
	while ((ADCSRA & 0b01000000) != 0) {} // Ootetaan muunnos loppuun
	return ADC; //Palautetaan muunnettu luku
}