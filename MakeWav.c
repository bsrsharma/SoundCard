/*
     From http://soundfile.sapp.org/doc/WaveFormat/
     
Offset  Size  Name             Description

The canonical WAVE format starts with the RIFF header:

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk 
                               following this number.  This is the size of the 
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some 
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this 
                               number.
44        *   Data             The actual sound data.

*/

/* Handshake tone for 0.1s @1400 Hz, 0.1s silence, 0.1s @ 2300Hz all at 8000 samples/s of 8 bits each; 2400 samples of 8 bits */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

union utag {
struct tagWaveFileHeader {

  char chunkId[4];
  unsigned int chunkSize;
  char format[4];

  char subChunk1Id[4];
  unsigned int subChunk1Size;
  unsigned short audioFormat;
  unsigned short numChannels;
  unsigned int sampleRate;
  unsigned int byteRate;
  unsigned short blockAlign;
  unsigned short bitsPerSample;

  char subChunk2Id[4];
  unsigned int subChunk2Size;

} stWaveFileHeader;
  char cWFH[44];
} unWaveFileHeader = {{
                     {'R', 'I', 'F', 'F'},
                     2436,                           /* File size - 8; Also equal to 36 + subChunk2Size */
                     {'W', 'A', 'V', 'E'},

                     {'f', 'm', 't', ' '},                     
                     16,
                     1,
                     1,
                     8000,
                     8000,
                     1,
                     8,

                     {'d', 'a','t','a'},
                     2400,                         /* Number of samples * number of channels * bits per channel/8 */


  } };
  
                  

int main()
{
  FILE *fp = fopen("HandshakeTone.wav", "w+b");
  int i,j;
  double dAmpl, dScaledAmpl;
  unsigned char ucAmpl;

  if (fp == NULL)
    {
      printf("fopen fail on HandshakeTone.wav\n");
      exit( -1);
    }

  for (i = 0; i < sizeof(unWaveFileHeader); i++)
     fprintf(fp, "%c", unWaveFileHeader.cWFH[i] );

 

  printf("wrote %d bytes of HandshakeTone.wav\n", sizeof(unWaveFileHeader));

  /* 800 samples of 1400Hz tone, 8 bits scaled 0 - 255; 0.1s of 8000 samples/s */

  /* 0.1s has 140 sinewaves in 800 samples; i.e. 7 sinewaves in 40 samples *
   * use 0.35*PAI for increment in computing waveform                      *
   *
   * and repeat the pattern 20 times                                       *
   */

  for (i=0; i < 20; i++)
    for (j=0; j < 40; j++)
    {
      dAmpl = sin((40.0*i+j)*0.35*3.14159265);
      dScaledAmpl = (dAmpl+1.0)*127.5;
      ucAmpl = round(dScaledAmpl);
      // printf("i = %d, j = %d, dAmpl = %f, dScaledAmpl = %f, ucAmpl = %d\n", i, j, dAmpl, dScaledAmpl, ucAmpl);

      fprintf(fp, "%c", ucAmpl);
    }

  /* 800 samples of silence, 0.1s at 8000 samples/s */

  for (i=0; i < 800; i++)
  {
    ucAmpl = 127;
    fprintf(fp, "%c", ucAmpl);
  }

  /* 800 samples of 2300Hz tone, 8 bits scaled 0 - 255; 0.1s of 8000 samples/s */

  /* 0.1s has 230 sinewaves in 800 samples; i.e. 23 sinewaves in 80 samples *
   * use (23/40)*PAI for increment in computing waveform                      *
   *
   * and repeat the pattern 10 times                                      *
   */

  for (i=0; i < 10; i++)
    for (j=0; j < 80; j++)
    {
      dAmpl = sin((80.0*i+j)*(23.0/40.0)*3.14159265);
      dScaledAmpl = (dAmpl+1.0)*127.5;
      ucAmpl = round(dScaledAmpl);
      // printf("i = %d, j = %d, dAmpl = %f, dScaledAmpl = %f, ucAmpl = %d\n", i, j, dAmpl, dScaledAmpl, ucAmpl);

      fprintf(fp, "%c", ucAmpl);
    }

  fclose(fp);

  fp = fopen("KissofTone.wav", "w+b");

  if (fp == NULL)
  {
      printf("fopen fail on KissoffTone.wav\n");
      exit( -1);
  }

  unWaveFileHeader.stWaveFileHeader.chunkSize = 7236;
  unWaveFileHeader.stWaveFileHeader.subChunk2Size = 7200;

  for (i = 0; i < sizeof(unWaveFileHeader); i++)
     fprintf(fp, "%c", unWaveFileHeader.cWFH[i] );

 

  printf("wrote %d bytes of KissoffTone.wav\n", sizeof(unWaveFileHeader));

  /* 7200 samples of 1400Hz tone, 8 bits scaled 0 - 255; 0.9s of 8000 samples/s */

  /* 0.9s has 1260 sinewaves in 7200 samples; i.e. 7 sinewave in 40 samples *
   * use 0.35*PAI for increment in computing waveform                      *
   *
   * and repeat the pattern 180 times                                      *
   */

  for (i=0; i < 180; i++)
    for (j=0; j < 40; j++)
    {
      dAmpl = sin((40.0*i+j)*0.35*3.14159265);
      dScaledAmpl = (dAmpl+1.0)*127.5;
      ucAmpl = round(dScaledAmpl);
      // printf("i = %d, j = %d, dAmpl = %f, dScaledAmpl = %f, ucAmpl = %d\n", i, j, dAmpl, dScaledAmpl, ucAmpl);

      fprintf(fp, "%c", ucAmpl);
    }

  fclose(fp);


  return 0;
}



