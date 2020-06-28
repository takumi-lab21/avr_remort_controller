/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */


// 赤外線通信
// 関数でまとめてみた
// 温度操作に対応

// これめっちゃ使える．
#define SBI(reg,bit) reg |= (1 << bit)
#define CBI(reg,bit) reg &= ~(1 << bit)
// #define SW() (PINB & 0x0f) 
// #define F_CPU 8000000UL//内部クロックの周波数

#include <avr/io.h>
#include <util/delay.h>
// #include <avr/sleep.h>
#include <avr/interrupt.h>

#define SwitchPin1 0 //PD0
#define SwitchPin2 3 //PD3
#define SwitchPin3 4 //PB4
#define SwitchPin4 6 //PD6


#define OutPin 5 //PD5
#define DataNum 18

//バージョンが古い場合はこれがいるかも
// >違う，Cでコンパイルする場合にいる
// typedef unsigned char bool;
// #define true 1          // 閏年
// #define false 0         // 閏年でない

// PWMの値書き換え許可フラグ
volatile bool isPWMchangeable = false;



// ISR(TIMER1_OVF_vect)// タイマー１がBottomに戻った時の割り込み処理
// ISR(TIMER1_CAPT_vect)

ISR(TIMER1_COMPB_vect)
{

	CBI(TCCR0A, COM0B1);
	CBI(TCCR0A, COM0B0);

	//切り替えを許可
	isPWMchangeable=true;


}


ISR(TIMER1_OVF_vect)// Bottomに戻った時
{

	// ３５kHzのPWMを出力
	SBI(TCCR0A, COM0B1);
	CBI(TCCR0A, COM0B0);

	// PORTD ^= 0xff;

	
}



// 24度
int on_data_cool[DataNum]={// ONボタン 冷房
	0x23,
	0xCB,
	0x26,
	0x1,
	0x0,
	0x20,
	0x18,
	0x8,
	0x36,
	0x49,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0xD4
};
// int on_data_dry[DataNum]={// ONボタン ドライ
// 	0x23,
// 	0xCB,
// 	0x26,
// 	0x1,
// 	0x0,
// 	0x20,
// 	0x10,
// 	0x8,
// 	0x34,
// 	0x49,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0xCA
// };

// int on_data_warm[DataNum]={// ONボタン warm
// 	0x23,
// 	0xCB,
// 	0x26,
// 	0x1,
// 	0x0,
// 	0x20,
// 	0x8,
// 	0x8,
// 	0x30,
// 	0x49,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0x0,
// 	0xBE
// };

// Offボタン
int off_data[DataNum]={
	0x23,
	0xCB,
	0x26,
	0x1,
	0x0,
	0x0,
	0x18,
	0x8,
	0x36,
	0x49,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0x0,
	0xB4
};




int read_switch(){
	int state=0;
    // bool state_c;
	//switch の状態を取得
	if ( !(( PIND >> SwitchPin1) & 0b01) ) return 1;
    else if( !(( PIND >> SwitchPin2) & 0b01) ) return 2;
    else if( !(( PINB >> SwitchPin3) & 0b01) ) return 3;
    else if ( !(( PIND >> SwitchPin4) & 0b01) ) return 4;

	return state;
}



void send_leader(){
	

	// PWMの設定を更新可能になるまで待つ
	while(1){
      if(isPWMchangeable==true)break;
	}
	// 出力設定
	SBI(DDRD,DDD5);

	// リーダー
	OCR1A = 600;
	OCR1B = 400;

	//切り替えを禁止
	isPWMchangeable=false;

	

	// _delay_us(4400);
}
void send_1bit(bool bit){
	// PWMの設定を更新可能になるまで待つ
	while(1){
      if(isPWMchangeable==true)break;
	}
	// PWMの設定を更新
	if(bit==true){
		// on
		OCR1A = 200;
		OCR1B = 50;
	}else{
		// off
		OCR1A = 100;
		OCR1B = 50;
	}


	//切り替えを禁止
	isPWMchangeable=false;

}

void send_data(int state, int* data_array){ // 8bit*DataNum分のデータを送信
	int data;
	bool bit_cr=false;

	for (int i = 0; i < DataNum; ++i){
		data=data_array[i];
		for (int j = 0; j < 8; ++j){ //8bitを順番に送る
			bit_cr = (data>>j)&1;//j個目のbitを取得
			// 送信
			send_1bit(bit_cr);
		}
	}
}


