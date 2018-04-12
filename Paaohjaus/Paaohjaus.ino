/*Mikroauton pääohjelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen ylös tai alas.

*/

#include <Arduino.h>
#include <EEPROM.h>

//Pinnien paikat
//interruptit
//kunnon
#define virratPoisPin 21 //PD0
#define vaihtoKytkinYlosPin 19 //Ajajan vaihtokytkin ylös, PD2
#define vaihtoKytkinAlasPin 20 //Ajajan vaihtokytkin alas, PD1
#define vilkkuOikeaKytkinPin 18 //PD3
#define vilkkuVasenKytkinPin 2 //PE4
//tilanvaihto
#define rajoitusKytkinPin 53 //Kierrostenrajoittimen avainkytkimen pinni, PB0
#define rajoitusKytkinBitti PB0
#define rajoitusKytkinPortti PINB
#define jarruKytkinPin 15 //Jarrukytkin, PJ0
#define jarruKytkinBitti PJ0
#define jarruKytkinPortti PINJ

//Nopeus laskuri
#define nopeuspulssitPit 47 //PL2 ( T5 )

//ADC
#define bensaSensoriPin 0 //Bensa sensori potikka (A0)
#define rpmADCPin 1

//PWM-pinnit (HUOM! Taajuuksia on vaihdettu, joten PWMmää on ohjattu suoraan rekisteriä manipuloimalla.)
#define ylosVaihtoKaskyPin 11 //Käskys moottorille vaihtaa ylös. kello1 OC1A, PB5
#define alasVaihtoKaskyPin 12 //Käskys moottorille vaihtaa alas. kello1 OC1B, PB6
#define rajoitusPWMpin 5 //kello3 duty=OCR3A, PE3
#define vilkkuOikeaPWMpin 6 //kello4 duty=OCR4A, PH3
#define vilkkuVasenPWMpin 7 //kello4 duty=OCR4B, PH4
//Digi output
#define servoajuriKytkentaPin 8 //Servon kytkentä pinni, PH5
#define servoKytkenta PORTH
#define servoKytkentaBitti PH5

#define jarruvaloPin 46 //PL3 
#define jarruValokytkenta PORTL
#define jarruValokytkentaBitti PL3

//Aikakeksytys ajat
//(16 MHz : 8) kello 16 bitin laskurilla = 0,5 us 
#define lyonti (double)(0.0000005) //0,5 us
#define lyonnit_10ms (uint16_t)(0.010/lyonti) //10 ms : 0,5 us = 10 ms
#define lyonnit_1ms (uint16_t)(0.001/lyonti) //1 ms
#define rpmMittausväli 0xFFFF

//Maskeja PWM kytkentään
#define rajoitusPaalle 0b10000010
#define rajoitusPois 0b11
#define rajoitusPWM TCCR3A

#define oikeallePaalle 0b01000000
#define vasemmallePaalle 0b00010000
#define vilkkuPois 0
#define vilkkuPWM TCCR4A

#define ylosPaalle 0b10000010
#define alasPaalle 0b00100010
#define vaihtoPois 0b10
#define vaihtoPWM TCCR1A

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
const uint16_t vaihtoNappiAika = 1000; //Aika minkä vaihto napin on oltava ylhäällä että voidaan uudestaa koittaa vaihtaan vaihetta.
const uint16_t vaihtoAika = 2000; //Vaihtopulssin kesto
const uint16_t nopeudenLaskentaAika = 100;
const uint16_t bensaMittausAika = 3000; //3s
const uint16_t rajoitusAika = 400;
const uint16_t vilkkuNopeus = 10000; //Laskuri vaihtaa tilan tässä luvussa.
const uint16_t vikkumisAika = 10000; //10s
const uint16_t pakkiRajoitus = 5; //Pakin nopeusrajoitus
const uint8_t minRajoitus = 5;
const uint8_t maxRajoitus = 40;
const uint8_t maxNayttoKierrokset = 11; //*1000
const uint16_t punaraja = 8000;
const uint8_t bensapalkkiKorkeus = 120;

const uint8_t vaihtoRaja1_2 = 10; //kh/h
const uint8_t vaihtoRaja2_3 = 20; //kh/h
const uint8_t vaihtoRaja3_2 = 50; //kh/h
const uint8_t vaihtoRaja2_1 = 30; //kh/h

