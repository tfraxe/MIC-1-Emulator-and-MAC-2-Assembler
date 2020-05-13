#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
using namespace std;
typedef  unsigned int word;
typedef  unsigned char byte;
typedef  unsigned long int microcode;


// Registradores e barramentos 
word mar, mdr, pc, sp, lv, cpp, tos, opc, h, bA, bB, bC, bS;
byte mbr;

// Flags da ULA. n - resultado diferente de 0; z - resultado igual a 0;
byte n, z;

/* Variáveis que vão guardar as respectivas partições da microinstrução. 
	bBus - guarda os bits da microinstrução correspondentes à leitura dos registradores
	mem - guarda os bits da microinstrução correspondentes às operações de memória
	jam - guarda os bits da microinstrução correspondentes às operações de jump
	alu - guarda os bits da microinstrução correspondentes às operações da ula e do shifter
	addr - guarda os bits da mmicroinstrução correspondentes ao endereço da próxima instrução a ser executada
	cBus - guarda os bits da microinstrução correspondentes à escrita nos registradores. 


 */

byte b, mem, jam, alu;
word addr, c; 
byte memoria[1024*1024*1024]; 
void mostrarRegistradores() {

	system("clear");

	cout << "Registradores: " << endl;
	printf("MAR: %u\nMDR: %u \nPC: %u \nMBR: %u \nSP:  %u \nLV:  %u \nCPP: %u \nTOS: %u \nOPC: %u \nH: %u\n", mar, mdr, pc, mbr, sp, lv, cpp, tos, opc, h);

	cout << "--------------------------------------------------------------" << endl;
	cout << "Frame de Variáveis Locais e Pilha de Operandos: " << endl;

	for(int i = lv; i <= sp; i++) {
		printf("pilha: %u\n", memoria[i*4]);
	}


	cout << "---------------------------------------------------------------" << endl;

	cout << "Área de Método: " << endl;

	//int dado = 0;
	for(int i = 0; i <= 5; i++) {
		printf("Endereço: %u     Instrução: %d\n\n", pc+i, memoria[pc+i]);
	}
	cout << "---------------------------------------------------------------" << endl;

}

void ler_registrador(byte ender) {
	switch(ender){
		case 0:
		bB = mdr;
		break;
		case 1:
		bB = pc;
		break;
		case 2:
		bB  = mbr;
		break;
		case 3: //Joga em B valor de MBR com extensão de sinal: os 24 bits mais à esquerda repetem o bit mais esquerda de MBR. 
		{
		word sinal = mbr >> 7;
		bB = (sinal)?0xFFFFFF00:0; //extensão de sinal. (repete bit de sinal)
		bB = bB | mbr;
		}
		break;
		case 4:
		bB = sp;
		break;
		case 5:
		bB = lv;
		break;
		case 6:
		bB = cpp;
		break;
		case 7:
		bB = tos;
			break;
		case 8:
		bB = opc;
		break;
		default:
		break;
	}
} //joga do regsitrador para o barramento.
void gravar_registrador(word ender) {
	if(ender & 1) mar = bC; //primeiro bit atibo
	if(ender & 2) mdr = bC; // segundo
	if(ender & 4) pc = bC; // etc..
	if(ender & 8) sp = bC;
	if(ender & 16) lv = bC;
	if(ender & 32) cpp = bC;
	if(ender & 64) tos = bC;
	if(ender & 128) opc = bC;
	if(ender & 256)	 h = bC; 
} //joga do barramento para registrador.//não podemos acessar dois registradores ao mesmo tempo.

