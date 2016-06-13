// UTFT_Demo_480x320 
// Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved
// web: http://www.RinkyDinkElectronics.com/
//
// This program is a demo of how to use most of the functions
// of the library with a supported display modules.
//
// This demo was made for modules with a screen resolution 
// of 480x320 pixels.
//
// This program requires the UTFT library.
//
#include <UTFT.h>
#include <UTouch.h>
#include <Arduino.h>
#include <avr/pgmspace.h>


// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Grotesk16x32[];
extern uint8_t Grotesk24x48[];
extern uint8_t Grotesk32x64[];
extern uint8_t SevenSegNumFont[];
extern uint8_t SevenSegment96x144Num[];
extern uint8_t SevenSeg_XXXL_Num[];
extern uint8_t GroteskBold32x64[];
extern unsigned short aareton[];
extern unsigned short bensaIkoni[];
// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ILI9481, 38, 39, 40, 41);

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

//---------------Pinnit		K�ytett�v�t 8, 9, 10, 11. Sarja 14, 15 
const int vastaanottoPin = 8;
const int lahetysPin = 9;
//const int Serial2TX = 14;
//const int serial4RX = 15;

//-----------RPM-asetuksia
const int MAXPISTEET = 600;
const int RIVIT = 2;
//Keskikohdat
const int xK = 460;
const int yK = 300;
//Keh�kaaret
const int ulkoA = 420 + 30;
const int ulkuB = 260;
const int sisaA = 380 + 30;
const int sisaB = 160;//160
					  //Asteikko
const int jako = 11;
const int mittarinMaks = jako * 1000;
const int punaraja = 8000;
//Taulukot ellipseille
short kehaPisteetUlko[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetSisa[RIVIT][MAXPISTEET] = { 0 };
short kehaPisteetSisaHidasdettu[RIVIT][MAXPISTEET] = { 0 };
//Indeksit ja laskurit
int pisteMaaraUlko = 0, pisteMaaraSisa = 0;
int rpmIndeksi = 0, rpmIndeksiEdellinen = 0;
int punarajaIndeksi = 0;

//---------Bensa asetukset
int bensapalkkiKorkeus = 120;
int bensapalkkiLeveys = 30;
int bensapalkkiXlahto = 5;
int bensapalkkiYlahto = 160;
int bensaVahissa = bensapalkkiYlahto - 20;


//----------V�rej�
#define rpmVari 0x05FF
#define rpmPuna 0xFA00
#define rpmTausta VGA_WHITE
#define rpmRajat VGA_BLACK
#define rpmNumerot VGA_BLACK
#define rpmNumerotPuna rpmPuna
#define vapaaVari 0x2EA1
#define mittarinTaustaVari VGA_WHITE
#define bensaOk 0x2EA1

//-----Fontit
#define rpmNumerotFont Grotesk16x32

//--------Sekalaita
const float pii = 3.14159;
const int loopViive = 5;

//---------Joitain kosketukseen liityv��
int x, y;
char stCurrent[20] = "";
int stCurrentLen = 0;
char stLast[20] = "";
int xKord = 0, yKord = 0; //Kosketus koordinaatit
bool printaaUudestaan = true;
bool koskettu = false;
const int trippiNollausAika = 750;

//--------N�ytett�v�t arvot
int rpm = 0; //Moottorin kierrosnopeus (1/min)
int nopeus = 8, nopeusEdellinen = 0;
char vaihde[1] = { 'N' }, vaihdeEdellinen[1];
int matka = 34, matkaEdellinen = 0;
unsigned int trippi = 12200, trippiEdellinen = 0;
int rajoitus = 3, rajoitusEdellinen = 0;
bool rajoitusPaalla = false, rajoitusPaallaEdellinen = true;
int bensa = 0, bensaEdellinen = 0;

bool vaihtoValo = false, vaihtoValo2 = false;
bool liikaaKierroksia = false;
bool jarruPohjassa = false;

//----------Funktioiden otsikot
int pisteetTaulukkoon(int xKeski, int yKeski, int a, int b, short kehaPisteet[RIVIT][MAXPISTEET]);
void jarjasta(short taulukko[RIVIT][MAXPISTEET], int maara);
void hidastaSisa(short kehaPisteetSisa[RIVIT][MAXPISTEET], short kehaPisteetSisaHidasdettu[RIVIT][MAXPISTEET], int pisteMaaraUlko, int pisteMaaraSisa);
void mittarinTausta();
void rajoituksenSyotto();
void piirraEllipsi(int xKeski, int yKeski, int a, int b);

void setup()
{
	Serial.begin(57600);
	Serial2.begin(250000);

	pinMode(vastaanottoPin, OUTPUT);
	pinMode(lahetysPin, OUTPUT);

	//RPM-laskuja
	pisteMaaraUlko = pisteetTaulukkoon(xK, yK, ulkoA, ulkuB, kehaPisteetUlko);
	pisteMaaraSisa = pisteetTaulukkoon(xK, yK, sisaA, sisaB, kehaPisteetSisa);
	jarjasta(kehaPisteetUlko, pisteMaaraUlko);
	jarjasta(kehaPisteetSisa, pisteMaaraSisa);
	hidastaSisa(kehaPisteetSisa, kehaPisteetSisaHidasdettu, pisteMaaraUlko, pisteMaaraSisa);
	punarajaIndeksi = (pisteMaaraUlko * punaraja) / mittarinMaks;

	// Setup the LCD
	myGLCD.InitLCD();
	//Kosketus asetukset
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);

	mittarinTausta();
}

