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
int bensapalkkiKorkeus = 120;
int bensapalkkiLeveys = 30;
int bensapalkkiXlahto = 5;
int bensapalkkiYlahto = 126;
int bensaVahissa = 20;


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
#define rajoitusFontti 6
#define rajotusPikkuFontti 1 
#define matkamittaritFontti 2
#define vaihdeFontti 2
#define rpmAsteikkoFontti 4
#define limitFontti 4
#define kmhFontti 4
#define rpmYksikko 1

//--------Sekalaita
const float pii = 3.14159;
unsigned long loopViimeAika = 0;
const int ruutuAika = 32; //ms noin 30 fps
const int tauvutMaara = 12; //Tulevat tavut

int minNopeus = 5, maxNopeus = 20;

//---------Joitain kosketukseen liityv‰‰

int xKord = 0, yKord = 0; //Kosketus koordinaatit
bool printaaUudestaan = true;
bool koskettu = false;
const int trippiNollausAika = 750;

//--------N‰ytett‰v‰t arvot
int rpm = 0, rpmEdellinen = 0, rpmVali = 0; //Moottorin kierrosnopeus (1/min)
int nopeus = 8, nopeusEdellinen = 0;
char vaihde = 'N', vaihdeEdellinen;
int matka = 34, matkaEdellinen = 0;
unsigned int trippi = 12200, trippiEdellinen = 0;
int rajoitus = 10, rajoitusEdellinen = 0;
bool rajoitusPaalla = false, rajoitusPaallaEdellinen = true;
int bensa = 0, bensaEdellinen = 0;

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

	// Setup the LCD
	tft.begin();
	tft.setRotation(3);
	//myGLCD.InitLCD();
	//Kosketus asetukset
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);

	mittarinTausta();
}

