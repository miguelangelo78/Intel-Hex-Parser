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

char * strdup(const char * s) {
	char * d = (char*)malloc(strlen(s) + 1);
	if (!d) return NULL;
	strcpy(d, s);
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
	return (uint16_t)strtol(four_digits, NULL, 16);
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

ihex_t * parse_ihex_file(char * hex, unsigned int hex_size, char * append, unsigned int append_size) {
	ihex_t * ret = 0;
	uint32_t record_count = 0;
	/* Count the number of records in the hex file: */
	for(int i = 0; i < hex_size; i++)
		if(hex[i] == '\n') 
			record_count++;
	
	/* Allocate more records that will fit the extra binary data being appended: */
	if(append)
		record_count += ((append_size-1) / 16) + 1;

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
				record_t * rec = &ret->record_list[type_off(hex, i) == REC_TYPE_EOF ? record_count - 1 : rec_count];
				
				/* Collect the data: */
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

				if(rec->type == REC_TYPE_EOF) { 
					/* Now we should append the binary file: */
					int first_null = 0;
					while(first_null < record_count && ret->record_list[first_null++].type_str);

					if(first_null-- < record_count) {
						/* Found the record slots for the binary file: */
						for(int s = 0, acc = 0, bin_rec = first_null; s < append_size; s++) {
							if(++acc == 16 || s == append_size - 1) {
								record_t * rec = &ret->record_list[bin_rec];

								rec->byte_count = acc;
								rec->address = ret->record_list[bin_rec - 1].address + ret->record_list[bin_rec - 1].byte_count + 13;
								rec->type = REC_TYPE_DATA; /* Data by default */
								rec->type_str = strdup("Data");

								rec->data = (uint8_t*)malloc(sizeof(uint8_t) * acc);
								memcpy(rec->data, append + (s-acc) + 1, acc);

								uint8_t data_sum = 0;
								for(int di = 0; di < rec->byte_count; di++)
									data_sum += rec->data[di];
								rec->checksum       = ~(rec->byte_count + ((rec->address & 0xFF00) >> 8) + (rec->address & 0xFF) + rec->type + data_sum) + 1;
								rec->checksum_calc  = rec->checksum;
								rec->checksum_valid = 1;

								bin_rec++;
								acc = 0;
							}
						}
					}
				}

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
		printf("\n\t\t- Size: %d (decimal)\n\t\t- Address: %04X\n\t\t- Type: %s (%d)\n\t\t- Checksum: %02X = %s", rec->byte_count, rec->address,
			rec->type_str, rec->type, rec->checksum, rec->checksum_valid ? "VALID" : "INVALID");
		if(!rec->checksum_valid)
			printf(" (calc: %02X)", rec->checksum_calc);
	}
}

void free_ihex(ihex_t * ihex) {
	if(!ihex) return;
	for(int i = 0; i < ihex->record_count; i++) {
		if(ihex->record_list[i].byte_count && ihex->record_list[i].data) {
			free(ihex->record_list[i].data);
			ihex->record_list[i].data = 0;
		}
		if(ihex->record_list[i].type_str) {
			free(ihex->record_list[i].type_str);
			ihex->record_list[i].type_str = 0;
		}
	}
	free(ihex->record_list);
	free(ihex);
	ihex = 0;
}

uint32_t ihex_total_size(ihex_t * ihex) {
	uint32_t hex_str_size = 0;
	for(int i = 0; i < ihex->record_count; i++)
		hex_str_size += 1+2+4+2+(ihex->record_list[i].byte_count*2)+2+2;
	return hex_str_size;
}

char * ihex_to_str(ihex_t * ihex) {
	uint32_t hex_str_size = ihex_total_size(ihex);

	char * hex_str = (char*)malloc(hex_str_size);
	memset(hex_str, 0, hex_str_size);

	for(int i = 0; i < ihex->record_count; i++) {
		char * single_rec = malloc(1+2+4+2+(ihex->record_list[i].byte_count*2)+2+2);
		char * data_str = malloc(ihex->record_list[i].byte_count*2+1);
		memset(data_str, 1, ihex->record_list[i].byte_count*2+1);
		for(int j = 0, k = 0; k < ihex->record_list[i].byte_count; j += 2, k++) {
			char two_digits[3];
			sprintf(two_digits, "%02X", ihex->record_list[i].data[k]);
			data_str[j] = two_digits[0];
			data_str[j+1] = two_digits[1];
		}
		data_str[ihex->record_list[i].byte_count*2] = '\0';
		sprintf(single_rec, ":%02X%04X%02X%s%02X\r\n", ihex->record_list[i].byte_count, ihex->record_list[i].address, ihex->record_list[i].type, data_str, ihex->record_list[i].checksum);
		strcat(hex_str, single_rec);

		free(data_str);
		free(single_rec);
	}
	return hex_str;	
}

