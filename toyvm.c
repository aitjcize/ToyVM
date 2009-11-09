/* toyvm.c - ToyVM

 * Copyright (C) 2009 -  Aitjcize <aitjcize@gmail.com>
 * All Rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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
 * 0.1.0   - First Release
 * 0.1.5   - Fix opcode '1' overflow
 *           Fix opcode 'E' pc <- R[t] to pc <- R[d]
 *           Minor bug fix
 * 0.1.6   - Add input range dectect
 *           Fix opcode '1', '2', '3', '4', '5', '6': constrain range.
 * 0.2.0   - Beta version
 *           New command line parser
 *           Add a gdb like debug mode
 * 0.2.1   - Finish breakpoint implement
 *           Fix some bugs
 * 0.2.2   - Implement input file feature
 *           Check every input hex range
 *           Add help list, version info
 * 0.2.3   - Add breakpoint range check
 *           Minor bug fixed.
 *           Stable release.
 * 0.2.4   - Modulize some part in main to gain readibility.
 * 0.2.4.1 - Fix opcode 'D'.
 * 0.2.4.2 - clear register in reset()
 * 0.2.5   - Add Function `FineInput', since it's stable, 0.2.5 will be stable release.
 * 0.2.6   - Add ListMem, enable A..B to specified list range.
 *           Add help to TdbLoop
 * 0.2.6.1 - Add cycle count.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define PROGRAM_NAME "toyvm"
#define VERSION "0.2.6.1"

#define MAX_CHAR 256
#define BREAK_MAX 16

/* flags */
bool use_external_input = false;

struct _tdb {
  unsigned int enabled  : 1;
  unsigned int mode     : 2;             /* 0: stopped,  1: running, 2: step*/
  unsigned int verbose  : 1;
} tdb;

/* global variables */
int mem[256];                                   /* memory file */
int reg[16];                                    /* register file */
int inp[256];                                   /* input file */
int total_inp_line;                             /* total input line */
int inp_index = 0;                              /* index for input line */
int pc = 16;                                    /* program counter */
char* toyfile = NULL;                           /* point to file name */
int cycle = 0;                                  /* total cycle */

/* function prototypes */
void ParseArgs(int argc, char** argv);
int TdbLoop(int* b_count, unsigned int* breakpoints);
int ReadProg2Mem(char *filename);
int ReadInput2Mem(char *filename);
void OpenError(char *filename);
int Hex2Int(char *str);
char* Int2Hex(int);
char* Get2ndArg(char* string);
void ListMem(char *string);
void reset(int mode);
int YesOrNo(char *message);
void FineInput(char* buf, int size);
int nmask(int n);                   /* mask for right shift negative numbers */
void lexit(int n);
void usage(int n);