void loop()
{
	
	if (ruutuAika < (millis() - loopViimeAika))
	{
		Serial.println("uusi");
		loopViimeAika = millis();
		//Tietojenhaku
		digitalWrite(vastaanottoPin, HIGH);

		//delayMicroseconds(5000);
		//Viivett‰ ylˆs oloon
		//digitalWrite(vastaanottoPin, LOW);
	}

	if (uudetArvot == true )
	{
		uudetArvot = false;
		/*
		fps++;

		if (millis() - fpsVanha > 1000)
		{
		tft.setTextColor(rpmRajat, mittarinTaustaVari);
		tft.drawNumber(fps, 200, 10, 4);
		fps = 0;
		fpsVanha = millis();
		}
		*/
		fps = 1000 / (millis() - fpsVanha);
		fpsVanha = millis();
		tft.setTextColor(rpmRajat, mittarinTaustaVari);
		tft.drawFloat(fps, 2, 200, 10, 4);

		//RPM
		//rpmFunktio();
	
		//Bensa
		//bensaPiirto();
		
		//Nopeus
		if (nopeus != nopeusEdellinen || printaaUudestaan == true)
		{
			//myGLCD.setFont(SevenSegment96x144Num);

			if (nopeus > 9)
			{
				//myGLCD.setColor(rpmRajat);
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				if (nopeus > 99)
				{
					//myGLCD.printNumI(99, 280, 166);
					tft.drawNumber(99, 280, 166, nopeusFontti);
				}
				else
				{
					//myGLCD.printNumI(nopeus, 280, 166);
					tft.drawNumber(nopeus, 280, 166, nopeusFontti);
				}
			}
			else
			{
				if (nopeusEdellinen > 9)
				{
					//myGLCD.setColor(mittarinTaustaVari);
					//myGLCD.fillRect(280, 166, 280 + 96 * 2, 166 + 144);
					tft.fillRect(280, 166, 96 * 2, 144, mittarinTaustaVari);

				}
				//myGLCD.setColor(rpmRajat);
				//myGLCD.printNumI(nopeus, 280 + 96 / 2, 166);
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				tft.drawNumber(nopeus, 280 + 96 / 2, 166, nopeusFontti);

			}
			nopeusEdellinen = nopeus;
		}
		
		
		//Rajoitus
		if (rajoitus != rajoitusEdellinen || printaaUudestaan == true || rajoitusPaalla != rajoitusPaallaEdellinen)
		{
			//myGLCD.setFont(SevenSegNumFont);

			if (rajoitusPaalla == true)
			{
				//myGLCD.setColor(mittarinTaustaVari);
				//myGLCD.fillRect(200 - 10, 220, 200 + 32 * 2 + 10, 220 + 50);
				tft.fillRect(200 - 10, 220, 32 * 2 + 10, 50, mittarinTaustaVari);

				if (rajoitus > 9)
				{
					//myGLCD.setColor(rpmRajat);
					//myGLCD.printNumI(rajoitus, 200, 220);
					tft.setTextColor(rpmRajat, mittarinTaustaVari);
					tft.drawNumber(rajoitus, 200, 220, rajoitusFontti);
				}
				else
				{
					//myGLCD.setColor(rpmRajat);
					//myGLCD.printNumI(rajoitus, 200 + 32 / 2, 220);
					tft.setTextColor(rpmRajat, mittarinTaustaVari);
					tft.drawNumber(rajoitus, 200 + 32 / 2, 220, rajoitusFontti);
				}
			}
			else
			{
				//myGLCD.setColor(mittarinTaustaVari);
				//myGLCD.fillRect(200, 220, 200 + 32 * 2, 220 + 50);
				tft.fillRect(200, 220, 32 * 2, 50, mittarinTaustaVari);

				// Dimensions    : 77x35 pixels
				//myGLCD.drawBitmap(196, 230, 77, 35, aareton);
				//myGLCD.setColor(rpmRajat);
				//myGLCD.setFont(SmallFont);
				//myGLCD.printNumI(rajoitus, 228, 260);
				drawIcon(aareton, 196, 230, 77, 35);
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
				tft.drawNumber(rajoitus, 228, 260, rajotusPikkuFontti);

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
			char print[8];
			matkaPrint.toCharArray(print, 8);
			/*
			for (int i = 0; i < matkaPrint.length(); i++)
			{
			print[i] = matkaPrint[i];
			}
			*/
			//myGLCD.setColor(rpmRajat);
			//myGLCD.setFont(BigFont);
			//myGLCD.print(matkaPrint, 86, 250);
			tft.setTextColor(rpmRajat, mittarinTaustaVari);
			tft.drawString(print, 86, 250, matkamittaritFontti);

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

			char print[10];
			trippiPrint.toCharArray(print, 8);

			//myGLCD.setBackColor(255, 255, 255);
			//myGLCD.setColor(rpmRajat);
			//myGLCD.setFont(BigFont);
			//myGLCD.print(trippiPrint, 86, 280);

			tft.setTextColor(rpmRajat, mittarinTaustaVari);
			tft.drawString(print, 86, 280, matkamittaritFontti);

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
				//myGLCD.setColor(vapaaVari);
				tft.setTextColor(vapaaVari, mittarinTaustaVari);
			}
			else if (rpm > punaraja)
			{
				//myGLCD.setColor(rpmPuna);
				tft.setTextColor(rpmPuna, mittarinTaustaVari);
			}
			else if (rpm < punaraja)
			{
				//myGLCD.setColor(rpmRajat);
				tft.setTextColor(rpmRajat, mittarinTaustaVari);
			}

			//myGLCD.setFont(GroteskBold32x64);
			//myGLCD.printChar(vaihde[0], 80, 10);
			//tft.drawChar(vaihde, 80, 10, vaihdeFontti);
			
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
	digitalWrite(vastaanottoPin, LOW);
}

void mittarinTausta()
{
	//myGLCD.clrScr();
	//myGLCD.fillScr(mittarinTaustaVari);//Tausta v‰ri
	//myGLCD.setBackColor(mittarinTaustaVari);
	tft.fillScreen(TFT_BLACK);
	tft.fillScreen(mittarinTaustaVari);

	//RPM-asteikko
	//myGLCD.setFont(rpmNumerotFont);
	//Grotesk16x32: X = 16, Y = 32, X + i
	int fonttiValistysX = 16;
	int fonttiValistysY = 34;

	for (int i = 1; i < jako + 1; i++)
	{
		if (i * 1000 >= punaraja)
		{
			tft.setTextColor(rpmNumerotPuna);
			//vari = rpmNumerotPuna;
			//myGLCD.setColor(rpmNumerotPuna);
		}
		else
		{
			tft.setTextColor(rpmNumerot);
			//vari = rpmNumerot;
			//myGLCD.setColor(rpmNumerot);
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
		//myGLCD.printNumI(i, kehaPisteetUlko[0][i * vali] - fonttiValistysX + i, kehaPisteetUlko[1][i * vali] - fonttiValistysY);
	}

	//RPM rajat
	//myGLCD.setColor(rpmRajat);
	piirraEllipsi(xK, yK, ulkoA + 1, ulkuB + 1, rpmRajat);
	piirraEllipsi(xK, yK, sisaA - 1, sisaB - 1, rpmRajat);
	tft.drawLine(xK - ulkoA, yK, xK - sisaA, yK, rpmRajat);
	tft.drawLine(xK, yK - ulkuB, xK, yK - sisaB, rpmRajat);

	//Rajoitus
	//myGLCD.setColor(VGA_BLACK);
	//myGLCD.setFont(BigFont);
	//myGLCD.print("LIMIT", 270 - 37 * 2 - 6, 220 - 20);
	tft.setTextColor(rpmNumerot);
	tft.drawString("LIMIT", 270 - 37 * 2 - 6, 220 - 20, limitFontti);

	//Nopeus yksikkˆ
	//myGLCD.setColor(VGA_BLACK);
	//myGLCD.setFont(BigFont);
	//myGLCD.print("km/h", 270 - 32 * 2, 220 + 70);
	tft.drawString("km/h", 270 - 32 * 2, 220 + 70, kmhFontti);

	//RPM yksikkˆ
	//myGLCD.setColor(VGA_BLACK);
	//myGLCD.setFont(SmallFont);
	//myGLCD.print("RPM X1000", 5, 305);
	tft.drawString("RPM X1000", 5, 305, rpmYksikko);

	//Bensammittari
	//myGLCD.setColor(VGA_BLACK);
	//myGLCD.drawRect(bensapalkkiXlahto - 1, bensapalkkiYlahto + 1, bensapalkkiXlahto + bensapalkkiLeveys + 1, bensapalkkiYlahto - bensapalkkiKorkeus - 1);
	//myGLCD.drawBitmap(bensapalkkiXlahto + bensapalkkiLeveys + 6, bensapalkkiYlahto - 40, 30, 33, bensaIkoni);
	tft.drawRect(bensapalkkiXlahto - 1,  bensapalkkiYlahto - bensapalkkiKorkeus - 1, bensapalkkiLeveys + 1, bensapalkkiKorkeus + 1, rpmRajat);
	drawIcon(bensaIkoni, bensapalkkiXlahto + bensapalkkiLeveys + 6, bensapalkkiYlahto - 40, 30, 33);
	//myGLCD.drawRect(340, 10, 450, 100);
}

//#define xPikselit 480
//#define yPiksetlit 320
void rajoituksenSyotto()
{
	bool ok = false;
	const XYpaikka OkNappi = { 360, 260 };
	const XYpaikka alku = {100, 160};
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

	saadinKeski.X = (liukupituus / float(maxNopeus - minNopeus)) * rajoitus;
	vanhaX = saadinKeski.X;

	tft.fillScreen(mittarinTaustaVari);
	tft.fillRect(saadinKeski.X - saadinXr, saadinKeski.Y - saadinYr, saadinXr * 2, saadinYr * 2, rpmVari);
	tft.fillRect(OkNappi.X - OkXr, OkNappi.Y - OkYr, OkXr * 2, OkYr * 2, rpmVari);

	tft.setTextColor(rpmRajat, mittarinTaustaVari);
	tft.drawFloat(1.3, 2, 200, 10, 4);

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

					tft.drawFloat(saadinKeski.X, 2, 200, 10, 4);

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
	while (Serial2.available() == 0){}

	while (true)
	{
		if (Serial2.read() == 'A')
		{
			minNopeus = Serial2.read();
			maxNopeus = Serial2.read();
			punaraja = Serial2.read();
			jako = Serial2.read();
			Serial2.write('O');
			break;
		}
		else
		{
			Serial2.write("V");
		}

	}
	mittarinMaks = jako * 1000;
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
			//tft.drawLine(kehaPisteetUlkoPaksunnos[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos[1][rpmIndeksiSisa], vari);
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
			//tft.drawLine(kehaPisteetUlkoPaksunnos[0][rpmIndeksiUlko], kehaPisteetUlkoPaksunnos[1][rpmIndeksiUlko], kehaPisteetSisaPaksunnos[0][rpmIndeksiSisa], kehaPisteetSisaPaksunnos[1][rpmIndeksiSisa], rpmTausta);
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
			Serial.println("nakki");
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
					//Serial.println(boolVastaanOtto, DEC);
					//Serial.println(rajoitusPaalla, DEC);
					uudetArvot = true;
					printaaUudestaan = true;
				}
				
			} while (Serial2.available() > 0);
		}
	}
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
	delay(80);
	digitalWrite(lahetysPin, LOW);

}
void laheta(char otsikko)
{
	Serial.print("Lahetettava otsikko: ");
	Serial.println(otsikko);
	digitalWrite(lahetysPin, HIGH);
	Serial2.write(otsikko);
	delay(100);
	digitalWrite(lahetysPin, LOW);
}

