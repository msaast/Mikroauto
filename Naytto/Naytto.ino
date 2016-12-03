struct XYpaikka
{
	int X;
	int Y;
};


//#include <UTFT.h>
#include <User_Setup.h>
#include <TFT_HX8357_Due.h>
#include "Free_Fonts.h" 

TFT_HX8357_Due tft = TFT_HX8357_Due();

#include <UTouch.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

/*
#define HX8357_TFTWIDTH  320
#define HX8357_TFTHEIGHT 480
*/

extern unsigned short aareton[];
extern unsigned short bensaIkoni[];


// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch(6, 5, 4, 3, 2);

//---------------Pinnit		K‰ytett‰v‰t 8, 9, 10, 11. Sarja 14, 15 
const int vastaanottoPin = 8;
const int lahetysPin = 9;
//const int Serial2TX = 14;
//const int serial4RX = 15;


#define xPikselit 480
#define yPiksetlit 320

//-----------RPM-asetuksia
const int MAXPISTEET = 600;
const int RIVIT = 2;
//Keskikohdat
const int xK = 460;
const int yK = 300;
//Keh‰kaaret
const int ulkoA = 420 + 30;
const int ulkuB = 260;
const int sisaA = 380 + 30;
const int sisaB = 160;//160
					  //Asteikko
