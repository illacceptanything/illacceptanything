#include<iostream>
#include<string>
#include<vector>
#include<stdlib.h>
#include<stdio.h>
#include<sys/ioctl.h>
#include<unistd.h>

struct winsize w;

void gotoxy(int x,int y);
void clear();
void adjustH();
void adjustW();
void doge();

using namespace std;
int main(){
	int x,y,r1,r2,v1,v2;
	srand((unsigned)time(NULL) );	
	std::vector<std::string> manyTexts;
	manyTexts.push_back("AMAZE");
	manyTexts.push_back("WOW");
	manyTexts.push_back("SO COLOR");
	manyTexts.push_back("TO THE MOON");
	manyTexts.push_back("AWESOME");
	manyTexts.push_back("MANY DATA");
	manyTexts.push_back("SO EXCITE");
	manyTexts.push_back("VERY TERMINAL");
	manyTexts.push_back("MANY COLORS");
	manyTexts.push_back("SUCH PROGRAM");
	manyTexts.push_back("MANY CODE");
	manyTexts.push_back("SO ASCII");
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	while(1){
		v1 = rand() % 8;
		r1 = v1 > 0 ? v1 + 41 : 0;
		if(r1!=r2){
			cout << "\x1b[" << r1 << "m" << endl;
			doge();
		}
		for(int i=0;i<10;i++){
			v2 = rand() % 8;
			r2 = v2 > 0 ? v2 + 31 : 0;
			if(r2!=r1){
				cout << "\x1b[" << r2 << "m" << endl;
				x = rand()%w.ws_row;
				y = rand()%w.ws_col;
				gotoxy(x,y);
				printf("%s",manyTexts[rand()%manyTexts.size()].c_str());
				usleep(1000000);
			}
		}
	}
	return 0;
}

void gotoxy(int x,int y){
    printf("%c[%d;%df",0x1B,x,y);
}

void clear(){
	system("clear");
}

void adjustH(){
	for(int i=0;i<(w.ws_row-49)/2;i++){
		printf("\n");
	}
}

void adjustW(){
	for(int i=0;i<(w.ws_col-99)/2;i++){
		printf(" ");
	}
}

