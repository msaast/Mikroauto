#pragma once

//****************Pinnien paikat****************************
//******Sis‰‰nmenot

//Ulkoiset keskeytykset
#define virratPois 19 //Virtojen p‰‰ll‰ olon tutkinta.
#define virratPoisBitti PD2
#define virratPoisDDR DDRD
#define virratPoisPIN PIND
#define virratPoisPORT PORTD

#define vaihtoKytkinAlas 18 //Ajajan vaihtokytkin alas
#define vaihtoKytkinAlasBitti PD3
#define vaihtoKytkinAlasDDR DDRD
#define vaihtoKytkinAlasPIN PIND
#define vaihtoKytkinAlasPORT PORTD

#define vaihtoKytkinYlos 2 //Ajajan vaihtokytkin ylˆs
#define vaihtoKytkinYlosBitti PE4
#define vaihtoKytkinYlosDDR DDRE
#define vaihtoKytkinYlosPIN PINE
#define vaihtoKytkinYlosPORT PORTE

//tilanvaihtokeskeytyket
#define rajoitusKytkin 53 //Kierrostenrajoittimen avainkytkimen pinni
#define rajoitusKytkinBitti PB0
#define rajoitusKytkinDDR DDRB
#define rajoitusKytkin PINB
#define rajoitusKytkinPORT PORTB

#define jarruKytkin 15 //Jarrukytkin
#define jarruKytkinBitti PJ0
#define jarruKytkinDDR DDRJ
#define jarruKytkinPIN PINJ
#define jarruKytkinPORT PORTJ

//Nopeus laskuri
#define nopeuspulssit 47 //Kello5:n ulkoinen kellol‰hde (T5)
#define nopeuspulssiBitti PL2
#define nopeuspulssiDDR DDRL
#define nopeuspulssiPIN PINL
#define nopeuspulssiPORT PORTL

//Taajuuslaskuri RPM
#define rpmSisaan 47
#define rpmSisaanBitti PL0
#define rpmSisaanDDR DDRL
#define rpmSisaanPIN PINL
#define rpmSisaanPORT PORTL

//Digipinnit sis‰‰n
#define vaihdeN 37
#define vaihdeNBitti PC0
#define vaihdeNDDR DDRC
#define vaihdeNPIN PINC
#define vaihdeNPORT PORTC

#define vaihde1 36
#define vaihde1Bitti PC1
#define vaihde1DDR DDRC
#define vaihde1PIN PINC
#define vaihde1PORT PORTC

#define vaihde2 35
#define vaihde2Bitti PC2
#define vaihde2DDR DDRC
#define vaihde2PIN PINC
#define vaihde2PORT PORTC

#define vaihde3 42
#define vaihde3Bitti PC3
#define vaihde3DDR DDRC
#define vaihde3PIN PC3
#define vaihde3PORT PORTC

#define vaihdeR 33
#define vaihdeRBitti PC4
#define vaihdeRDDR DDRC
#define vaihdeRPIN PINC
#define vaihdeRPORT PORTC

#define vilkkuOikeaKytkin 31 
#define vilkkuOikeaKytkiBitti PC6
#define vilkkuOikeaKytkiDDR DDRC
#define vilkkuOikeaKytkiPIN PINC
#define vilkkuOikeaKytkiPORT PORTC

#define vilkkuVasenKytkin 30
#define vilkkuVasenKytkinBitti PC7
#define vilkkuVasenKytkinDDR DDRC
#define vilkkuVasenKytkinPIN PINC
#define vilkkuVasenKytkiPORT PORTC

//ADC
#define bensaSensori A4 //Bensa sensori
#define bensaSensoriBitti PF4
#define bensaSensoriDDR DDRF
#define bensaSensoriPIN PINF
#define bensaSensoriPORT PORTF

#define akkuJannite A0 //Akun j‰nnite
#define akkuJanniteBitti PF0
#define akkuJanniteDDR DDRF
#define akkuJannitePIN PINF
#define akkuJannitePORT PORTF

//******Ulostulot

//PWM
#define rajoitusPWM 5 //Tappokytkimen kytkin
#define rajoitusPWMBitti PE3
#define rajoitusPWMDDR DDRR
#define rajoitusPWMPIN PINE
#define rajoitusPWMPORT PORTE
#define rajoitusPWMOCR OCR3A //kello3

//Digi ulostulo
#define vilkkureleVasen 22
#define vilkkureleVasenBitti PA0
#define vilkkureleVasenDDR DDRA
#define vilkkureleVasenPIN PINA
#define vilkkureleVasenPORT PORTA

#define vilkkureleOikea 23
#define vilkkureleOikeaBitti PA1
#define vilkkureleOikeaDDR DDRA
#define vilkkureleOikeaPIN PINA
#define vilkkureleOikeaPORT PORTA

#define vaihtoKaskyAlas 24
#define vaihtoKaskyAlasBitti PA2
#define vaihtoKaskyAlasDDR DDRA
#define vaihtoKaskyAlasPIN PINA
#define vaihtoKaskyAlasPORT PORTA

#define vaihtoKaskyYlos 25
#define vaihtoKaskyYlosBitti PA3
#define vaihtoKaskyYlosDDR DDRA
#define vaihtoKaskyYlosPIN PINA
#define vaihtoKaskyYlosPORT PORTA

#define pakkipiipppi 26
#define pakkipiipppiBitti PA4
#define pakkipiipppiDDR DDRA
#define pakkipiipppiPIN PINA
#define pakkipiipppiPORT PORTA

#define virtalukonOhistus 27
#define virtalukonOhistusBitti PA5
#define virtalukonOhistusDDR DDRA
#define virtalukonOhistusPIN PINA
#define virtalukonOhistusPORT PORTA

//*****V‰yl‰t

//UART Duelle
//TODO Varmaan tariii jotain muutakin, jos tekee joskus oman UART-toteutuksen.
#define UARTDueRX 17
#define UARTDueRXBitti PH0
#define UARTDueRXDDR DDRH
#define UARTDueRXPIN PINH
#define UARTDueRXPORT PORTH

#define UARTDueTX 16
#define UARTDueTXBitti PH1
#define UARTDueTXDDR DDRH
#define UARTDueTXPIN PINH
#define UARTDueTXPORT PORTH

//I toiseen C
//TODO Jos t‰m‰nkin tekee ite, pit‰‰ tutkia lis‰‰
#define I2CKello 21
#define I2CKelloBitti PD0
#define I2CKelloDDR DDRD
#define I2CKelloPIN PIND
#define I2CKelloPORT PORTD

#define I2CData 20
#define I2CDataBitti PD1
#define I2CDataDDR DDRD
#define I2CDataPIN PIND
#define I2CDataPORT PORTD
//*****************************************************