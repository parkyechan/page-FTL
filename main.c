#include "ftl_box.h"

/*
 * Number of blocks: 151552
 * Number of pages per block: 128
 * Number of pages: 19398656 (=151552*128) (Phyiscal key space)
 * Page size: 4 Byte (integer)
 * Key range: 0 ~ 16777215 (Logical key space)
 */

/*
 * Argument to main: trace file (trace1.bin, trace2.bin, trace3.bin
 * trace: A large sequence of 1-byte type('r' or 'w'), 4-byte key, 4-byte value
 * You can input your arbitrary trace
 */

/*
 * Trace file format
 * File size: 1.3GB (total 3 files)
 * Total cycle: 11 times (sequential init + repeat 10 times)
 * No duplicate values (Not important)
 */

/*
 * trace1.bin: sequential key
 * cycle 1: write 0 ~ 16,777,215 keys sequentially
 * cycle 2: read/write(5:5) 0 ~ 16,777,215 keys sequentailly
 * repeat cycle 2 10 times
 */

/*
 * trace2.bin: random key
 * cycle 1: write 0 ~ 16,777,215 keys sequentially
 * cycle 2: read/write(5:5) 0 ~ 16,777,215 keys randomly
 * repeat cycle 2 10 times
 */

/*
 * trace3.bin: random & skewed key
 * cycle 1: write 0 ~ 16,777,215 keys sequentially
 * cycle 2: read/write(5:5) 0 ~ 16,777,215 keys randomly (20% of keys occupy 80%)
 * repeat cycle 2 10 times
 */

/*
 * Implement your own FTL (Page FTL or Anything else)
 * This main function starts FTLBox and reads traces
 */
int OOB[NOB * PPB] = {};
int bn = 0, pn = 0;

typedef struct invalid_table {
	int32_t bn;
	int32_t num;
} _invalid_table;
_invalid_table it[NOB];

typedef struct table {
	int32_t pbn;
	int32_t ppn;
} _table;
_table lo_table[NUMKEY];

void clean_block(int bn){
	int tmp_pn = 0;
	int tmp_OOB[PPB] = {};
	for(int i=0; i<PPB; i++){
		if(OOB[bn*PPB + i] >= 0){
			tmp_OOB[tmp_pn] = OOB[bn*PPB + i];
			flash_page_write(NOB-1, tmp_pn, flash_blocks[bn].pages[i]);
			tmp_pn++;
		}
	}
	flash_block_erase(bn);
	for(int i=0; i<tmp_pn; i++){
		flash_page_write(bn, i, flash_blocks[NOB-1].pages[i]);
		OOB[bn*PPB + i] = tmp_OOB[i];
	}
	// printf("will erase block : %d", NOB);
	flash_block_erase(NOB-1);
	for(int i=0; i<PPB; i++){
		OOB[(NOB-1)*PPB+i] = -1;
		// if(NOB-1+i == 151551) printf(">>>is there exist error ! <<<\n");
	}
	for(int i=tmp_pn; i<PPB; i++){
		OOB[bn*PPB + i] = -1;
	}
	// for(int i=0; i<PPB; i++){
	// 	printf("Block: %d | index : %d | Page : %d | OOB: %d\n", bn, i, flash_blocks[bn].pages[i], OOB[bn*PPB+i]);
	// }
	pn = tmp_pn;
}

int ret_max_idx(){
	int max = 0, max_idx = 0;
	for(int i=0; i<NOB-1; i++){
		if(it[i].num > max){
			max = it[i].num;
			max_idx = it[i].bn;
			// printf("max it[i].num %d\n", it[i].num);
		}
	}
	bn = max_idx;
	// printf("will delete block num : %d %d\n", bn, bn * PPB);
	return max_idx;
}

