/*
Volume Seven, Issue Fifty

                                     13 of 16
 
                         ===============================
                         DTMF Encoding and Decoding In C
                                    by Mr. Blue
                         ===============================

  
Introduction
------------
    DTMF tones are the sounds emitted when you dial a number on your touch 
tone phone.  Modems have traditionally been the device used to generate
these tones from a computer.  But the more sophisticated modems on the
market today are nothing more than a DSP (digital signal processor) with
accompanying built-in software to generate and interpet analog sounds into
digital data.  The computers sitting on your desk have more cpu power,
a more complex OS, and very often a just as sophisticated DSP.  There is
no reason you can not duplicate the functionality of a modem from right
inside of unix software, providing you with a lot easier to understand and
modify code.  

    In this article I provide the source code to both encode and decode
DTMF tones.  There are numerous uses for this code, for use in unix based 
phone scanning and war dialing programs, voice mail software, automated
pbx brute force hacking, and countless other legitimate and not so
legitimate uses.

    I will not go into depth explaining the underlying mathematical
theories behind this code.  If you are of a sufficient math background I
would encourage you to research and learn about the algorithms used from
your local college library; it is not my intent to summarize these
algorithms, only to provide unix C code that can be used on its own or
expanded to be used as part of a larger program.  

    Use the extract utility included with Phrack to save the individual
source files out to the dtmf/ directory.  If you find this code useful, I
would encourage you to show your appreciation by sharing some of your own
knowledge with Phrack.    
*/

/* <++> dtmf/detect.h */

/* 
 *
 * goertzel aglorithm, find the power of different
 * frequencies in an N point DFT.
 *
 * ftone/fsample = k/N   
 * k and N are integers.  fsample is 8000 (8khz)
 * this means the *maximum* frequency resolution
 * is fsample/N (each step in k corresponds to a
 * step of fsample/N hz in ftone)
 *
 * N was chosen to minimize the sum of the K errors for
 * all the tones detected...  here are the results :
 *
 * Best N is 240, with the sum of all errors = 3.030002
 * freq  freq actual   k     kactual  kerr
 * ---- ------------  ------ ------- -----
 *  350 (366.66667)   10.500 (11)    0.500
 *  440 (433.33333)   13.200 (13)    0.200
 *  480 (466.66667)   14.400 (14)    0.400
 *  620 (633.33333)   18.600 (19)    0.400
 *  697 (700.00000)   20.910 (21)    0.090
 *  700 (700.00000)   21.000 (21)    0.000
 *  770 (766.66667)   23.100 (23)    0.100
 *  852 (866.66667)   25.560 (26)    0.440
 *  900 (900.00000)   27.000 (27)    0.000
 *  941 (933.33333)   28.230 (28)    0.230
 * 1100 (1100.00000)  33.000 (33)    0.000
 * 1209 (1200.00000)  36.270 (36)    0.270
 * 1300 (1300.00000)  39.000 (39)    0.000
 * 1336 (1333.33333)  40.080 (40)    0.080
 **** I took out 1477.. too close to 1500
 * 1477 (1466.66667)  44.310 (44)    0.310
 ****
 * 1500 (1500.00000)  45.000 (45)    0.000
 * 1633 (1633.33333)  48.990 (49)    0.010
 * 1700 (1700.00000)  51.000 (51)    0.000
 * 2400 (2400.00000)  72.000 (72)    0.000
 * 2600 (2600.00000)  78.000 (78)    0.000
 *
 * notice, 697 and 700hz are indestinguishable (same K)
 * all other tones have a seperate k value.  
 * these two tones must be treated as identical for our
 * analysis.
 *
 * The worst tones to detect are 350 (error = 0.5, 
 * detet 367 hz) and 852 (error = 0.44, detect 867hz). 
 * all others are very close.
 *
 */

#define FSAMPLE  8000
#define N        240