void ula(byte op) {

	byte operacao = op & 63; //Pega os primeiros 6 bits, que são os bits que correspondem às operações na ula.
	byte shifter = (op >> 6) & 3; //Pega os últimos 2 bits, que são os bits que correspondem às operações no deslocador.
	bA = h; //Joga o que está no registrador h para o barramento A.
	switch(operacao) { //Os números aqui usados são a representação decimal dos números binários que aparecem na página 246 da sexta edição em inglês do livro de arquitetura.
		case 24: 
		bS = bA;
		break;
		case 20:
		bS = bB;
		break;
		case 26:
		bS = ~bA;
		break;
		case 44:
		bS = ~bB;
		break;
		case 60:
		bS = bA + bB;
		break;
		case 61:
		bS = bA + bB + 1;
		break;
		case 57:
		bS = bA + 1;
		break;
		case 53:
		bS = bB + 1;
		break;
		case 63:
		bS = bB - bA;
		break;
		case 54:
		bS = bB - 1;
		break;
		case 59:
		bS = -1 * bA;
		break;
		case 12:
		bS = bA & bB;
		break;
		case 28:
		bS = bA | bB;
		break;
		case 16:
		bS = 0;
		break;
		case 49:
		bS = 1;
		break;
		case 50:
		bS = -1;
		break;
		default:
		break;
		
	}

	if(shifter == 0) {
		bC = bS;	
	} else {
		if(shifter & 1){
			/*word sign = bS >> 31;
			word shifted = bS >> 1; // Bit SRA1 ativo. Desloca bits 1 bit para direita.
			bC = shifted | (sign << 31); */
			bC = bS >> 1;
		} 	
		if(shifter & 2)		bC = bS << 8; //Bit SLL8 ativo. Desloca bits 1 byte para esquerda
	}

	if (bC == 0) { //Se o resultado da ula for zero, z = 1;
		z = 1;
		n = 0;
	} else { // Caso contrário, n = 1;
		z = 0;
		n = 1;
	}
}

void partir(microcode mi) { //Dada uma microinstrução, parte os bits conforme descrito no livro.
	b = mi & 15;
	mem = (mi >> 4) & 7;
	c = (mi >> 7) & 511;
	alu = (mi >> 16) & 255;
	jam = (mi >> 24) & 7;
	addr = (mi >> 27) & 511;

}

word proximoEndereco() {
	word proximo = addr;

	if(jam & 1) proximo = proximo | z << 8; //O bit JAMZ está ativo
	if(jam & 2) proximo = proximo | n << 8; //O bit JAMN está ativo
	if(jam & 4) proximo = proximo | mbr; //O bit JMPC está ativo

	return proximo;
}

//Nossa memória RAM terá 1GB. Podemos diminuir isso, porque os programos que vamos fazer não vão chegar a esse tamanho. Porém, 4GB é o tamanho máximo que um programa para esta máquina pode ter.
void operacaoMem() {
	//memoria[0] = 0; Usado pra fazer teste de READ e FETCH
	//memoria[1] = 255;
	//memoria[2] = 0;
	//memoria[3] = 255;
	if (mem & 1)  mbr = memoria[pc]; // Bit FETCH ativo
	if (mem & 2)  memcpy(&mdr, &memoria[mar*4], 4); // Bit READ ativo. Lê o dado da memória e o coloca em mdr
	if (mem & 4)  memcpy(&memoria[mar*4], &mdr, 4); // Bit WRITE ativo. Escreve dado que está em MDR na word da memória especificada por MAR.
}

microcode firmware[512];
void readRom() { //Carrega microprograma, fornecido pelo professor, para o firmware
	ifstream rom;
	char buffer[8];
	rom.open("microprog.rom", ios::in | ios::binary);

	for(int i = 0; i < 512; i++) {
		rom.read(buffer, 8);
		memcpy(&firmware[i], buffer, 8);
	}

	rom.close();
}

void readProg() {
	ifstream prog;

	char bufferTamanho[4];
	char bufferInit[20];

	prog.open("prog.exe", ios::in | ios::binary);

	int tamanhoProg = 0;

	prog.read(bufferTamanho, 4);
	memcpy(&tamanhoProg, bufferTamanho, 4);

	prog.read(bufferInit, 20);
	memcpy(&memoria[0], bufferInit, 20);

	char bufferPrograma[tamanhoProg - 20];
	prog.read(bufferPrograma, tamanhoProg - 20);
	memcpy(&memoria[0x0401], bufferPrograma, tamanhoProg-20);

	prog.close();

	/*delete[] bufferTamanho;
	delete[] bufferInit;
	delete[] bufferPrograma;*/

}

