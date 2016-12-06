/*Mikroauton pääojelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen ylös tai alas.

*/

#include <Arduino.h>
#include <EEPROM.h>

//Pinnien paikat
//interruptit
//kunnon
#define virratPoisPin 21 //PD0
#define vaihtoKytkinAlasPin 20 //Ajajan vaihtokytkin alas, PD1
#define vaihtoKytkinYlosPin 19 //Ajajan vaihtokytkin ylös, PD2
#define vilkkuOikeaKytkinPin 18 //PD3
#define vilkkuVasenKytkinPin 2 //PE4
//tilanvaihto
#define rajoituksenKytkentaPin 53 //Kierrostenrajoittimen avainkytkimen pinni, PB0
#define jarruKytkinPin 15 //Jarrukytkin, PJ0

//PWM-pinnit (HUOM! Taajuuksia on vaihdettu, joten PWMmää on ohjattu suoraan rekisteriä manipuloimalla.)
#define alasVaihtoKaskyPin 10 //Käskys moottorille vaihtaa alas. kello2 OC2A, PB4
#define ylosVaihtoKaskyPin 9 //Käskys moottorille vaihtaa ylös. kello2 OC2B, PH6
#define rajoitusPWMpin 5 //kello3 duty=OCR3A, PE3
#define vilkkuOikeaPWMpin 6 //kello4 duty=OCR4A, PH3
#define vilkkuVasenPWMpin 7 //kello4 duty=OCR4B, PH4

//ADC
#define bensaSensoriPin 0 //Bensa sensori potikka (A0)
//Digi output
#define servoajuriKytkentaPin 8 //Servon kytkentä pinni, PH5
#define jarruvaloPin 46 //PL3 

						 //Vakioita
const double pii = 3.14159; //Pii
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
const float pyoraHalkaisija = 0.3; //Pyörän halkaisija metreinä.
const uint16_t vaihtoRpm = 1800; //Moottorin kierrosnopeus, jonka alle ollessa saa vaihtaa vaihteen (1/min).
const uint16_t pulssitPerKierrosNopeus = 20; //Montako pulssia tulee per kierros (nopeus).
const uint16_t vaihtoNappiAika = 2000; //Aika minkä vaihto napin on oltava ylhäällä että voidaan uudestaa koittaa vaihtaan vaihetta.
const uint16_t vaihtoAika = 1000; //Vaihtopulssin kesto
const uint16_t nopeudenLaskentaAika = 100;
const uint16_t bensaMittausAika = 3000; //3s
const uint16_t rajoitusAika = 50;
const uint16_t vilkkuNopeus = 20000; //Alustavasti hyvä taajuus
const uint16_t vikkumisAika = 3000; //3s
const uint16_t pakkiRajoitus = 5; //Pakin nopeusrajoitus
const uint8_t minRajoitus = 5;
const uint8_t maxRajoitus = 30;
const uint8_t maxNayttoKierrokset = 11; //*1000
const uint16_t punaraja = 8000;
const uint8_t bensapalkkiKorkeus = 120;

const uint8_t vaihtoRaja1_2 = 10; //kh/h
const uint8_t vaihtoRaja2_3 = 20; //kh/h
const uint8_t vaihtoRaja3_2 = 50; //kh/h
const uint8_t vaihtoRaja2_1 = 30; //kh/h

//Globaalit muuttujat
volatile uint16_t nopeusPulssit = 0; //Muuttuja johon lasketaan pyörältä tulleet pulssit.
volatile uint16_t rpmMuunnnos = 0;
bool laskeNopeus = true;
bool laskeKierrokset = true;
bool laskeBensa = true;

