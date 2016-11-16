/*Mikroauton pääojelma
Ohjelma laskee moottorin kierroslukua, auton nopeutta ja vaihtaa vaihteen ylös tai alas.

*/
#include <Arduino.h>
#include <EEPROM.h>


//Funktioiden otsikot
void nopeusInterruot();
void rpmInterrupt();
void nopeusLaskuri();
void rpmLaskuri();
void vaihtoKasky(int kaskyPin);
void laheta();
void vastaanota();

//Pinnien paikat
//interruptit
const int rpmPin = 3; //CDI-pulssit
const int nopeusPin = 2; //Pyörän pyörimis anturi
const int vastaanotaPin = 18; //
const int lahetaPin = 19;
const int virratPoisPin = 20;
const int rajoituksenKytkentaPin = 21; //Kierrostenrajoittimen avainkytkimen pinni

									   //PWM-pinnit (HUOM! Taajuuksia on vaihdettu,joten PWMmää on ohjattu suoraan rekisteriä manipuloimalla.)
const int ylosVaihtoKaskyPin = 6; //Käskys moottorille vaihtaa ylös. kello4
const int alasVaihtoKaskyPin = 7; //Käskys moottorille vaihtaa alas. kello4
const int rajoitusPWMpin = 5; //kello3
const int vilkkuOikeaPWMpin = 44; //kello5 duty=OCR5C
const int vilkkuVasenPWMpin = 45; //kello5 duty=OCR5B

const int servoajuriKytkentaPin = 8; //Servon kytkentä pinni
const int bensaSensoriPin = A0; //Bensa sensori potikka

								//Kytkimet
const int vaihtoKytkinAlasPin = 22; //Ajajan vaihtokytkin alas
const int vaihtoKytkinYlosPin = 23; //Ajajan vaihtokytkin ylös
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
const int kierrosTaulukkoKOKO = 20;
const int nopeusTaulukkoKOKO = 10;

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
const int vaihtoRpm = 1800; //Moottorin kierrosnopeus, jonka alle ollessa saa vaihtaa vaihteen (1/min).
const int pulssitPerKierrosRpm = 2; //Montako pulssia tulee per kierros (RPM).
const int pulssitPerKierrosNopeus = 20; //Montako pulssia tulee per kierros (nopeus).
const int vaihtoNappiAika = 2000; //Aika minkä vaihto napin on oltava ylhäällä että voidaan uudestaa koittaa vaihtaan vaihetta.
const int vaihtoAika = 1000; //Vaihtopulssin kesto
const int nopeudenLaskentaAika = 200;
const int rpmLaskentaAika = 20;
const int bensaMittausAika = 10000;
const int rajoitusAika = 50;
const int vilkkuNopeus = 20000; //Alustavasti hyvä taajuus
const int pakkiRajoitus = 5; //Pakin nopeusrajoitus
const int minRajoitus = 5;
const int maxRajoitus = 30;
const int maxNayttoKierrokset = 11; //*1000
const int punaraja = 8000;
const int bensapalkkiKorkeus = 120;

//Globaalit muuttujat
volatile uint8_t nopeusPulssit = 0; //Muuttuja johon lasketaan pyörältä tulleet pulssit.
volatile uint8_t rpmPulssit = 0; //Muuttuja johon lasketaan moottorilta tulleet pulssit.
volatile bool kuittaus = true; //Vaihteen vaihtajan kuittas muuttuja
volatile bool rajoitus = false; //Kierrosten rajoittimen muuttuja
bool vaihtoNappiYlhaalla = true; //
bool liikaaKierroksia = false;
unsigned long vaihdetaanVanhaAika = 0;
unsigned long vaihtoNappiVanhaAika = 0; //
unsigned long nopeusPulssitAika = 0; //Aika jolloin aloitettiin laskemaan nopeus pulsseja.
unsigned long rpmPullsiAika = 0; //Aika jolloin aloitettiin laskemaan kierros pulsseja.
unsigned long rpmLaskentaAikaVanha = 0;
unsigned long nopeudenLaskentaAikaVanha = 0;
unsigned long bensaMittausAikaVanha = 0;
int vaihdeKytkinYlos = HIGH; //Vaihteen vaihto kytkin ylös tila
int vaihdeKytkinAlas = HIGH; //Vaihteen vaihto kytkin alas tila
int nopeus = 0; //Auton nopeus (km/h)
int rpm = 0; //Moottorin kierrosnopeus (1/min)
int nopeusRajoitus = 20;
int bensa = 0;
double matka = 0;
double trippi = 0;
char vaihde = 'N'; //Päällä oleva vaihde
bool jarruvalo = HIGH;

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

int kierrosLuku[kierrosTaulukkoKOKO] = { 0 }; //Taulukko, johon tallennetaan kierroslukuja, että voidaan laskea keskiavaja.
int kierrosIndeksi = 0;
int nopeusTaulukko[nopeusTaulukkoKOKO] = { 0 };
int nopeusIndeksi = 0;