int main(int argc, char const *argv[])
{
	
	//mdr = 0b11001100110011001100110011001100; Usado para fazer teste de write
	/* firmware[0] = 0b000000001000001100010000010000000100;
	firmware[1] = 0b000000010000001101010000010000000100;
	firmware[2] = 0b000000011000001101010000010000000100;
	firmware[3] = 0b000000100000001101010000010000000100;
	firmware[4] = 0b000000101000000101001000000000000100;
	firmware[5] = 0b000000101000001111000000100000000101; 
	-- MICROPROGRAMA USADO PARA TESTES. COLOCA MÚLTIPLOS DE 4 EM LV
	OBS: Se for testar, desativa readRom()
	-- 
	*/
	

	/*memoria[1] = 0x73; //init

	memoria[4] = 0x0006; //(CPP inicia com o valor 0x0006 guardado na palavra 1 – bytes 4 a 7.)

	word tmp = 0x1001; //LV

	memcpy(&(memoria[8]), &tmp, 4); //(LV inicia com o valor de tmp guardado na palavra 2 – bytes 8 a 11)

	tmp = 0x0400; //PC

	memcpy(&(memoria[12]), &tmp, 4); //(PC inicia com o valor de tmp guardado na palavra 3 – bytes 12 a 15)

	tmp = 0x1004; //SP

	//SP (Stack Pointer) é o ponteiro para o topo da pilha.

	//A base da pilha é LV e ela já começa com algumas variáveis empilhadas (dependendo do programa).

	//Cada variável gasta uma palavra de memória. Por isso a soma de LV com num_of_vars.

	memcpy(&(memoria[16]), &tmp, 4); //(SP inicia com o valor de tmp guardado na palavra 4 – bytes 16 a 19)

	memoria[0x0401] = 0x19;
	memoria[0x0402] = 0x0a;
	memoria[0x0403] = 0x22;
	memoria[0x0404] = 0x00;
	memoria[0x0405] = 0x19;
	memoria[0x0406] = 0x0a;
	memoria[0x0407] = 0x22;
	memoria[0x0408] = 0x01;
	memoria[0x0409] = 0x1c;
	memoria[0x040a] = 0x00;
	memoria[0x040b] = 0x1c;
	memoria[0x040c] = 0x01;
	memoria[0x040d] = 0x02;
	memoria[0x040e] = 0x22;
	memoria[0x040f] = 0x02;
	memoria[0x0410] = 0x1c;
	memoria[0x0411] = 0x02;
	memoria[0x0412] = 0x19;
	memoria[0x0413] = 0x19;
	memoria[0x0414] = 0x4b;
	memoria[0x0415] = 0x00;
	memoria[0x0416] = 0x0d;
	memoria[0x0417] = 0x1c;
	memoria[0x0418] = 0x00;
	memoria[0x0419] = 0x19;
	memoria[0x041a] = 0x01;
	memoria[0x041b] = 0x05;
	memoria[0x041c] = 0x22;
	memoria[0x041d] = 0x00;
	memoria[0x041e] = 0x3c;
	memoria[0x041f] = 0x00;
	memoria[0x0420] = 0x07;
	memoria[0x0421] = 0x19;
	memoria[0x0422] = 0x0d;
	memoria[0x0423] = 0x22;
	memoria[0x0424] = 0x01;
	memoria[0x0425] = 0x19;
	memoria[0x0426] = 0x19; */

	readRom();
	microcode microinstrucao;
	word mpc = 0;
	readProg();

	while (1) {


		/*Ciclo:

			0) 'Pegar', do firmware, microprograma que está na posição indicada em MPC
			1) Ler registrador (isto é, 'jogar' do registrador para o barramento B)
			2) Fazer operações correspondentes, na ULA, com os dados que estão no barramento B e no barramento A
			3) Gravar nos registradores (isto é, escrever resultado do processamento, que está no barramento C, nos registradores escolhidos)
			4) Enviar sinal de READ, WRITE OU FETCH para a memória
			5) Escolher (isto é, colocar em MPC) próximo endereço de firmware que vai ser acessado (com base nos bits de NEXT ADDRESS e nos bits de JUMP)
			Dúvida: a escolha do próximo endereço do microprograma é feito antes ou depois das operações de memória?
		 */
		mostrarRegistradores();
		microinstrucao = firmware[mpc];
		partir(microinstrucao);
		ler_registrador(b);
		ula(alu);
		gravar_registrador(c);
		operacaoMem();
		mpc = proximoEndereco();

		/*printf("mar: %u\n", mar);
		for (int i = 4; i < 8; i++) {
			printf("mem: %u\n", memoria[i]);
		} Usado para fazer teste de WRITE*/


		getchar();
	}



	return 0;
}