int jako = 11;
int mittarinMaks = jako * 1000;
int punaraja = 8000;
//Taulukot ellipseille
short kehaPisteetUlko[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetSisa[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetUlkoPaksunnos[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetSisaPaksunnos[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetUlkoPaksunnos2[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetSisaPaksunnos2[RIVIT][MAXPISTEET] = { 0 };
//Indeksit ja laskurit
int pisteMaaraUlko = 0, pisteMaaraSisa = 0, pisteMaaraUlkoPaksunnos = 0, pisteMaaraSisaPaksunnos = 0, pisteMaaraUlkoPaksunnos2 = 0, pisteMaaraSisaPaksunnos2 = 0;
int rpmIndeksiUlko = 0, rpmIndeksiSisa = 0;
int punarajaIndeksiUlko = 0, punarajaIndeksiSisa = 0;

//---------Bensa asetukset
const int bensapalkkiKorkeus = 120;
const int bensapalkkiLeveys = 30;
const int bensapalkkiXlahto = 5;
const int bensapalkkiYlahto = 126;
const int bensaVahissa = 20;


//----------V‰rej‰
#define rpmVari 0x05FF
#define rpmPuna 0xFA00
#define rpmTausta TFT_WHITE
#define rpmRajat TFT_BLACK
#define rpmNumerot TFT_BLACK
#define rpmNumerotPuna rpmPuna
#define vapaaVari 0x2EA1
#define mittarinTaustaVari TFT_WHITE
#define bensaOk 0x2EA1

//-----Fontit
#define rpmNumerotFont Grotesk16x32
#define nopeusFontti 8
#define rajoitusFontti 7
#define rajotusPikkuFontti 2 
#define matkamittaritFontti 4
#define vaihdeFontti 4
#define rpmAsteikkoFontti 4
#define limitFontti 4
#define kmhFontti 4
#define rpmYksikko 2

//--------Sekalaita
const float pii = 3.14159;
unsigned long loopViimeAika = 0;
const uint8_t ruutuAika = 32; //ms noin 30 fps
const uint8_t tauvutMaara = 12; //Tulevat tavut

uint8_t minRajoitus = 5, maxRajoitus = 20;

//---------Joitain kosketukseen liityv‰‰

uint16_t xKord = 0, yKord = 0; //Kosketus koordinaatit
bool printaaUudestaan = true;
bool koskettu = false;
const int trippiNollausAika = 750;

//--------N‰ytett‰v‰t arvot
int rpm = 0, rpmEdellinen = 0, rpmVali = 0; //Moottorin kierrosnopeus (1/min)
int nopeus = 8, nopeusEdellinen = 0;
char vaihde = 'N', vaihdeEdellinen = 'A';
int matka = 34, matkaEdellinen = 0;
unsigned int trippi = 12200, trippiEdellinen = 0;
uint8_t rajoitus = 10, rajoitusEdellinen = 0;
bool rajoitusPaalla = false, rajoitusPaallaEdellinen = true;
int bensa = 0, bensaEdellinen = 1;

bool vaihtoValo = false, vaihtoValo2 = false;
bool liikaaKierroksia = false;
bool jarruPohjassa = false;
bool uudetArvot = true;

float fps = 0;
unsigned long fpsVanha = 0;
int kierto = 0;


//----------Funktioiden otsikot
int pisteetTaulukkoon(int xKeski, int yKeski, int a, int b, short kehaPisteet[RIVIT][MAXPISTEET]);
void jarjasta(short taulukko[RIVIT][MAXPISTEET], int maara);
void hidastaSisa(short kehaPisteetSisa[RIVIT][MAXPISTEET], short kehaPisteetSisaHidasdettu[RIVIT][MAXPISTEET], int pisteMaaraUlko, int pisteMaaraSisa);
void mittarinTausta();
//void rajoituksenSyotto();
void piirraEllipsi(int xKeski, int yKeski, int a, int b, int vari);
int onkoSuorakaiteessa(XYpaikka tarkasta, XYpaikka keski, int xR, int yR);

void setup()
{
	Serial.begin(57600);
	Serial2.begin(9600);

	// Setup the LCD
	tft.begin();
	tft.setRotation(3);
	//Kosketus asetukset
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);

	alkuarvojenHaku();

	pinMode(vastaanottoPin, OUTPUT);
	pinMode(lahetysPin, OUTPUT);

	//RPM-laskuja
	pisteMaaraUlko = pisteetTaulukkoon(xK, yK, ulkoA, ulkuB, kehaPisteetUlko);
	pisteMaaraSisa = pisteetTaulukkoon(xK, yK, sisaA, sisaB, kehaPisteetSisa);
	
	pisteMaaraUlkoPaksunnos = pisteetTaulukkoon(xK, yK, ulkoA - 1, ulkuB - 1, kehaPisteetUlkoPaksunnos);
	pisteMaaraSisaPaksunnos = pisteetTaulukkoon(xK, yK, sisaA + 1, sisaB + 1, kehaPisteetSisaPaksunnos);
	pisteMaaraUlkoPaksunnos2 = pisteetTaulukkoon(xK + 1, yK + 1, ulkoA - 2, ulkuB - 2, kehaPisteetUlkoPaksunnos2);
	pisteMaaraSisaPaksunnos2 = pisteetTaulukkoon(xK - 1, yK - 1, sisaA + 2, sisaB + 2, kehaPisteetSisaPaksunnos2);
	
	jarjasta(kehaPisteetUlko, pisteMaaraUlko);
	jarjasta(kehaPisteetSisa, pisteMaaraSisa);
	jarjasta(kehaPisteetUlkoPaksunnos, pisteMaaraUlkoPaksunnos);
	jarjasta(kehaPisteetSisaPaksunnos, pisteMaaraSisaPaksunnos);
	jarjasta(kehaPisteetUlkoPaksunnos2, pisteMaaraUlkoPaksunnos2);
	jarjasta(kehaPisteetSisaPaksunnos2, pisteMaaraSisaPaksunnos2);
	punarajaIndeksiUlko = (pisteMaaraUlko * punaraja) / mittarinMaks;
	punarajaIndeksiSisa = (pisteMaaraSisa * punaraja) / mittarinMaks;

	mittarinTausta();
}

void loop()
{
	
	if (ruutuAika < (millis() - loopViimeAika))
	{
		loopViimeAika = millis();
		//Tietojenhaku
		Serial2.write('L');
	}

	if (uudetArvot == true )
	{
		uudetArvot = false;

		fps = 1000 / (millis() - fpsVanha);
		fpsVanha = millis();
		tft.setTextColor(rpmRajat, mittarinTaustaVari);
		tft.drawFloat(fps, 2, 200, 10, 4);

		//RPM
		rpmFunktio();
	
		//Bensa
		bensaPiirto();
		
		//Nopeus
		if (nopeus != nopeusEdellinen || printaaUudestaan == true)
		{
			tft.setTextSize(2);

			if (nopeus > 9)
			{
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				if (nopeus > 99)
				{
					tft.drawNumber(99, 262, 162, nopeusFontti);
				}
				else
				{
					tft.drawNumber(nopeus, 262, 162, nopeusFontti);
				}
			}
			else
			{
				if (nopeusEdellinen > 9)
				{
					tft.fillRect(260, 160, xPikselit - 268, yPiksetlit - 160, mittarinTaustaVari);

				}
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				
				tft.drawNumber(nopeus, 270 + 96 / 2, 162, nopeusFontti);
				

			}
			tft.setTextSize(1);
			nopeusEdellinen = nopeus;
		}
		
		//Rajoitus
		if (rajoitus != rajoitusEdellinen || printaaUudestaan == true || rajoitusPaalla != rajoitusPaallaEdellinen)
		{
			if (rajoitusPaalla == true)
			{
				tft.setFreeFont(FSSB18);
				tft.setTextSize(2);
				tft.fillRect(172, 220, 90, 70, mittarinTaustaVari);

				if (rajoitus > 9)
				{

					tft.setTextColor(rpmRajat, mittarinTaustaVari);
					tft.drawNumber(rajoitus, 174, 230, GFXFF);
				}
				else
				{
					tft.setTextColor(rpmRajat, mittarinTaustaVari);
					tft.drawNumber(rajoitus, 174 + 32 / 2, 230, GFXFF);
				}
				tft.setTextSize(1);
				tft.setFreeFont(NULL);
			}
			else
			{
				//Serial.println("Rajoitus uudetaan");
				tft.fillRect(172, 220, 90, 70, mittarinTaustaVari);
				drawIcon(aareton, 172, 230, 77, 35);
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				if (rajoitus > 9)
				{
					tft.drawNumber(rajoitus, 200, 264, rajotusPikkuFontti);
				}
				else
				{
					tft.drawNumber(rajoitus, 206, 264, rajotusPikkuFontti);
				}

			}
			rajoitusPaallaEdellinen = rajoitusPaalla;
			rajoitusEdellinen = rajoitus;
		}
		
		//Matkamittari
		if (matka != matkaEdellinen || printaaUudestaan == true)
		{
			tft.setTextColor(rpmRajat, mittarinTaustaVari);
			tft.drawNumber(matka, 86, 242, matkamittaritFontti);

			matkaEdellinen = matka;
		}
		
		//Trippi
		if (trippi != trippiEdellinen || printaaUudestaan == true)
		{
			float trippi2 = float(trippi) / 1000.0;
			if (floor(trippi2) == 0)
			{
				tft.fillRect(84, 270, 100, 26, mittarinTaustaVari);
			}
			tft.setTextColor(rpmRajat, mittarinTaustaVari);
			tft.drawFloat(trippi2, 1, 86, 274, matkamittaritFontti);

			trippiEdellinen = trippi;
		}
		
		//Vaide
		if (rpm > punaraja)
		{
			vaihtoValo = true;
		}
		else
		{
			vaihtoValo = false;
		}
		if (vaihde != vaihdeEdellinen || printaaUudestaan == true || vaihtoValo != vaihtoValo2)
		{
			
			if (vaihde == 'N')
			{
				tft.setTextColor(vapaaVari, mittarinTaustaVari);
			}
			else if (rpm > punaraja)
			{
				tft.setTextColor(rpmPuna, mittarinTaustaVari);
			}
			else if (rpm < punaraja)
			{
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
			}
			tft.fillRect(58, 10, 100, 76, mittarinTaustaVari);
			tft.setFreeFont(FSSB24);
			tft.setTextSize(2);
			tft.drawChar(vaihde, 60, 80, GFXFF);
			tft.setTextSize(1);
			tft.setFreeFont(NULL);
			vaihdeEdellinen = vaihde;

			printaaUudestaan = false;

			if (vaihtoValo == true)
			{
				vaihtoValo2 = true;
			}
			else
			{
				vaihtoValo2 = false;
			}

		}
		
	}
	
	//Kosketus
	if (myTouch.dataAvailable() == true && koskettu == false)
	{
		myTouch.read();
		xKord = myTouch.getX();
		yKord = myTouch.getY();

		if (xKord >= 200 && xKord <= 470 && yKord >= 160 && yKord <= 310 && rajoitusPaalla != true)
		{
			rajoituksenSyotto();
			mittarinTausta();
			rpmEdellinen = 1;
			printaaUudestaan = true;
			uudetArvot = true;
		}
		else if (xKord >= 0 && xKord <= 199 && yKord >= 160 && yKord <= 310)
		{
			unsigned long vali = millis();
			while (myTouch.dataAvailable() == true)
			{
				if (millis() - vali >= trippiNollausAika)
				{
					//L‰hetet‰‰n trippi p‰‰ohjaukselle
					laheta('N');

					break;
				}
			}
		}
		koskettu = true;
	}
	else if (myTouch.dataAvailable() == false)
	{
		koskettu = false;
	}
}

void mittarinTausta()
{
	tft.fillScreen(TFT_BLACK);
	tft.fillScreen(mittarinTaustaVari);

	//RPM-asteikko
	int fonttiValistysX = 16;
	int fonttiValistysY = 34;

	for (int i = 1; i < jako + 1; i++)
	{
		if (i * 1000 >= punaraja)
		{
			tft.setTextColor(rpmNumerotPuna);
		}
		else
		{
			tft.setTextColor(rpmNumerot);
		}
		if (i >= 10)
		{
			fonttiValistysX = 32;
		}
		else
		{
			fonttiValistysX = 16;
		}
		int vali = pisteMaaraUlko / jako;
		tft.drawNumber(i, kehaPisteetUlko[0][i * vali] - fonttiValistysX + i, kehaPisteetUlko[1][i * vali] - fonttiValistysY, rpmAsteikkoFontti);
	}

	//RPM rajat
	piirraEllipsi(xK, yK, ulkoA + 1, ulkuB + 1, rpmRajat);
	piirraEllipsi(xK, yK, sisaA - 1, sisaB - 1, rpmRajat);
	tft.drawLine(xK - ulkoA, yK, xK - sisaA, yK, rpmRajat);
	tft.drawLine(xK, yK - ulkuB, xK, yK - sisaB, rpmRajat);

	//Rajoitus
	tft.setTextColor(rpmNumerot);
	tft.drawString("LIMIT", 180, 220 - 20, limitFontti);

	//Nopeus yksikkˆ
	tft.drawString("km/h", 190, 220 + 70, kmhFontti);

	//RPM yksikkˆ
	tft.drawString("RPM X1000", 5, 303, rpmYksikko);

	//TRIP
	tft.drawString("TRIP", 102, 300, rpmYksikko);

	//ODO
	tft.drawString("ODO", 106, 224, rpmYksikko);

	//Bensammittari
	tft.drawRect(bensapalkkiXlahto - 2,  bensapalkkiYlahto - bensapalkkiKorkeus - 1, bensapalkkiLeveys + 4, bensapalkkiKorkeus + 4, rpmRajat);
	drawIcon(bensaIkoni, bensapalkkiXlahto + bensapalkkiLeveys + 6, bensapalkkiYlahto - 40, 30, 33);
}

//#define xPikselit 480
//#define yPiksetlit 320
void rajoituksenSyotto()
{
	bool ok = false;
	const XYpaikka OkNappi = { 360, 260 };
	const XYpaikka alku = {50, 160};
	const XYpaikka loppu = { xPikselit - alku.X, alku.Y};
	XYpaikka saadinKeski;
	saadinKeski.Y = alku.Y;
	int vanhaX = 0, vanhaX2;
	XYpaikka temp;

	const int saadinXr = 80 / 2;
	const int saadinYr = 140 / 2;
	const int OkXr = 40 / 2;
	const int OkYr = 40 / 2;
	const int liukupituus = xPikselit - alku.X * 2;

	saadinKeski.X = (liukupituus / float(maxRajoitus - minRajoitus)) * rajoitus - alku.X / 2;
	
	vanhaX = saadinKeski.X;

	tft.fillScreen(mittarinTaustaVari);
	tft.fillRect(saadinKeski.X - saadinXr, saadinKeski.Y - saadinYr, saadinXr * 2, saadinYr * 2, rpmVari);
	tft.fillRect(OkNappi.X - OkXr, OkNappi.Y - OkYr, OkXr * 2, OkYr * 2, rpmVari);

	tft.setTextColor(rpmRajat, mittarinTaustaVari);

	tft.drawNumber(rajoitus, 200, 20, 6);
	tft.drawNumber(minRajoitus, 20, 20, 6);
	tft.drawNumber(maxRajoitus, 400, 20, 6);

	int laskuri = 0;
	const int toleranssi = 40;
	while (myTouch.dataAvailable() == true){}
	while (true)
	{

		if (myTouch.dataAvailable() == true)
		{
			myTouch.read();
			temp.X = myTouch.getX();
			temp.Y = myTouch.getY();

			if (onkoSuorakaiteessa(temp, saadinKeski, saadinXr, saadinYr) == EXIT_SUCCESS)
			{
				while (myTouch.dataAvailable() == true)
				{
					delay(20);
					myTouch.read();
					saadinKeski.X = myTouch.getX();
					
					//tarkasta.X >= keski.X - xR && tarkasta.X <= keski.X + xR
					if (!(saadinKeski.X >= vanhaX - toleranssi && saadinKeski.X <= vanhaX + toleranssi))// (saadinKeski.X < 0 || saadinKeski.X > xPikselit)
					{
						saadinKeski.X = vanhaX;
						//Serial.println("toleranssi");
					}


					if (saadinKeski.X < alku.X)
					{
						saadinKeski.X = alku.X;
						//Serial.println("alku");
					}
					else if (saadinKeski.X > loppu.X)
					{
						saadinKeski.X = loppu.X;
						//Serial.println("loppu");
					}

					Serial.println(saadinKeski.X);

					tft.drawFloat(((maxRajoitus - minRajoitus) * (saadinKeski.X + alku.X / 2)) / float(liukupituus), 2, 200, 20, 6);

					if (saadinKeski.X != vanhaX)
					{
						int erotus = vanhaX - saadinKeski.X;

						if (erotus < 0)
						{
							//Kumita vanha
							tft.fillRect(vanhaX - saadinXr, saadinKeski.Y - saadinYr, abs(erotus), saadinYr * 2, mittarinTaustaVari);
							//Piirr‰ uusi
							tft.fillRect(vanhaX + saadinXr, saadinKeski.Y - saadinYr, abs(erotus), saadinYr * 2, rpmVari);
						}
						else
						{
							//Kumita vanha
							tft.fillRect(vanhaX + saadinXr - erotus, saadinKeski.Y - saadinYr, abs(erotus), saadinYr * 2, mittarinTaustaVari);
							//Piirr‰ uusi
							tft.fillRect(saadinKeski.X - saadinXr, saadinKeski.Y - saadinYr, abs(erotus), saadinYr * 2, rpmVari);

						}
						//Piirr‰ uusi
						//tft.fillRect(saadinKeski.X - saadinXr, saadinKeski.Y - saadinYr, saadinXr * 2, saadinYr * 2, rpmVari);
						vanhaX = saadinKeski.X;
					}
				}
			}
			else if (onkoSuorakaiteessa(temp, OkNappi, OkXr, OkYr) == EXIT_SUCCESS)
			{
				uint8_t apuRajoitus = round(((maxRajoitus - minRajoitus) * (saadinKeski.X + alku.X / 2)) / float(liukupituus));
				laheta('R', apuRajoitus);
				return;
			}
		}
	}

}

int onkoSuorakaiteessa(XYpaikka tarkasta, XYpaikka keski, int xR, int yR)
{
	//xKord >= 200 && xKord <= 470 && yKord >= 160 && yKord <= 310
	if (tarkasta.X >= keski.X - xR && tarkasta.X <= keski.X + xR && tarkasta.Y >= keski.Y - yR && tarkasta.Y <= keski.Y + yR)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int keskiLaskin(int uusiPikseli, int liukupituus)
{

}

void alkuarvojenHaku()
{
	Serial2.write('a');
	while (true)
	{
		while (Serial2.available() == 0) {}
		if (Serial2.read() == 'A')
		{
			while (Serial2.available() == 0) {}
			minRajoitus = Serial2.read();
			while (Serial2.available() == 0) {}
			maxRajoitus = Serial2.read();
			while (Serial2.available() == 0) {}
			punaraja = Serial2.read();
			while (Serial2.available() == 0) {}
			jako = Serial2.read();
			Serial2.write('O');
			break;
		}
		else
		{
			Serial2.write("V");
		}

	}
	punaraja*= 100;
	mittarinMaks = jako * 1000;
	Serial.println(minRajoitus);
	Serial.println(maxRajoitus);
	Serial.println(punaraja);
	Serial.println(jako);
}

void piirraEllipsi(int xKeski, int yKeski, int a, int b, int vari)
{
	int a2 = a * a;
	int b2 = b * b;
	int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;

	/* first half */
	for (x = 0, y = b, sigma = 2 * b2 + a2*(1 - 2 * b); b2*x <= a2*y; x++)
	{
		//delay(1);

		//myGLCD.drawPixel(xc + x, yc + y);
		//myGLCD.drawPixel(xc - x, yc + y);
		//myGLCD.drawPixel(xc + x, yc - y);
		//myGLCD.drawPixel(xKeski - x, yKeski - y);
		tft.drawPixel(xKeski - x, yKeski - y, vari);

		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}

	/* second half */
	for (x = a, y = 0, sigma = 2 * a2 + b2*(1 - 2 * a); a2*y <= b2*x; y++)
	{
		//delay(1);

		//myGLCD.drawPixel(xc + x, yc + y);
		//myGLCD.drawPixel(xc - x, yc + y);
		//myGLCD.drawPixel(xc + x, yc - y);
		//myGLCD.drawPixel(xKeski - x, yKeski - y);
		tft.drawPixel(xKeski - x, yKeski - y, vari);

		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}
}

int pisteetTaulukkoon(int xKeski, int yKeski, int a, int b, short kehaPisteet[RIVIT][MAXPISTEET])
{
	int a2 = a * a;
	int b2 = b * b;
	int fa2 = 4 * a2, fb2 = 4 * b2;
	int x, y, sigma;
	int pisteet = 0;

	/* second half */
	for (x = a, y = 0, sigma = 2 * a2 + b2*(1 - 2 * a); a2*y <= b2*x; y++)
	{
		kehaPisteet[0][pisteet] = xKeski - x;
		kehaPisteet[1][pisteet] = yKeski - y;
		pisteet++;
		if (sigma >= 0)
		{
			sigma += fb2 * (1 - x);
			x--;
		}
		sigma += a2 * ((4 * y) + 6);
	}

	/* first half */
	for (x = 0, y = b, sigma = 2 * b2 + a2*(1 - 2 * b); b2*x <= a2*y; x++)
	{
		kehaPisteet[0][pisteet] = xKeski - x;
		kehaPisteet[1][pisteet] = yKeski - y;
		pisteet++;
		if (sigma >= 0)
		{
			sigma += fa2 * (1 - y);
			y--;
		}
		sigma += b2 * ((4 * x) + 6);
	}
	return pisteet;
}

void jarjasta(short taulukko[RIVIT][MAXPISTEET], int maara)
{
	int vali = 0;

	for (int vaihe = 0; vaihe < maara - 1; vaihe++)
	{
		for (int j = 0; j < maara - 1; j++)
		{
			if (taulukko[0][j] > taulukko[0][j + 1])
			{
				vali = taulukko[0][j];
				taulukko[0][j] = taulukko[0][j + 1];
				taulukko[0][j + 1] = vali;
			}
		}
	}
	for (int vaihe = 0; vaihe < maara - 1; vaihe++)
	{
		for (int j = 0; j < maara - 1; j++)
		{
			if (taulukko[1][j] < taulukko[1][j + 1])
			{
				vali = taulukko[1][j];
				taulukko[1][j] = taulukko[1][j + 1];
				taulukko[1][j + 1] = vali;
			}
		}
	}
}

void hidastaSisa(short kehaPisteetSisa[RIVIT][MAXPISTEET], short kehaPisteetSisaHidasdettu[RIVIT][MAXPISTEET], int pisteMaaraUlko, int pisteMaaraSisa)
{
	float hidastusLaskin = 0;

	for (int i = 0, j = 0; i < pisteMaaraUlko; i++)
	{
		/*
		if (j <= pisteMaaraSisa)
		{
		kehaPisteetSisaHidasdettu[0][i] = kehaPisteetSisa[0][j];
		kehaPisteetSisaHidasdettu[1][i] = kehaPisteetSisa[1][j];
		}
		else
		{
		kehaPisteetSisaHidasdettu[0][i] = kehaPisteetSisa[0][pisteMaaraSisa - 1];
		kehaPisteetSisaHidasdettu[1][i] = kehaPisteetSisa[1][pisteMaaraSisa - 1];
		}
		*/
		kehaPisteetSisaHidasdettu[0][i] = kehaPisteetSisa[0][j];
		kehaPisteetSisaHidasdettu[1][i] = kehaPisteetSisa[1][j];

		if (hidastusLaskin >= 2)
		{
			j++;
			hidastusLaskin = 0;
		}

		if (i % 2 == 0)
		{
			hidastusLaskin = hidastusLaskin + 2;
		}
		else if (i % 3 == 0)
		{
			hidastusLaskin = hidastusLaskin + 1.5;
		}
		else
		{
			hidastusLaskin = hidastusLaskin + 2;
		}
	}
}

void rpmFunktio()
{
	int rpmInkrementti = 14;
	if (rpm % rpmInkrementti != 0)
	{
		rpm = rpm - (rpm % rpmInkrementti);
	}

	if (rpm > rpmEdellinen) //Piirt‰‰ uutta
	{
		int vari;
		for (rpmVali = rpmEdellinen; rpmVali <= rpm; rpmVali+= rpmInkrementti)
		{
			rpmIndeksiUlko = round((pisteMaaraUlko * rpmVali) / float(mittarinMaks));
			rpmIndeksiSisa = round((pisteMaaraSisa * rpmVali) / float(mittarinMaks));

			rpmIndeksiSisa = rpmIndeksiSisa - 10;
			if (rpmIndeksiSisa < 0)
			{
				rpmIndeksiSisa = 0;
			}

			if (rpmIndeksiUlko > pisteMaaraUlko || rpmIndeksiSisa > pisteMaaraSisa)
			{
				rpmIndeksiUlko = pisteMaaraUlko - 1;
				rpmIndeksiSisa = pisteMaaraSisa - 1;
			}

			if (rpmIndeksiUlko < punarajaIndeksiUlko)
			{
				vari = rpmVari;
			}
			else
			{
				vari = rpmPuna;
			}
			
			tft.drawLine(kehaPisteetUlko[0][rpmIndeksiUlko], kehaPisteetUlko[1][rpmIndeksiUlko], kehaPisteetSisa[0][rpmIndeksiSisa], kehaPisteetSisa[1][rpmIndeksiSisa], vari);
			tft.drawLine(kehaPisteetUlkoPaksunnos[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos[1][rpmIndeksiSisa], vari);
			//tft.drawLine(kehaPisteetUlkoPaksunnos2[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos2[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos2[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos2[1][rpmIndeksiSisa], vari);
		}
	}
	else if (rpm < rpmEdellinen) //Pyyhkii vanhaa
	{
		for (rpmVali = rpmEdellinen; rpmVali >= rpm; rpmVali-= rpmInkrementti)
		{
			if (rpmVali < 0)
			{
				rpmVali = 1;
			}
			rpmIndeksiUlko = round((pisteMaaraUlko * rpmVali) / float(mittarinMaks));
			rpmIndeksiSisa = round((pisteMaaraSisa * rpmVali) / float(mittarinMaks));

			rpmIndeksiSisa = rpmIndeksiSisa - 10;
			if (rpmIndeksiSisa < 0)
			{
				rpmIndeksiSisa = 0;
			}

			tft.drawLine(kehaPisteetUlko[0][rpmIndeksiUlko], kehaPisteetUlko[1][rpmIndeksiUlko], kehaPisteetSisa[0][rpmIndeksiSisa], kehaPisteetSisa[1][rpmIndeksiSisa], rpmTausta);
			tft.drawLine(kehaPisteetUlkoPaksunnos[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos[1][rpmIndeksiSisa], rpmTausta);
			//tft.drawLine(kehaPisteetUlkoPaksunnos2[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos2[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos2[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos2[1][rpmIndeksiSisa], rpmTausta);
		}
	}

	rpmEdellinen = rpm;
}

void serialEvent2()
{
	//Tiedon haku
	//digitalWrite(vastaanottoPin, LOW);
	//Serial.println(Serial2.available());
	if (Serial2.available() >= tauvutMaara)
	{
		while (Serial2.available() > 0)
		{
			//Serial.println("nakki");
			do
			{
				if (Serial2.read() == 'A')
				{
					byte boolVastaanOtto = Serial2.read();
					rajoitus = Serial2.read();
					vaihde = Serial2.read();
					int rpmVali1 = Serial2.read();
					int rpmVali2 = Serial2.read();
					nopeus = Serial2.read();
					int matkaVali1 = Serial2.read();
					int matkaVali2 = Serial2.read();
					int trippiVali1 = Serial2.read();
					int trippiVali2 = Serial2.read();
					bensa = Serial2.read();

					rpm = (rpmVali1 << 8) | rpmVali2;
					matka = (matkaVali1 << 8) | matkaVali2;
					trippi = (trippiVali1 << 8) | trippiVali2;
					/*
					B00000001 = Rajoitus p‰‰ll‰/pois
					B00000010 = Liikaa kierroksia vaihtoon
					B00000100 = Jarru pohjassa
					B00001000 =
					B00010000 =
					B00100000 =
					B01000000 =
					B10000000 =							*/
					rajoitusPaalla = boolVastaanOtto & B00000001;
					liikaaKierroksia = (boolVastaanOtto & B00000010) >> 1;
					jarruPohjassa = (boolVastaanOtto & B00000100) >> 2;
					Serial.println(vaihde);
					//Serial.println(rpm);
					//Serial.println(matka);
					//Serial.println(trippi);
					//Serial.println(rajoitusPaalla);
					uudetArvot = true;
					//printaaUudestaan = true;
				}
				
			} while (Serial2.available() > 0);
		}
	}
}

void laheta(char otsikko, int data)
{
	Serial.print("Lahetettava data: ");
	Serial.println(data, DEC);
	//digitalWrite(lahetysPin, HIGH);

	laheta(otsikko);
	Serial2.write(data);

	//delay(80);
	//digitalWrite(lahetysPin, LOW);

}
void laheta(char otsikko)
{
	Serial.print("Lahetettava otsikko: ");
	Serial.println(otsikko);
	//digitalWrite(lahetysPin, HIGH);

	Serial2.write('V');
	Serial2.write(otsikko);

	//delay(100);
	//digitalWrite(lahetysPin, LOW);
}

void bensaPiirto()
{
	int vari;
	if (bensa != bensaEdellinen || printaaUudestaan == true)
	{
		if (bensa > bensaVahissa)
		{
			vari = vapaaVari;
		}
		else
		{
			vari = rpmPuna;
		}

		for (int i = 0; i < bensapalkkiKorkeus; i++)
		{
			if (i > bensa)
			{
				vari = rpmTausta;
			}

			tft.drawFastHLine(bensapalkkiXlahto, bensapalkkiYlahto - i, bensapalkkiLeveys, vari);
		}

		bensaEdellinen = bensa;
	}
}

//====================================================================================
// This is the function to draw the icon stored as an array in program memory (FLASH)
//====================================================================================

// To speed up rendering we use a 64 pixel buffer
#define BUFF_SIZE 64

// Draw array "icon" of defined width and height at coordinate x,y
// Maximum icon size is 255x255 pixels to avoid integer overflow

void drawIcon(const unsigned short* icon, int16_t x, int16_t y, int8_t width, int8_t height) {

	uint16_t  pix_buffer[BUFF_SIZE];   // Pixel buffer (16 bits per pixel)

									   // Set up a window the right size to stream pixels into
	tft.setWindow(x, y, x + width - 1, y + height - 1);

	// Work out the number whole buffers to send
	uint16_t nb = ((uint16_t)height * width) / BUFF_SIZE;

	// Fill and send "nb" buffers to TFT
	for (int i = 0; i < nb; i++) {
		for (int j = 0; j < BUFF_SIZE; j++) {
			pix_buffer[j] = pgm_read_word(&icon[i * BUFF_SIZE + j]);
		}
		tft.pushColors(pix_buffer, BUFF_SIZE);
	}

	// Work out number of pixels not yet sent
	uint16_t np = ((uint16_t)height * width) % BUFF_SIZE;

	// Send any partial buffer left over
	if (np) {
		for (int i = 0; i < np; i++) pix_buffer[i] = pgm_read_word(&icon[nb * BUFF_SIZE + i]);
		tft.pushColors(pix_buffer, np);
	}
}
