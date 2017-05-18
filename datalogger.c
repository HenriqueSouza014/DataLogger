#include <stdio.h>

#include <Wire.h>

#define at24c16_address 0x50

#define index 0


/* Rotina auxiliar para comparacao de strings */

int str_cmp(char *s1, char *s2, int len) {

/* Compare two strings up to length len. Return 1 if they are

equal, and 0 otherwise.

*/

int i;

for (i = 0; i < len; i++) {

if (s1[i] != s2[i]) return 0;

if (s1[i] == '\0') return 1;

}

return 1;

}


/* Processo de bufferizacao. Caracteres recebidos sao armazenados em um buffer. Quando um caractere

de fim de linha ('\n') e recebido, todos os caracteres do buffer sao processados simultaneamente.

*/


/* Buffer de dados recebidos */

#define MAX_BUFFER_SIZE 50

typedef struct {

char data[MAX_BUFFER_SIZE];

unsigned int tam_buffer;

} serial_buffer;


/* Teremos somente um buffer em nosso programa, O modificador volatile

informa ao compilador que o conteudo de Buffer pode ser modificado a qualquer momento. Isso

restringe algumas otimizacoes que o compilador possa fazer, evitando inconsistencias em

algumas situacoes (por exemplo, evitando que ele possa ser modificado em uma rotina de interrupcao

enquanto esta sendo lido no programa principal).

*/

serial_buffer Buffer;


/* Todas as funcoes a seguir assumem que existe somente um buffer no programa e que ele foi

declarado como Buffer. Esse padrao de design - assumir que so existe uma instancia de uma

determinada estrutura - se chama Singleton (ou: uma adaptacao dele para a programacao

nao-orientada-a-objetos). Ele evita que tenhamos que passar o endereco do

buffer como parametro em todas as operacoes (isso pode economizar algumas instrucoes PUSH/POP

nas chamadas de funcao, mas esse nao eh o nosso motivo principal para utiliza-lo), alem de

garantir um ponto de acesso global a todas as informacoes contidas nele.

*/


/* Limpa buffer */

void buffer_clean() {

Buffer.tam_buffer = 0;

}


/* Adiciona caractere ao buffer */

int buffer_add(char c_in) {

if (Buffer.tam_buffer < MAX_BUFFER_SIZE) {

Buffer.data[Buffer.tam_buffer++] = c_in;

return 1;

}

return 0;

}



/* Flags globais para controle de processos da interrupcao */

int flag_check_command = 0;


/* Rotinas de interrupcao */


/* Ao receber evento da UART */

void serialEvent() {

char c;

while (Serial.available()) {

c = Serial.read();

if (c == '\n') {

buffer_add('\0'); /* Se recebeu um fim de linha, coloca um terminador de string no buffer */

flag_check_command = 1;

} else {

buffer_add(c);

}

}

}

void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) { /*função destinada à escrita na EEPROM*/

Wire.beginTransmission(deviceaddress);

Wire.write((int)(eeaddress));

Wire.write(data);

Wire.endTransmission();

delay(5);

}


byte readEEPROM(int deviceaddress, unsigned int eeaddress ) { /*função destinada à leitura da EEPROM*/

byte data = 0xF;


Wire.beginTransmission(deviceaddress);

Wire.write((int)(eeaddress));

Wire.endTransmission();

Wire.requestFrom(deviceaddress, 1);


if (Wire.available()) {

data = Wire.read();

}

return data;

}


/* Funcoes internas ao void main() */

int address = 0;

int EEPROM_element = 0;

int N = 0;


void setup() {

/* Inicializacao */

buffer_clean();

flag_check_command = 0;

Serial.begin(9600);

Wire.begin();

writeEEPROM(at24c16_address, address, 0);

pinMode(13, OUTPUT);


//Pinos ligados aos pinos 4, 5, 6 e 7 do teclado - Linhas

pinMode(4, OUTPUT);

pinMode(5, OUTPUT);

pinMode(6, OUTPUT);

pinMode(7, OUTPUT);


//Pinos ligados aos pinos 8, 9, e 10 do teclado - Colunas

pinMode(8, INPUT);

//Ativacao resistor pull-up

digitalWrite(8, HIGH);

pinMode(9, INPUT);

digitalWrite(9, HIGH);

pinMode(10, INPUT);

digitalWrite(10, HIGH);

}


int ldrPin = 0;

int ldrValor = 0;

int flag = 0;

int aux = 0;