void doge(){
	clear();
	adjustH();
	adjustW();
	printf("                                                                                                    \n");
	adjustW();
	printf("                                                               ````                                 \n");
	adjustW();
	printf("                                                             `/yMMy                                 \n");
	adjustW();
	printf("                 `oooooo`                                   :sMmyMy                                 \n");
	adjustW();
	printf("                odMdyydMdo                                  yN+++ms                                 \n");
	adjustW();
	printf("              `-dNso+++omNN/.                            .+Nmd:/+hdN.                               \n");
	adjustW();
	printf("              /Nmdo++oo++sddh///                       ./mNh+:://+yM.                               \n");
	adjustW();
	printf("              /Myo+ooooo++++oymN                      sdMyo::::::+yNy+                              \n");
	adjustW();
	printf("              /My+ooooooooo+++ymh+`   `````````:hhhhhhNh+::::::::++oNd`                             \n");
	adjustW();
	printf("             -oMs+ooooooooooo+++syd:--hNddddddddmo/////::::::::::///hdN:                            \n");
	adjustW();
	printf("             dNds++oshhhssooooooooohhyhy::::::::::::::::::::::::::::/oMs/                           \n");
	adjustW();
	printf("             mdo+++osMMMmhosooooooo//:::::::::::::::::::::::::::::::::omMy:                         \n");
	adjustW();
	printf("             md+++ossMMMMmssssoooo+::::::::::::::::::::::::::::::::::::///shhdm:.                   \n");
	adjustW();
	printf("             md+++ossmNMMMNhysoo+/::::::::::::::::::::::::::::::::::::::::://odhy:::                \n");
	adjustW();
	printf("             md+++oossydMMMMms+/::::::::::::::::::::::::::::::::::::::::::::::::/ymN                \n");
	adjustW();
	printf("             md+++ooosssNNNhs/::::::::::::::::::::::::::::::::::::::::::/:::::::::hN`               \n");
	adjustW();
	printf("             md+++ooossoo+/::::::::::::::::::::::::::::::::::::::::::/+symy///:::://m+              \n");
	adjustW();
	printf("             md+++oooo+/::::::::::::::::::::::::::::::::::::::::::::oos++MMNNh::::::do/             \n");
	adjustW();
	printf("             md+++o++::::::::::::::::::::::::/syy+::::::::::::::::::dmomMMMMMh/::::::yM`            \n");
	adjustW();
	printf("           ``md+++/::::::::::::::::::::::::/sdmNMmddo:::::::::::::::++odmNNNo+/::::::/+h+           \n");
	adjustW();
	printf("           +mmh+++:::::::::::::::::::::::/+mmd+sMMMMNm+/:::::::::::::::/++++/::::::::::Ny           \n");
	adjustW();
	printf("           oMhhyso::::::::---------::::::yNMs`dNMMMMMMhs/:::::::::::::::::::::::::-----hs/`         \n");
	adjustW();
	printf("           oMMms/:::::::-............--::+ssssNMMMMMMM///::::::::::::::::::::::---......oM-         \n");
	adjustW();
	printf("          .sMo/::::::--...````````````.----:/++++ooooo//::::::::::::---:////:::.````````+M:         \n");
	adjustW();
	printf("          mmd/:::::::...````````````````...-------:::::::::::::::---://ydmmNNmm/:```````/d/-        \n");
	adjustW();
	printf("          My:::::::::..````````````````````........--------------...dNMMMMMMMMMMd`````````dd        \n");
	adjustW();
	printf("        +yNy:::::::--..`````````````````````````````````````````````mMMMMMMMMMMMd`````````dd        \n");
	adjustW();
	printf("        yM+/:::::::-...`````````````````````````````````````````````hmmNMMMMMMMMd`````````dd        \n");
	adjustW();
	printf("      `:hM:::::::::--..`````````````````````````````````````````````...sddmMMNddy`````````dd        \n");
	adjustW();
	printf("      .Mdy:::::::::::..```````````````````````````````````````````````````:ss/````````````dd        \n");
	adjustW();
	printf("      .Mo::::::::::::...``````````````````````````````````````````````````........````````dd        \n");
	adjustW();
	printf("     .:Mo::::::::::::--.``````````````````````````````````````````````----ommmmmmm:```````dd        \n");
	adjustW();
	printf("     hNh+:::::::::::::-...`````````````````````````````-///////:..-//+mmmmmNNmdddh:```````dd        \n");
	adjustW();
	printf("     dm:::::::::::::::::-..````````````````````````````:ooyMMNNdyyhmyysssoooo/---.````````dh        \n");
	adjustW();
	printf("    `dm:::::::::::::::::---.``````````````````````````````./////////-....``````````````./h+-        \n");
	adjustW();
	printf("   :mmd::::::::::::::::::::-.``````````````````````````````````````````````````````````dmm-         \n");
	adjustW();
	printf("  /sM+:::::::::::::::::::::::-.``````````````````````````````````````````````````````:+Ny           \n");
	adjustW();
	printf("  Nms/::::::::::::::::::::::::--.```````````````````````````````````````````````````.sMo:           \n");
	adjustW();
	printf("  Nd::::::::::::::::::::::::::::..`````````````````````````````````````````````````.dNM`            \n");
	adjustW();
	printf("  Nd:::::::::::::::::::::::::::::--```````````````````````````````````````````````-/MMM`            \n");
	adjustW();
	printf("  Nd:::::::::::::::::::::::::::::::..```````````````````````````````````````````.omMsdMo:           \n");
	adjustW();
	printf("  Nd:::::::::::::::::::::::::::::::..``.yyyy+..````````````````````````````....shMs/`-/Ny           \n");
	adjustW();
	printf("  Nd:::::::::::::::::::::::::::::-...```:::mmhdy...````````````````````...oddhhmd:.````Ny           \n");
	adjustW();
	printf("  Nd::::::::::::::::::::--------...```````````sddNN//////////:``````://dMddd:-.````````Ny           \n");
	adjustW();
	printf("  Nd:::::::::::::::-----..........```````````````/syyyyyyyyymNssssssNNyyy:::--.````````ooo.         \n");
	adjustW();
	printf("  Nd::::::::::::-.-..........`````````````````````.:::::::::+oooooooo+:::..-..``````````+M:         \n");
	adjustW();
	printf("  Nh------::::-...`````````````````````````````````....--::::::::::::::....`````````````/NNh        \n");
	adjustW();
	printf("  dy...........`````````````````````````````````````````...--:::::-----..`````````````````dd        \n");
	adjustW();
	printf("  Nh```````````````````````````````````````````````````````..-----....````````````````````dd        \n");
	adjustH();
}