void send_last_pulse(){
	// PWMの設定を更新可能になるまで待つ
	while(1){
      if(isPWMchangeable==true)break;
	}
	// PWMの設定を更新
	OCR1A = 212;
	OCR1B = 50;
	//切り替えを禁止
	isPWMchangeable=false;


	// _delay_us(1600);
}





void send(int state, int* data_array){

	CBI(DDRD,DDD5);//PD5//切断
	sei();//割り込み許可

	for (int k = 0; k < 2; ++k){// 二回繰り返す
		// 第k波
		send_leader();// リーダーコード
		
		send_data(state, data_array);// dataを送信

		send_last_pulse();// ラストのパルス
	}

	// 最後の波形がON->OFFになるまで待つ
	while(1){
      if(isPWMchangeable==true) break;
	}

	// 停止
	cli();
	CBI(TCCR0A, COM0B1);
	CBI(TCCR0A, COM0B0);
	
}

void Chage_EC_State(int state){
	// Switch ON
	


	if(state==1)send(state,on_data_cool);
	else if(state==2){
		// 温度上昇
		// 最大値：206
		if( on_data_cool[17] < 220){
			on_data_cool[DataNum-1]+=1;
			on_data_cool[7]+=1;
		}
		send(state,on_data_cool);
	}
	else if(state==3){
		// 温度低下
		// 最小値：206
		if( on_data_cool[17] > 206){
			on_data_cool[7]-=1;
			on_data_cool[17]-=1;
		}
		send(state,on_data_cool);
	}
	else if(state==4)send(state,off_data);//Switch OFF

}

// void set_temperature(int direction){
// 	// 温度上昇
// 	if(direction==0)


// }



// bool state=false;//センサの状態

int main(){

	//スイッチピンの設定
	CBI(DDRD, SwitchPin1);//入力状態
    SBI(PORTD, SwitchPin1);//HIGHにする．

    CBI(DDRD, SwitchPin2);//入力状態
    SBI(PORTD, SwitchPin2);//HIGHにする．

    CBI(DDRB, SwitchPin3);//入力状態
    SBI(PORTB, SwitchPin3);//HIGHにする．

    CBI(DDRD, SwitchPin4);//入力状態
    SBI(PORTD, SwitchPin4);//HIGHにする．

    
	// 16bit Timer
	// 出力ピン PB5
	SBI(DDRD, DDD5);

	// タイマー０の設定．35kHzでLEDを点滅させる
	// stp = 0.125us(1分周）
	TCCR0A = (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << WGM02) | (0 << CS02) | (0 << CS01) | (1 << CS00);

	// 周波数＝38kHz になるように設定．
	OCR0A = 212;
	OCR0B = 68;//OCR0B = 68;

	// 切断
	CBI(TCCR0A, COM0B1);
	CBI(TCCR0A, COM0B0);


	// タイマー１の設定．35kHzの点灯をオンオフする．
	// 64分周
	// stp = 8us
	TCCR1A = (1 << WGM11) | (1 << WGM10);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (1 << CS10);

	OCR1A = 0;
	OCR1B = 0;


	// 割り込み設定
	// TIMSK = 0b10100000;//タイマー割り込みの許可
	//タイマー割り込みの許可
	TIMSK1 = (1 <<  TOIE1) | (1<< OCIE1B);

	// 切断
	CBI(TCCR1A, COM1B1);
	CBI(TCCR1A, COM1B0);


	int state;//センサの状態
	// データベクトル保存用ポインタ
	int* data_array;// 配列格納用ポインタ
	while(1){
		state = 0;
		state=read_switch();
		// state = 1;
		if(state){
			// データを獲得
	  		Chage_EC_State(state);
	    	_delay_us(500000);


			// send_leader();// リーダーコード
	  	}
	}
	




 //    TCCR1A = (1 << WGM11) | (1 << WGM10);
	// TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (1 << CS10);

	// // 割り込み許可
 //    TIMSK1 = (1 <<  ICIE1) | (1<< OCIE1B);


	// OCR1A = 400;
	// OCR1B = 200;

	// SBI(DDRD, DDD6);
	// sei();//割り込み許可



	return 0;
}

/*
便利なコードをメモしておくよ
・指定したbitをONする
#define SBI(reg,bit) reg |= (1 << bit)
・指定したbitをOFFする
#define CBI(reg,bit) reg &= ~(1 << bit)


ビットの反転 ＞＞ LEDのオンオフなど
PORTB ^= 0b00010000;


*/