void loop() {

int x, y;

char out_buffer[50];

int flag_write = 0;

ldrValor = analogRead(ldrPin);

ldrValor = ldrValor / 4; /*simplesmente para melhor representar, uma vez que o valor lido está entre 0 e 1023*/

int elements = 0;



for (int porta = 4; porta < 8; porta++)

{

//Alterna o estado dos pinos das linhas

digitalWrite(4, HIGH);

digitalWrite(5, HIGH);

digitalWrite(6, HIGH);

digitalWrite(7, HIGH);

digitalWrite(porta, LOW);

//Verifica se alguma tecla da coluna 1 foi pressionada

if (digitalRead(8) == LOW)

{

imprime_linha_coluna(porta - 3, 1);

while (digitalRead(8) == LOW) {}

}


//Verifica se alguma tecla da coluna 2 foi pressionada

if (digitalRead(9) == LOW)

{

imprime_linha_coluna(porta - 3, 2);

while (digitalRead(9) == LOW) {};

}


//Verifica se alguma tecla da coluna 3 foi pressionada

if (digitalRead(10) == LOW)

{

imprime_linha_coluna(porta - 3, 3);

while (digitalRead(10) == LOW) {}

}

}

delay(10);


/* A flag_check_command permite separar a recepcao de caracteres

(vinculada a interrupca) da interpretacao de caracteres. Dessa forma,

mantemos a rotina de interrupcao mais enxuta, enquanto o processo de

interpretacao de comandos - mais lento - nao impede a recepcao de

outros caracteres. Como o processo nao 'prende' a maquina, ele e chamado

de nao-preemptivo.

*/

if (aux == 0) { /*a variável aux atua de modo a controlar o momento de interrupção do modo de medição automática*/


if (flag_check_command == 1) {

if (str_cmp(Buffer.data, "PING", 4) ) {

sprintf(out_buffer, "PONG\n");

flag_write = 1;

}


if (str_cmp(Buffer.data, "ID", 2) ) {

sprintf(out_buffer, "DATALOGGER - Henrique e Nakamura\n");

flag_write = 1;

}


if (str_cmp(Buffer.data, "SUM", 3) ) {

sscanf(Buffer.data, "%*s %d %d", &x, &y);

sprintf(out_buffer, "SUM = %d\n", x + y);

flag_write = 1;

}


if (str_cmp(Buffer.data, "MEASURE", 7) ) { /*realiza a medição do LDR*/

sprintf(out_buffer, "%d\n", ldrValor);

flag_write = 1;

}


if (str_cmp(Buffer.data, "RECORD", 6) ) { /*realiza a gravação do dado auferido na memória*/

address++;

writeEEPROM(at24c16_address, index, address);

writeEEPROM(at24c16_address, address, ldrValor);

buffer_clean();

}


if (str_cmp(Buffer.data, "MEMSTATUS", 9) ) { /*informa o número de elementos gravados na memória*/

elements = readEEPROM(at24c16_address, index);

sprintf(out_buffer, "Numero de Elementos: %d\n", elements);

flag_write = 1;

}


if (str_cmp(Buffer.data, "RESET", 5) ) { /*zera o endereço de gravação/leitura de tal forma a ignorar os outros elementos*/

address = 0;

writeEEPROM(at24c16_address, index, address);

buffer_clean();

}

if (str_cmp(Buffer.data, "GET", 3) ) { /*permite a obtenção do elemento de qualquer posição N da memória*/

sscanf(Buffer.data, "%*s %d", &N);

EEPROM_element = readEEPROM(at24c16_address, N + 1);

sprintf(out_buffer, "%d\n", EEPROM_element);

flag_write = 1;

}

if (flag == 11) { /*quando a tecla 1 do TM é apertada, pisca-se o LED*/

Serial.println("Pisca LED");

digitalWrite(13, HIGH);

delay(1000);

digitalWrite(13, LOW);

delay(1000);

flag = 0;

}


if (flag == 12) { /*quando a tecla 2 do TM é apertada, uma medida é auferida e gravada na memória*/

Serial.println("Medida salva na memoria");

address++;

writeEEPROM(at24c16_address, index, address);

writeEEPROM(at24c16_address, address, ldrValor);

flag = 0;

}


if (flag == 13) { /*quando a tecla 3 do TM é apertada, entra-se em modo de medição automática*/

aux = 1;

}

else { /*caso algo que não é esperado aconteça na serial, como por exemplo uma série de letras aleatórias, limpa-se o buffer*/

buffer_clean();

}


flag_check_command = 0;

}

}


if (aux == 1) { /*entrada do modo de medição automática*/

Serial.println("Medicao automatica");

address++;

writeEEPROM(at24c16_address, index, address);

writeEEPROM(at24c16_address, address, ldrValor);

if (flag == 14) {

aux = 0; /*caso a tecla 4 do teclado seja apertada, encerra-se o modo de medição automática*/

Serial.println("Medicao automatica encerrada");

}

}


/* Posso construir uma dessas estruturas if(flag) para cada funcionalidade

do sistema. Nesta a seguir, flag_write e habilitada sempre que alguma outra

funcionalidade criou uma requisicao por escrever o conteudo do buffer na

saida UART.

*/

if (flag_write == 1) {

Serial.write(out_buffer);

buffer_clean();

Buffer.data[0] = '\0';

flag_write = 0;

}

}



void imprime_linha_coluna(int x, int y)

{

if (flag == 1) {

if (x != 1 && x != 2) flag = 0;

if (x == 2 && y != 1) flag = 0;

}

if (flag >= 2) {

if ( x != 4) flag = 0;

if (x == 4 && y != 1) flag = 0;

}

if (x == 4 && y == 3) flag = 1;

if (x == 1 && y == 1 && flag == 1) flag = flag + 1;

if (x == 1 && y == 2 && flag == 1) flag = flag + 2;

if (x == 1 && y == 3 && flag == 1) flag = flag + 3;

if (x == 2 && y == 1 && flag == 1) flag = flag + 4;

if (x == 4 && y == 1 && flag >= 2) flag = flag + 9;

if (flag > 10) flag_check_command = 1;

}
