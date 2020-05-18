/************************************
|   Konstante za smer kretanja      |
************************************/
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

/************************************
|   Konfiguracija GLCD-a            |
|   Izvor mikroe.com - glcd library |
************************************/
char GLCD_DataPort at PORTD;    // GLCD port za podatke
sbit GLCD_CS1 at LATB0_bit;     // Chip Select 1 line
sbit GLCD_CS2 at LATB1_bit;     // Chip Select 2 line
sbit GLCD_RS at LATB2_bit;      // Register select line
sbit GLCD_RW at LATB3_bit;      // Register Read/Write line
sbit GLCD_EN at LATB4_bit;      // Enable line
sbit GLCD_RST at LATB5_bit;     // Reset line
// Direction
sbit GLCD_CS1_Direction at TRISB0_bit;      // Direction of the Chip Select 1 pin
sbit GLCD_CS2_Direction at TRISB1_bit;      // Direction of the Chip Select 2 pin
sbit GLCD_RS_Direction at TRISB2_bit;       // Direction of the Register select pin
sbit GLCD_RW_Direction at TRISB3_bit;       // Direction of the Read/Write pin
sbit GLCD_EN_Direction at TRISB4_bit;       // Direction of the Enable pin
sbit GLCD_RST_Direction at TRISB5_bit;      // Direction of the Reset pin.

/************************************
|    Inicijalizacija/deklaracija    |
************************************/
// koordinate
int snakeCoordinateX[20];   // Vrednost koordinate X
int snakeCoordinateY[20];   // Vrednost koordinate Y
// fleg bitovi za tastere RC4, RC5, RC6, RC7
bit oldstate7,oldstate6,oldstate5,oldstate4,flag7,flag6,flag5,flag4;
// interrupt flag, prekid sa tajmera
int flag;
// stanje kretanja zmije -> UP, DOWN, RIGHT, LEFT
int direction;
// brojaci za for petlje
int i,j,k;
// x i y promenljive za proveru pozicije zmije
int x,y;

/***************************************
|   Interrupt rutina (prekid)          |
|   Tajmer 50ms generisan uz pomoc     |
|   app "Timer_Calculator_Build.4.0.0" |
***************************************/
// link za app https://libstock.mikroe.com/projects/view/398/timer-calculator
void interrupt() {             // glavni interrupt
    if (TMR0IF_bit) {
        TMR0IF_bit = 0;
        TMR0H = 0x3C;
        TMR0L = 0xB0;
        flag=1;
    }
}

/************************************
|   Funkcija barrier_collision      |
|   proverava da li je doslo do     |
|   sudara sa nekom od prepreka     |
************************************/
int barrier_collision(){

     // kriticne tacke - ivice prepreke u koju zmija moze da udari (po dokumentaciji za Glcd_Rectangle_Round_Edges_Fill treba nam
     // X,Y gore levo, i X,Y dole desno za crtanje tela, odnosno u ovom slucaju prepreke)
    int x_rect_1[11] = {10,11,12,13,14,15,16,17,18,19,20};
    int x_rect_2[21] = {54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74};
    int y_rect_1[11] = {10,11,12,13,14,15,16,17,18,19,20};
    int y_rect_2[21] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42};

    /************************************
    |          Manja prepreka           |
    ************************************/
    // proveravamo da li se desio sudar sa prvom(manjom) preprekom
    for(x = 0; x < 11; x++){
        for(y = 0; y < 11; y++){
                // usov koji proverava da li je doslo do sudara
                if(snakeCoordinateX[0] == x_rect_1[x] && snakeCoordinateY[0] == y_rect_1[y]){
                    return 1;
                }

        }
    }

    /************************************
    |           Veca prepreka           |
    ************************************/
    // proveravamo da li se desio sudar sa drugom(vecom) preprekom
    for(x = 0; x < 21; x++){
        for(y = 0; y < 21; y++){
                // usov koji proverava da li je doslo do sudara
                if(snakeCoordinateX[0] == x_rect_2[x] && snakeCoordinateY[0] == y_rect_2[y]){
                    return 1;
                }

        }
    }
    return 0;
}