int main(int argc, char *argv[])
{
  int b_count = 0;                              /* count break points */
  int op, rd, rs, rt, addr;
  int i, tmp, insc;
  char sinput[5];
  unsigned int breakpoints[BREAK_MAX];          /* store break points */
  bool do_print = false;

  ParseArgs(argc, argv);

  /* start processing */
  while(true) {

    /* check break point then step */
    if(b_count > 0) {
      for(i = 0; i < b_count; i++)
        if(pc == breakpoints[i]) {
          tdb.mode = 2;
          printf("Breakpoint %d at %s: 0x%s\n", i +1, toyfile, Int2Hex(breakpoints[i]));
          break;
        }
    }

    /* verbose output */
    if((tdb.verbose && tdb.enabled && tdb.mode != 0) || 
        (tdb.verbose && !tdb.enabled) || tdb.mode == 2) {
      printf("%s: ", Int2Hex(pc) +2);
      printf("%s\n", Int2Hex(mem[pc]));
    }

    TdbLoop(&b_count, breakpoints);

    /* Fetch instructions to IR */
    insc = mem[pc];
    op   = (insc >> 12) & 0xF;
    rd   = (insc >>  8) & 0xF;
    rs   = (insc >>  4) & 0xF;
    rt   = (insc >>  0) & 0xF;
    addr = (insc >>  0) & 0xFF;
    pc++;

    switch(op) {
      case 0:
        printf("\nProgram exited. Total cycles: %d.\n", cycle);
        if(!tdb.enabled)
          lexit(0);
        reset(0);
        continue;
      case 1:
        reg[rd] = (reg[rs] + reg[rt]) %65536;
        break;
      case 2:
        reg[rd] = reg[rs] - reg[rt];
        if(reg[rd] < 0)
          reg[rd] = reg[rd] +65536;
        break;
      case 3:
        reg[rd] = reg[rs] & reg[rt];
        break;
      case 4:
        reg[rd] = reg[rs] ^ reg[rt];
        break;
      case 5:
        reg[rd] = (reg[rs] << reg[rt]) %65536;
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
          if(use_external_input) {
            if(inp_index < total_inp_line && inp_index < 256) {
              reg[rd] = inp[inp_index++];
              printf("] %s\n", Int2Hex(reg[rd]));
              continue;
            }
            else
              fprintf(stderr, "error: no more input data, please input manually.\n");
          }
          do {
            printf("] ");
            FineInput(sinput, 5);
            tmp = Hex2Int(sinput);
            if(tmp > 65535 || tmp == -1)
              fprintf(stderr, "error: wrong range, please reenter.\n");
          } while(tmp > 65535 || tmp == -1);
          reg[rd] = tmp;
        } else
          reg[rd] = mem[addr];
        break;
      case 9:
        if(addr == 255) do_print = true;
        mem[addr] = reg[rd];
        break;
      case 10:
        if(reg[rt] < 0 || reg[rt] > 256) {
          fprintf(stderr, "error: illegal address `%s', abort.\n", Int2Hex(reg[rt]));
          lexit(1);
        }
        reg[rd] = mem[reg[rt]];
        break;
      case 11:
        if(reg[rt] < 0 || reg[rt] > 256) {
          fprintf(stderr, "error: illegal address `%s', abort.\n", Int2Hex(reg[rt]));
          lexit(1);
        }
        mem[reg[rt]] = reg[rd];
        break;
      case 12:
        if(reg[rd] == 0) pc = addr;
        break;
      case 13:
        if(reg[rd] > 0 && reg[rd] < 32768) pc = addr;
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
    cycle++;
  }
  return 0;
}

void ParseArgs(int argc, char** argv)
{
  int total_line = 0, i;
  if(argc > 5) {
    fprintf(stderr, "Usage: toyvm [-d] [-v] toyfile [input file]\n");
    lexit(1);
  }

  for(i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-d") == 0) {
      tdb.enabled = true;
      printf("ToyVM Debug Mode - ToyVM Ver. %s\n", VERSION);
      printf("Copyright (C) 2009 Aitjcize (Wei-Ning Huang)\n");
      printf("License GPLv2 <http://gnu.org/licenses/gpl.html>\n\n");
    }
    else if(strcmp(argv[i], "-v") == 0)
      tdb.verbose = true;
    else if(strcmp(argv[i], "--version") == 0) {
      printf("ToyVM Ver. %s\n", VERSION);
      printf("Copyright (C) 2009 Aitjcize (Wei-Ning Huang)\n");
      printf("License GPLv2 <http://gnu.org/licenses/gpl.html>\n");
      printf("This is free software: you are free to change and redistribute it.\n");
      printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
      printf("Written by Aitjcize (Wei-Ning Huang).\n");
      lexit(0);
    }
    else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
      usage(0);
    else if(total_line == 0) {
      total_line = ReadProg2Mem(argv[i]);
      toyfile = argv[i];
    }
    else {
      total_inp_line = ReadInput2Mem(argv[i]);
      use_external_input = true;
    }
  }

  if(toyfile == NULL) {
    fprintf(stderr, "error: no input file.\n");
    lexit(1);
  }
}