int k[] = { 11, 13, 14, 19, 21, 23, 26, 27, 28, 33, 36, 39, 40,
 /*44,*/ 45, 49, 51, 72, 78, };

/* coefficients for above k's as:
 *   2 * cos( 2*pi* k/N )
 */
float coef[] = {
1.917639, 1.885283, 1.867161, 1.757634, 
1.705280, 1.648252, 1.554292, 1.520812, 1.486290, 
1.298896, 1.175571, 1.044997, 1.000000, /* 0.813473,*/ 
0.765367, 0.568031, 0.466891, -0.618034, -0.907981,  };

#define X1    0    /* 350 dialtone */
#define X2    1    /* 440 ring, dialtone */
#define X3    2    /* 480 ring, busy */
#define X4    3    /* 620 busy */

#define R1    4    /* 697, dtmf row 1 */
#define R2    5    /* 770, dtmf row 2 */
#define R3    6    /* 852, dtmf row 3 */
#define R4    8    /* 941, dtmf row 4 */
#define C1   10    /* 1209, dtmf col 1 */
#define C2   12    /* 1336, dtmf col 2 */
#define C3   13    /* 1477, dtmf col 3 */
#define C4   14    /* 1633, dtmf col 4 */

#define B1    4    /* 700, blue box 1 */
#define B2    7    /* 900, bb 2 */
#define B3    9    /* 1100, bb 3 */
#define B4   11    /* 1300, bb4 */
#define B5   13    /* 1500, bb5 */
#define B6   15    /* 1700, bb6 */
#define B7   16    /* 2400, bb7 */
#define B8   17    /* 2600, bb8 */

#define NUMTONES 18 

/* values returned by detect 
 *  0-9     DTMF 0 through 9 or MF 0-9
 *  10-11   DTMF *, #
 *  12-15   DTMF A,B,C,D
 *  16-20   MF last column: C11, C12, KP1, KP2, ST
 *  21      2400
 *  22      2600
 *  23      2400 + 2600
 *  24      DIALTONE
 *  25      RING
 *  26      BUSY
 *  27      silence
 *  -1      invalid
 */
#define D0    0
#define D1    1
#define D2    2
#define D3    3
#define D4    4
#define D5    5
#define D6    6
#define D7    7
#define D8    8
#define D9    9
#define DSTAR 10
#define DPND  11
#define DA    12
#define DB    13
#define DC    14
#define DD    15
#define DC11  16
#define DC12  17
#define DKP1  18
#define DKP2  19
#define DST   20
#define D24   21 
#define D26   22
#define D2426 23
#define DDT   24
#define DRING 25
#define DBUSY 26
#define DSIL  27

/* translation of above codes into text */
char *dtran[] = {
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "*", "#", "A", "B", "C", "D", 
  "+C11 ", "+C12 ", " KP1+", " KP2+", "+ST ",
  " 2400 ", " 2600 ", " 2400+2600 ",
  " DIALTONE ", " RING ", " BUSY ","" };

#define RANGE  0.1           /* any thing higher than RANGE*peak is "on" */
#define THRESH 100.0         /* minimum level for the loudest tone */
#define FLUSH_TIME 100       /* 100 frames = 3 seconds */

/*<-->
<++> dtmf/detect.c
*/

/*
 * detect.c
 * This program will detect MF tones and normal
 * dtmf tones as well as some other common tones such
 * as BUSY, DIALTONE and RING.
 * The program uses a goertzel algorithm to detect
 * the power of various frequency ranges.
 *
 * input is assumed to be 8 bit samples.  The program
 * can use either signed or unsigned samples according
 * to a compile time option:
 *
 *    cc  -DUNSIGNED detect.c -o detect
 *
 * for unsigned input (soundblaster) and:
 *
 *    cc  detect.c -o detect
 *
 * for signed input (amiga samples)
 * if you dont want flushes,  -DNOFLUSH
 * 
 *                            Tim N.
 */

#include <stdio.h>
#include <math.h>
/*
 #include "detect.h"
 */