bool vaihtaaVaihdetta = false; //Vaihteen vaihtajan kuittas muuttuja
bool rajoitusPaalla = false; //Kierrosten rajoittimen muuttuja
bool vaihtoNappiYlhaalla = true; //
bool liikaaKierroksia = false;
bool vilkutaOikea = false;
bool vilkutaVasen = false;
unsigned long vaihdetaanVanhaAika = 0;
volatile unsigned long vaihtoNappiVanhaAika = 0; //
unsigned long nopeusPulssitAika = 0; //Aika jolloin aloitettiin laskemaan nopeus pulsseja.
unsigned long vilkkuAika = 0;
volatile uint16_t nopeudenLaskentaAikaLaskuri = 0;
volatile uint16_t bensaMittausAikaLaskuri = 0;
bool vaihdeKytkinYlos = false; //Vaihteen vaihto kytkin ylös tila
bool vaihdeKytkinAlas = false; //Vaihteen vaihto kytkin alas tila
uint8_t nopeus = 0; //Auton nopeus (km/h)
uint16_t rpm = 0; //Moottorin kierrosnopeus (1/min)
uint8_t nopeusRajoitus = 20;
uint8_t bensa = 0;
double matka = 0;
double trippi = 0;
char vaihde = 'N'; //Päällä oleva vaihde
bool jarruPohjassa = false;

/*
B00000001 = Rajoitus päällä/pois
B00000010 = Liikaa kierroksia vaihtoon
B00000100 = Jarru pohjassa
B00001000 =
B00010000 =
B00100000 =
B01000000 =
B10000000 =							*/
uint8_t boolLahetysTavu = 0;

uint16_t kierrosLuku[kierrosTaulukkoKOKO] = { 0 }; //Taulukko, johon tallennetaan kierroslukuja, että voidaan laskea keskiavaja.
uint8_t kierrosIndeksi = 0;
uint8_t nopeusTaulukko[nopeusTaulukkoKOKO] = { 0 };
uint8_t nopeusIndeksi = 0;
uint8_t bensaTaulukko[bensaTaulukkoKOKO] = { 0 };
uint8_t bensaIndeksi = 0;

double nopeusSumma = 0;
double rpmSumma = 0;
double bensaSumma = 0;

void setup()
{
	Serial.begin(57600);
	Serial2.begin(115200);

	alkuarvojenLahetys();

	//bitSet(rekisteri, bitti);
	//PWM
	//TODO katon nämä joskus vähän siistimmiksi.
	pinMode(ylosVaihtoKaskyPin, OUTPUT); //Vaiteen vaihto käsky ylös
	pinMode(alasVaihtoKaskyPin, OUTPUT); //Vaiteen vaihto alas ylös
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
	TCCR2A = 0; //TODO päätä joskus mitä haluat käyttää.
	bitSet(TCCR2A, COM2A1);
	bitSet(TCCR2A, COM2B1);
	bitSet(TCCR2A, WGM21);
	bitSet(TCCR2A, WGM20);
	*/
	// Esijakaja 1.  0b00000001;
	TCCR2B = (1 << CS20); 
	//bitSet(TCCR2B, CS20);
	OCR2A = 0; 
	OCR2B = 0;

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
	//Aika keskeytykset päälle. A-Kanava
	TIMSK1 |= (1 << OCIE1A);

	//kello5 ulkoinen laskuri
	TCCR5A = 0;
	TCCR5B = (1 << CS52) | (1 << CS51) | (1 << CS50); //0b00000111
	TCCR5C = 0;

	//Ulkoiset interuptit
	pinMode(virratPoisPin, INPUT);
	pinMode(vaihtoKytkinAlasPin, INPUT_PULLUP); //Vaihde alas kytkin
	pinMode(vaihtoKytkinYlosPin, INPUT_PULLUP); //Vaihde ylos kytkin
	pinMode(vilkkuOikeaKytkinPin, INPUT_PULLUP); //Vilkku oikealle kytkin
	pinMode(vilkkuVasenKytkinPin, INPUT_PULLUP); //Villku vasemmalle kytkin
	EICRA = 0b11111110;
	EICRB = 0b11;
	//EIMSK = 0b00011111;
	EIMSK = 0b00011110; //ROMin kirjotus poissa

	pinMode(jarruKytkinPin, INPUT_PULLUP); //Jaaruvalon kytkin vaihto interuptiks
	pinMode(rajoituksenKytkentaPin, INPUT_PULLUP);
	PCICR = 0b011; //Tilan muutosinterupt
	PCMSK0 = 0b1; //Arduino PIN53
	PCMSK1 = 0b10; //Arduino PIN15

	//Vaihteet INPUT PULLUP
	DDRC = 0;
	PORTC = 0xFF;

	pinMode(jarruvaloPin, OUTPUT); //Ulostulo jarruvalon releelle
	pinMode(servoajuriKytkentaPin, OUTPUT); //Servon kytkentä

	DIDR0 = 0b11; //Anologi päälle A0, A1

	rajoitusPaalla = (PINB & 1); //Mikä on rajoituskykimen tila
	bitWrite(boolLahetysTavu, 0, rajoitusPaalla);

	lueROM();

	//Täytetään bensa taulukko
	for (uint8_t i = 0; i < bensaTaulukkoKOKO; i++)
	{
		bensaTutkinta();
	}

	matka = 32413.3;
	trippi = 113121.2;
}