void bensaPiirto()
{
	int vari;
	if (bensa != bensaEdellinen)
	{
		if (bensa > bensaVahissa)
		{
			//myGLCD.setColor(vapaaVari);
			vari = vapaaVari;
		}
		else
		{
			//myGLCD.setColor(rpmPuna);
			vari = rpmPuna;
		}

		for (int i = 0; i < bensapalkkiKorkeus; i++)
		{
			if (i > bensa)
			{
				vari = rpmTausta;
				//myGLCD.setColor(rpmTausta);
			}
			//myGLCD.drawHLine(bensapalkkiXlahto, bensapalkkiYlahto - i, bensapalkkiLeveys);
			tft.drawFastHLine(bensapalkkiXlahto, bensapalkkiYlahto - i, bensapalkkiLeveys, vari);
		}
		/*
		if (bensa > bensaEdellinen)
		{
			for (int i = 0; i < bensa - bensaEdellinen; i++)
			{
				if (bensa > bensaVahissa)
				{
					myGLCD.setColor(vapaaVari);
					//vari = rpmVari;
				}
				else
				{
					myGLCD.setColor(rpmPuna);
					//vari = rpmPuna;
				}

				myGLCD.drawHLine(bensapalkkiXlahto, bensapalkkiYlahto - bensaEdellinen - i, bensapalkkiLeveys);
			}
		}
		else if (bensa < bensaEdellinen)
		{
			myGLCD.setColor(rpmTausta);
			for (int i = 0; i < bensaEdellinen - bensa; i++)
			{
				myGLCD.drawHLine(bensapalkkiXlahto, bensapalkkiYlahto - bensaEdellinen + i, bensapalkkiLeveys);
			}
		}
		*/
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
