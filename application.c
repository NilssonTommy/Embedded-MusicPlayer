#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sioTinyTimber.h"
 /*
***********Welcome to RTSB5 Musicplayer*************
* Press p to play brother John melody
* Press s to stop brother John melody
* Press m to mute
* Press i to increase volume (MAX volume 20)
* Press d down to decrease volume (MIN volume 1)
* Choose value between 60 and 240 and press t to change tempo
* Choose value between -5 and 5 and press k to change pitch
* Press TAB to switch between conductor mode and musician mode
* Press USER button four times to switch tempo between [30,300] BPM
* Hold USER button 1000ms to enter hold mode.
* Hold USER button 2000ms to RESET and set tempo to 120BPM.
 */
#define MUSICIAN_MODE 0
#define CONDUCTOR_MODE 1
 
#define MELODY  {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0}
#define PERIOD {2025,1911,1804,1703,1607,1517,1432,1351,1276,1204,1136,1073,1012,956,902,851,804,758,716,676,638,602,568,536,506}
#define BEAT {'a', 'a', 'a','a', 'a', 'a', 'a', 'a', 'a', 'a', 'b', 'a', 'a', 'b', 'c', 'c','c','c', 'a', 'a','c', 'c', 'c', 'c', 'a', 'a','a','a','b', 'a', 'a','b'}
#define max_index 14
#define min_index -10

#define DAC_register ((unsigned volatile char*)0x4000741C)

#define initmsg {1, 1, 6, {}}


typedef struct {
    Object super;
	char buffer[128];  // Lagrar in siffror från knapptryck
	int index;//används i buffer
	int brother_john_melody[32];
	int period[25];
	char beat[32];
	int tempo[32];
	int index_pb;
	int melody_key[32];
	int flag;
	int mode;
	CANMsg msg;
	Timer timer_Momentary;
	Time time_Momentary;
	Timer timer_Hold;
	Time time_Hold;
	int button;
	int user_Tempo[3];
	int user_Index;
	int button_value;
	int LED_flag;
	int LED_start;
	int flag_Hold;
	Timer timer_LED;
	Time time_LED;
	int LED_flag_a;
	int LED_flag_a2;
	int LED_flag_b;
	int LED_flag_b2;
	int LED_flag_c;

}App;

typedef struct {
	Object super;
	int volume;
	int mute;
	int restore_volume;
	int tone_generator_flag;
}Tone;


void reader(App*, int);
void close_tone(Tone*, int);
void Tempo(App*, int);
void Tone_Generator(Tone*, int );
void Delay_Gap(App*, int );
void Change_Mode(App*, int );
void receiver(App*, int);
void Start_Musicplayer(App*, int );
void Stop_Musicplayer(App*, int );
void USER_Call_back(App*, int );
void Check_Time(App *, int);
void USER_Tempo(App *, int );
void LED_Blinking(App *, int );

Tone tone = { initObject(), 5, 0, 0, 5};

App app = { initObject(), {}, 0, MELODY, PERIOD, BEAT, {}, 0, MELODY, 0, 1, initmsg, initTimer(), 0,initTimer(), 0, 0, {}, 0, 0, 1, 0, 0, initTimer(), 0,0,0,0, 0, 0}; // Deklarerar och initierar ett objekt.

Can can0 = initCan(CAN_PORT0, &app, receiver);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

SysIO sio0 = initSysIO(SIO_PORT0, &app, USER_Call_back);

// Skriver ut till Coolterm
void print(char *format, int c){
	char buffer[128];
	snprintf(buffer, 128, format, c); // lagrar format i buffer
	SCI_WRITE(&sci0, buffer); // skriver ut buffer till Coolterm
}

void Change_Mode(App *self, int unused){
	
	self->mode ^= 1;
		if( 0 == self->mode)
			print("Musician mode on\n", 0);
		else
			print("Conductor mode on\n", 0);

}
void Increase_Volume(Tone *self, int unused){
	
	if( self->volume < 20)
		self->volume++;	
	if( 1 == self->mute){
		self->volume = self->restore_volume;
		self->mute = 0;
	}
	print("Current Volume: %d\n", self->volume);
}
	
void Decrease_Volume(Tone *self, int unused){
	
	if( self->volume > 1)
		self->volume--;
		
	if( 1 == self->mute){
		self->volume = self->restore_volume;
		self->mute = 0;
	}
	print("Current Volume: %d\n", self->volume);
}

void Mute_Volume(Tone *self, int unused){
	

	self->mute ^= 1;

	if(self->mute){
		self->restore_volume = self->volume;
		self->volume = 0;
	}
	else
		self->volume = self->restore_volume;
		
	if(self->mute)
		print("Current Volume: Mute\n", 0);
	else
		print("Current Volume: %d\n", self->volume);
}