/*
 * calculate the power of each tone according
 * to a modified goertzel algorithm described in
 *  _digital signal processing applications using the
 *  ADSP-2100 family_ by Analog Devices
 *
 * input is 'data',  N sample values
 *
 * ouput is 'power', NUMTONES values
 *  corresponding to the power of each tone 
 */

calc_power(data,power)
#ifdef UNSIGNED
unsigned char *data;
#else
char *data;
#endif
float *power;
{
  float u0[NUMTONES],u1[NUMTONES],t,in;
  int i,j;
  
  for(j=0; j<NUMTONES; j++) {
    u0[j] = 0.0;
    u1[j] = 0.0;
  }
  for(i=0; i<N; i++) {   /* feedback */
#ifdef UNSIGNED
    in = ((int)data[i] - 128) / 128.0;
#else
    in = data[i] / 128.0;
#endif
    for(j=0; j<NUMTONES; j++) {
      t = u0[j];
      u0[j] = in + coef[j] * u0[j] - u1[j];
      u1[j] = t;
    }
  }
  for(j=0; j<NUMTONES; j++)   /* feedforward */
    power[j] = u0[j] * u0[j] + u1[j] * u1[j] - coef[j] * u0[j] * u1[j]; 
  return(0);
}


/*
 * detect which signals are present.
 *
 * return values defined in the include file
 * note: DTMF 3 and MF 7 conflict.  To resolve
 * this the program only reports MF 7 between
 * a KP and an ST, otherwise DTMF 3 is returned
 */
decode(data)
char *data;
{
  float power[NUMTONES],thresh,maxpower;
  int on[NUMTONES],on_count;
  int bcount, rcount, ccount;
  int row, col, b1, b2, i;
  int r[4],c[4],b[8];
  static int MFmode=0;
  
  calc_power(data,power);
  for(i=0, maxpower=0.0; i<NUMTONES;i++)
    if(power[i] > maxpower)
      maxpower = power[i]; 
/*
for(i=0;i<NUMTONES;i++) 
  printf("%f, ",power[i]);
printf("\n");
*/

  if(maxpower < THRESH)  /* silence? */ 
    return(DSIL);
  thresh = RANGE * maxpower;    /* allowable range of powers */
  for(i=0, on_count=0; i<NUMTONES; i++) {
    if(power[i] > thresh) { 
      on[i] = 1;
      on_count ++;
    } else
      on[i] = 0;
  }

/*
printf("%4d: ",on_count);
for(i=0;i<NUMTONES;i++)
  putchar('0' + on[i]);
printf("\n");
*/

  if(on_count == 1) {
    if(on[B7]) 
      return(D24);
    if(on[B8])
      return(D26);
    return(-1);
  }
 
  if(on_count == 2) {
    if(on[X1] && on[X2])
      return(DDT);
    if(on[X2] && on[X3])
      return(DRING);
    if(on[X3] && on[X4])
      return(DBUSY);
    
    b[0]= on[B1]; b[1]= on[B2]; b[2]= on[B3]; b[3]= on[B4];
    b[4]= on[B5]; b[5]= on[B6]; b[6]= on[B7]; b[7]= on[B8];
    c[0]= on[C1]; c[1]= on[C2]; c[2]= on[C3]; c[3]= on[C4];
    r[0]= on[R1]; r[1]= on[R2]; r[2]= on[R3]; r[3]= on[R4];

    for(i=0, bcount=0; i<8; i++) {
      if(b[i]) {
        bcount++;
        b2 = b1;
        b1 = i;
      }
    }
    for(i=0, rcount=0; i<4; i++) {
      if(r[i]) {
        rcount++;
        row = i;
      }
    }
    for(i=0, ccount=0; i<4; i++) {
      if(c[i]) {
        ccount++;
        col = i;
      }
    }

    if(rcount==1 && ccount==1) {   /* DTMF */
      if(col == 3)  /* A,B,C,D */
        return(DA + row);
      else {
        if(row == 3 && col == 0 ) 
           return(DSTAR);
        if(row == 3 && col == 2 )
           return(DPND);
        if(row == 3)
           return(D0);
        if(row == 0 && col == 2) {   /* DTMF 3 conflicts with MF 7 */
          if(!MFmode)
            return(D3);
        } else 
          return(D1 + col + row*3);
      }
    }

    if(bcount == 2) {       /* MF */
      /* b1 has upper number, b2 has lower */
      switch(b1) {
        case 7: return( (b2==6)? D2426: -1); 
        case 6: return(-1);
        case 5: if(b2==2 || b2==3)  /* KP */
                  MFmode=1;
                if(b2==4)  /* ST */
                  MFmode=0; 
                return(DC11 + b2);
        /* MF 7 conflicts with DTMF 3, but if we made it
         * here then DTMF 3 was already tested for 
         */
        case 4: return( (b2==3)? D0: D7 + b2);
        case 3: return(D4 + b2);
        case 2: return(D2 + b2);
        case 1: return(D1);
      }
    }
    return(-1);
  }

  if(on_count == 0)
    return(DSIL);
  return(-1); 
}

