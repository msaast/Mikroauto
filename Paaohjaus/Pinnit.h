#pragma once

//****************Pinnien paikat****************************
//******Sis‰‰nmenot

//Ulkoiset keskeytykset
#define virratPois 19 //Virtojen p‰‰ll‰ olon tutkinta.
#define virratPoisBitti (1U << PD2)
#define virratPoisDDR DDRD
#define virratPoisPIN PIND
#define virratPoisPORT PORTD
#define virratPoisINT INT2

#define vaihtoKytkinAlas 18 //Ajajan vaihtokytkin alas
#define vaihtoKytkinAlasBitti (1U << PD3)
#define vaihtoKytkinAlasDDR DDRD
#define vaihtoKytkinAlasPIN PIND
#define vaihtoKytkinAlasPORT PORTD

#define vaihtoKytkinYlos 2 //Ajajan vaihtokytkin ylˆs
#define vaihtoKytkinYlosBitti (1U << PE4)
#define vaihtoKytkinYlosDDR DDRE
#define vaihtoKytkinYlosPIN PINE
#define vaihtoKytkinYlosPORT PORTE

#define vaihtoKytkinINTmaski ((1U << INT3) | (1U << INT4))

//tilanvaihtokeskeytyket
#define rajoitusKytkin 53 //Kierrostenrajoittimen avainkytkimen pinni
#define rajoitusKytkinBitti (1U << PB0)
#define rajoitusKytkinDDR DDRB
#define rajoitusKytkinPIN PINB
#define rajoitusKytkinPORT PORTB
#define rajoitusKytkinPCINTbitti 0b1

#define jarruKytkin 15 //Jarrukytkin
#define jarruKytkinBitti (1U << PJ0)
#define jarruKytkinDDR DDRJ
#define jarruKytkinPIN PINJ
#define jarruKytkinPORT PORTJ
#define jarruKytkinPCINTbitti 0b10

//Nopeus laskuri
#define nopeuspulssit 47 //Kello5:n ulkoinen kellol‰hde (T5)
#define nopeuspulssiBitti (1U << PL2)
#define nopeuspulssiDDR DDRL
#define nopeuspulssiPIN PINL
#define nopeuspulssiPORT PORTL

//Taajuuslaskuri RPM
#define rpmSisaan 47
#define rpmSisaanBitti (1U << PL0)
#define rpmSisaanDDR DDRL
#define rpmSisaanPIN PINL
#define rpmSisaanPORT PORTL

//Digipinnit sis‰‰n
#define vaihdeN 37
#define vaihdeNBitti (1U << PC0)
#define vaihdeNDDR DDRC
#define vaihdeNPIN PINC
#define vaihdeNPORT PORTC

#define vaihde1 36
#define vaihde1Bitti (1U << PC1)
#define vaihde1DDR DDRC
#define vaihde1PIN PINC
#define vaihde1PORT PORTC

#define vaihde2 35
#define vaihde2Bitti (1U << PC2)
#define vaihde2DDR DDRC
#define vaihde2PIN PINC
#define vaihde2PORT PORTC

#define vaihde3 42
#define vaihde3Bitti (1U << PC3)
#define vaihde3DDR DDRC
#define vaihde3PIN PC3
#define vaihde3PORT PORTC

#define vaihdeR 33
#define vaihdeRBitti (1U << PC4)
#define vaihdeRDDR DDRC
#define vaihdeRPIN PINC
#define vaihdeRPORT PORTC

#define vaihdeBititMaski (vaihdeNBitti | vaihde1Bitti | vaihde2Bitti | vaihde3Bitti | vaihdeRBitti)
#define vaihdeDDR DDRC
#define vaihdePIN PINC
#define vaihdePORT PORTC

#define vilkkukytkinOikea 31 
#define vilkkukytkinOikeaBitti (1U << PC6)
#define vilkkukytkinOikeaDDR DDRC
#define vilkkukytkinOikeaPIN PINC
#define vilkkukytkinOikeaPORT PORTC

#define vilkkukytkinVasen 30
#define vilkkukytkinVasenBitti (1U << PC7)
#define vilkkukytkinVasenDDR DDRC
#define vilkkukytkinVasenPIN PINC
#define vilkkukytkinVasenPORT PORTC