void find_invalid(){
	for(int i=0; i<NOB-1; i++){
		it[i].bn = 0;
		it[i].num = 0;
	}
	for(int i=0; i<(NOB-1) * PPB; i++){
		if(OOB[i] == -2){
			it[i / PPB].num += 1;
			it[i / PPB].bn = i / PPB;
		}
	}
	int max_idx = ret_max_idx();
	// printf("max idx : %d\n", max_idx);
	clean_block(max_idx);
}

void make_num(int trace_key, int i){
	bn = i/128;
	pn = i%128;
	if(bn == 1183 && pn == 127){
		printf("index pointer : %d\n", i);
		printf("OOB : %d \n", OOB[i]);
		printf("flash_blocks[b_idx].pages[p_idx] : %d\n", flash_blocks[1183].pages[127]);
	}
	lo_table[trace_key].pbn = bn;
	lo_table[trace_key].ppn = pn;
}

void input_data(int trace_key, int trace_value, int i){
	make_num(trace_key, i);
	flash_page_write(bn, pn, trace_value);
	OOB[bn*PPB+pn] = trace_key;
	if(bn == 1183 && pn == 127){
		printf("trace_key : %d\n", trace_key);
		printf("OOB : %d \n", bn*PPB+pn);
		printf("flash_blocks[b_idx].pages[p_idx] : %d\n", flash_blocks[1183].pages[127]);
		printf("-----\n");
	}
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
			for(int i=0; i<(NOB-1)*PPB; i++){
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
		int search_flag = 0;
		for(int i=bn*PPB+pn; i<(NOB-1)*PPB; i++){
			if(OOB[i] == -1){
				search_flag = 1;
				input_data(trace_key, trace_value, i);
				break;
			}
		}
		if(search_flag == 0){
			for(int i=0; i<(NOB-1)*PPB; i++){
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


		// for(int i=0; i<NOB-2; i++){
		// 	if(flash_blocks[i].min_page != 127){
		// 		printf("%d\n", i);
		// 		printf("bn : %d\n", bn);
		// 		bn = i;
		// 		for(int j=0; j<PPB; j++){
		// 			if(OOB[bn*PPB+j] == -1){
		// 				pn = j;
		// 				// printf("hi");
		// 				break;
		// 			}
		// 		}
		// 		break;
		// 	}
		// 	if(i == NOB -3){
		// 		find_invalid();
		// 	}
		// }
		// flash_page_write(bn, pn, trace_value);
		// OOB[bn*pn] = trace_key;









		// pn++;

		// if(pn > 127){
		// 	// bn += 1;
		// 	for(int i=0; i<NOB-1; i++){
		// 		if(flash_blocks[i].min_page != 127){
		// 			bn = i;
		// 			for(int i=0; i<PPB; i++){
		// 				if(OOB[bn*PPB+i] == -1){
		// 					pn = i;
		// 					break;
		// 				}else{
		// 					pn = 0;
		// 				}
		// 			}
		// 			break;
		// 		}
		// 		else{
		// 			pn = 0;
		// 		}
		// 	}
		// }
		// if(bn > NOB - 2){
		// 	// find_invalid();
		// 	// bn = 0;
		//
		// 	int cnt = 0;
		// 	printf("cnt2-1 : %d\n", cnt);
		// 	for(int i=0; i<NOB-1; i++){
		// 		if(flash_blocks[i].min_page == 127) cnt++;
		// 	}
		// 	if(cnt == NOB-1) find_invalid();
		// 	printf("cnt2-2 : %d\n", cnt);
		// }
		// OOB[lo_table[trace_key].pbn * lo_table[trace_key].ppn] = -2;
		// OOB[bn*pn] = trace_key;
		// flash_page_write(bn, pn, trace_value);
		//
		// lo_table[trace_key].pbn = bn;
		// lo_table[trace_key].ppn = pn;
		// pn++;
	}
}

int main(int argc, char *argv[])
{
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

	puts("ftl_box end");
	fclose(fp);
	box_destroy();

	return 0;
}