read_frame(fd,buf)
int fd;
char *buf;
{
  int i,x;

  for(i=0; i<N; ) {
    x = read(fd, &buf[i], N-i);
    if(x <= 0) 
      return(0);
    i += x;
  } 
  return(1);
}

/*
 * read in frames, output the decoded
 * results
 */
dtmf_to_ascii(fd1, fd2)
int fd1;
FILE *fd2;
{
  int x,last= DSIL;
  char frame[N+5];
  int silence_time;

  while(read_frame(fd1, frame)) {
    x = decode(frame); 
/*
if(x== -1) putchar('-');
if(x==DSIL) putchar(' ');
if(x!=DSIL && x!=-1) putchar('a' + x);
fflush(stdout);
continue;
*/

    if(x >= 0) {
      if(x == DSIL)
        silence_time += (silence_time>=0)?1:0 ;
      else
        silence_time= 0;
      if(silence_time == FLUSH_TIME) {
        fputs("\n",fd2);
        silence_time= -1;   /* stop counting */
      }

      if(x != DSIL && x != last &&
         (last == DSIL || last==D24 || last == D26 ||
          last == D2426 || last == DDT || last == DBUSY ||
          last == DRING) )  { 
        fputs(dtran[x], fd2);
#ifndef NOFLUSH
        fflush(fd2);
#endif
      }
      last = x;
    }
  }
  fputs("\n",fd2);
}

main(argc,argv) 
int argc;
char **argv;
{
  FILE *output;
  int input;

  input = 0;
  output = stdout;
  switch(argc) {
    case 1:  break;
    case 3:  output = fopen(argv[2],"w");
             if(!output) {
               perror(argv[2]);
               return(-1);
             }
             /* fall through */
    case 2:  input = open(argv[1],0);
             if(input < 0) {
               perror(argv[1]);
               return(-1);
             }
             break;
     default:
        fprintf(stderr,"usage:  %s [input [output]]\n",argv[0]);
        return(-1);
  }
  dtmf_to_ascii(input,output);
  fputs("Done.\n",output);
  return(0);
}

#if 0
/*
<-->
<++> dtmf/gen.c
*/

/* -------- local defines (if we had more.. seperate file) ----- */
#define FSAMPLE   8000   /* sampling rate, 8KHz */

/*
 * FLOAT_TO_SAMPLE converts a float in the range -1.0 to 1.0 
 * into a format valid to be written out in a sound file
 * or to a sound device 
 */
#ifdef SIGNED
#  define FLOAT_TO_SAMPLE(x)    ((char)((x) * 127.0))
#else
#  define FLOAT_TO_SAMPLE(x)    ((char)((x + 1.0) * 127.0))
#endif

#define SOUND_DEV  "/dev/dsp"
typedef char sample;
/* --------------------------------------------------------------- */

