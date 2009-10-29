/* toyvm.c - ToyVM

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
 * 0.2.0 - beta version
 *         new command line parser
 *         Add a gdb like debug mode
 * 0.2.1 - Finish breakpoint implement
 *         Fix some bugs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define PROGRAM_NAME "toyvm"
#define VERSION "0.2.1 beta"

#define MAX_CHAR 256

#define DEBUG           printf("Here\n")
#define VDEBUG(a, b)    printf(#a ": %" #b "\n", a)

/* flags */
bool use_external_input = false;

struct _tdb {
  unsigned int enabled  : 1;
  unsigned int mode     : 2;                  /* 0: stopped,  1: running, 2: step*/
  unsigned int verbose  : 1;
} tdb;

/* global variables */
int mem[256];                                   /* memory file */
int reg[16];                                    /* register file */
int pc = 16;                                    /* program counter */
int total_line;                                 /* total line */
int inp[256];                                   /* input file */
int total_inp_line;                             /* total input line */
int inp_index;

/* function prototypes */
void ReadProg2Mem(char *filename);
void ReadInput2Mem(char *filename);
int OpenError(char *filename);
int Hex2Int(char *str);
int GetHexBit(int num, int bit);
char* Int2Hex(int);
int YesOrNo(char *message);
int nmask(int n);                   /* mask for right shift negative numbers */

int main(int argc, char *argv[])
{
  /* usage: toyvm [-d] foo.toy bar.input */
  int i, tmp,
      b_count = 0;                                  /* count break points */
  int op, rd, rs, rt, addr;
  char sinput[5], dinput[32];
  bool do_print = false;
  unsigned int breakpoints[16];
  
  if(argc > 5) {
    fprintf(stderr, "Usage: toyvm [-d] toyfile [input file]\n");
    exit(1);
  }

  for(i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-d") == 0) {
      tdb.enabled = true;
      printf("ToyVM Debug Mode - ToyVM Ver %s\n", VERSION);
      printf("Copyright (C) 2009 Aitjcize (Wei-Ning Huang)\n");
      printf("License GPLv2 <http://gnu.org/licenses/gpl.html>\n\n");
    }
    else if(total_line == 0)
      ReadProg2Mem(argv[i]);
    else
      ReadInput2Mem(argv[i]);
  }


  /* start processing */
  while(true) {

    /* check break point then step */
    if(b_count > 0) {
      for(i = 0; i < b_count; i++)
        if(pc == breakpoints[i])
          break;
      if(i != b_count) {
        tdb.mode = 2;
        printf("Breakpoint %d at %s: 0x%s\n", i +1, "toy", Int2Hex(breakpoints[i]));
      }
    }

    /* verbose output */
    if(tdb.verbose || tdb.mode == 2) {
      printf("%s: ", Int2Hex(pc) +2);
      printf("%s\n", Int2Hex(mem[pc]));
    }

    while(tdb.enabled && tdb.mode != 1) {
      printf("(tdb) ");
      fgets(dinput, 32, stdin);
      for(i = 0; i < strlen(dinput); i++)
        if(dinput[i] == '\n') dinput[i] = 0;
      if(strlen(dinput) >= 32)
        while(getchar() != '\n');

      if(strcmp(dinput, "run") == 0 || strcmp(dinput, "r") == 0) {
        if(tdb.mode == 1) {
          int res = YesOrNo("The program being debugged has been started already.\nStart it from the beginning? (y or n) ");
          if(res == 'y') {
            printf("Starting program: %s\n", "toy");
            pc = 16;
            break;
          }
        }
        printf("Starting program: %s\n", "toy");
        tdb.mode = 1;
        break;
      } else if(strcmp(dinput, "step") == 0 || strcmp(dinput, "s") == 0) {
        tdb.mode = 2;
        break;
      } else if(strcmp(dinput, "continue") == 0 || strcmp(dinput, "c") == 0) {
        tdb.mode = 1;
        break;
      } else if(strcmp(dinput, "next") == 0 || strcmp(dinput, "n") == 0) {
        if(pc == 16) {
          int res = YesOrNo("Program is not running, do you want to run it with step mode? (y or n) ");
          if(res == 'n') continue;
          tdb.mode = 2;
        }
        break;
      } else if(strncmp(dinput, "break", 5) == 0 || strncmp(dinput, "b", 1) == 0) {
        char* ch;
        ch = strtok(dinput, " ");
        ch = strtok(NULL, " ");
        if(Hex2Int(ch) > 255)
          printf("error: no this line.");
        else {
          breakpoints[b_count++] = Hex2Int(ch);
          printf("Breakpoint %d at %s: 0x00%s\n", i +1, "toy", ch);
        }
      } else if(strncmp(dinput, "delete", 5) == 0 || strncmp(dinput, "d", 1) == 0) {
        char* ch;
        ch = strtok(dinput, " ");
        ch = strtok(NULL, " ");
        breakpoints[atoi(ch) -1] = 256;
      } else if(strcmp(dinput, "info") == 0) {
        printf("Num    Address\n");
        for(i = 0; i < b_count; i++)
          if(breakpoints[i] != 256)
            printf("%2d     0x%s\n", i +1, Int2Hex(breakpoints[i]));
      } else if(strcmp(dinput, "verbose") == 0 || strcmp(dinput, "v") == 0) {
        tdb.verbose = true;
      } else if(strcmp(dinput, "list") == 0 || strcmp(dinput, "l") == 0) {
        for(i = pc -3 *(pc > 3); i <= pc +2; i++) {
          /* Int2Hex returns a local static, so we must print it with two lines. */
          printf("%s: ", Int2Hex(i) +2);
          printf("%s\n", Int2Hex(mem[i]));
        }
      } else if(strcmp(dinput, "reg") == 0) {
        for(i = 0; i < 16; i++) {
          printf("R[%c] = %s  ", (i < 10)? i +'0': i -10 +'A', Int2Hex(reg[i]));
          if(i % 4 == 3) printf("\n");
        }
      } else if(strcmp(dinput, "quit") == 0 || strcmp(dinput, "q") == 0) {
        exit(0);
      }
    }

    /* Fetch instructions to IR */
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
        printf("\nProgram exited.\n");
        if(!tdb.enabled)
          exit(0);
        tdb.mode = 0;
        pc = 16;
        continue;
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
      printf("> %s\n", Int2Hex(mem[255]));
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
  int line;
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
    total_line++;
  }
  fclose(in);
}

void ReadInput2Mem(char *filename)
{
  char buf[MAX_CHAR];
  FILE* in;
  if((in = fopen(filename, "r")) == NULL)
    OpenError(filename);
  while((fgets(buf, MAX_CHAR, in) != NULL)) {
    if(buf[0] == '\n') continue;
    buf[strlen(buf) -1] = 0;
    inp[total_inp_line++] = Hex2Int(buf);
  }
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

char* Int2Hex(int num)
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

int YesOrNo(char *message)
{
  char ch = 0;
  while(ch != 'y' && ch != 'n') {
    printf("%s", message);
    ch = getchar();
    if (ch != '\n') while(getchar() != '\n');
  }
  return ch;
}