ihex_t * bin_to_ihex(char * bin_str, uint32_t bin_strlen) {
	if(!bin_strlen) return 0;
	ihex_t * bin_ihex = 0;
	uint32_t record_count = ((bin_strlen-1) / 16) + 2;
	
	bin_ihex = (ihex_t*)malloc(sizeof(ihex_t));
	bin_ihex->record_count = record_count;
	bin_ihex->record_list = (record_t*)malloc(sizeof(record_t) * record_count);
	for(int s = 0, acc = 0, bin_rec = 0; s < bin_strlen; s++) {
		if(++acc == 16 || s == bin_strlen - 1) {
			record_t * rec = &bin_ihex->record_list[bin_rec];

			rec->byte_count = acc;
			rec->address = !bin_rec ? 0 : bin_ihex->record_list[bin_rec - 1].address + bin_ihex->record_list[bin_rec - 1].byte_count + 13;
			rec->type = REC_TYPE_DATA;
			rec->type_str = strdup("Data");

			rec->data = (uint8_t*)malloc(sizeof(uint8_t) * acc);
			memcpy(rec->data, bin_str + (s-acc) + 1, acc);

			uint8_t data_sum = 0;
			for(int di = 0; di < rec->byte_count; di++)
				data_sum += rec->data[di];
			rec->checksum      = ~(rec->byte_count + ((rec->address & 0xFF00) >> 8) + (rec->address & 0xFF) + rec->type + data_sum) + 1;
			rec->checksum_calc = rec->checksum;
			rec->checksum_valid = 1;

			bin_rec++;
			acc = 0;
		}
	}
	/* Create EOF record: */
	record_t * rec = &bin_ihex->record_list[record_count - 1];
	rec->byte_count = 0;
	rec->address = 0;
	rec->type = REC_TYPE_EOF;
	rec->type_str = strdup("End Of File");

	rec->data = 0;

	rec->checksum      =  0xFF;
	rec->checksum_calc = rec->checksum_calc;
	rec->checksum_valid = 1;

	free(bin_str);
	return bin_ihex;
}

void help() {
	printf("\n> Usage: ihex hexfile.hex [options]\n> Options:\n\
   1: -h Show Help\n\
   2: --ob Output binary data into file 'hexfile.bin'\n\
   3: --oh Output hexadecimal file into a copy file, whether the original hex file was altered or not\n\
   4: --ab <binfile> Append binary file into the hex file\n\
   5: --ah <hexfile> Append hex file into the current hex file\n\
   6: -c <binfile> Convert binary file into hex file. Use this in the place of the original hex file");
}