void loop()
{
	//Tietojenhaku
	//vastaanotto();
	digitalWrite(vastaanottoPin, HIGH);
	delay(5);
	//RPM
	rpmFunktio();

	digitalWrite(vastaanottoPin, LOW);

	//Nopeus
	if (nopeus != nopeusEdellinen || printaaUudestaan == true)
	{
		myGLCD.setFont(SevenSegment96x144Num);

		if (nopeus > 9)
		{
			myGLCD.setColor(rpmRajat);
			if (nopeus > 99)
			{
				myGLCD.printNumI(99, 280, 166);
			}
			else
			{
				myGLCD.printNumI(nopeus, 280, 166);
			}
		}
		else
		{
			if (nopeusEdellinen > 9)
			{
				myGLCD.setColor(mittarinTaustaVari);
				myGLCD.fillRect(280, 166, 280 + 96 * 2, 166 + 144);
			}
			myGLCD.setColor(rpmRajat);
			myGLCD.printNumI(nopeus, 280 + 96 / 2, 166);
		}
		nopeusEdellinen = nopeus;
	}

	//Rajoitus
	if (rajoitus != rajoitusEdellinen || printaaUudestaan == true || rajoitusPaalla != rajoitusPaallaEdellinen)
	{
		myGLCD.setFont(SevenSegNumFont);

		if (rajoitusPaalla == true)
		{
			myGLCD.setColor(mittarinTaustaVari);
			myGLCD.fillRect(200 - 10, 220, 200 + 32 * 2 + 10, 220 + 50);

			if (rajoitus > 9)
			{
				myGLCD.setColor(rpmRajat);
				myGLCD.printNumI(rajoitus, 200, 220);
			}
			else
			{

				myGLCD.setColor(rpmRajat);
				myGLCD.printNumI(rajoitus, 200 + 32 / 2, 220);
			}
		}
		else
		{
			myGLCD.setColor(mittarinTaustaVari);
			myGLCD.fillRect(200, 220, 200 + 32 * 2, 220 + 50);
			// Dimensions    : 77x35 pixels
			myGLCD.drawBitmap(196, 230, 77, 35, aareton);
			myGLCD.setColor(rpmRajat);
			myGLCD.setFont(SmallFont);
			myGLCD.printNumI(rajoitus, 228, 260);

		}
		rajoitusPaallaEdellinen = rajoitusPaalla;
		rajoitusEdellinen = rajoitus;
	}

	//Matkamittari
	if (matka != matkaEdellinen || printaaUudestaan == true)
	{
		String matkaVali = String(matka);
		int pituus = matkaVali.length();
		String matkaPrint;
		for (byte i = 0; i < 4 - pituus; i++)
		{
			matkaPrint = matkaPrint + " ";
		}
		matkaPrint = matkaPrint + matkaVali + "km";

		myGLCD.setColor(rpmRajat);
		myGLCD.setFont(BigFont);
		myGLCD.print(matkaPrint, 86, 250);
		matkaEdellinen = matka;
	}

	//Trippi
	if (trippi != trippiEdellinen || printaaUudestaan == true)
	{
		float trippi2 = float(trippi) / 1000;
		String trippiVali = String(trippi2, 1);

		int pituus = trippiVali.length();
		String trippiPrint;
		for (byte i = 0; i < 4 - pituus; i++)
		{
			trippiPrint = trippiPrint + " ";
		}
		trippiPrint = trippiPrint + trippiVali + "km";

		myGLCD.setBackColor(255, 255, 255);
		myGLCD.setColor(rpmRajat);
		myGLCD.setFont(BigFont);
		myGLCD.print(trippiPrint, 86, 280);
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
	if (vaihde[0] != vaihdeEdellinen[0] || printaaUudestaan == true || vaihtoValo != vaihtoValo2)
	{

		if (vaihde[0] == 'N')
		{
			myGLCD.setColor(vapaaVari);
		}
		else if (rpm > punaraja)
		{
			myGLCD.setColor(rpmPuna);
		}
		else if (rpm < punaraja)
		{
			myGLCD.setColor(rpmRajat);
		}


		myGLCD.setFont(GroteskBold32x64);
		myGLCD.printChar(vaihde[0], 30, 30);

		vaihdeEdellinen[0] = vaihde[0];

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
			rpmIndeksiEdellinen = 0;
			printaaUudestaan = true;
		}
		else if (xKord >= 0 && xKord <= 199 && yKord >= 160 && yKord <= 310)
		{
			unsigned long vali = millis();
			while (myTouch.dataAvailable() == true)
			{
				if (millis() - vali >= trippiNollausAika)
				{
					//L�hetet��n trippi p��ohjaukselle
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


	//delay(loopViive);
}

void mittarinTausta()
{
	myGLCD.clrScr();
	myGLCD.fillScr(mittarinTaustaVari);//Tausta v�ri
	myGLCD.setBackColor(mittarinTaustaVari);

	//RPM-asteikko
	myGLCD.setFont(rpmNumerotFont);
	//Grotesk16x32: X = 16, Y = 32, X + i
	int fonttiValistysX = 16;
	int fonttiValistysY = 34;
	for (int i = 1; i < jako + 1; i++)
	{
		if (i * 1000 >= punaraja)
		{
			myGLCD.setColor(rpmNumerotPuna);
		}
		else
		{
			myGLCD.setColor(rpmNumerot);
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
		//myGLCD.drawCircle(kehaPisteetUlko[0][i * vali], kehaPisteetUlko[1][i * vali], 3);
		myGLCD.printNumI(i, kehaPisteetUlko[0][i * vali] - fonttiValistysX + i, kehaPisteetUlko[1][i * vali] - fonttiValistysY);
	}

	//RPM rajat
	myGLCD.setColor(rpmRajat);
	piirraEllipsi(xK, yK, ulkoA + 1, ulkuB + 1);
	piirraEllipsi(xK, yK, sisaA - 1, sisaB - 1);
	myGLCD.drawLine(xK - ulkoA, yK, xK - sisaA, yK);
	myGLCD.drawLine(xK, yK - ulkuB, xK, yK - sisaB);

	//Rajoitus
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setFont(BigFont);
	myGLCD.print("LIMIT", 270 - 37 * 2 - 6, 220 - 20);

	//Nopeus yksikk�
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setFont(BigFont);
	myGLCD.print("km/h", 270 - 32 * 2, 220 + 70);

	//RPM yksikk�
	myGLCD.setColor(VGA_BLACK);
	myGLCD.setFont(SmallFont);
	myGLCD.print("RPM X1000", 5, 305);

	//Bensammittari
	myGLCD.setColor(VGA_BLACK);
	myGLCD.drawRect(bensapalkkiXlahto - 1, bensapalkkiYlahto + 1, bensapalkkiXlahto + bensapalkkiLeveys + 1, bensapalkkiYlahto - bensapalkkiKorkeus - 1);
	myGLCD.drawBitmap(bensapalkkiXlahto + bensapalkkiLeveys + 6, bensapalkkiYlahto - 30, 30, 33, bensaIkoni);

	//myGLCD.drawRect(340, 10, 450, 100);
}

void rajoituksenSyotto()
{

	nappaimisto();

	while (true)
	{
		if (myTouch.dataAvailable())
		{
			myTouch.read();
			x = myTouch.getX();
			y = myTouch.getY();

			if ((y >= 10) && (y <= 60))  // Upper row
			{
				if ((x >= 10) && (x <= 60))  // Button: 1
				{
					waitForIt(10, 10, 60, 60);
					updateStr('1');
				}
				if ((x >= 70) && (x <= 120))  // Button: 2
				{
					waitForIt(70, 10, 120, 60);
					updateStr('2');
				}
				if ((x >= 130) && (x <= 180))  // Button: 3
				{
					waitForIt(130, 10, 180, 60);
					updateStr('3');
				}
				if ((x >= 190) && (x <= 240))  // Button: 4
				{
					waitForIt(190, 10, 240, 60);
					updateStr('4');
				}
				if ((x >= 250) && (x <= 300))  // Button: 5
				{
					waitForIt(250, 10, 300, 60);
					updateStr('5');
				}
			}

			if ((y >= 70) && (y <= 120))  // Center row
			{
				if ((x >= 10) && (x <= 60))  // Button: 6
				{
					waitForIt(10, 70, 60, 120);
					updateStr('6');
				}
				if ((x >= 70) && (x <= 120))  // Button: 7
				{
					waitForIt(70, 70, 120, 120);
					updateStr('7');
				}
				if ((x >= 130) && (x <= 180))  // Button: 8
				{
					waitForIt(130, 70, 180, 120);
					updateStr('8');
				}
				if ((x >= 190) && (x <= 240))  // Button: 9
				{
					waitForIt(190, 70, 240, 120);
					updateStr('9');
				}
				if ((x >= 250) && (x <= 300))  // Button: 0
				{
					waitForIt(250, 70, 300, 120);
					updateStr('0');
				}
			}

			if ((y >= 130) && (y <= 180))  // Upper row
			{
				if ((x >= 10) && (x <= 150))  // Button: Clear
				{
					waitForIt(10, 130, 150, 180);
					stCurrent[0] = '\0';
					stCurrentLen = 0;
					myGLCD.setColor(0, 0, 0);
					myGLCD.fillRect(0, 224, 319, 239);
				}

				if ((x >= 160) && (x <= 300))  // Button: Enter
				{
					waitForIt(160, 130, 300, 180);

					laheta('R', atoi(stCurrent)); //L�hetyksen otsikko 'R' = rajoitus

					while (Serial2.available() > 0)
					{
						if (Serial2.read() == 'K')
						{
							char otsitko = Serial2.read();
							switch (otsitko)
							{
							case 'K':
								return;
								break;
							case 'E':
								myGLCD.print("Rajoitus ei ole sallituissa rajoissa", CENTER, 200);
								break;
							default:
								break;
							}
						}

					}


					if (stCurrentLen>0)
					{
						for (x = 0; x<stCurrentLen + 1; x++)
						{
							stLast[x] = stCurrent[x];
						}

						stCurrent[0] = '\0';
						stCurrentLen = 0;
						myGLCD.setColor(0, 0, 0);
						myGLCD.fillRect(0, 208, 319, 239);
						myGLCD.setColor(0, 255, 0);
						myGLCD.print(stLast, LEFT, 208);

						return;
					}
					else
					{
						myGLCD.setColor(255, 0, 0);
						myGLCD.print("BUFFER EMPTY", CENTER, 192);
						delay(500);
						myGLCD.print("            ", CENTER, 192);
						delay(500);
						myGLCD.print("BUFFER EMPTY", CENTER, 192);
						delay(500);
						myGLCD.print("            ", CENTER, 192);
						myGLCD.setColor(0, 255, 0);
					}
				}
			}
		}
	}
}

void nappaimisto()
{
	//N�yt� tyhjennys
	myGLCD.setFont(BigFont);
	myGLCD.setBackColor(0, 0, 255);
	myGLCD.fillScr(255, 255, 255);


	// Draw the upper row of buttons
	for (x = 0; x<5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10 + (x * 60), 10, 60 + (x * 60), 60);
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10 + (x * 60), 10, 60 + (x * 60), 60);
		myGLCD.printNumI(x + 1, 27 + (x * 60), 27);
	}
	// Draw the center row of buttons
	for (x = 0; x<5; x++)
	{
		myGLCD.setColor(0, 0, 255);
		myGLCD.fillRoundRect(10 + (x * 60), 70, 60 + (x * 60), 120);
		myGLCD.setColor(255, 255, 255);
		myGLCD.drawRoundRect(10 + (x * 60), 70, 60 + (x * 60), 120);
		if (x<4)
			myGLCD.printNumI(x + 6, 27 + (x * 60), 87);
	}
	myGLCD.print("0", 267, 87);
	// Draw the lower row of buttons
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(10, 130, 150, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(10, 130, 150, 180);
	myGLCD.print("Clear", 40, 147);
	myGLCD.setColor(0, 0, 255);
	myGLCD.fillRoundRect(160, 130, 300, 180);
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(160, 130, 300, 180);
	myGLCD.print("Enter", 190, 147);
	myGLCD.setBackColor(0, 0, 0);
}

void updateStr(int val)
{
	if (stCurrentLen<20)
	{
		stCurrent[stCurrentLen] = val;
		stCurrent[stCurrentLen + 1] = '\0';
		stCurrentLen++;
		myGLCD.setColor(0, 255, 0);
		myGLCD.print(stCurrent, LEFT, 224);
	}
	else
	{
		myGLCD.setColor(255, 0, 0);
		myGLCD.print("BUFFER FULL!", CENTER, 192);
		delay(500);
		myGLCD.print("            ", CENTER, 192);
		delay(500);
		myGLCD.print("BUFFER FULL!", CENTER, 192);
		delay(500);
		myGLCD.print("            ", CENTER, 192);
		myGLCD.setColor(0, 255, 0);
	}
}

// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
	myGLCD.setColor(255, 0, 0);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
	while (myTouch.dataAvailable())
		myTouch.read();
	myGLCD.setColor(255, 255, 255);
	myGLCD.drawRoundRect(x1, y1, x2, y2);
}

void piirraEllipsi(int xKeski, int yKeski, int a, int b)
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
		myGLCD.drawPixel(xKeski - x, yKeski - y);

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
		myGLCD.drawPixel(xKeski - x, yKeski - y);


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

void demo()
{
	for (int i = 0; i < pisteMaaraUlko; i++)
	{
		if (i < (pisteMaaraUlko / 5) * 4)
		{
			myGLCD.setColor(0, 191, 255);
		}
		else
		{
			myGLCD.setColor(255, 64, 0);
		}
		myGLCD.drawLine(kehaPisteetUlko[0][i], kehaPisteetUlko[1][i], kehaPisteetSisaHidasdettu[0][i], kehaPisteetSisaHidasdettu[1][i]);
		//delay(5);
	}
	delay(1000);


	myGLCD.setColor(255, 255, 255);

	for (int i = pisteMaaraUlko - 1; i > -1; i--)
	{
		myGLCD.drawLine(kehaPisteetUlko[0][i], kehaPisteetUlko[1][i], kehaPisteetSisaHidasdettu[0][i], kehaPisteetSisaHidasdettu[1][i]);
		//delay(5);
	}
}

void rpmFunktio()
{
	//rpm = map(analogRead(A1), 0, 1023, 1, 8000);

	rpmIndeksi = round((pisteMaaraUlko * rpm) / float(mittarinMaks));

	if (rpmIndeksi > pisteMaaraUlko)
	{
		rpmIndeksi = pisteMaaraUlko;
	}
	/*
	myGLCD.setColor(rpmVari);
	myGLCD.setFont(SmallFont);
	myGLCD.printNumI(rpm, 5, 5);
	myGLCD.printNumI(analogRead(A1), 5, 20);
	*/

	if (rpmIndeksi > rpmIndeksiEdellinen) //Piirt�� uutta
	{
		for (int i = 0; i < rpmIndeksi - rpmIndeksiEdellinen; i++)
		{
			//int vari;
			if (rpmIndeksiEdellinen + i < punarajaIndeksi)
			{
				myGLCD.setColor(rpmVari);
				//vari = rpmVari;
			}
			else
			{
				myGLCD.setColor(rpmPuna);
				//vari = rpmPuna;
			}

			myGLCD.drawLine(kehaPisteetUlko[0][rpmIndeksiEdellinen + i], kehaPisteetUlko[1][rpmIndeksiEdellinen + i], kehaPisteetSisaHidasdettu[0][rpmIndeksiEdellinen + i], kehaPisteetSisaHidasdettu[1][rpmIndeksiEdellinen + i]);
			//tft.drawLine(kehaPisteetUlko[0][rpmIndeksiVanha + i], kehaPisteetUlko[1][rpmIndeksiVanha + i], kehaPisteetSisaHidasdettu[0][rpmIndeksiVanha + i], kehaPisteetSisaHidasdettu[1][rpmIndeksiVanha + i], vari);
		}
	}
	else if (rpmIndeksi < rpmIndeksiEdellinen) //Pyyhkii vanhaa
	{
		for (int i = 0; i < rpmIndeksiEdellinen - rpmIndeksi; i++)
		{
			myGLCD.setColor(rpmTausta);
			myGLCD.drawLine(kehaPisteetUlko[0][rpmIndeksiEdellinen - i], kehaPisteetUlko[1][rpmIndeksiEdellinen - i], kehaPisteetSisaHidasdettu[0][rpmIndeksiEdellinen - i], kehaPisteetSisaHidasdettu[1][rpmIndeksiEdellinen - i]);
			//tft.drawLine(kehaPisteetUlko[0][rpmIndeksiVanha - i], kehaPisteetUlko[1][rpmIndeksiVanha - i], kehaPisteetSisaHidasdettu[0][rpmIndeksiVanha - i], kehaPisteetSisaHidasdettu[1][rpmIndeksiVanha - i], rpmTausta);

		}
	}
	rpmIndeksiEdellinen = rpmIndeksi;
}

void serialEvent2()
{
	Serial.println("Jeee");
	//Tiedon haku
	//digitalWrite(vastaanottoPin, HIGH);

	while (Serial2.available() > 0)
	{
		if (Serial2.read() == 'A')
		{
			if (Serial2.available() >= 10)
			{
				byte boolVastaanOtto = Serial2.read();
				rajoitus = Serial2.read();
				vaihde[0] = Serial2.read();
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
				B00000001 = Rajoitus p��ll�/pois
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
				//Serial.println(boolVastaanOtto, DEC);
				//Serial.println(rajoitusPaalla, DEC);
			}
		}
	}
	//digitalWrite(vastaanottoPin, LOW);
}

void laheta(char otsikko, int data)
{
	Serial.print("Lahetettava otsikko: ");
	Serial.println(otsikko);
	Serial.print("Lahetettava data: ");
	Serial.println(data, DEC);

	digitalWrite(lahetysPin, HIGH);

	Serial2.write(otsikko);
	Serial2.write(data);

	digitalWrite(lahetysPin, LOW);

}
void laheta(char otsikko)
{
	digitalWrite(lahetysPin, HIGH);
	Serial2.write(otsikko);
	digitalWrite(lahetysPin, LOW);
}


void bensaPiirto()
{
	if (bensa != bensaEdellinen)
	{
		if (bensa > bensaEdellinen)
		{
			for (int i = 0; i < bensa - bensaEdellinen; i++)
			{
				if (rpmIndeksiEdellinen + i < punarajaIndeksi)
				{
					myGLCD.setColor(rpmVari);
					//vari = rpmVari;
				}
				else
				{
					myGLCD.setColor(rpmPuna);
					//vari = rpmPuna;
				}

				myGLCD.drawVLine(bensapalkkiXlahto, bensapalkkiYlahto + bensa + i, bensapalkkiLeveys);
			}
		}
		else if (bensa < bensaEdellinen)
		{
			for (int i = 0; i < bensaEdellinen - bensa; i++)
			{
				myGLCD.drawVLine(bensapalkkiXlahto, bensapalkkiYlahto + bensa + i, bensapalkkiLeveys);
			}
		}
		bensaEdellinen = bensa;
	}
}
