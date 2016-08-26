#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

enum RECORD_TYPE {
	REC_TYPE_DATA,
	REC_TYPE_EOF,
	REC_TYPE_EXTENDED_SEGMENT,
	REC_TYPE_START_SEGMENT,
	REC_TYPE_EXTENDED_LINEAR,
	REC_TYPE_START_LINEAR
};

typedef struct record {
	uint32_t start_off; /* Colon position in the file */
	uint8_t byte_count;
	uint16_t address;
	uint8_t type;
	uint8_t * type_str;
	uint8_t * data;
	uint8_t checksum;
	uint8_t checksum_calc; /* Used to check if the calculation is equal to the checksum found on the hex file */
	uint8_t checksum_valid;
} __attribute__((packed)) record_t;

typedef struct ihex {
	record_t * record_list;
	uint32_t record_count;
} __attribute__((packed)) ihex_t;

char * strdup (const char * s) {
	char * d = (char*)malloc(strlen(s) + 1);
	if (!d) return NULL;
	strcpy (d, s);
	return d;
}

unsigned int filesize(FILE * fp) {
	if(!fp) return 0;
	fseek(fp, 0, SEEK_END);
	unsigned int ret = (unsigned int)ftell(fp);
	rewind(fp);
	return ret;
}

char * fileread(FILE * fp) {
	if(!fp) return 0;
	unsigned int buffer_size = filesize(fp);
	char * buff = (char*)malloc(buffer_size);
	fread(buff, sizeof(char), buffer_size, fp);
	return buff;
}

uint8_t byte_count_off(char * ihex, uint32_t colon_off) {
	char two_digits[3];
	char * pos = ihex + colon_off;
	two_digits[0] = *(pos + 1);
	two_digits[1] = *(pos + 2);
	two_digits[2] = '\0';
	return (uint8_t)strtol(two_digits, NULL, 16);
}

uint16_t address_off(char * ihex, uint32_t colon_off) {
	char four_digits[5];
	char * pos = ihex + colon_off + 2;
	four_digits[0] = *(pos + 1);
	four_digits[1] = *(pos + 2);
	four_digits[2] = *(pos + 3);
	four_digits[3] = *(pos + 4);
	four_digits[4] = '\0';
	return (uint8_t)strtol(four_digits, NULL, 16);
}

uint8_t type_off(char * ihex, uint32_t colon_off) {
	char two_digits[3];
	char * pos = ihex + colon_off + 2 + 4;
	two_digits[0] = *(pos + 1);
	two_digits[1] = *(pos + 2);
	two_digits[2] = '\0';
	return (uint8_t)strtol(two_digits, NULL, 16);
}

void data_off(char * ihex, uint32_t colon_off, uint8_t * data_ptr, uint32_t data_len) {
	char * pos = ihex + colon_off + 2 + 4 + 2 + 1;

	int i = 0;
	for(char * ptr = pos; ptr < pos + (data_len * 2) - 1; ptr += 2) {
		char two_digits[3];
		two_digits[0] = *(ptr);
		two_digits[1] = *(ptr + 1);
		two_digits[2] = '\0';
		data_ptr[i++] = (uint8_t)strtol(two_digits, NULL, 16);		
	}
}

uint8_t checksum_off(char * ihex, uint32_t colon_off, uint32_t data_len) {
	char two_digits[3];
	char * pos = ihex + colon_off + 2 + 4 + 2 + (data_len * 2);
	two_digits[0] = *(pos + 1);
	two_digits[1] = *(pos + 2);
	two_digits[2] = '\0';
	return (uint8_t)strtol(two_digits, NULL, 16);
}

uint8_t checksum_calculate(char * ihex, uint32_t colon_off, uint32_t data_len) {
	uint8_t csum = 0;
	for(char * ptr = ihex + colon_off + 1; ptr < ihex + colon_off + 2 + 4 + 2 + (data_len * 2); ptr += 2) {
		char two_digits[3];
		two_digits[0] = *(ptr);
		two_digits[1] = *(ptr + 1);
		two_digits[2] = '\0';
		csum += (uint8_t)strtol(two_digits, NULL, 16);	
	}
	return (~csum) + 1;
}

