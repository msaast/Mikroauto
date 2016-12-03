/*Mikroauton pääojelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen ylös tai alas.

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
//kunnon
#define virratPoisPin 21 
#define vaihtoKytkinAlasPin 20 //Ajajan vaihtokytkin alas
#define vaihtoKytkinYlosPin 19 //Ajajan vaihtokytkin ylös
#define vilkkuOikeaKytkinPin 18
#define vilkkuVasenKytkinPin 2
//tilanvaihto
#define rajoituksenKytkentaPin 53 //Kierrostenrajoittimen avainkytkimen pinni
#define jarruKytkinPin 15 //Jarrukytkin

//PWM-pinnit (HUOM! Taajuuksia on vaihdettu,joten PWMmää on ohjattu suoraan rekisteriä manipuloimalla.)
#define ylosVaihtoKaskyPin 9 //Käskys moottorille vaihtaa ylös. kello2 OC2B 
#define alasVaihtoKaskyPin 10 //Käskys moottorille vaihtaa alas. kello2 OC2A 
#define rajoitusPWMpin 5 //kello3
#define vilkkuOikeaPWMpin 6 //kello4 duty=OCR4A
#define vilkkuVasenPWMpin 7 //kello4 duty=OCR4B

#define servoajuriKytkentaPin 8 //Servon kytkentä pinni
#define bensaSensoriPin 0 //Bensa sensori potikka (A0)

//Valot
#define jarruvaloPin 46

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
const uint16_t nopeudenLaskentaAika = 200;
const uint16_t bensaMittausAika = 3000; //3s
const uint16_t rajoitusAika = 50;
const uint16_t vilkkuNopeus = 20000; //Alustavasti hyvä taajuus
const uint16_t pakkiRajoitus = 5; //Pakin nopeusrajoitus
const uint8_t minRajoitus = 5;
const uint8_t maxRajoitus = 30;
const uint8_t maxNayttoKierrokset = 11; //*1000
const uint16_t punaraja = 8000;
const uint8_t bensapalkkiKorkeus = 120;

//Globaalit muuttujat
volatile uint16_t nopeusPulssit = 0; //Muuttuja johon lasketaan pyörältä tulleet pulssit.
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
bool vaihdeKytkinYlos = true; //Vaihteen vaihto kytkin ylös tila
bool vaihdeKytkinAlas = true; //Vaihteen vaihto kytkin alas tila
uint8_t nopeus = 0; //Auton nopeus (km/h)
uint16_t rpm = 0; //Moottorin kierrosnopeus (1/min)
uint8_t nopeusRajoitus = 20;
uint8_t bensa = 0;
double matka = 0;
double trippi = 0;
char vaihde = 'N'; //Päällä oleva vaihde
bool jarruvalo = true;

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

double nopeusSumma = 0;
double rpmSumma = 0;

void setup()
{
	Serial.begin(57600);
	Serial2.begin(9600);

	alkuarvojenLahetys();

	//bitSet(rekisteri, bitti);
	//PWM
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

	rajoitinKytkinInterrupt();
	lueROM();

	bensaTutkinta();
	matka = 32413.3;
	trippi = 113121.2;
}

void loop()
{
	//TODO Mieti tekiskö kolme eri taajuista aikakeskeytystä joille jakais fiksusti tutkittavat asiat.
	//Aikakeskeytys liputtaa.
	if (nopeudenLaskentaAikaLaskuri >= nopeudenLaskentaAika)
	{
		nopeudenLaskentaAikaLaskuri = 0;
		nopeusLaskuri();
		valot();
		
		vaihde = vaihteet[(PINC ^ 0xFF) >> 1]; //Mega
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

	//Jos nopeusrajoitus on päällä rajoitetaan nopeutta.
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

	//Tutkitaan onko vaihtokytkin ollut tarpeeksi kauan ylhäällä
	if (millis() - vaihtoNappiVanhaAika > vaihtoNappiAika)
	{
		vaihtoNappiYlhaalla = true;
	}

	//Tutkitaan kuinka kauan on vaihteenvaihtomoottorille annettu PWMmää.
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


	//Vaihteen vaihtoon mennää, jos jompaa kumpaa vaihto nappia on painettu .
	if (vaihdeKytkinAlas == LOW || vaihdeKytkinYlos == LOW)//Aika rajoitus
	{

		vaihtoNappiVanhaAika = millis();

		if (kuittaus == true && vaihtoNappiYlhaalla == true)
		{
			vaihtoNappiYlhaalla = false;

			//TODO  Mieti vaihteen vaihto rajoitukset kunnolla. Nopeuden ja vaihteen suhteen tai joitan.
			if (rpm < vaihtoRpm) //Päästään vaihtamaan jos moottorin kierrosnopeus on tietyn rajan alle.
			{
				kuittaus = false; //Kirjotetaan vaihteen vaihto kuittas epätodeksi, koska ruvetaan vaihtamaan vaihdetta.
				bitWrite(boolLahetysTavu, 1, 0);
				switch (vaihde)
				{
				case 'R': //Pakilta annetaan vaihtaa vain ylös päin.
					if (vaihdeKytkinYlos == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case 'N': //Vappaalta voi vaihtaa kumpaankin suuntaa, mutta jarru pitää olla painettuna.

					if (vaihdeKytkinAlas == LOW && jarruvalo == LOW)
					{
						vaihtoKasky(alasVaihtoKaskyPin);
					}
					else if (vaihdeKytkinYlos == LOW && jarruvalo == LOW)
					{
						vaihtoKasky(ylosVaihtoKaskyPin);
					}
					break;
				case '1': //Ykköseltä voi vaihtaa kumpaankin suuntaa.
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
				case '3': //Kolmeoselta voi vaihtaa vain alas päin.
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
				bitWrite(boolLahetysTavu, 1, 1); //Kirjoitettaan lähetystavuun, että liikaa kierroksia vaihteen vaihtoon.
												 //Kaasu ylös!!
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
//Kierros ADC
ISR(ADC_vect)
{
	rpmMuunnnos = ADC;
	laskeKierrokset = true;
}
//Sammutus EEPROM
/* Kirjoitettaan tarvittavat tiedot EEPROMiin talteen, kun virrat katkaistaan.
Tutki millainen konkka tarvitaan pitämään virrat päällä tarpeekksi kauvan.
*/
ISR(INT0_vect)
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
	//Otetaan että virrat katkee
	while (true)
	{
	}
}
//Vaihto alas
ISR(INT1_vect)
{

}
//Vaihto ylös
ISR(INT2_vect)
{

}
//Villku oikea
ISR(INT3_vect)
{

}
//Vilkku vasen
ISR(INT4_vect)
{

}
//Rajoitus
ISR(PCINT0_vect)
{
	cli();
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
	sei();
}
//Jarru
ISR(PCINT0_vect)
{

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

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois päältä poistetaan rajoitus.*/
void rajoitinKytkinInterrupt()
{

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

void valot()
{
	//TODO väärät rekisterit
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
		bitWrite(boolLahetysTavu, 2, 1); ///Kirjoitetaan lähetystavuun, että jarru on painettuna.
	}
	else
	{
		digitalWrite(jarruvaloPin, LOW);
		bitWrite(boolLahetysTavu, 2, 0); ///Kirjoitetaan lähetystavuun, että jarru on ylhäällä.
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