//Globaalit muuttujat
uint32_t kello0YlivuotoLaskuri = 0; //Kellon 32 ylintä bitti
uint32_t millisekuntit = 0;
volatile uint16_t nopeusPulssit = 0; //Muuttuja johon lasketaan pyörältä tulleet pulssit.
volatile uint16_t rpmMuunnnos = 0;
bool laskeNopeus = true;
bool laskeKierrokset = true;
bool laskeBensa = true;

bool lippu10ms = true;

//bool vaihtaaVaihdetta = false; //Vaihteen vaihtajan kuittas muuttuja
bool rajoitusPaalla = false; //Kierrosten rajoittimen muuttuja
bool vaihtoNappiYlhaalla = true; //
bool liikaaKierroksia = false;

unsigned long vaihdetaanVanhaAika = 0;
volatile unsigned long vaihtoNappiVanhaAika = 0; //
unsigned long nopeusPulssitAika = 0; //Aika jolloin aloitettiin laskemaan nopeus pulsseja.
volatile unsigned long vilkkuAika = 0;
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
#define rajoitusBitti 0
#define liikaaRPMbitti 1
#define jarruPojasssaBitti 2
uint8_t boolLahetysTavu = 0; //Tavu millä voidaan lähettää 8 bool bittiä.

uint16_t kierrosLuku[kierrosTaulukkoKOKO] = { 0 }; //Taulukko, johon tallennetaan kierroslukuja, että voidaan laskea keskiarvoja.
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

	//Kello0
	TIMSK0 = 0; //Laitetaan Arduino-kirjaston millis() pois päältä.

	//Kello1
	//Aikakeskeytys
	OCR1A = lyonnit_1ms;
	OCR1B = lyonnit_10ms;
	//OCR1C = 
	TCCR1A = 0b01010000; //Toggle A ja B, normaali laskri
	TCCR1B = 0b00000010; //Jakaja 8, katto 0xFFFF : ylivuoto 32,768 ms
	TIMSK1 = 0b00000111; //A ja B vertailu- ja ylivuotokesketys päällä

	//Kello3
	////PWM 30Hz PIN5
	pinMode(rajoitusPWMpin, OUTPUT); //Nopeusrajoitus
	//Fast PWM nolla OCR, katto ICR, asettaa pohjalla
	TCCR3A = rajoitusPois; //0b10000011 A = päällä
	TCCR3B = 0b00011101;
	ICR3 = 520;
	OCR3A = rajoitusAika;
	

	//kello4 0,8Hz Hz A PIN6 ja B PIN7
	pinMode(vilkkuOikeaPWMpin, OUTPUT); //Ulostulo oikelle vilkkureleelle
	pinMode(vilkkuVasenPWMpin, OUTPUT); //Ulostulo vasemmalle vilkkureleelle
	//CTC, tilanvaihto ja nollaus OCR, 50% kanttiaaltoa
	TCCR4A = vilkkuPois; //0b0100000 A, 0b00010000 B = päälle
	TCCR4B = 0b00001101;
	OCR4A = vilkkuNopeus; //PIN6, OCR4A, PH3
	OCR4B = vilkkuNopeus; //PIN7, OCR4B, PH4



	//kello5 ulkoisensignaalin laskuri
	//Normaali laskuri, katto 16-bit max
	pinMode(nopeuspulssitPit, INPUT);
	TCCR5A = 0;
	TCCR5B = (1 << CS52) | (1 << CS51) | (1 << CS50); //0b00000111
	TCCR5C = 0;

	//Ulkoiset interuptit
	pinMode(virratPoisPin, INPUT);
	pinMode(vaihtoKytkinAlasPin, INPUT_PULLUP); //Vaihde alas kytkin
	pinMode(vaihtoKytkinYlosPin, INPUT_PULLUP); //Vaihde ylos kytkin
	pinMode(vilkkuOikeaKytkinPin, INPUT_PULLUP); //Vilkku oikealle kytkin
	pinMode(vilkkuVasenKytkinPin, INPUT_PULLUP); //Villku vasemmalle kytkin
	EICRA = 0b10101010; //10 laskeva reuna
	EICRB = 0b10;
	//EIMSK = 0b00011111; //Interuptimaski
	EIMSK = 0b00011110; //ROMin kirjotus poissa

	pinMode(jarruKytkinPin, INPUT_PULLUP); //Jaaruvalon kytkin vaihto interuptiks
	pinMode(rajoitusKytkinPin, INPUT_PULLUP);
	PCICR = 0b011; //Tilan muutosinterupt
	PCMSK0 = 0b1; //Arduino PIN53
	PCMSK1 = 0b10; //Arduino PIN15

	//Vaihteet INPUT PULLUP
	DDRC = 0;
	PORTC = 0xFF;

	pinMode(jarruvaloPin, OUTPUT); //Ulostulo jarruvalon releelle
	pinMode(servoajuriKytkentaPin, OUTPUT); //Servon kytkentä

	DIDR0 = 0b11; //Anologi päälle A0, A1

	rajoitusPaalla = bitRead(rajoitusKytkinPortti, jarruKytkinBitti); //Mikä on rajoituskykimen tila
	bitWrite(boolLahetysTavu, rajoitusBitti, rajoitusPaalla);

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

	//if (vaihtaaVaihdetta == true)
	if (vaihtoPWM != vaihtoPois) //Onko vaihto PWMmä on päällä 
	{
		//Serial.println("vaihedaan");
		//Tutkitaan kuinka kauan on vaihteenvaihtomoottorille annettu PWMmää.
		if (millis() - vaihdetaanVanhaAika > vaihtoAika)
		{
			//Serial.println("Vaihde vaihdettu");
			vaihtoPWM = vaihtoPois; //OC1 pois päältä
			bitWrite(servoKytkenta, servoKytkentaBitti, LOW); //Servon jarru päälle, PIN8
			//vaihtaaVaihdetta = false;
		}
	}

	//Vaihteen vaihtoon mennää, jos jompaa kumpaa vaihto nappia on painettu .
	if (vaihdeKytkinAlas == true || vaihdeKytkinYlos == true)//Aika rajoitus
	{
		//if (vaihtaaVaihdetta == false)
		if (vaihtoPWM == vaihtoPois)
		{
			//TODO  Mieti vaihteen vaihto rajoitukset kunnolla. Nopeuden ja vaihteen suhteen tai joitan.
			//Mietin yhden kerran ja tulin siihen tulokseen, että ei välttämättä tarvii.
			//Kovassa vauhdissa liian pienelle vahtaminen lienee suurin ongelma, mutta se nyt ei ole kovin vaarallista.

			if (rpm < vaihtoRpm) //Päästään vaihtamaan jos moottorin kierrosnopeus on tietyn rajan alle.
			{
				bitWrite(boolLahetysTavu, liikaaRPMbitti, LOW);

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
			}
			else
			{
				bitWrite(boolLahetysTavu, liikaaRPMbitti, HIGH); //Kirjoitettaan lähetystavuun, että liikaa kierroksia vaihteen vaihtoon.
												 //Kaasu ylös!!
			}
		}

		vaihdeKytkinAlas = false;
		vaihdeKytkinYlos = false;
	}
}