void loop()
{
	//Aikakeskeytys liputtaa.
	if (laskeKierrokset == true)
	{
		laskeKierrokset = false;
		rpmLaskuri();
	}
	if (nopeudenLaskentaAikaLaskuri >= nopeudenLaskentaAika)
	{
		nopeudenLaskentaAikaLaskuri = 0;
		nopeusLaskuri();
		valot();
		
		vaihde = vaihteet[(PINC ^ 0xFF) >> 1]; //Mega
	}
	if (bensaMittausAikaLaskuri >= bensaMittausAika)
	{
		bensaMittausAikaLaskuri = 0;
		bensaTutkinta();
	}

	//Jos nopeusrajoitus on päällä rajoitetaan nopeutta.
	rajoitus();

	if (vaihtaaVaihdetta == true)
	{
		//Tutkitaan kuinka kauan on vaihteenvaihtomoottorille annettu PWMmää.
		if (millis() - vaihdetaanVanhaAika > vaihtoAika)
		{
			OCR2A = 0; //PIN10, PB4
			OCR2B = 0; //PIN9, PH6
			PORTH = PINH ^ (1 << PH5); //PIN8
			vaihtaaVaihdetta = false;
		}
	}

	//Vaihteen vaihtoon mennää, jos jompaa kumpaa vaihto nappia on painettu .
	if (vaihdeKytkinAlas == true || vaihdeKytkinYlos == true)//Aika rajoitus
	{
		//Tutkitaan vaihdetaanko vaihdetta ja onko vaihtokytkin ollut tarpeeksi kauan ylhäällä.
		if ((vaihtaaVaihdetta == false) && ((millis() - vaihtoNappiVanhaAika) > vaihtoNappiAika))
		{
			//TODO  Mieti vaihteen vaihto rajoitukset kunnolla. Nopeuden ja vaihteen suhteen tai joitan.
			//Mietin yhden kerran ja tulin siihen tuylokseen, että ei vättämättä tarvii.
			//Kovassa vauhdissa liian pienelle vahtaminen on varmaan suurin ongelma ja se nyt ei ole kovin vaarallista.
			if (rpm < vaihtoRpm) //Päästään vaihtamaan jos moottorin kierrosnopeus on tietyn rajan alle.
			{
				vaihtaaVaihdetta = true;
				bitWrite(boolLahetysTavu, 1, 0);

				switch (vaihde)
				{
					case 'R': //Pakilta annetaan vaihtaa vain ylös päin.
						if (vaihdeKytkinYlos == true)
						{
							vaihtoKasky(ylosVaihtoKaskyPin);
						}
						break;
					case 'N': //Vappaalta voi vaihtaa kumpaankin suuntaa, mutta jarru pitää olla painettuna.

						if (vaihdeKytkinAlas == true && jarruPohjassa == true)
						{
							vaihtoKasky(alasVaihtoKaskyPin);
						}
						else if (vaihdeKytkinYlos == true && jarruPohjassa == true)
						{
							vaihtoKasky(ylosVaihtoKaskyPin);
						}
						break;
					case '1': //Ykköseltä voi vaihtaa kumpaankin suuntaa.
						if (vaihdeKytkinAlas == true)
						{
							vaihtoKasky(alasVaihtoKaskyPin);
						}
						else if (vaihdeKytkinYlos == true)
						{
							vaihtoKasky(ylosVaihtoKaskyPin);
						}
						break;
					case '2': //Kakkoselta voi vaihtaa kumpaankin suuntaa.
						if (vaihdeKytkinAlas == true)
						{
							vaihtoKasky(alasVaihtoKaskyPin);
						}
						else if (vaihdeKytkinYlos == true)
						{
							vaihtoKasky(ylosVaihtoKaskyPin);
						}
						break;
					case '3': //Kolmeoselta voi vaihtaa vain alas päin.
						if (vaihdeKytkinAlas == true)
						{
							vaihtoKasky(alasVaihtoKaskyPin);
						}
						break;
					default:
						break;
				}
				vaihdeKytkinAlas == false;
				vaihdeKytkinYlos == false;
			}
			else
			{
				bitWrite(boolLahetysTavu, 1, 1); //Kirjoitettaan lähetystavuun, että liikaa kierroksia vaihteen vaihtoon.
												 //Kaasu ylös!!
			}
		}
	}
}