ihex_t * parse_ihex_file(char * hex, unsigned int hex_size) {
	ihex_t * ret = 0;
	uint32_t record_count = 0;
	for(int i = 0; i < hex_size; i++)
		if(hex[i] == '\n') 
			record_count++;
	
	if(record_count) {
		ret = (ihex_t*)malloc(sizeof(ihex_t));
		memset(ret, 0, sizeof(ihex_t));
		ret->record_count = record_count;

		ret->record_list = (record_t*)malloc(sizeof(record_t) * record_count);
		memset(ret->record_list, 0, sizeof(record_t) * record_count);
		
		/* Fill up every record: */
		uint32_t rec_count = 0;
		for(int i = 0; i < hex_size; i++) {
			if(hex[i] == ':')  {
				/* Found the start of a record: */
				record_t * rec = &ret->record_list[rec_count];
				
				/* Collect the data: */
				rec->start_off = i;
				rec->byte_count = byte_count_off(hex, i);
				rec->address = address_off(hex, i);
				rec->type = type_off(hex, i);
				switch(rec->type) {
					case REC_TYPE_DATA: rec->type_str = strdup("Data"); break;
					case REC_TYPE_EOF: rec->type_str = strdup("End Of File"); break;
					case REC_TYPE_EXTENDED_SEGMENT: rec->type_str = strdup("Extended Segment Address"); break;
					case REC_TYPE_START_SEGMENT: rec->type_str = strdup("Start Segment Address"); break;
					case REC_TYPE_EXTENDED_LINEAR: rec->type_str = strdup("Extended Linear Address"); break;
					case REC_TYPE_START_LINEAR: rec->type_str = strdup("Start Linear Address"); break;
					default: rec->type_str = strdup("Unknown"); break;
				}
				if(rec->byte_count) {
					rec->data = (uint8_t*)malloc(sizeof(uint8_t) * rec->byte_count);
					data_off(hex, i, rec->data, rec->byte_count);
				}
				rec->checksum = checksum_off(hex, i, rec->byte_count);
				rec->checksum_calc = checksum_calculate(hex, i, rec->byte_count);
				rec->checksum_valid = rec->checksum_calc == rec->checksum;
				rec_count++;
			}
			/* Walk through the rest of the hex file until a colon is found */			
		}
	}
	return ret;
}

void dump_ihex(ihex_t * ihex) {
	for(int i = 0; i < ihex->record_count; i++) {
		record_t * rec = &ihex->record_list[i];
		printf("\n\t* Record %d:\n\t\t- Data: ", i + 1);
		for(int j = 0; j < rec->byte_count; j++)
			printf("%02X%s", rec->data[j], j < rec->byte_count - 1 ? "," : "");
		printf("\n\t\t- Size: %d (decimal)\n\t\t- Address: %02X\n\t\t- Type: %s (%d)\n\t\t- Checksum: %02X = %s", rec->byte_count, rec->address,
			rec->type_str, rec->type, rec->checksum, rec->checksum_valid ? "VALID" : "INVALID");
		if(!rec->checksum_valid)
			printf(" (calc: %02X)", rec->checksum_calc);
	}
}

void free_ihex(ihex_t * ihex) {
	if(!ihex) return;
	for(int i = 0; i < ihex->record_count; i++) {
		if(ihex->record_list[i].byte_count && ihex->record_list[i].data)
			free(ihex->record_list[i].data);
		if(ihex->record_list[i].type_str)
			free(ihex->record_list[i].type_str);
	}
	free(ihex->record_list);
	free(ihex);
	ihex = 0;
}

void help() {
	printf("\n> Usage: ihex hexfile.hex [options]\n> Options:\n\
   1: -o Output binary data into file 'hexfile.bin' (the name is a placeholder)\n");
}

int main(char argc, char ** argv) {
	printf(">>>> Intel Hex file Parser <<<<\n");
	if(argc <= 1) {
		help();
		return 1;
	}

	char flag_output_bin = 0;
	char * hexfile = 0;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-o")) flag_output_bin = 1;
		else if(!strcmp(argv[i], "-h")) { help(); return 2; }
		else hexfile = argv[i];
	}

	if(!hexfile) {
		help();
		return 3;
	}

	FILE * fp = fopen(hexfile, "rb");
	if(!fp) {
		printf("\n> ERROR: Could not open '%s'", hexfile);
		return 4;
	}

	printf("\n> Loading '%s' ...\n", hexfile);
	unsigned int hex_size = filesize(fp);
	char * hex_contents   = fileread(fp);
	fclose(fp);
	printf("> Parsing '%s' ...\n", hexfile);
	ihex_t * ihex = parse_ihex_file(hex_contents, hex_size);
	if(ihex) {
		printf("> Parsing Complete!\n> Dumping Intel hex file ...\n");
		dump_ihex(ihex);

		/* Create binary file: */
		if(flag_output_bin) {
			char * dot_pos = strrchr(hexfile,'.');
			char * output_bin_filename = 0;
			int dot_idx = 0;
			if(dot_pos) 
				dot_idx = (int)(dot_pos - hexfile);
			if(dot_idx) {
				output_bin_filename = malloc(sizeof(char) * (dot_idx + 5));
				strncpy(output_bin_filename, hexfile, dot_idx);
				memcpy(output_bin_filename+dot_idx, ".bin", 5);
			} else { 
				output_bin_filename = hexfile;
			}

			FILE * bin_fp = fopen(output_bin_filename, "wb");
			for(int i = 0; i < ihex->record_count; i++)
				fwrite(ihex->record_list[i].data, ihex->record_list[i].byte_count, 1, bin_fp);
			if(dot_idx && output_bin_filename)
				free(output_bin_filename);
			fclose(bin_fp);
		}
		printf("\n\n> Dump Complete!\n> Cleaning up ...");
		free_ihex(ihex);
	}

	printf("\n\nExiting...");
	return 0;
}
