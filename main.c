#include "ftl_box.h"

int OOB[NOB * PPB] = {};
int bn = 0, pn = 0;
int invalid_block[NOB]	= {};
int invalid_max, invalid_max_block_num;
typedef struct invalid_table {
	int32_t bn;
	int32_t num;
} _invalid_table;
// _invalid_table it[NOB];

typedef struct table {
	int32_t pbn;
	int32_t ppn;
} _table;
_table lo_table[NUMKEY];


void clean_block(int willdead){
	int tmp_pn = 0;
	int tmp_OOB[PPB] = {};
	for(int i=0; i<PPB; i++){
		if(OOB[willdead*PPB + i] >= 0){
			tmp_OOB[tmp_pn] = OOB[willdead*PPB + i];
			flash_page_write(NOB-1, tmp_pn, flash_blocks[willdead].pages[i]);
			tmp_pn++;
		}
	}
	// if(tmp_pn == 128){
	// 	printf("invalid table num : %d & bn : %d\n", it[willdead].num, it[willdead].bn);
	// 	// for(int i=0; i<PPB; i++){
	// 	// 	printf("OOB[%d] : %d\n", willdead*PPB+i, OOB[willdead*PPB+i]);
	// 	// }
	// }
	flash_block_erase(willdead);
	invalid_max = 0;
	for(int i=0; i<tmp_pn; i++){
		flash_page_write(willdead, i, flash_blocks[NOB-1].pages[i]);
		OOB[willdead*PPB + i] = tmp_OOB[i];
	}
	// printf("will erase block : %d", NOB);
	flash_block_erase(NOB-1);
	for(int i=0; i<PPB; i++){
		OOB[(NOB-1)*PPB+i] = -1;
		// if(NOB-1+i == 151551) printf(">>>is there exist error ! <<<\n");
	}
	for(int i=tmp_pn; i<PPB; i++){
		OOB[willdead*PPB + i] = -1;
	}
	// for(int i=0; i<PPB; i++){
	// 	printf("Block: %d | index : %d | Page : %d | OOB: %d\n", bn, i, flash_blocks[bn].pages[i], OOB[bn*PPB+i]);
	// }
	if(tmp_pn >=128)printf("claen block function, bn : %d, pn : %d, tmp_pn :%d\n", bn, pn, tmp_pn);
	pn = tmp_pn;
}

// int ret_max_idx(){
// 	int max = 0, max_idx = 0;
// 	for(int i=0; i<NOB-1; i++){
// 		if(it[i].num > max){
// 			max = it[i].num;
// 			max_idx = it[i].bn;
// 			// printf("max it[i].num %d\n", it[i].num);
// 		}
// 	}
// 	bn = max_idx;
// 	// printf("will delete block num : %d %d\n", bn, bn * PPB);
// 	return max_idx;
// }
void find_invalid(){

	_invalid_table it[NOB] = {};
	// memcpy(&it, &temp_it, sizeof(struct invalid_table));
	// memset(it, 0, NOB);

	// for(int i=0; i<NOB-1; i++){
	// 	it[i].bn = 0;
	// 	it[i].num = 0;
	// }
	// for(int i=0; i<NOB; i++){
	// 	printf("invalide : %d %d\n", it[i].bn, it[i].num);
	// }


	// for(int i=0; i<(NOB-1) * PPB; i++){
	// 	if(OOB[i] == -2){
	// 		it[i / PPB].num += 1;
	// 		//it[i / PPB].bn = i / PPB;
	// 	}
	// }


	// int max_idx = ret_max_idx();
	// int max = 0, max_idx = 0;
	// for(int i=0; i<NOB-1; i++){
	// 	if(it[i].num > max){
	// 		max = it[i].num;
	// 		max_idx = i;
	// 		// if(max>64) break;
	// 		// printf("max it[i].num %d\n", it[i].num);
	// 	}
	// }
	bn = invalid_max_block_num;
	// printf("will delete block num : %d %d\n", bn, bn * PPB);
	// return max_idx;
	clean_block(bn);
}

void make_num(int trace_key, int i){
	bn = i/128;
	pn = i%128;
	lo_table[trace_key].pbn = bn;
	lo_table[trace_key].ppn = pn;
}