//Aikakeskeytys noin 1ms
ISR(TIMER1_COMPA_vect)
{
	rpmADC();
	nopeudenLaskentaAikaLaskuri++;
	bensaMittausAikaLaskuri++;
}

//Kierros ADC
ISR(ADC_vect)
{
	rpmMuunnnos = ADC;
	laskeKierrokset = true;
}

/* Kirjoitettaan tarvittavat tiedot EEPROMiin talteen, kun virrat katkaistaan.
Tutki millainen konkka tarvitaan pitämään virrat päällä tarpeekksi kauvan. */
ISR(INT0_vect)
{
	cli();
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
	//Otetaan että virrat katkee
	while (true)
	{
	}
}

//Vaihto alas
ISR(INT1_vect)
{
	vaihdeKytkinAlas = true;
	vaihtoNappiVanhaAika = millis();
}

//Vaihto ylös
ISR(INT2_vect)
{
	vaihdeKytkinYlos = true;
	vaihtoNappiVanhaAika = millis();
}

//Vilkku oikea
ISR(INT3_vect)
{
	vilkutaOikea = true;
}

//Vilkku vasen
ISR(INT4_vect)
{
	vilkutaVasen = true;
}

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois päältä poistetaan rajoitus.*/
ISR(PCINT0_vect)
{
	cli();
	if ((PINB & 1) == 1) //rajoituksenKytkentaPin
	{
		rajoitusPaalla = true;
		bitWrite(boolLahetysTavu, 0, 1);
	}
	else
	{
		rajoitusPaalla = false;
		bitWrite(boolLahetysTavu, 0, 0);
	}
	sei();
}

//Jarru
ISR(PCINT1_vect)
{
	//jarruvalo = (PINJ & (1 << PJ0)) >> PJ0;
	jarruPohjassa = PINJ & 1;
	PORTL = PINL | (0xFF & (jarruPohjassa << PL3));
	bitWrite(boolLahetysTavu, 2, jarruPohjassa); ///Kirjoitetaan lähetystavuun, että jarrun tila.
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
		//Serial.println("Vastaant ota");
		vastaanota();
	}
	else if (sarjaVali == 'a')
	{
		alkuarvojenLahetys();
	}
}