#include <fcntl.h>

/*
 * take the sine of x, where x is 0 to 65535 (for 0 to 360 degrees)
 */
float mysine(in)
short in;
{
  static coef[] = {
     3.140625, 0.02026367, -5.325196, 0.5446778, 1.800293 };
  float x,y,res;
  int sign,i;
 
  if(in < 0) {       /* force positive */
    sign = -1;
    in = -in;
  } else
    sign = 1;
  if(in >= 0x4000)      /* 90 degrees */
    in = 0x8000 - in;   /* 180 degrees - in */
  x = in * (1/32768.0); 
  y = x;               /* y holds x^i) */
  res = 0;
  for(i=0; i<5; i++) {
    res += y * coef[i];
    y *= x;
  }
  return(res * sign); 
}

/*
 * play tone1 and tone2 (in Hz)
 * for 'length' milliseconds
 * outputs samples to sound_out
 */
two_tones(sound_out,tone1,tone2,length)
int sound_out;
unsigned int tone1,tone2,length;
{
#define BLEN 128
  sample cout[BLEN];
  float out;
  unsigned int ad1,ad2;
  short c1,c2;
  int i,l,x;
   
  ad1 = (tone1 << 16) / FSAMPLE;
  ad2 = (tone2 << 16) / FSAMPLE;
  l = (length * FSAMPLE) / 1000;
  x = 0;
  for( c1=0, c2=0, i=0 ;
       i < l;
       i++, c1+= ad1, c2+= ad2 ) {
    out = (mysine(c1) + mysine(c2)) * 0.5;
    cout[x++] = FLOAT_TO_SAMPLE(out);
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * silence on 'sound_out'
 * for length milliseconds
 */
silence(sound_out,length)
int sound_out;
unsigned int length;
{
  int l,i,x;
  static sample c0 = FLOAT_TO_SAMPLE(0.0);
  sample cout[BLEN];

  x = 0;
  l = (length * FSAMPLE) / 1000;
  for(i=0; i < l; i++) {
    cout[x++] = c0;
    if (x==BLEN) {
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
  }
  write(sound_out, cout, x);
}

/*
 * play a single dtmf tone
 * for a length of time,
 * input is 0-9 for digit, 10 for * 11 for #
 */
dtmf(sound_fd, digit, length)
int sound_fd;
int digit, length;
{
  /* Freqs for 0-9, *, # */
  static int row[] = {
    941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941 };
  static int col[] = {
    1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1447,
    1209, 1477 };

  two_tones(sound_fd, row[digit], col[digit], length);
}

/*
 * take a string and output as dtmf
 * valid characters, 0-9, *, #
 * all others play as 50ms silence 
 */
dial(sound_fd, number)
int sound_fd;
char *number;
{
  int i,x;
  char c;

  for(i=0;number[i];i++) {
     c = number[i];
     x = -1;
     if(c >= '0' && c <= '9')
       x = c - '0';
     else if(c == '*')
       x = 10;
     else if(c == '#')
       x = 11;
     if(x >= 0)
       dtmf(sound_fd, x, 50);
     silence(sound_fd,50);
  }
}

main()
{
  int sfd;
  char number[100];

  sfd = open(SOUND_DEV,O_RDWR);
  if(sfd<0) {
    perror(SOUND_DEV);
    return(-1);
  }
  printf("Enter fone number: ");
  gets(number);
  dial(sfd,number);
}

/*
<-->
<++> dtmf/Makefile
*/

/*
#
# Defines:
#  UNSIGNED  -  use unsigned 8 bit samples
#               otherwise use signed 8 bit samples
#

CFLAGS= -DUNSIGNED

default:	detect gen

detect: detect.c
	$(CC) detect.c -o detect

gen:	gen.c
	$(CC) gen.c -o gen

clobber: clean
	rm -rf detect gen 

clean:
	rm -rf *.o core a.out
<-->

EOF
*/

#endif
