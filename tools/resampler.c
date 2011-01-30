// resampler.c
// Copyright 2011 Gordon JC Pearce <gordon@gjcp.net>
// Part of nekosynth gyoza
// GPLv3 applies

#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <math.h>

#define TABLE_SIZE 24576
#define OUTPUT "wave.h"

int main(int argc, char *argv[]) {
	SNDFILE *input;
	FILE *output;
	SF_INFO sf_info;
	int samplerate, frames, out_rate;
	
	float y;
	
	int i,j,p;
	
	float *in_buf, *mono_buf, *out_buf;
	
	
	if (argc != 2) {
		printf("./resampler <filename>\n");
		return 1;
	}

	memset (&sf_info, 0, sizeof(sf_info)) ;

	input = sf_open(argv[1], SFM_READ, &sf_info);
	if (!input) {
		printf("Cannot open %s\n", argv[1]);
		return 1;
	}

	samplerate = sf_info.samplerate;
	frames = (int)sf_info.frames;
		
	out_rate = (int)((TABLE_SIZE/(float)frames)*samplerate);
	printf("Sample is %d samples long and %d Hz sample rate\n", frames, samplerate);
	printf("For %d samples, resample to %dHz\n", TABLE_SIZE, out_rate);
	
	in_buf = malloc(frames * sf_info.channels * sizeof(float));
	sf_read_float(input, in_buf, sf_info.channels*frames);
	
	mono_buf = malloc(frames * sizeof(float));
	
	// ugh, FIXME here
	// basic idea is, however many channels per frame, sum the channels and move it to the mono
	// buffer, suitably scaled
	p = 0;
	for(i = 0; i < frames; i++) {
		y = 0;
		for (j = 0; j < sf_info.channels; j++) {
			y += in_buf[p++];
		}
		mono_buf[i] = y/(j+1);	
	}
	free(in_buf);   // we're actually done with this now
	sf_close(input);	// and the file handle you rode in on

	out_buf = malloc(TABLE_SIZE * sizeof(float));
	y = 0;
	for(i = 0; i <  TABLE_SIZE; i++) {
		j = (int)y;
		y += (float)frames/TABLE_SIZE;
		out_buf[i] = mono_buf[j];
	}
	free(mono_buf);
	
	// normalise (very important if you've only got 8 bits to play with)
	y = 0;
	for(i = 0; i < TABLE_SIZE; i++) {
		if (fabs(out_buf[i]) > y) y = fabs(out_buf[i]);
	}
	if (y != 0) {
		y = 1/y; 
		for(i = 0; i < TABLE_SIZE; i++) {
			out_buf[i] *= y;
		}
	}

#if 0
	// write the sample as an 8-bit wav at the correct sample rate
	sf_info.channels=1;
	sf_info.samplerate = out_rate;
	sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
	input = sf_open("out.wav", SFM_WRITE, &sf_info);
	sf_write_float(input, out_buf, TABLE_SIZE);
	sf_close(input);
#endif
	
	// create the output file, blatting any existing one
	output = fopen(OUTPUT, "w");
	fprintf(output, "// "OUTPUT" - contains the sample data as a big hex dump\n");
	fprintf(output, "// Copyright 2011 Gordon JC Pearce <gordon@gjcp.net>\n");
	fprintf(output, "// automatically generated by gyoza resampler\n\n");
	fprintf(output, "#include <avr/pgmspace.h>\n#define SAMPLE_RATE %d\n\n", out_rate);
	
	fprintf(output, "PROGMEM  prog_uchar wave[]  = {\n");
	for (i = 0; i < TABLE_SIZE; i++) {
		j = 127 + (128 * out_buf[i]);
		fprintf(output, "0x%02x, ", (unsigned char)j);
		if ((i & 0xff)==0xff) fprintf(output, "\n");
	}
	fprintf(output, "};\n");
	free(out_buf);
	close(output);

}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