double nopeusSumma = 0;
double rpmSumma = 0;

long fps = 0;
unsigned long fpsVanha = 0;

void setup()
{
	Serial.begin(57600);
	Serial2.begin(460800);

	//Jos rajoitus kytkin on päällä, kun autoon kytketään virrat, niin laitetaan rajoitus päälle.
	pinMode(rajoituksenKytkentaPin, INPUT);
	if (digitalRead(rajoituksenKytkentaPin) == HIGH)
	{
		rajoitus = true;
	}
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
	//kello4 80kHz PIN6 ja PIN7
	TCCR4B = (TCCR4B & B11100000) | B00011001;
	TCCR4A = (TCCR4A & B11111100) | B00000010;
	ICR4 = 199;
	OCR4A = 0; //
	OCR4B = 0;
	//kello5 0,8Hz PIN44 ja PIN45
	TCCR5B = (TCCR5B & B11100000) | B00011101;
	TCCR5A = (TCCR5A & B11111100) | B00000010;
	ICR5 = vilkkuNopeus;
	OCR5B = 0;
	OCR5C = 0;

	//kello1 aikakeskeytykset
	TCCR1A = 0;     // set entire TCCR1A register to 0
	TCCR1B = 0;     // same for TCCR1B
	TCNT1 = 0;
	// set compare match register to desired timer count:
	OCR1A = 3125; //Nopeus, 200ms
	OCR0B = 156; //Kierrokset, noin 20ms
	// turn on CTC mode:
	TCCR1B |= (1 << WGM12);
	// Set CS10 and CS12 bits for 1024 prescaler:
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS12);

	//Aika keskytykset päälle. A- ja B-kanava
	TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);


	attachInterrupt(digitalPinToInterrupt(nopeusPin), nopeusInterruot, CHANGE); //Nopeus pulssit
	attachInterrupt(digitalPinToInterrupt(rpmPin), rpmInterrupt, CHANGE); //Kierros pulssit
	attachInterrupt(digitalPinToInterrupt(lahetaPin), laheta, RISING);
	attachInterrupt(digitalPinToInterrupt(vastaanotaPin), vastaanota, FALLING);
	//attachInterrupt(digitalPinToInterrupt(virratPoisPin), kirjoitaROM, FALLING);
	attachInterrupt(digitalPinToInterrupt(rajoituksenKytkentaPin), rajoitinKytkinInterrupt, CHANGE);
	pinMode(vaihtoKytkinAlasPin, INPUT_PULLUP); //Vaihde ylös kytkin
	pinMode(vaihtoKytkinYlosPin, INPUT_PULLUP); //Vaihde alas kytkin
	pinMode(jarruKytkinPin, INPUT_PULLUP); //Jaaruvalon kytkin
	pinMode(vilkkuOikeaKytkinPin, INPUT_PULLUP); //Vilkku oikealle kytkin
	pinMode(vilkkuVasenKytkinPin, INPUT_PULLUP); //Villku vasemmalle kytkin

	//Vaihteet
	pinMode(vapaaPin, INPUT_PULLUP); //N
	pinMode(ykkonenPin, INPUT_PULLUP); //1
	pinMode(kakkonenPin, INPUT_PULLUP); //2
	pinMode(kolmonenPin, INPUT_PULLUP); //3
	pinMode(pakkiPin, INPUT_PULLUP); //R

	pinMode(jarruvaloPin, OUTPUT); //Ulostulo jarruvalon releelle
	pinMode(servoajuriKytkentaPin, OUTPUT); //Servon kytkentä

	lueROM();
	matka = 324.3;
	trippi = 1121.2;

}