int TdbLoop(int *b_count, unsigned int* breakpoints)
{
  int i, tmp;
  char dinput[32];

  while(tdb.enabled && tdb.mode != 1) {
    printf("(tdb) ");
    FineInput(dinput, 32);

    if(strcmp(dinput, "run") == 0 || strcmp(dinput, "r") == 0) {
      if(tdb.mode == 2) {
        int res = YesOrNo("The program being debugged has been started already.\nStart it from the beginning? (y or n) ");
        if(res == 'y') {
          printf("Starting program: %s\n", toyfile);
          reset(1);
          break;
        }
      } else {
        printf("Starting program: %s\n", toyfile);
        tdb.mode = 1;
        break;
      }
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
      char* ch = Get2ndArg(dinput);
      if(ch == NULL) {
        fprintf(stderr, "error: invalid breakpoint.\n");
        continue;
      }
      tmp = Hex2Int(ch);
      if(tmp > 255 || tmp == -1) {
        fprintf(stderr, "error: no this line.\n");
        continue;
      }
      if(*b_count == BREAK_MAX) {          /* breakpoint is full, check from the beginning */
        for(i = 0; i < BREAK_MAX; i++)
          if(breakpoints[i] == 256) {
            breakpoints[i] = tmp;
            printf("Breakpoint %d at %s: 0x00%s\n", i +1, toyfile, ch);
            break;
          }
        if(i == BREAK_MAX)
          fprintf(stderr, "error: all available breakpoints are used, please delete some.\n");
      } else {
        breakpoints[(*b_count)++] = tmp;
        printf("Breakpoint %d at %s: 0x00%s\n", *b_count, toyfile, ch);
      }
    } else if(strncmp(dinput, "delete", 5) == 0 || strncmp(dinput, "d", 1) == 0) {
      char* ch = Get2ndArg(dinput);
      if(ch == NULL) {
        fprintf(stderr, "error: invalid breakpoint.\n");
        continue;
      } else if(atoi(ch) > BREAK_MAX) {
        fprintf(stderr, "error: max breakpoint is %d.\n", BREAK_MAX);
        continue;
      }
      breakpoints[atoi(ch) -1] = 256;         /* set to 256 to delete the breakpoint */
    } else if(strcmp(dinput, "info") == 0) {
      printf("Num    Address\n");
      for(i = 0; i < *b_count; i++)
        if(breakpoints[i] != 256)
          printf("%2d     0x%s\n", i +1, Int2Hex(breakpoints[i]));
    } else if(strcmp(dinput, "verbose") == 0 || strcmp(dinput, "v") == 0) {
      tdb.verbose = true;
    } else if(strcmp(dinput, "noverbose") == 0 || strcmp(dinput, "nv") == 0) {
      tdb.verbose = false;
    } else if(strncmp(dinput, "list", 4) == 0 || strncmp(dinput, "l", 1) == 0) {
      char* ch = Get2ndArg(dinput);
      if(ch != NULL)
        ListMem(ch);
      else
        ListMem(Int2Hex(pc));
    } else if(strcmp(dinput, "reg") == 0) {
      for(i = 0; i < 16; i++) {
        printf("R[%c] = %s  ", (i < 10)? i +'0': i -10 +'A', Int2Hex(reg[i]));
        if(i % 4 == 3) printf("\n");
      }
    } else if(strcmp(dinput, "help") == 0 || strcmp(dinput, "h") == 0) {
      usage(1);
    } else if(strcmp(dinput, "quit") == 0 || strcmp(dinput, "q") == 0) {
      lexit(0);
    }
  }
  return 0;
}

void OpenError(char *filename)
{
  fprintf(stderr, "error: can't open `%s'.\n", filename);
  lexit(1);
}


int ReadProg2Mem(char *filename)
{
  int line, total_line = 0;
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
    if(line > 255 || line == -1) {
      fprintf(stderr, "error: invalid line - %d.\n", line);
      lexit(1);
    }
    mem[line] = Hex2Int(buf +4);
    if(mem[line] == -1) {
      fprintf(stderr, "error: wrong instruction in line %d, abort.\n", line);
      lexit(1);
    }
    total_line++;
  }
  fclose(in);
  return total_line;
}

int ReadInput2Mem(char *filename)
{
  int total_inp_line = 0;
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
  return total_inp_line;
}