//********************Keskeytyspalvelut****************************//

//Kello1, ylivuotokeskeytys
//32 bit + 16 bit muuttuja 0,5 us askeleella = noin 1600 d = 4,5 a
ISR(TIMER1_OVF_vect)
{
	kello0YlivuotoLaskuri++;
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
	lippu10ms = true;
	OCR1B = OCR1B + lyonnit_1ms;
}

ISR(TIMER1_COMPC_vect)
{
	laskeKierrokset = true;
	OCR1C = OCR1C + rpmMittausväli;
}

		//Aikakeskeytys noin 1ms
		ISR(TIMER2_COMPA_vect)
		{
			rpmADC(); //Kierros ADC pöölle, jäädään oottamaa interuptia.
			//Inkrementoidaan muita aikalaskureita.
			nopeudenLaskentaAikaLaskuri++;
			bensaMittausAikaLaskuri++;
		}

		//Kierros ADC
		ISR(ADC_vect)
		{
			//Kierros ADC valmis.
			rpmMuunnnos = ADC; //Otetaan muunnos muistiin.
			laskeKierrokset = true; //Nostaan kierrosten laskulippu.
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
	if ((millis() - vaihtoNappiVanhaAika) > vaihtoNappiAika)
	{
		vaihdeKytkinAlas = true;
		vaihtoNappiVanhaAika = millis();
		//Serial.println("Alas vahtaa");
	}
}