/********************************************
|   Funkcija draw_barrier                   |
|   iscrtava prepreke na koordinatama       |
|   gornjih levih temena (54,22) i (10,10)  |
|   i dužinama stranica 20 i 10 respektivno |
********************************************/
void draw_barrier(){
    // Manja prepreka
    Glcd_Rectangle_Round_Edges_Fill(10,10,20,20,0,1);  //gore levo X,Y, dole desno X,Y, radius = 0, crna boja => 10x10
    // Veca prepreka
    Glcd_Rectangle_Round_Edges_Fill(54,22,74,42,0,1);  //gore levo X,Y, dole desno X,Y, radius = 0, crna boja => 20x20
}

/********************************************
|   Funkcija state_reset                    |
|   resetuje stanje zmije, odnosno niz sa   |
|   koordinatama postavlja na nule i vraca  |
|   zmiju na pocetnu poziciju.              |
********************************************/
void state_reset() {

    Glcd_Fill(0x00);   // "ciscenje" ekrana
    draw_barrier();    // crtanje prepreka nakon "ciscenja" ekrana

    // birsanje piksel po piksel
    for ( i = 19; i > 0; i-- ){
        snakeCoordinateX[i]=0; snakeCoordinateY[i]=0;
    }

     // setovanje na pocetnu poziciju
     snakeCoordinateX[0]=0;
     snakeCoordinateY[0]=0;
     // podrazumevano ide desno
     direction=RIGHT;

}


/********************************************
|   Funkcija pause_and_continue             |
|   pritiskom na taster RC5 - pauza,        |
|   ponovnim pritiskom na RC5 igra se       |
|   nastavlja od trenutka kada je pauzirana |
********************************************/
void pause_and_continue(){
    if(flag5 == 1) {     // proveravamo da li postoji zahtev za prekid
        flag5 = 0;
    }

    // provera - ukoliko je drugi put pritisnut taster RC5 zmija nastavlja sa kretanjem od trenutka kada je pauzirana
    do {
        
        for(k=0; k<20; k++){
            // crtanje zmije na poziciji gde se trenutno nalazi
            // parametri: koordinate X i Y, 1 = boja (0 - brise tacku, 1 crta, 2 invertuje)
            Glcd_Dot(snakeCoordinateX[k], snakeCoordinateY[k], 1); 

        }

        // &PORTC, 5, 1, 0 - referenca na port &PORTC, pin 5, duzina za detektovanje pritiska - vremensko ogranicenje(ms),
        // koliko se drzi taster da se detektuje prekid, 0 ili 1 odnosno HIGH ili LOW tj. da li je pritisnut ili ne
        // prakticno provera da li je dugme pritisnuto
        if (Button(&PORTC, 5, 1, 1)) {
            oldstate5 = 1;
        }

        // &PORTC, 5, 1, 0 - referenca na port &PORTC, pin 5, duzina za detektovanje pritiska - vremensko ogranicenje(ms),
        // koliko se drzi taster da se detektuje prekid, 0 ili 1 odnosno HIGH ili LOW tj. da li je pritisnut ili ne
        // na ovaj nacin se obezbedjuje da se izvrsi samo ukoliko je taster pritisnut SAMO jednom
        // oldstate se menja u odnosu na prethodni if kada je doslo do promene stanja, dugme je promenilo stanje iz 1 u 0
        if (oldstate5 && Button(&PORTC, 5, 1, 0)) {
            oldstate5 = 0;
            flag5 = 1;
        }

    }while(flag5==0); // dok se ne pritisne taster RC5 opet, fleg je setovan na 0

}