void input_data(int trace_key, int trace_value, int i){
	make_num(trace_key, i);
	if(pn >= 128) printf("bn : %d, pn : %d\n", bn, pn);
	flash_page_write(bn, pn, trace_value);
	OOB[bn*PPB+pn] = trace_key;
}

void page_write(int trace_key, int trace_value){
	if(lo_table[trace_key].pbn == -1 && lo_table[trace_key].ppn == -1){
		int search_flag = 0;
		for(int i=bn*PPB+pn; i<(NOB-1)*PPB; i++){
			if(OOB[i] == -1){
				search_flag = 1;
				input_data(trace_key, trace_value, i);
				break;
			}
		}
		if(search_flag == 0){
			for(int i=0; i<bn*PPB+pn; i++){
				if(OOB[i] == -1){
					search_flag = 1;
					input_data(trace_key, trace_value, i);
					break;
				}
			}
		}
		if(search_flag == 0){
			find_invalid();
			flash_page_write(bn, pn, trace_value);
			OOB[bn*PPB+pn] = trace_key;
		}

	}else{
		OOB[lo_table[trace_key].pbn*PPB+lo_table[trace_key].ppn] = -2;
		invalid_block[lo_table[trace_key].pbn]++;
		if(invalid_block[lo_table[trace_key].pbn] > invalid_max){
			invalid_max = invalid_block[lo_table[trace_key].pbn];
			invalid_max_block_num = lo_table[trace_key].pbn;
		}
		int search_flag = 0;
		for(int i=bn*PPB+pn; i<(NOB-1)*PPB; i++){
			if(OOB[i] == -1){
				search_flag = 1;
				input_data(trace_key, trace_value, i);
				break;
			}
		}
		if(search_flag == 0){
			for(int i=0; i<bn*PPB+pn; i++){
				if(OOB[i] == -1){
					search_flag = 1;
					input_data(trace_key, trace_value, i);
					break;
				}
			}
		}
		if(search_flag == 0){
			// printf("2 here\n");
			find_invalid();
			invalid_block[lo_table[trace_key].pbn] = 0;
			flash_page_write(bn, pn, trace_value);
			OOB[bn*PPB+pn] = trace_key;
		}
	}
}

int main(int argc, char *argv[])
{
	invalid_max = 0;
	invalid_max_block_num = 0;
	for(int i=0; i< NOB * PPB; i++){
		OOB[i] = -1;
	}
	for(int i=0; i< NUMKEY; i++){
		lo_table[i].pbn = -1;
		lo_table[i].ppn = -1;
	}
	FILE * fp;
	char *buf;
	int trace_key, trace_value;

	if (argc > 2 || argc <= 1) {
		puts("argument error");
		abort();
	}
	if (!fopen(argv[1], "rt")) {
		puts("trace open error");
		abort();
	}
	printf("Input Trace File : %s\n", (argv[1]));

	fp = fopen(argv[1], "rb");
	buf = (char*)malloc(sizeof(char) * 1);

	box_create();
	/* Read Trace file */
	while (1) {
		if (!fread((void*)buf, 1, 1, fp)) {
			if (feof(fp)) {
				puts("trace eof");
				break;
			}
			puts("fread error");
			abort();
		}

		if (*buf == 'w') {
			fread((void*)&trace_key, 4, 1, fp);
			fread((void*)&trace_value, 4, 1, fp);

			/* Trace check */
			// printf("type: %c, key: %d, value: %d\n", *buf, trace_key, trace_value);

			/* Implement your page_write function */
			page_write(trace_key, trace_value);
		}
		else if (*buf == 'r') {
			fread((void*)&trace_key, 4, 1, fp);

			/* Trace check */
			// printf("type: %c, key: %d, value: %d\n", *buf, trace_key, trace_value);

			/* Implement your page_read function */
			//page_read(trace_key, trace_value);
		}
		else {
			puts("trace error");
			abort();
		}
	}
	// printf("time : %d\n", tt);
	puts("ftl_box end");
	fclose(fp);
	box_destroy();

	return 0;
}