/*Nopeuden lasku fuktio
Laskee globaaliin muuttajaan "nopeus" auton nopeuden kilometreinä tunnissa. */
void nopeusLaskuri()
{
	nopeusPulssit = TCNT5;
	TCNT5 = 0; //Nollataan pulssi laskuri.
	double matkaVali = ((float)nopeusPulssit / (float)pulssitPerKierrosNopeus) * pii * pyoraHalkaisija; //Metrejä
	double valiAika = (millis() - nopeusPulssitAika) / 1000.0; //Sekunteja
	nopeusPulssitAika = millis(); //Laitetetaan aika muistiin.

	matka = matka + (matkaVali / 1000.0); //Kilometrejä
	trippi = trippi + matkaVali; //Metrejä

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

	if (nopeusIndeksi == nopeusTaulukkoKOKO)//Indeksin ympäripyöritys
	{
		nopeusIndeksi = 0;
	}
	nopeusSumma = nopeusSumma - nopeusTaulukko[nopeusIndeksi];

	//Lasketaan auton nopeus (km/h). v=n*d*pii	n=1/t

	nopeusTaulukko[nopeusIndeksi] = round((matkaVali / valiAika) * 3.6);

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
//Laskee globaaliin muuttujaan "rpm" moottorin kierrosnopeuden (1/min).
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

void vaihtoKasky(uint8_t kaskyPin)
{
	PORTH = PINH | (1 << PH5); //PIN8
	if (kaskyPin == 10)
	{
		OCR2A = 250; //PIN10, PB4
	}
	else if (kaskyPin == 9)
	{
		OCR2B = 250; //PIN9, PH6
	}
	vaihdetaanVanhaAika = millis();
}

void laheta()
{
	//Serial.println("laheta");
	cli();//stop interrupts
	int matkaVali = round(matka);
	int trippiVali = round(trippi / 100) * 100; //Pyöristetään satojen tarkkuuteeen

	Serial2.write('A');
	Serial2.write(boolLahetysTavu);//Kyllä/ei tietojen lähetys.
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
	while (Serial2.available() == 0){} //Ootellaan että saadaan tavua
	while (Serial2.available() > 0)
	{
		otsikko = Serial2.read();
		switch (otsikko)
		{
		case 'R':
			while (Serial2.available() == 0) {} //Ootellaan että saadaan tavua
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
		/*
		Serial.print(" otsikko: ");
		Serial.print(otsikko);
		Serial.print(" data: ");
		Serial.println(param, DEC);
		*/
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

void valot()
{
	if (vilkutaOikea == true || vilkutaVasen == true)
	{
		if (millis() - vilkkuAika > vikkumisAika)
		{
			vilkutaOikea = false;
			vilkutaVasen = false;
		}

		if (vilkutaOikea == true)
		{
			OCR4A = vilkkuNopeus / 2; //PIN6, OCR4A, PH3
			vilkkuAika = millis();
		}
		else
		{
			OCR4A = 0;
		}

		if (vilkutaVasen == true)
		{
			OCR4B = vilkkuNopeus / 2; //PIN7, OCR4B, PH4
			vilkkuAika = millis();
		}
		else
		{
			OCR4B = 0;
		}
	}
}

void bensaTutkinta()
{
	if (bensaIndeksi == bensaTaulukkoKOKO)
	{
		bensaIndeksi = 0;
	}

	bensaSumma = bensaSumma - bensaTaulukko[bensaIndeksi];

	bensaTaulukko[bensaIndeksi] = map(ADRead(bensaSensoriPin), 0, 1023, 0, bensapalkkiKorkeus);

	bensaSumma = bensaSumma + bensaTaulukko[bensaIndeksi];

	bensa = round((bensaSumma / float(bensaTaulukkoKOKO)));
	bensaIndeksi++;
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
	ADMUX = (0b01000000 | 1); //5V pin referenssinä ja luku 1
	ADCSRA = 0b11001110; //Ennabloi AD, ajoon ja interupti. Prescailer 64	
}

int ADRead(uint8_t pin)
{
	ADMUX = (0b01000000 | pin); //5V pin referenssinä
	ADCSRA = 0b11000110; //Ennabloi AD ja laita päälle. Ei intteruptia, scaler 64
	while ((ADCSRA & 0b01000000) != 0) {} // Ootetaan muunnos loppuun
	return ADC; //Palautetaan muunnettu luku
}

void rajoitus()
{
	if ((rajoitusPaalla == true) || (vaihde == 'R'))
	{
		if ((nopeus > nopeusRajoitus) && (nopeus > pakkiRajoitus))
		{
			OCR3A = rajoitusAika; //PIN5, PE3
		}
		else
		{
			OCR3A = 0; //PIN5, PE3
		}
	}
	else
	{
		OCR3A = 0; //PIN5, PE3
	}

}