/***********************
|   Funkcija main()    |
***********************/
void main() {

    // ANSEL => ansel=1 (port analogni), ansel=0 (port digitalni)
    // Svi portovi koje koristimo su digitalni ulazi/izlazi
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;

    // registar TRISC u kom je PORTC postavljen u input rezim
    TRISC=0xFF;

    // konfiguracija za tajmer, tajmer kalkulator generise conf deo, ovo je potrebno da bi se prvi put izvrsilo sve
    T0CON = 0x80;
    TMR0H = 0x3C;
    TMR0L = 0xB0;
    // interrupt enable
    TMR0IE_bit = 1;


    // general interrupt enable, omogucava da se prekid izvrsava
    GIE_bit = 1;
    
    // mora da postoji inicijalizacija GLCD modula
    Glcd_Init();  

    // preventivno ciscenje ekrana da budemo sigurni da nije nista ostalo
    state_reset();

    // inicijalizacija flegova, svi su pocetno setovani na 0 dok se ne desi potreba za prekidom
    flag7=0;
    flag6=0;
    flag5=0;
    flag4=0;


    do {

        // Napomena: za svaki taster proveravamo uvek da li je taster pritisnut jednom, odnosno da li menja stanje

        /********************************
        |   pritisak na  taster RC7     |
        |   taster za kontrolu kretanja |
        ********************************/
        if (Button(&PORTC, 7, 1, 1)) {
            oldstate7 = 1;
        }
        if (oldstate7 && Button(&PORTC, 7, 1, 0)) {
            flag7 =1;
            oldstate7 = 0;
        }


        /********************************
        |   pritisak na  taster RC6     |
        |   taster za kontrolu kretanja |
        ********************************/
        if (Button(&PORTC, 6, 1, 1)) {
            oldstate6 = 1;
        }
        if (oldstate6 && Button(&PORTC, 6, 1, 0)) {
            flag6 =1;
            oldstate6 = 0;
        }


        /********************************
        |   pritisak na  taster RC5     |
        |   pauza ili nastavak          |
        ********************************/
        if (Button(&PORTC, 5, 1, 1)) {
            oldstate5 = 1;
        }
        if (oldstate5 && Button(&PORTC, 5, 1, 0)) {
            oldstate5 = 0;
            pause_and_continue();    // nije potrebno setovati fleg jer se provera desava u funkciji "pause_and_continue"
        }


        /********************************
        |   pritisak na  taster RC4     |
        |   ponovno pokretanje igre     |
        ********************************/
        if (Button(&PORTC, 4, 1, 1)) {
            oldstate4 = 1;
        }
        if (oldstate4 && Button(&PORTC, 4, 1, 0)) {
            flag4 =1;
            oldstate4 = 0;
        }



        // provera da li je doslo do prekida sa tajmera u interrupt rutini (bilo koji fleg utice na to se desi prekid)
        // ukoliko jeste izvrsava sve dole navedeno
        if (flag){   
          
            // brisanje zmije od repa da bi se videlo kretanje
            Glcd_Dot(snakeCoordinateX[19], snakeCoordinateY[19], 0);
            
            // proveravamo da li je doslo do kolizije sa preprekom, ili telom zmije, za svaku koordinatu
            for ( i = 19; i > 0; i-- ){

                /***********************
                |   tip kolizije:      |
                |   SA PREPREKOM       |
                ***********************/
                if(barrier_collision()) {
                    // "ciscenje" ekrana
                    Glcd_fill(0x00);
                    // poruke                                       // 55 jer je GLCD 128x64, da bi bilo negde na sredini
                    Glcd_Write_Text("Igra je zavrsena!", 55, 2, 1); // parametri: poruka, pozicija na ekranu, red, "boja"(0 -nista, 1- crno, 2-invertovano)
                    Glcd_Write_Text("Pocni novu igru pritiskom na RC4", 48, 4, 1);

                    // program ocekuje pritisak na taster RC4 nakon sto je poruka ispisana
                    do {
                        
                        if (Button(&PORTC, 4, 1, 1)) {
                             oldstate4 = 1;
                        }
                        if (oldstate4 && Button(&PORTC, 4, 1, 0)) {
                             flag4 =1;
                             oldstate4 = 0;
                        }


                    // program ocekuje promenu na flag4 kako bi nastavio dalje sa radom
                    } while(flag4 != 1);
                }


                /***********************
                |   tip kolizije:      |
                |   SA TELOM ZMIJE     |
                ***********************/                
                // provera da li je prva takca razlicita od poslednje, odnosno da li su koordinate glave i repa razlicite
                // bez ovoga igra bi odmah detektovala koliziju
                if(snakeCoordinateX[0] != snakeCoordinateX[19]){ 
                    if(snakeCoordinateX[0]==snakeCoordinateX[i] && snakeCoordinateY[0]==snakeCoordinateY[i]){// provera koordinata glave i repa

                        // "ciscenje" ekrana
                        Glcd_fill(0x00);
                        // poruke 
                        Glcd_Write_Text("Igra je zavrsena!", 55, 2, 1); // parametri: poruka, pozicija na ekranu, red, "boja"(0 -nista, 1- crno, 2-invertovano)
                        Glcd_Write_Text("Pocni novu igru pritiskom na RC4", 48, 4, 1);


                        do {
                            if (Button(&PORTC, 4, 1, 1)) {
                               oldstate4 = 1;
                            }
                            if (oldstate4 && Button(&PORTC, 4, 1, 0)) {
                               flag4 =1;
                               oldstate4 = 0;
                            }

                        } while(flag4 != 1);
                    }
                }
                   // kretanje zmije => prethodna koordinata se postavlja na trenutnu, trenutna na sledecu, npr 19 postaje 18 i td, tako se krece
                   snakeCoordinateX[i]=snakeCoordinateX[i-1];
                   snakeCoordinateY[i]=snakeCoordinateY[i-1];

            }

            /***********************
            |   Provera smera      |
            |   u kom se trenutno  |
            |   zmija krece        |
            ***********************/  
            switch (direction){
                
                
                /************
                |   GORE    |
                ************/
                // ukoliko se zmija krece u smeru ka GORE, onda se Y koordinata smanjuje
                case UP:
                    snakeCoordinateY[0]--;
                    // na ovaj nacin se omogucava da zmija prodje kroz zid, proverom trenutne koordinate i setovanjem sledece
                    if (snakeCoordinateY[0]<0) snakeCoordinateY[0]=63;
                    if (flag7){ // ako je doslo to pritiska na taster RC7 smer se menja u levo
                        direction = LEFT;
                        flag7=0;
                    }
                    if (flag6){ // ako je doslo to pritiska na taster RC6 smer se menja u desno
                        direction = RIGHT;
                        flag6 = 0;
                    }
                    if(flag4){ // ako je pritisnut taster RC4 pocinje nova igra
                        flag4 = 0;
                        state_reset();
                    }
                    break;

                /************
                |   DOLE    |
                ************/
                case DOWN:
                    snakeCoordinateY[0]++;
                    // visina glcd je 63
                    if (snakeCoordinateY[0]>63) snakeCoordinateY[0]=0;
                    if (flag7){
                        direction = RIGHT;
                        flag7=0;
                    }
                    if (flag6){
                        direction = LEFT;
                        flag6 = 0;
                    }
                    if(flag4){
                        flag4 = 0;
                        state_reset();
                    }
                    break;

                /************
                |   DESNO   |
                ************/
                case RIGHT:
                    snakeCoordinateX[0]++;
                    // sirina glcd je 127
                    if (snakeCoordinateX[0]>127) snakeCoordinateX[0]=0;
                    if (flag7){
                        direction = UP;
                        flag7=0;
                    }
                    if (flag6){
                        direction = DOWN;
                        flag6 = 0;
                    }
                    if(flag4){
                        flag4 = 0;
                        state_reset();
                    }
                    break;

                /************
                |   LEVO    |
                ************/
                case LEFT:
                    snakeCoordinateX[0]--;
                    if (snakeCoordinateX[0]<0) snakeCoordinateX[0]=127;
                    if (flag7){
                        direction = DOWN;
                        flag7=0;
                    }
                    if (flag6){
                        direction = UP;
                        flag6 = 0;
                    }
                    if(flag4){
                        flag4 = 0;
                        state_reset();
                    }
                    break;


            }

            // resetuje se flag za TMR0
            flag=0; 

            for ( j = 0; j < 20; j++ ){

                // crtanje tela zmije nakon svih provera, na mestu gde se trenutno nalazi
                Glcd_Dot(snakeCoordinateX[j], snakeCoordinateY[j], 1);
                
            }


        }

    } while(1);

}