int Hex2Int(char *str)
{
  int i, sum = 0;
  for(i = 0; i < strlen(str); i++) {
    if((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'a' &&
          str[i] <= 'f') || (str[i] >= 'A' && str[i] <= 'F')) {
      if(str[i] >= 'a' || str[i] >= 'A')         /* a to f */
        sum += (str[i] -(str[i] >= 'a'? 'a': 'A') +10) *pow(16, strlen(str) -i -1);
      else
        sum += (str[i] -'0') *pow(16, strlen(str) -i -1);
    }
    else
      return -1;
  }
  return sum;
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
  for(i = 0; i < 2; i++) {
    tmp = bits[i];
    bits[i] = bits[3 -i];
    bits[3 -i] = tmp;
  }
  return bits;
}

char* Get2ndArg(char* string)
{
  strtok(string, " ");
  return strtok(NULL, " ");
}

void reset(int mode)
{
  int i;
  tdb.mode = mode;
  cycle = 0;
  pc = 16;
  inp_index = 0;
  /* clear register */
  for(i = 0; i < 16; i++)
    reg[i] = 0;
  /* clear memory */
  for(i = 0; i < 256; i++)
    mem[i] = 0;
  /* reload program */
  ReadProg2Mem(toyfile);
}

void ListMem(char *string)
{
  int a, b, i, two = false;
  for(i = 0; i < strlen(string) -2; i++)
    if(strncmp(string +i, "..", 2) == 0)
      two = true;
  if(two) {
    a = Hex2Int(strtok(string, "."));
    b = Hex2Int(strtok(NULL, "."));
  }
  else {
    a = Hex2Int(string) -6;
    b  = a + 12;
    if(a < 6) a = 0;
    if(b > 256) b = 256;
  }
  if(a == -1 || b == -1 || a > 256 || b > 256) {
    fprintf(stderr, "error: invalid line.\n");
    return;
  }
  for(i = a; i <= b; i++) {
    printf("%s: ", Int2Hex(i));
    printf("%s\n", Int2Hex(mem[i]));
  }
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

void FineInput(char* buf, int size) 
{  
  int i; 
  fgets(buf, size, stdin);
  for(i = 0; i < strlen(buf); i++)
    if(buf[i] == '\n') buf[i] = 0;
  if(strlen(buf) >= size -1) while(getchar() != '\n');
}

void lexit(int n)
{
#ifdef WIN32
  system("pause");
#endif
  exit(n);
}

void usage(int n)
{
  if(n == 0) {
    printf("Usage: toyvm [-d] [-v] ToyFile [InputFile]\n");
    printf("       toyvm [--version| -h| --help]\n\n");
    printf("NORMAL MODE\n");
    printf("    -d            Enter Debug Mode\n");
    printf("    -v            Verbose mode\n");
    printf("    ToyFile       *.toy file you want to run.\n");
    printf("    InputFile     This is optional, using InputFile instead of manually input.\n");
    printf("    -h, --help    Show this help list.\n");
    printf("    --version     Show version.\n\n");
  }
  printf("DEBUG MODE\n");
  printf("    run, r        Run program\n");
  printf("    step, s       Run program in step mode, each line is shown before executing.\n");
  printf("    next, n       Execute next program line (after stopping).\n");
  printf("    continue, c   Continue  running your program (after stopping, e.g. at a\n");
  printf("                  break point).\n");
  printf("    break [LINE]  Set a breakpoint, program will pause at break point.\n");
  printf("    info          Show breakpoint information.\n");
  printf("    delete [NUM]  Delete a breakpoint, NUM can be found by the `info' command.\n");
  printf("    reg           Show register data.\n");
  printf("    list [FMT]    List memory file. FMT can be a line or a range. If a line\n");
  printf("                  entered, list will show 13 line around it. If a ragne like\n");
  printf("                  A..B is entered, list will show lines from A to B.\n");
  printf("    verbose, v    Verbose mode, every instruction is shown before executing.\n");
  printf("    noverbose, nv Disable verbose mode.\n");
  printf("    quit, q       Quit toyvm debuger.\n\n");
  if(n == 1) return;
  printf("Please report bugs to Aitjcize <aitjcize@gmail.com>\n");
  lexit(0);
}