void Key(App *self, int key){
	
	for(int i = 0; i<32; i++){
	self->melody_key[i] = key + self->brother_john_melody[i];
	self->melody_key[i] = self->melody_key[i] + 10;
	}
}



void Tempo(App *self, int tempo){
	
	if( (60000000/tempo) == self->tempo[0]) return;
	
	self->LED_start = 1;
	

	self->time_LED = 10*T_SAMPLE(&(self->timer_LED));
	
	if(self->beat[self->index_pb-1] == 'a'){
		if( self->time_LED < self->tempo[0]/2){
			self->LED_flag_a = 1;
		
		}else{
			self->LED_flag_a2 = 1;

		}
	}
	
	if(self->beat[self->index_pb-1] == 'b'){
		if(self->time_LED >= (self->tempo[0] + (self->tempo[0]/2))){
			self->LED_flag_b2 = 1;

		}else{
			self->LED_flag_b = 1;

		}
	}
	
	if(self->beat[self->index_pb-1] == 'c'){
		self->LED_flag_c = 1;
	}

	for(int i = 0; i<32; i++){
	if('a' == self->beat[i])
		self->tempo[i] = (60000000/tempo);
	if('b' == self->beat[i])
		self->tempo[i] = (60000000/tempo)*2;
	if('c' == self->beat[i])
		self->tempo[i] = (60000000/tempo)/2;
	}
	
	
}

void MusicPlayer(App *self, int unsued){
	
	if(self->flag){
		
	if(self->index_pb == 32)
		self->index_pb = 0;
	
	T_RESET(&(self->timer_LED));
	
	SEND(0,USEC(150),&tone, close_tone, 0);

	SEND(0,USEC(200),&tone, Tone_Generator, self->period[self->melody_key[self->index_pb]]); 
	
	SEND(USEC(self->tempo[self->index_pb]-70000),USEC(75), &tone, close_tone, 1);

	if(self->LED_start){
	
	self->LED_start = 0;

	SEND(0,USEC(100), self, LED_Blinking, self->tempo[0]/2);
	}

	SEND(USEC(self->tempo[self->index_pb++]),USEC(300), self, MusicPlayer, 0);

	}
}
//self->tempo[self->index_pb++]
//self->period[self->melody_key[self->index_pb]
void close_tone(Tone *self, int flag){
	
	self->tone_generator_flag = flag;
	
}

void LED_Blinking(App *self, int tempo){
	if( self->LED_flag == 1){
		return;
	}
	if( (self->LED_flag_a2 == 1) && (tempo != self->tempo[0]/2)){
		self->LED_flag_a2 = 0;
		return;
	}
	if( (self->LED_flag_b2 == 1) && ( tempo != self->tempo[0]/2)){
		self->LED_flag_b2 = 0;
		return;
	}


	if( (self->LED_flag_c == 1) && (tempo != self->tempo[0]/2) ){
		self->LED_flag_c = 0;
		return;
	}

	
	SIO_TOGGLE(&sio0);
	
	
	if( self->LED_flag_a == 1){
		self->LED_flag_a = 0;
		return;
	}

	if(self->LED_flag_b == 1){
		self->time_LED = T_SAMPLE(&(self->timer_LED))/100;


		if( self->time_LED  >= (tempo*3)/1000 ){
			self->LED_flag_b = 0;
			return;
		}
	}
	
	SEND(USEC(tempo),USEC(120), self, LED_Blinking, tempo);
	
}


void Start_Musicplayer(App *self, int unused){
	
	SIO_WRITE(&sio0,1);

	self->LED_start = 1;
	self->LED_flag = 0;
	self->index_pb = 0;
	self->flag = 1;
	SEND(0,USEC(50),self, Tempo, 120);
	SEND(0,USEC(75),self, Key, 0);
	SEND(0,USEC(100),self,MusicPlayer, 0);
	SEND(0,USEC(150),&tone, close_tone, 0);
	
}



void Stop_Musicplayer(App *self, int ){
	
	self->LED_flag = 1;
	self->flag = 0;
	SIO_WRITE(&sio0,0);
	SEND(0,USEC(50),self,MusicPlayer, 0);
	SEND(0,USEC(75),&tone, close_tone, 1);

	
}


void Tone_Generator(Tone *self, int period){
	
	if(self->tone_generator_flag)
		return;

	if(*DAC_register == 0)
		*DAC_register = self->volume;
	else
		*DAC_register = 0;

	SEND(USEC(period),USEC(150), self, Tone_Generator, period);
}