#define vilkkukytkimetMaski (vilkkukytkinOikeaBitti | vilkkukytkinVasenBitti)
#define vilkkukytkimetPIN PINC

//ADC
#define bensaSensori A4 //Bensa sensori
#define bensaSensoriADC 4
#define bensaSensoriBitti (1U << PF4)
#define bensaSensoriDDR DDRF
#define bensaSensoriPIN PINF
#define bensaSensoriPORT PORTF
#define bensaSensoriDIDR DIDR0
#define bensaSensoriADCD (1U << ADC4D)

#define akkuJannite A0 //Akun j‰nnite
#define akkuJanniteADC 0
#define akkuJanniteBitti (1U << PF0)
#define akkuJanniteDDR DDRF
#define akkuJannitePIN PINF
#define akkuJannitePORT PORTF
#define akkuJanniteDIDR DIDR0
#define akkuJanniteADCD (1U << ADC0D)

//******Ulostulot

//PWM
#define rajoitusSSR 5 //Tappokytkimen kytkin
#define rajoitusSSRBitti (1U << PE3)
#define rajoitusSSRDDR DDRE
#define rajoitusSSRPIN PINE
#define rajoitusSSRPORT PORTE
#define rajoitusSSROCR OCR3A //kello3

//Digi ulostulo
#define vilkkureleVasen 22
#define vilkkureleVasenBitti (1U << PA0)
#define vilkkureleVasenDDR DDRA
#define vilkkureleVasenPIN PINA
#define vilkkureleVasenPORT PORTA

#define vilkkureleOikea 23
#define vilkkureleOikeaBitti (1U << PA1)
#define vilkkureleOikeaDDR DDRA
#define vilkkureleOikeaPIN PINA
#define vilkkureleOikeaPORT PORTA

#define vilkkureleetMaski (vilkkureleVasenBitti | vilkkureleOikeaBitti)
#define vilkkureleePORT PORTA

#define vaihtoKaskyAlas 24
#define vaihtoKaskyAlasBitti (1U << PA2)
#define vaihtoKaskyAlasDDR DDRA
#define vaihtoKaskyAlasPIN PINA
#define vaihtoKaskyAlasPORT PORTA

#define vaihtoKaskyYlos 25
#define vaihtoKaskyYlosBitti (1U << PA3)
#define vaihtoKaskyYlosDDR DDRA
#define vaihtoKaskyYlosPIN PINA
#define vaihtoKaskyYlosPORT PORTA

#define pakkipiipppi 26
#define pakkipiipppiBitti (1U << PA4)
#define pakkipiipppiDDR DDRA
#define pakkipiipppiPIN PINA
#define pakkipiipppiPORT PORTA

#define virtalukonOhistus 27
#define virtalukonOhistusBitti (1U << PA5)
#define virtalukonOhistusDDR DDRA
#define virtalukonOhistusPIN PINA
#define virtalukonOhistusPORT PORTA

//*****V‰yl‰t

//UART Duelle
//TODO Varmaan tariii jotain muutakin, jos tekee joskus oman UART-toteutuksen.
#define UARTDueRX 17
#define UARTDueRXBitti (1U << PH0)
#define UARTDueRXDDR DDRH
#define UARTDueRXPIN PINH
#define UARTDueRXPORT PORTH

#define UARTDueTX 16
#define UARTDueTXBitti (1U << PH1)
#define UARTDueTXDDR DDRH
#define UARTDueTXPIN PINH
#define UARTDueTXPORT PORTH

//I toiseen C
//TODO Jos t‰m‰nkin tekee ite, pit‰‰ tutkia lis‰‰
#define I2CKello 21
#define I2CKelloBitti (1U << PD0)
#define I2CKelloDDR DDRD
#define I2CKelloPIN PIND
#define I2CKelloPORT PORTD

#define I2CData 20
#define I2CDataBitti (1U << PD1)
#define I2CDataDDR DDRD
#define I2CDataPIN PIND
#define I2CDataPORT PORTD
//*****************************************************