//Vaihto ylös
ISR(INT2_vect)
{
	if ((millis() - vaihtoNappiVanhaAika) > vaihtoNappiAika)
	{
		vaihdeKytkinYlos = true;
		vaihtoNappiVanhaAika = millis();
		//Serial.println("Ylos vahtaa");
	}
}

//Vilkku oikea
ISR(INT3_vect)
{
	vilkkuPWM = oikeallePaalle; //Vilkku PWM pöölle.
	vilkkuAika = millis(); //Aika muistiin.
	//Serial.println("Vilkkuu oikea");
}

//Vilkku vasen
ISR(INT4_vect)
{
	vilkkuPWM = vasemmallePaalle; //Vilkku PWM pöölle.
	vilkkuAika = millis(); //Aika muistiin.
	//Serial.println("Vilkkuu vasen");
}

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois päältä poistetaan rajoitus.*/
ISR(PCINT0_vect)
{
	rajoitusPaalla = bitRead(rajoitusKytkinPortti, jarruKytkinBitti); //Mikä on rajoituskykimen tila
	bitWrite(boolLahetysTavu, rajoitusBitti, rajoitusPaalla);
}

//Jarrun tilan tutkinta interupti
ISR(PCINT1_vect)
{
	//Pinni on ylös vedetty niin pitää kääntää sen tila, että tämä toimii niin kun olen ajatellu sen alunpittäin.
	jarruPohjassa = bitRead(jarruKytkinPortti, jarruKytkinBitti) ^ true;
	bitWrite(jarruValokytkenta, jarruValokytkentaBitti, jarruPohjassa);
	bitWrite(boolLahetysTavu, jarruPojasssaBitti, jarruPohjassa); ///Kirjoitetaan lähetystavuun jarrun tila.
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
Laskee globaaliin muuttajaan "nopeus" auton nopeuden kilometreinä tunnissa.*/
void nopeusLaskuri()
{
	nopeusPulssit = TCNT5; //Luetaan nopeuspulssit laskurista.
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

	//TODO Tee oikea muunnos. Vaikka joku kerrointaulukko kun on saatu mitattua jännitteitä eri taajuuksilla.
	rpm = round((rpmSumma / float(kierrosTaulukkoKOKO)) * 140);
	//rpm = round((rpmSumma / float(kierrosTaulukkoKOKO)) * 10);
	//Serial.println(rpm);
	kierrosIndeksi++;
}

void vaihtoKasky(uint8_t kaskyPin)
{
	bitWrite(servoKytkenta, servoKytkentaBitti, HIGH); //Servon jarru pois PIN8
	if (kaskyPin == ylosVaihtoKaskyPin)
	{
		vaihtoPWM = ylosPaalle; //OC1A päälle
	}
	else if (kaskyPin == alasVaihtoKaskyPin)
	{
		vaihtoPWM = alasPaalle; //OC1B päälle
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

//Näytöltä tulevan datan vastaanotto.
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
	//Vilkut
	if (vilkkuPWM != vilkkuPois)
	{
		//Serial.println("pitas vilkku");
		if (millis() - vilkkuAika > vikkumisAika)
		{
			//Serial.println("vilkku valmis");
			vilkkuPWM = vilkkuPois;
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

//Näytölle menevien asetuksien lähetysfunktio.
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

//Kierrosten ADC-funktio
void rpmADC()
{
	ADMUX = (0b01000000 | 1); //5V pin referenssinä ja lukee 1 kanavaa.
	ADCSRA = 0b11001110; //Ennabloi AD, ajoon ja interupti. Prescailer 64	
}


//ADC-funktio
//Parametrina
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
			rajoitusPWM = rajoitusPaalle; //OC3A päälle, PIN5, PE3
		}
		else
		{
			rajoitusPWM = rajoitusPois; //OC3A pois
		}
	}
	else
	{
		rajoitusPWM = rajoitusPois; //OC3A pois
	}

}
