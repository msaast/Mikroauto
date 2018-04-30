#pragma once

//****************Pinnien paikat****************************
//******Sis‰‰nmenot

//Ulkoiset keskeytykset
#define virratPois 19 //Virtojen p‰‰ll‰ olon tutkinta.
#define virratPoisBitti (1UL << PD2)
#define virratPoisDDR DDRD
#define virratPoisPIN PIND
#define virratPoisPORT PORTD

#define vaihtoKytkinAlas 18 //Ajajan vaihtokytkin alas
#define vaihtoKytkinAlasBitti (1UL << PD3)
#define vaihtoKytkinAlasDDR DDRD
#define vaihtoKytkinAlasPIN PIND
#define vaihtoKytkinAlasPORT PORTD

#define vaihtoKytkinYlos 2 //Ajajan vaihtokytkin ylˆs
#define vaihtoKytkinYlosBitti (1UL << PE4)
#define vaihtoKytkinYlosDDR DDRE
#define vaihtoKytkinYlosPIN PINE
#define vaihtoKytkinYlosPORT PORTE

//tilanvaihtokeskeytyket
#define rajoitusKytkin 53 //Kierrostenrajoittimen avainkytkimen pinni
#define rajoitusKytkinBitti (1UL << PB0)
#define rajoitusKytkinDDR DDRB
#define rajoitusKytkin PINB
#define rajoitusKytkinPORT PORTB

#define jarruKytkin 15 //Jarrukytkin
#define jarruKytkinBitti (1UL << PJ0)
#define jarruKytkinDDR DDRJ
#define jarruKytkinPIN PINJ
#define jarruKytkinPORT PORTJ

//Nopeus laskuri
#define nopeuspulssit 47 //Kello5:n ulkoinen kellol‰hde (T5)
#define nopeuspulssiBitti (1UL << PL2)
#define nopeuspulssiDDR DDRL
#define nopeuspulssiPIN PINL
#define nopeuspulssiPORT PORTL

//Taajuuslaskuri RPM
#define rpmSisaan 47
#define rpmSisaanBitti (1UL << PL0)
#define rpmSisaanDDR DDRL
#define rpmSisaanPIN PINL
#define rpmSisaanPORT PORTL

//Digipinnit sis‰‰n
#define vaihdeN 37
#define vaihdeNBitti (1UL << PC0)
#define vaihdeNDDR DDRC
#define vaihdeNPIN PINC
#define vaihdeNPORT PORTC

#define vaihde1 36
#define vaihde1Bitti (1UL << PC1)
#define vaihde1DDR DDRC
#define vaihde1PIN PINC
#define vaihde1PORT PORTC

#define vaihde2 35
#define vaihde2Bitti (1UL << PC2)
#define vaihde2DDR DDRC
#define vaihde2PIN PINC
#define vaihde2PORT PORTC

#define vaihde3 42
#define vaihde3Bitti (1UL << PC3)
#define vaihde3DDR DDRC
#define vaihde3PIN PC3
#define vaihde3PORT PORTC

#define vaihdeR 33
#define vaihdeRBitti (1UL << PC4)
#define vaihdeRDDR DDRC
#define vaihdeRPIN PINC
#define vaihdeRPORT PORTC

#define vilkkuOikeaKytkin 31 
#define vilkkuOikeaKytkiBitti (1UL << PC6)
#define vilkkuOikeaKytkiDDR DDRC
#define vilkkuOikeaKytkiPIN PINC
#define vilkkuOikeaKytkiPORT PORTC

#define vilkkuVasenKytkin 30
#define vilkkuVasenKytkinBitti (1UL << PC7)
#define vilkkuVasenKytkinDDR DDRC
#define vilkkuVasenKytkinPIN PINC
#define vilkkuVasenKytkiPORT PORTC

//ADC
#define bensaSensori A4 //Bensa sensori
#define bensaSensoriBitti (1UL << PF4)
#define bensaSensoriDDR DDRF
#define bensaSensoriPIN PINF
#define bensaSensoriPORT PORTF

#define akkuJannite A0 //Akun j‰nnite
#define akkuJanniteBitti (1UL << PF0)
#define akkuJanniteDDR DDRF
#define akkuJannitePIN PINF
#define akkuJannitePORT PORTF

//******Ulostulot

//PWM
#define rajoitus 5 //Tappokytkimen kytkin
#define rajoitusBitti (1UL << PE3)
#define rajoitusDDR DDRE
#define rajoitusPIN PINE
#define rajoitusPORT PORTE
#define rajoitusOCR OCR3A //kello3

//Digi ulostulo
#define vilkkureleVasen 22
#define vilkkureleVasenBitti (1UL << PA0)
#define vilkkureleVasenDDR DDRA
#define vilkkureleVasenPIN PINA
#define vilkkureleVasenPORT PORTA

#define vilkkureleOikea 23
#define vilkkureleOikeaBitti (1UL << PA1)
#define vilkkureleOikeaDDR DDRA
#define vilkkureleOikeaPIN PINA
#define vilkkureleOikeaPORT PORTA

#define vaihtoKaskyAlas 24
#define vaihtoKaskyAlasBitti (1UL << PA2)
#define vaihtoKaskyAlasDDR DDRA
#define vaihtoKaskyAlasPIN PINA
#define vaihtoKaskyAlasPORT PORTA

#define vaihtoKaskyYlos 25
#define vaihtoKaskyYlosBitti (1UL << PA3)
#define vaihtoKaskyYlosDDR DDRA
#define vaihtoKaskyYlosPIN PINA
#define vaihtoKaskyYlosPORT PORTA

#define pakkipiipppi 26
#define pakkipiipppiBitti (1UL << PA4)
#define pakkipiipppiDDR DDRA
#define pakkipiipppiPIN PINA
#define pakkipiipppiPORT PORTA

#define virtalukonOhistus 27
#define virtalukonOhistusBitti (1UL << PA5)
#define virtalukonOhistusDDR DDRA
#define virtalukonOhistusPIN PINA
#define virtalukonOhistusPORT PORTA

//*****V‰yl‰t

//UART Duelle
//TODO Varmaan tariii jotain muutakin, jos tekee joskus oman UART-toteutuksen.
#define UARTDueRX 17
#define UARTDueRXBitti (1UL << PH0)
#define UARTDueRXDDR DDRH
#define UARTDueRXPIN PINH
#define UARTDueRXPORT PORTH

#define UARTDueTX 16
#define UARTDueTXBitti (1UL << PH1)
#define UARTDueTXDDR DDRH
#define UARTDueTXPIN PINH
#define UARTDueTXPORT PORTH

//I toiseen C
//TODO Jos t‰m‰nkin tekee ite, pit‰‰ tutkia lis‰‰
#define I2CKello 21
#define I2CKelloBitti (1UL << PD0)
#define I2CKelloDDR DDRD
#define I2CKelloPIN PIND
#define I2CKelloPORT PORTD

#define I2CData 20
#define I2CDataBitti (1UL << PD1)
#define I2CDataDDR DDRD
#define I2CDataPIN PIND
#define I2CDataPORT PORTD
//*****************************************************