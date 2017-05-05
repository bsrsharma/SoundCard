/*
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

/* #define SOUND_DEV  "/dev/dsp" */
#define SOUND_DEV  "/dev/snd/pcmC0D0p" 


#include <stdio.h>

typedef char sample;
/* --------------------------------------------------------------- */

#include <fcntl.h>

/*
 * take the sine of x, where x is 0 to 65535 (for 0 to 360 degrees)
 */
float mysine(short in)
{
  static coef[] = {
     3.140625, 0.02026367, -5.325196, 0.5446778, 1.800293 };
  float x,y,res;
  int sign,i;
 
  printf("(%d, ", in);
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
  printf("%f )", res*sign);
  return(res * sign); 
}

/*
 * play tone1 and tone2 (in Hz)
 * for 'length' milliseconds
 * outputs samples to sound_out
 */
two_tones(int sound_out, unsigned int tone1, unsigned int tone2, unsigned int length)
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
  printf("<");
  for( c1=0, c2=0, i=0 ;
       i < l;
       i++, c1+= ad1, c2+= ad2 ) {
    out = (mysine(c1) + mysine(c2)) * 0.5;
    cout[x++] = FLOAT_TO_SAMPLE(out);
    if (x==BLEN) {
      printf("%d ", cout[x-1]);
      write(sound_out, cout, x * sizeof(sample));
      x=0;
    }
    // printf("\n");
  }
  write(sound_out, cout, x);
  printf("> ");
}

/*
 * silence on 'sound_out'
 * for length milliseconds
 */
silence(int sound_out, unsigned int length)
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
dtmf(int sound_fd, int digit, int length)
{
  /* Freqs for 0-9, *, # */
  static int row[] = {
    941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941 };
  static int col[] = {
    1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1447,
    1209, 1477 };
  printf("{%d %d} ", row[digit], col[digit]);
  two_tones(sound_fd, row[digit], col[digit], length);
}

/*
 * take a string and output as dtmf
 * valid characters, 0-9, *, #
 * all others play as 50ms silence 
 */
dial(int sound_fd, char *number)
{
  int i,x;
  char c;

  printf ("dial ");
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
     {
       printf("%d ", x);
       dtmf(sound_fd, x, 50);
     }
     silence(sound_fd,50);
  }
  printf("\n");
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
  fgets(number,98, stdin);
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
