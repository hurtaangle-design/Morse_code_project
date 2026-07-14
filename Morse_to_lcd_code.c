#include <REG52.H>
#include <string.h>
#include <intrins.h>
char code char_dict[]={
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
  'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5','6','7','8','9',
	'?','!','.',',',';',':','+','-','/','='
};

char code * code morse_dict[]={
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", 
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", 
    "..-", "...-", ".--", "-..-", "-.--", "--..", // End of Alphabet A->Z
    "-----", ".----", "..---", "...--", "....-",  // 0 -> 4
    ".....", "-....", "--...", "---..", "----.",//4-->9
		"..--..", "-.-.--", ".-.-.-", "--..--", "-.-.-.",//start is ? last one is ;
		"---...", ".-.-.", "-....-", "-..-.", "-...-"//last of characters

};


volatile unsigned char buzzer_active = 0;//determines if buzz or not
volatile unsigned int isr_tick_count = 0;// ticks which will help to determine dots and dashes
unsigned int button_hold_time = 0;// we will use this to compare if it is a dot or dash
char current_morse[8];
unsigned int morse_index = 0;
unsigned char bottom_cursor = 0;
void delay_ms(unsigned int ms){
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 112; j++); ///dela
}

void lcd_cmd(unsigned char data_byte){
	P3&=~(1<<5);//P3.5(RS) = 0(cmd reg)
	P3&=~(1<<6);// P3.6(RW)/ = 0 write
	P2= data_byte;//P2.0-P2.7
	P3|=(1<<7);// (E) P3.7
	_nop_();
	P3&=~(1<<7);//(E) P3.7
	delay_ms(1);
}  

void lcd_data (unsigned char data_byte){
	P3|=(1<<5);//P3.5(RS) = 1(cmd reg)
	P3&=~(1<<6);// P3.6(RW) = 0 write
	P2=data_byte;//P2.0-P2.7
	P3|=(1<<7);    //(E) P3.7
	_nop_();
	P3&=~(1<<7); //(E) P3.7
	delay_ms(1);
}

void Isr_TO(void) interrupt 1 {
	isr_tick_count++;
	if(buzzer_active){
	
		//We will maintain the status of a 250us wave but only want to act at every 500 us for the buzz
		// So we use the Isr_count to hold how many isr's go off and at a even num like the count being 2
		// shows that we have gone off at 500us so it acts at this point 
		//we are compensating essentailly and compromising with that fact we onlt have a clk in 8 bit mode that counts to 256
		//Since we want the 500us wave where are a 1000us cycle to create a 1Khz frequency for the buzzer 
		if(isr_tick_count % 2 ==0){//this counts for when 2 intterrupts go off
			P1^=0x01;//toggle p1.0
		}
	}else{
		P1&=~0x01;//buzzer off garunteed
	}	
}


void initalize_system(void){
		P3|=(0x1C);       // set up P3.2 and P3.3 for buttons
    P1&=~(0x01);      // buz is off P1.0
    P1|=(1<<4);       // led is OFF attached to P1.4
		TH0=0x06;
		TL0=0x06;
	  TMOD=(TMOD & 0XF0)|(0X02);//first clear and set up
	//we are using timer 0 in 8 bit auto reload
	  
	  IE|=0x82;//ET0 is one and EA is one
		TCON|=(1<<4);

}
void lcd_print_string(char *str) {//handling funcion
    unsigned int i = 0;
    while (str[i] != '\0') {
        lcd_data(str[i]);
        i++;
    }
}

void lcd_update_top_row(void){
  lcd_cmd(0x80);
  lcd_print_string(current_morse);
	lcd_print_string("        ");
}
void decode_and_print() {
  unsigned int i;
	unsigned char matched_char = '?';
	for(i=0; i<46;i++){
		if(strcmp(current_morse, morse_dict[i]) == 0){
		    matched_char = char_dict[i];
		    break;	
		}
	}
	
	
	lcd_cmd(0xC0 + bottom_cursor);	
  lcd_data(matched_char);
	
	bottom_cursor++;
	if(bottom_cursor>=16){
		bottom_cursor= 0;
	}
	
	morse_index = 0;
  current_morse[0] = '\0';
}	
	
void main (void){
	unsigned int start_tick; 
	initalize_system();
	
	delay_ms(50);
	lcd_cmd(0x38);	
	delay_ms(50);
	
	lcd_cmd(0x38);
	lcd_cmd(0x06);
	lcd_cmd(0x0E);
	lcd_cmd(0x01);
	delay_ms(2);

	
	while(1){
		if ((P3 & 0x04) == 0) { //new morse p3.2
			delay_ms(20);
			if ((P3 & 0x04) == 0) {    // Morse button P3.2 is pressed        
				buzzer_active=1;// isr deals with sound now buzz is one
				P1 &= ~(1<<4);//led on P1.4 is 0 low_lvl_logic
				button_hold_time = 0;
				start_tick = isr_tick_count;//we create the start tick
		
				while ((P3& 0x04)==0);
				
				IE&=~(0x80);//EA=0
				button_hold_time=(isr_tick_count-start_tick); 
				IE|=(0x80);//EA=1
				
				buzzer_active = 0;
				P1|= (1<<4);//led off 
		 
				delay_ms(20);

				if(button_hold_time<800){
					current_morse[morse_index] = '.';
					}else{
					current_morse[morse_index] = '-';
				}
			
				if (morse_index < 6) { 
				morse_index++;
				}
				current_morse[morse_index]= '\0';
		
				lcd_update_top_row();
		
			}
		}		
		
		if ((P3 & 0x08)== 0){//new enter button p3.3
			delay_ms(20);
			if ((P3 & 0x08)== 0){
				if(morse_index>0){
					decode_and_print();
					lcd_update_top_row();
				}
				while((P3 & 0x08)==0);
				delay_ms(20);
			} 
		}
		if ((P3 & 0x10) == 0) { //new clear button
			delay_ms(20); //hold
			if ((P3 & 0x10) == 0) {
        start_tick = isr_tick_count;
        
        while ((P3 & 0x10) == 0); // Wait for user to let go
        
        IE &= ~(0x80);//interupt off
        button_hold_time = (isr_tick_count - start_tick); //counts the ticks
        IE |= (0x80);//interrupt back on  
        delay_ms(20); //hold delay
        if (button_hold_time < 800) {//if the button is pressed it will just clear the top row
           
            morse_index = 0;
            current_morse[0] = '\0';
            lcd_update_top_row();
        } 
        else {
            morse_index = 0;//if held everything is cleared
            current_morse[0] = '\0';
            bottom_cursor = 0;
            
            lcd_cmd(0x01);
            delay_ms(2);  
            lcd_update_top_row();
        }
    }
}		
		
		
		
		
	
	}
}

