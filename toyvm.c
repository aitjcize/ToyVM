/* toysim.c - Toy Sim

 * Copyright (C) 2009 -  Aitjcize <aitjcize@gmail.com>
 * All Rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.  
 *
 * --------------------------------------------------------------------
 * Change Log:
 * 0.1.0 - First Release
 * 0.1.5 - Fix opcode '1' overflow
 *         Fix opcode 'E' pc <- R[t] to pc <- R[d]
 *         Minor bug fix
 * 0.1.6 - Add input range dectect
 *         Fix opcode '1', '2', '3', '4', '5', '6': constrain range.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define PROGRAM_NAME "toysim"
#define VERSION "0.1.7"

#define MAX_CHAR 256

/* flags */
bool use_external_input = false;

/* global variables */
int mem[256];                                   /* memory file */
int reg[16];                                    /* register file */
int pc = 16;                                    /* program counter */
int total_line;                                 /* total line */

/* function prototypes */
void ReadProg2Mem(char *filename);
int OpenError(char *filename);
int Hex2Int(char *str);
int GetHexBit(int num, int bit);
char* tohex(int);
int nmask(int n);                   /* mask for right shift negative numbers */

int main(int argc, char *argv[])
{
  /* usage: toysim foo.toy bar.input */
  int op, rd, rs, rt, addr;
  int tmp;
  char sinput[5];
  bool do_print = false;
  //FILE* input;

  if(argc >= 2)
    ReadProg2Mem(argv[1]);

  printf("------ Program Start ------\n");

  /* start processing */
  while(true) {
    op = GetHexBit(mem[pc], 3);
    rd = GetHexBit(mem[pc], 2);
    rs = GetHexBit(mem[pc], 1);
    rt = GetHexBit(mem[pc], 0);
    pc++;
    if((op >= 7 && op <= 9) || op == 12 || op == 13 || op == 15) {/* format 2 */
      addr = rs *16 + rt;
    }
    switch(op) {
      case 0:
        printf("------- Program End -------\n");
        exit(0);
      case 1:
        reg[rd] = reg[rs] + reg[rt];
        if(reg[rd] > 32767)
          reg[rd] = -32768 + (reg[rd] +32768) % 65536;
        break;
      case 2:
        reg[rd] = reg[rs] - reg[rt];
        if(reg[rd] < -32768)
          reg[rd] = 32768 + (reg[rd] -32768) % 65536;
        break;
      case 3:
        reg[rd] = reg[rs] & reg[rt];
        break;
      case 4:
        reg[rd] = reg[rs] ^ reg[rt];
        break;
      case 5:
        reg[rd] = reg[rs] << reg[rt];
        if(reg[rd] > 32767)
          reg[rd] = -32768 + (reg[rd] +32768) % 65536;
        break;
      case 6:
        tmp = reg[rs] >> reg[rt];
        if(reg[rs] > 32767) {              /* negative if > 32767 (2^15 -1)*/
          reg[rd] = tmp | nmask(reg[rt]);
        }
        else
          reg[rd] = tmp;
        break;
      case 7:
        reg[rd] = addr;
        break;
      case 8:
        if(addr == 255) {
          do {
            int i;
            printf("] ");
            fgets(sinput, 5, stdin);
            for(i = 0; i < strlen(sinput); i++)
              if(sinput[i] == '\n') sinput[i] = 0;
            if(strlen(sinput) < 4)
              fprintf(stderr, "error: wrong format, please reenter.\n");
            else
              while(getchar() != '\n');
          } while(strlen(sinput) < 4);

          reg[rd] = Hex2Int(sinput);
        } else
          reg[rd] = mem[addr];
        break;
      case 9:
        if(addr == 255) do_print = true;
        mem[addr] = reg[rd];
        break;
      case 10:
        reg[rd] = mem[reg[rt]];
        break;
      case 11:
        mem[reg[rt]] = reg[rd];
        break;
      case 12:
        if(reg[rd] == 0) pc = addr;
        break;
      case 13:
        if(reg[rd] > 0) pc = addr;
        break;
      case 14:
        pc = reg[rd];
        break;
      case 15:
        reg[rd] = pc;
        pc = addr;
        break;
    }
    if(do_print) {
      printf("> %s\n", tohex(mem[255]));
      do_print = false;
    }
  }

  return 0;
}

int OpenError(char *filename)
{
  fprintf(stderr, "error: can't open `%s'.\n", filename);
  exit(1);
}

void ReadProg2Mem(char *filename)
{
  int count = 0, line;
  char buf[MAX_CHAR], tmp[MAX_CHAR];
  FILE* in;
  if((in = fopen(filename, "r")) == NULL)
    OpenError(filename);
  while((fgets(buf, MAX_CHAR, in) != NULL)) {
    if(buf[0] == '\n') continue;
    buf[strlen(buf) -1] = 0;
    strcpy(tmp, buf);
    tmp[2] = 0;
    line = Hex2Int(tmp);
    if(line > 255) {
      fprintf(stderr, "error: invalid line.\n");
      exit(1);
    }
    mem[line] = Hex2Int(buf +4);
    count++;
  }
  total_line = count;
  fclose(in);
}

int Hex2Int(char *str)
{
  int i, sum = 0;
  for(i = 0; i < strlen(str); i++) {
    if(str[i] >= 'a' || str[i] >= 'A')         /* a to f */
      sum += (str[i] -(str[i] >= 'a'? 'a': 'A') +10) *pow(16, strlen(str) -i -1);
    else
      sum += (str[i] -'0') *pow(16, strlen(str) -i -1);
  }
  return sum;
}

int GetHexBit(int num, int bit)
{
  int bits[4] = { 0 };
  int count = 0;
  while(num) {
    bits[count++] = num % 16;
    num /= 16;
  }
  return bits[bit];
}

char* tohex(int num)
{
  static char bits[5] = { 0 };
  char tmp;
  int count = 0, get, i;
  while(num) {
    get = num % 16;
    num /= 16;
    if(get >= 10)
      bits[count++] = get +'A' -10;
    else
      bits[count++] = get +'0';
  }
  for(i = 0; i < 4 -count; i++)
    bits[count +i] = '0';
  for(i = 0; i < 2; i++)
  {
    tmp = bits[i];
    bits[i] = bits[3 -i];
    bits[3 -i] = tmp;
  }
  return bits;
}

int nmask(int n)                 /* mask for right shift negative numbers */
{
  int i, sum = 0;
  for(i = 15; i >= 16 -n; i--)
    sum += pow(2, i);
  return sum;
}