void reader(App *self, int c) {

	if(c == '\n')
		return;
	
	switch(c){
		case 't':
			self->buffer[self->index] = '\0';
			if( (60 <= atoi(self->buffer)) && (240 >= atoi(self->buffer))){
				self->msg.msgId = c;
				self->msg.buff[0] = self->buffer[0];
				self->msg.buff[1] = self->buffer[1];
				self->msg.buff[2] = self->buffer[2];
				self->msg.buff[self->index] = 0;
				CAN_SEND(&can0, &(self->msg));
				if(self->mode){
				print("Conductor mode: Choosen BPM: %d \n", atoi(self->buffer));
				SEND(0,USEC(20),self,Tempo,atoi(self->buffer));
				}
			}
			else
			print("Only intervall [60, 240] allowed\n", c);
			self->index = 0;
		break;
		case 'k':
			self->buffer[self->index] = '\0';
			if( (5 >= atoi(self->buffer)) && (-5 <= atoi(self->buffer))){
				self->msg.buff[0] = self->buffer[0];
				self->msg.buff[1] = self->buffer[1];
				self->msg.buff[self->index] = 0;
				self->msg.msgId = c;
				CAN_SEND(&can0, &(self->msg));
				if(self->mode){
					print("Conductor mode: Choosen key: %d \n", atoi(self->buffer));
					SEND(0,USEC(100),self,Key,atoi(self->buffer));
				}
			}
			else
				print("Conductor mode: Only intervall [-5, 5] allowed\n", c);
			self->index = 0;
		break;
		case 'd':
			self->msg.msgId = 'd';
			self->msg.buff[0] = 0;
			CAN_SEND(&can0, &(self->msg));
			if(self->mode){
				print("Conductor mode: ", 0);
				SEND(0,USEC(50),&tone,Decrease_Volume,0);
			}
		break;
		case 'i':
			self->msg.msgId = 'i';
			self->msg.buff[0] = 0;
			CAN_SEND(&can0, &(self->msg));
			if(self->mode){
				print("Conductor mode: ", 0);
				SEND(0,USEC(50),&tone,Increase_Volume,0);
			}
		break;
		case 'm':
			self->msg.msgId = c;
			self->msg.buff[0] = 0;
			CAN_SEND(&can0, &(self->msg));
		if(self->mode){
				print("Conductor mode: ", 0);
				SEND(0,USEC(50),&tone,Mute_Volume,0);
		}
		break;
		case 'p':
			if(self->flag) break;
			self->msg.msgId = c;
			self->msg.buff[0] = 0;
			CAN_SEND(&can0, &(self->msg));
			if(self->mode){
			ASYNC(self, Start_Musicplayer,0);
			print("Conductor mode: Musicplayer is playing\n", 0);
			}
		break;
		case 's':
			if(!self->flag) break;
			self->msg.msgId = c;
			self->msg.buff[0] = 0;
			CAN_SEND(&can0, &(self->msg));
			if(self->mode){
				ASYNC(self, Stop_Musicplayer,0);
				print("Conductor mode: Musicplayer stopped\n", 0);
			}
		break;
		case 9:
			ASYNC(self, Change_Mode, 0);
		break;
		default:
			if((c >= '0' && c <= '9') || (c =='-')){ // gör så vi bara tar emot siffror och "-"
			print("Pressed key: '%c'\n", c);
			self->buffer[self->index++] = c; //lagrar knapptrycket i buffer.
			}
			else{
				print("Press m to mute\nPress + to increase volume\nPress - to decrease volume \n", 0);
				print("Choose value between 60 and 240 and press t to change tempo\n", 0);
				print("Choose value between -5 and 5 and press k to change pitch\n", 0);
				print("Press TAB to switch between conductor mode and musician mode\n", 0);
			}
		break;
	}
}