int main(char argc, char ** argv) {
	printf(">>>> Intel Hex file Parser <<<<\n");
	if(argc <= 1) {
		help();
		return 1;
	}

	char flag_output_bin = 0;
	char flag_output_hex = 0;
	char * hexfile = 0;
	char * append_bin_file = 0;
	char * append_hex_file = 0;
	char * conv_bin_file = 0;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--ob")) flag_output_bin = 1;
		else if(!strcmp(argv[i], "-h")) { help(); return 2; }
		else if(!strcmp(argv[i], "--ab") && i < argc) append_bin_file = argv[++i];
		else if(!strcmp(argv[i], "--ah")) append_hex_file = argv[++i];
		else if(!strcmp(argv[i], "--oh")) flag_output_hex = 1;
		else if(!strcmp(argv[i], "-c")) conv_bin_file = argv[++i];
		else hexfile = argv[i];
	}

	if(!hexfile && !conv_bin_file) {
		help();
		return 3;
	}

	FILE * fp = 0;
	if(hexfile){
		/* Open the hex file normally: */
		fp = fopen(hexfile, "rb");
		if(!fp) {
			printf("\n> ERROR: Could not open '%s'", hexfile);
			return 4;
		}
	} else {
		/* Open the binary file that will be converted into hex instead: */
		if(conv_bin_file) {
			fp = fopen((hexfile = conv_bin_file), "rb");
			if(!fp) {
				printf("\n> ERROR: Could not open '%s'", hexfile);
				return 5;
			}
		}
	}

	FILE * fp_append_bin = 0;
	if(append_bin_file) {
		fp_append_bin = fopen(append_bin_file, "rb");
		if(!fp_append_bin) {
			printf("\n> ERROR: Could not open '%s' (selected by the option --ab)", append_bin_file);
			return 6;
		}
	}

	FILE * fp_append_hex = 0;
	if(append_hex_file) {
		fp_append_hex = fopen(append_hex_file, "rb");
		if(!fp_append_hex) {
			printf("\n> ERROR: Could not open '%s' (selected by the option --ah)", append_hex_file);
			return 7;	
		}
	}

	printf("\n> Loading '%s' ", hexfile);
	if(fp_append_bin)
		printf("and '%s' ", append_bin_file);
	if(fp_append_hex)
		printf("and '%s' ", append_hex_file);
	printf("...\n");

	ihex_t * ihex_from_bin = 0;
	if(conv_bin_file)
		ihex_from_bin = bin_to_ihex(fileread(fp), filesize(fp));

	unsigned int hex_size = ihex_from_bin ? ihex_total_size(ihex_from_bin) : filesize(fp);
	char * hex_contents   = ihex_from_bin ? ihex_to_str(ihex_from_bin) : fileread(fp);
	unsigned int bin_size = fp_append_bin ? filesize(fp_append_bin) : 0;
	char * append_bin_contents = fp_append_bin ? fileread(fp_append_bin) : 0;
	unsigned int hex_app_size = fp_append_hex ? filesize(fp_append_hex) : 0;
	char * append_hex_contents = fp_append_hex ? fileread(fp_append_hex) : 0;
	if(ihex_from_bin)
		free_ihex(ihex_from_bin);
	fclose(fp);
	if(fp_append_bin) 
		fclose(fp_append_bin);
	if(fp_append_hex)
		fclose(fp_append_hex);
	
	printf("> Parsing '%s' ...\n", hexfile);
	ihex_t * ihex = parse_ihex_file(hex_contents, hex_size, append_bin_contents, bin_size);
	ihex_t * ihex_append = 0;
	if(ihex) {
		printf("> Parsing Complete!\n");
		if(append_hex_file && hex_app_size) {
			printf("> Appending hex file '%s' to '%s' ...", append_hex_file, hexfile);
			ihex_append = parse_ihex_file(append_hex_contents, hex_app_size, 0, 0);
			if(ihex_append && ihex_append->record_count) {
				uint32_t old_record_count = ihex->record_count - 1;

				ihex->record_list = (record_t*)realloc(ihex->record_list, (sizeof(record_t)) * (ihex->record_count + ihex_append->record_count - 1));
				ihex->record_count += ihex_append->record_count - 1;

				for(int i = old_record_count, j = 0; i < ihex->record_count; i++, j++) {
					record_t * rec_dst = &ihex->record_list[i];
					record_t * rec_src = &ihex_append->record_list[j];

					rec_dst->byte_count     = rec_src->byte_count;
					rec_dst->address        = rec_src->address;
					rec_dst->type           = rec_src->type;
					rec_dst->type_str       = rec_src->type_str;
					rec_dst->data           = rec_src->data;
					rec_dst->checksum       = rec_src->checksum;
					rec_dst->checksum_calc  = rec_src->checksum_calc;
					rec_dst->checksum_valid = rec_src->checksum_valid;
				}
			} else {
				printf(" ERROR: '%s' is not an hex file", append_hex_file);
			}
			printf("\n");
		}

		printf("> Dumping Intel hex file ...\n");
		dump_ihex(ihex);
		printf("\n\n> Dump Complete!");

		/* Create binary file: */
		if(flag_output_bin) {
			printf("\n> Creating binary output ...");
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
				output_bin_filename = malloc(sizeof(char) * (strlen(hexfile) + 5));
				sprintf(output_bin_filename, "%s.bin", hexfile);
			}

			FILE * bin_fp = fopen(output_bin_filename, "wb");
			for(int i = 0; i < ihex->record_count; i++)
				fwrite(ihex->record_list[i].data, ihex->record_list[i].byte_count, 1, bin_fp);
			if(dot_idx && output_bin_filename)
				free(output_bin_filename);
			fclose(bin_fp);
		}

		/* Create hex file: */
		if(flag_output_hex) {
			printf("\n> Creating hex output ...");
			char new_filename[6] = "a.hex";
			FILE * hex_fp = fopen(new_filename, "wb");
			char * hex_str = ihex_to_str(ihex);
			fwrite(hex_str, ihex_total_size(ihex), 1, hex_fp);
			fclose(hex_fp);
			free(hex_str);
		}

		printf("\n> Cleaning up ...");
		free(hex_contents);
		free_ihex(ihex);
		if(ihex_append) {
			free(ihex_append->record_list);
			free(ihex_append);
		}
	} else {
		printf("> ERROR: '%s' is not an hex file", hexfile);
	}

	printf("\n\nExiting...");
	return 0;
}