void loop()
{
	fps = micros() - fpsVanha;
	fpsVanha = micros();

	//Aika mikä valein nopeus lasketaan.
	if (millis() - nopeudenLaskentaAikaVanha >= nopeudenLaskentaAika)
	{
		nopeusLaskuri();
		nopeudenLaskentaAikaVanha = millis();
		//Serial.println("nopeus");
		//Serial.println(nopeus);
	}
	//Aika mikä valein kierrosnopeus lasketaan.
	if (millis() - rpmLaskentaAikaVanha >= rpmLaskentaAika)
	{
		rpmLaskuri();
		rpmLaskentaAikaVanha = millis();
		//Serial.println("rpm");
		//Serial.println(rpm);
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

	valot();
	bensaTutkinta();

	//vaihde = vaihteet[(PIOC->PIO_PDSR ^ B11111) >> 1]; //Due
	vaihde = vaihteet[(PINC ^ B11111) >> 1]; //Mega

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

//Nopeus pulssien lasku interrupt-funktio
void nopeusInterruot()
{
	nopeusPulssit++;
}

//Moottorin kierrosluvun lasku interrupt-funktio
void rpmInterrupt()
{
	rpmPulssit++;
}

/*Rajoitin kytkimen interrupt-funktio
Kun kytkimen tila muuttuu muutetaan "rajoitus" muuttujaa ja jos rajoitin laitetaan pois päältä poistetaan rajoitus.*/
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
Laskee globaaliin muuttajaan "nopeus" auton nopeuden kilometreinä tunnissa. */
void nopeusLaskuri()
{

	double matkaVali = ((float)nopeusPulssit / (float)pulssitPerKierrosNopeus) * pii * pyoraHalkaisija; //Metrejä
	double valiAika = (millis() - nopeusPulssitAika) / 1000.0;

	matka = matka + (matkaVali / 1000); //Kilometrejä
	trippi = trippi + matkaVali; //Metrejä

								 /*
								 Serial.print("pulssit: ");
								 Serial.println(nopeusPulssit, DEC);
								 Serial.print("aika: ");
								 Serial.println(millis() - nopeusPulssitAika, DEC);
								 Serial.print("Vali: ");
								 Serial.println(round(matkaVali), DEC);
								 Serial.print("Nopeus: ");
								 Serial.println(round(matkaVali / valiAika), DEC);
								 */

	if (nopeusIndeksi == nopeusTaulukkoKOKO)//Indeksin ympäripyöritys
	{
		nopeusIndeksi = 0;
	}


	nopeusSumma = nopeusSumma - nopeusTaulukko[nopeusIndeksi];

	//Lasketaan auton nopeus (km/h). v=n*d*pii	n=1/t

	nopeusTaulukko[nopeusIndeksi] = round(matkaVali / valiAika * 3.6);
	nopeusPulssitAika = millis(); //Laitetetaan aika muistiin.
	nopeusPulssit = 0; //Nollataan pulssi laskuri.

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

/*Kierrosnopeus lasku funktio
Laskee globaaliin muuttujaan "RPM" moottorin kierrosnopeuden (1/min).*/
void rpmLaskuri()
{
	/*
	Serial.print("Kierrospulssit ");
	Serial.println(rpmPulssit, DEC);
	Serial.print("Kierrokset ");
	Serial.println(rpm, DEC);
	Serial.print("Aika ");
	Serial.println(millis() - rpmPullsiAika, DEC);
	Serial.print("Indeksi ");
	Serial.println(kierrosIndeksi, DEC);
	Serial.print("Taulukko rpm ");
	Serial.println(kierrosLuku[kierrosIndeksi], DEC);
	*/

	if (kierrosIndeksi == kierrosTaulukkoKOKO)
	{
		kierrosIndeksi = 0;
	}

	rpmSumma = rpmSumma - kierrosLuku[kierrosIndeksi];

	//Sijoitetaan arvot taulukkoon, josta lasketaan keskiarvo
	//Lasketaan kierrosnopeus (1/min). n=1/t
	kierrosLuku[kierrosIndeksi] = round((rpmPulssit / (float)pulssitPerKierrosRpm) / ((double)(millis() - rpmPullsiAika) / 1000) * 60);
	rpmPullsiAika = millis();
	rpmPulssit = 0;

	rpmSumma = rpmSumma + kierrosLuku[kierrosIndeksi];

	/*
	for (int i = 0; i < kierrosTaulukkoKOKO; i++)
	{
	rpmSumma = rpmSumma + kierrosLuku[i];
	}
	*/
	rpm = round(rpmSumma / float(kierrosTaulukkoKOKO));
	kierrosIndeksi++;
}

/*Vaihteen vaihto käsky fuktio. (Huom toimii nyt vain megalla pinneillä 6 ja 7, taajuuksia vaihdettu)
Funktioon syötetään pinni, mitä halutaan käyttää ylhäällä. Eli pinniä mikä vastaa suuntaa mihin haluttan vaihtaa.
Vaihteen vaihtaja suorittaa ylös- tai alasvaihto funktion riipuen siitä kumpaan pinniin saa käskyn.*/
void vaihtoKasky(int kaskyPin)
{
	//analogWrite(kaskyPin, 250);

	digitalWrite(servoajuriKytkentaPin, HIGH);
	if (kaskyPin == 6)
	{
		OCR4A = 195; //6 PWM duty %, 98%=195
	}
	else
	{
		OCR4B = 195; //7 PWM duty %, 98%=195
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

	while (Serial2.available() > 0)
	{
		otsikko = Serial2.read();
		switch (otsikko)
		{
		case 'R':
			param = Serial2.read();
			if (param >= minRajoitus && param <= maxRajoitus)
			{
				nopeusRajoitus = param;
				Serial2.write('K');
				Serial2.write('O');
			}
			else
			{
				Serial2.write('K');
				Serial2.write('V');
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
Tutki millainen konkka tarvitaan pitämään virrat päällä tarpeekksi kauvan.
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
	//Otetaan että virrat katkee
	while (true)
	{
	}
}

void valot()
{
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
	int bensaVali = map(analogRead(bensaSensoriPin), 0, 1023, 0, bensapalkkiKorkeus);
	bensa = bensaVali;
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
		Serial2.write(punaraja);
		Serial2.write(maxNayttoKierrokset);

		while (Serial2.available() == 0){}

		kuittaus = Serial2.read();
	}

}