void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
	
	if(self->mode){
    SCI_WRITE(&sci0, "Can msg received: ");
	SCI_WRITE(&sci0, msg.buff);
	print("   msgId: %c \n", msg.msgId);
	}
	

	if(!self->mode){
	switch(msg.msgId){
		case 't':
				print("Musician mode: Choosen BPM: %d \n", atoi((char*)msg.buff));
				SEND(0,USEC(100),self,Tempo, atoi((char*)msg.buff));
		break;
		case 'k':
				print("Musician mode: Choosen key: %d \n", atoi((char*)msg.buff));
				SEND(0,USEC(100),self,Key,atoi((char*)msg.buff));
		break;
		case 'd':
			print("Musician mode: ", 0);
				SEND(0,USEC(50),&tone,Decrease_Volume,0);
		break;
		case 'i':
				print("Musician mode: ", 0);
				SEND(0,USEC(50),&tone,Increase_Volume,0);
		break;
		case 'm':
				print("Musician mode: ", 0);
				SEND(0,USEC(50),&tone,Mute_Volume,0);
		break;
		case 'p':
				if(self->flag) break;
				ASYNC(self, Start_Musicplayer,0);
				self->LED_flag = 1;
				print("Musician mode: Musicplayer playing\n", 0);
		break;
		case 's':
			if(!self->flag) break;
				ASYNC(self, Stop_Musicplayer,0);
				self->LED_flag = 0;
				print("Musician mode: Musicplayer stopped\n", 0);
		break;
		
	}
	}
}

void USER_Tempo(App *self, int unused){
	
	int averageBPM;
	averageBPM = 60000/((self->user_Tempo[0] + self->user_Tempo[1] + self->user_Tempo[2])/3);
	if((abs(self->user_Tempo[0] - self->user_Tempo[1]) > 100) || (abs(self->user_Tempo[0] - self->user_Tempo[2]) > 100)){
		print("Inter-arrival times differ more than 100 ms\n", 0);
		return;
		}
	if(averageBPM > 300 || averageBPM < 30){
		print("BPM to high/low : %d\n", averageBPM);
		return;
	}
	SEND(0,USEC(20),self,Tempo,averageBPM);
	print("Setting new BPM %d\n", averageBPM);
}

void Check_Time(App *self, int value){
	if(self->button_value == value){
		if(!self->button){
			self->time_Hold = T_SAMPLE(&(self->timer_Hold));
			if((self->time_Hold/100 >= 1000) && (self->time_Hold/100 < 2000)){
				print("Entering hold mode!  Time passed: %d ms\n", self->time_Hold/100);
				self->flag_Hold = 1;
			}
			if(self->time_Hold/100 >= 2000){
				print("RESET! BPM = 120 Time passed: %d ms\n", self->time_Hold/100);
				SEND(0,USEC(20),self,Tempo,120);
				self->time_Momentary = 0;
				self->user_Index = 0;
				self->button_value = 0;
				
			}
		}
	}
}

void USER_Call_back(App* self, int unsued){
	
	self->button = SIO_READ(&sio0);
	
	if(!self->button){
		SIO_TRIG(&sio0, 1);
		T_RESET(&(self->timer_Hold));
		SEND(MSEC(1000), USEC(40), self, Check_Time, self->button_value);
		SEND(MSEC(2000), USEC(40), self, Check_Time, self->button_value);

	}else if(self->time_Momentary == 0){
		
		SIO_TRIG(&sio0, 0);
		
		if(self->flag_Hold){
		self->time_Hold = T_SAMPLE(&(self->timer_Hold));
		print("Leaving hold mode! Time passed: %d ms\n", self->time_Hold/100);
		T_RESET(&(self->timer_Hold));
		self->flag_Hold = 0;
			if((self->time_Hold/100) >= 2000)return;
		}
		
		T_RESET(&(self->timer_Momentary));
		self->time_Momentary = 1;
		print("Button pressed 1 time\n", 0);
		self->button_value++;

	}else{
		
		if(self->flag_Hold){
		self->time_Hold = T_SAMPLE(&(self->timer_Hold));
		print("Leaving hold mode! Time passed: %d ms\n", self->time_Hold/100);
		T_RESET(&(self->timer_Hold));
		self->flag_Hold = 0;
		}
		self->button_value++;
		self->time_Momentary = T_SAMPLE(&(self->timer_Momentary));
		if(self->time_Momentary/100 >= 100){
			T_RESET(&(self->timer_Momentary));
			if(self->user_Index < 3){
				print("%d Inter-arrival time: ", self->user_Index+1);
				print("%d ms\n", self->time_Momentary/100);
				print("Button pressed %d times\n",  self->user_Index+2);
				self->user_Tempo[self->user_Index++] = self->time_Momentary/100;
			}
		}else{
			print("Contact bounce inter-arrival < 100ms\n", 0);
		}
			

		if(self->user_Index == 3){
			ASYNC(self,USER_Tempo, 0);
			self->user_Index = 0;
			self->time_Momentary = 0;
		}
			SIO_TRIG(&sio0, 0);
	}
}

		




void startApp(App *self, int arg) {

	CAN_INIT(&can0);
	SCI_INIT(&sci0);
	SIO_INIT(&sio0);
	SIO_TRIG(&sio0, 0);

	
}
	


int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
	INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}


