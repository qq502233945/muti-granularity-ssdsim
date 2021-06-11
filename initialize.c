#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include "initialize.h"
#include "pagemap.h"

#define FALSE		0
#define TRUE		1

#define ACTIVE_FIXED 0
#define ACTIVE_ADJUST 1


/************************************************************************
* Compare function for AVL Tree
************************************************************************/
extern int keyCompareFunc(TREE_NODE* p, TREE_NODE* p1)
{
	struct buffer_group* T1 = NULL, * T2 = NULL;

	T1 = (struct buffer_group*)p;
	T2 = (struct buffer_group*)p1;


	if (T1->group < T2->group) return 1;
	if (T1->group > T2->group) return -1;

	return 0;
}


extern int freeFunc(TREE_NODE* pNode)
{

	if (pNode != NULL)
	{
		free((void*)pNode);
	}


	pNode = NULL;
	return 1;
}


/**********   initiation   ******************
*modify by zhouwen
*November 08,2011
*initialize the ssd struct to simulate the ssd hardware
*1.this function allocate memory for ssd structure
*2.set the infomation according to the parameter file
*******************************************/
struct ssd_info* initiation(struct ssd_info* ssd)
{
    unsigned int x=0,y=0,i=0,j=0,k=0,l=0,m=0,n=0;
	unsigned int temp;
	errno_t err;
	char buffer[300];
	struct parameter_value* parameters;
	FILE* fp = NULL;
	float s1, s2, s3[4];
	//	printf("input parameter file name:");
	//	gets(ssd->parameterfilename);


	//strcpy_s(ssd->statisticfilename2 ,16,"statistic2.dat");

	//导入ssd的配置文件
	parameters = load_parameters(ssd->parameterfilename);
	ssd->parameter = parameters;
	ssd->min_lsn = 0x7fffffff;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		ssd->page[i] = parameters->granularity_channel[i]*ssd->parameter->chip_channel[0] * ssd->parameter->die_chip * ssd->parameter->plane_die * ssd->parameter->block_plane * (ssd->parameter->block_capacity / ssd->parameter->granularity_size[0]);
	}
	//初始化 dram
	ssd->dram = (struct dram_info*)malloc(sizeof(struct dram_info));
	alloc_assert(ssd->dram, "ssd->dram");
	memset(ssd->dram, 0, sizeof(struct dram_info));
	initialize_dram(ssd);

	//初始化粒度域
	ssd->granularity_head = (struct granularity_info*)malloc(ssd->parameter->granularity_num * sizeof(struct granularity_info));
	alloc_assert(ssd->granularity_head, "ssd->granularity_head");
	memset(ssd->granularity_head, 0, ssd->parameter->granularity_num * sizeof(struct granularity_info));
	initialize_granularity(ssd);
	//初始化lpn区间
	temp = 0;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		temp += (ssd->parameter->block_capacity / (ssd->parameter->granularity_size[0])) * ssd->parameter->block_plane * ssd->parameter->plane_die * ssd->parameter->die_chip * ssd->parameter->chip_channel[0] * ssd->parameter->granularity_channel[i];
		ssd->lpn_granu[i] = temp;
	}

	//初始化各闪存芯片擦除系数，越耐磨擦除系数越大
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
			ssd->earse_factor[i] = earse_time(ssd,ssd->parameter->granularity_size[0])/ earse_time(ssd, ssd->parameter->granularity_size[i]);
	}
	//初始化mapping系数
	float total=0.0;
	float channel_factor = 4;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		s1 = (25* ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]*2+Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]))/(ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]/512);
		s2 = (25 * ssd->parameter->granularity_size[i] * 2 + Prog_time(ssd, ssd->parameter->granularity_size[i]))/(ssd->parameter->granularity_size[i]/512);
		s3[i] = ((float)ssd->parameter->granularity_size[i] * (float)ssd->parameter->granularity_channel[i]*(s1/s2)/ ssd->earse_factor[i]);
		total += s3[i];
	}
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		ssd->map_factor[i] = s3[i] / total;
	}
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		s1 = (25 * ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1] *2  + Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) / (ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1] / 512);
		s2 = (25 * ssd->parameter->granularity_size[i] * 4 + Prog_time(ssd, ssd->parameter->granularity_size[i])) / (ssd->parameter->granularity_size[i] / 512);
		s3[i] = ((float)ssd->parameter->granularity_size[i] * (float)ssd->parameter->granularity_channel[i] * (s1 / s2) / ssd->earse_factor[i]);
		total += s3[i];
	}
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		ssd->map_factor1[i] = s3[i] / total;
	}
	printf("\n");
	if ((err = fopen_s(&ssd->outputfile, ssd->outputfilename, "w")) != 0)
	{
		printf("the output file can't open\n");
		return NULL;
	}

	printf("\n");
	if ((err = fopen_s(&ssd->statisticfile, ssd->statisticfilename, "w")) != 0)
	{
		printf("the statistic file can't open\n");
		return NULL;
	}

	printf("\n");
	// 	if((err=fopen_s(&ssd->statisticfile2,ssd->statisticfilename2,"w"))!=0)
	// 	{
	// 		printf("the second statistic file can't open\n");
	// 		return NULL;
	// 	}

	fprintf(ssd->outputfile, "parameter file: %s\n", ssd->parameterfilename);
	fprintf(ssd->outputfile, "trace file: %s\n", ssd->tracefilename);
	fprintf(ssd->statisticfile, "parameter file: %s\n", ssd->parameterfilename);
	fprintf(ssd->statisticfile, "trace file: %s\n", ssd->tracefilename);

	fflush(ssd->outputfile);
	fflush(ssd->statisticfile);

	if ((err = fopen_s(&fp, ssd->parameterfilename, "r")) != 0)
	{
		printf("\nthe parameter file can't open!\n");
		return NULL;
	}

	//fp=fopen(ssd->parameterfilename,"r");

	fprintf(ssd->outputfile, "-----------------------parameter file----------------------\n");
	fprintf(ssd->statisticfile, "-----------------------parameter file----------------------\n");
	while (fgets(buffer, 300, fp))
	{
		fprintf(ssd->outputfile, "%s", buffer);
		fflush(ssd->outputfile);
		fprintf(ssd->statisticfile, "%s", buffer);
		fflush(ssd->statisticfile);
	}

	fprintf(ssd->outputfile, "\n");
	fprintf(ssd->outputfile, "-----------------------simulation output----------------------\n");
	fflush(ssd->outputfile);

	fprintf(ssd->statisticfile, "\n");
	fprintf(ssd->statisticfile, "-----------------------simulation output----------------------\n");
	fflush(ssd->statisticfile);

	fclose(fp);
	printf("initiation is completed!\n");

	return ssd;
}


struct dram_info* initialize_dram(struct ssd_info* ssd)
{
	unsigned int page_num=0;

	struct dram_info* dram = ssd->dram;
	dram->dram_capacity = ssd->parameter->dram_capacity;
	dram->buffer = (tAVLTree*)avlTreeCreate((void*)keyCompareFunc, (void*)freeFunc);
	dram->buffer->max_buffer_sector = ssd->parameter->dram_capacity / SECTOR; //512

	dram->map = (struct map_info*)malloc(sizeof(struct map_info));
	alloc_assert(dram->map, "dram->map");
	memset(dram->map, 0, sizeof(struct map_info));

	//计算总页数
	for (int i = 0; i < 8; i++)
	{
		page_num += ssd->page[i];
	}

	dram->map->map_entry = (struct entry*)malloc(sizeof(struct entry) * page_num); //每个物理页和逻辑页都有对应关系
	alloc_assert(dram->map->map_entry, "dram->map->map_entry");
	memset(dram->map->map_entry, 0, sizeof(struct entry) * page_num);

	return dram;
}

struct page_info* initialize_page(struct page_info* p_page)
{
	p_page->valid_state = 0;
	p_page->free_state = PG_SUB;
	p_page->lpn = -1;
	p_page->written_count = 0;
	return p_page;
}

struct blk_info* initialize_block(struct blk_info* p_block, struct parameter_value* parameter, unsigned int granularity_size)
{
	unsigned int i;
	struct page_info* p_page;

	p_block->free_page_num = parameter->block_capacity/ granularity_size;	// all pages are free
	p_block->last_write_page = -1;	// no page has been programmed
	p_block->size = parameter->block_capacity;

	p_block->page_head = (struct page_info*)malloc((parameter->block_capacity / granularity_size) * sizeof(struct page_info));
	alloc_assert(p_block->page_head, "p_block->page_head");
	memset(p_block->page_head, 0, (parameter->block_capacity / granularity_size) * sizeof(struct page_info));

	for (i = 0; i < parameter->block_capacity/granularity_size; i++)
	{
		p_page = &(p_block->page_head[i]);
		initialize_page(p_page);
	}
	return p_block;

}

struct plane_info* initialize_plane(struct plane_info* p_plane, struct parameter_value* parameter, unsigned int granularity_size)
{
	unsigned int i;
	struct blk_info* p_block;
	p_plane->add_reg_ppn = -1;  //plane 里面的额外寄存器additional register -1 表示无数据
	p_plane->free_page = parameter->block_plane * parameter->block_capacity/granularity_size;
	p_plane->blk_head = (struct blk_info*)malloc(parameter->block_plane * sizeof(struct blk_info));
	p_plane->active_block = 0;
	alloc_assert(p_plane->blk_head, "p_plane->blk_head");
	memset(p_plane->blk_head, 0, parameter->block_plane * sizeof(struct blk_info));

	for (i = 0; i < parameter->block_plane; i++)
	{
		p_block = &(p_plane->blk_head[i]);
		initialize_block(p_block, parameter, granularity_size);
	}
	return p_plane;
}

struct die_info* initialize_die(struct die_info* p_die, struct parameter_value* parameter, unsigned int granularity_size)
{
	unsigned int i;
	struct plane_info* p_plane;

	p_die->token = 0;

	p_die->plane_head = (struct plane_info*)malloc(parameter->plane_die * sizeof(struct plane_info));
	alloc_assert(p_die->plane_head, "p_die->plane_head");
	memset(p_die->plane_head, 0, parameter->plane_die * sizeof(struct plane_info));


	for (i = 0; i < parameter->plane_die; i++)
	{
		p_plane = &(p_die->plane_head[i]);
		initialize_plane(p_plane, parameter, granularity_size);
	}

	return p_die;
}

struct chip_info* initialize_chip(struct chip_info* p_chip, struct parameter_value* parameter, long long current_time, unsigned int granularity_size)
{
	unsigned int i=0;
	struct die_info* p_die;

	p_chip->current_state = CHIP_IDLE;
	p_chip->next_state = CHIP_IDLE;
	p_chip->current_time = current_time;
	p_chip->next_state_predict_time = 0;
	p_chip->die_num = parameter->die_chip;
	p_chip->plane_num_die = parameter->plane_die;
	p_chip->block_num_plane = parameter->block_plane;
	p_chip->page_num_block = parameter->page_block;
	p_chip->subpage_num_page = parameter->subpage_page;
	p_chip->ers_limit = parameter->ers_limit;
	p_chip->token = 0;
	p_chip->ac_timing = parameter->time_characteristics;
	p_chip->read_count = 0;
	p_chip->program_count = 0;
	p_chip->erase_count = 0;

	p_chip->die_head = (struct die_info*)malloc(parameter->die_chip * sizeof(struct die_info));
	alloc_assert(p_chip->die_head, "p_chip->die_head");
	memset(p_chip->die_head, 0, parameter->die_chip * sizeof(struct die_info));

	for (i = 0; i < parameter->die_chip; i++)
	{
		p_die = &(p_chip->die_head[i]);
		initialize_die(p_die, parameter, granularity_size);
	}

	return p_chip;
}



struct ssd_info* initialize_channels(struct ssd_info* ssd, struct granularity_info* granularity, unsigned int granularity_num)
{
	unsigned int i=0, j=0;
	struct channel_info* p_channel;
	struct chip_info* p_chip;

	// set the parameter of each channel
	granularity->channel_head = (struct channel_info*)malloc(ssd->parameter->granularity_channel[granularity_num] * sizeof(struct channel_info));
	alloc_assert(granularity->channel_head, "granularity->channel_head");
	memset(granularity->channel_head, 0, ssd->parameter->granularity_channel[granularity_num] * sizeof(struct channel_info));
		for (i = 0; i < ssd->parameter->granularity_channel[granularity_num]; i++)
		{
			p_channel = &(granularity->channel_head[i]);
			p_channel->chip = ssd->parameter->chip_channel[0];
			p_channel->current_state = CHANNEL_IDLE;
			p_channel->next_state = CHANNEL_IDLE;
			p_channel->granularity_size = ssd->parameter->granularity_size[granularity_num];
			
			p_channel->chip_head = (struct chip_info*)malloc(ssd->parameter->chip_channel[0] * sizeof(struct chip_info));
			alloc_assert(p_channel->chip_head, "p_channel->chip_head");
			memset(p_channel->chip_head, 0, ssd->parameter->chip_channel[0] * sizeof(struct chip_info));

			for (j = 0; j < ssd->parameter->chip_channel[0]; j++)
			{
				p_chip = &(p_channel->chip_head[j]);
				initialize_chip(p_chip, ssd->parameter, ssd->current_time, p_channel->granularity_size);
			}
		}

	return ssd;
}


struct ssd_info* initialize_granularity(struct ssd_info* ssd)
{
	unsigned int i;
	struct granularity_info* p_granularity;

	ssd->granularity_head = (struct granularity_info*)malloc(ssd->parameter->granularity_num * sizeof(struct granularity_info));
	alloc_assert(ssd->granularity_head, "ssd->granularity_head");
	memset(ssd->granularity_head, 0, ssd->parameter->granularity_num * sizeof(struct granularity_info));

	for ( i = 0; i < ssd->parameter->granularity_num; i++)
	{
		p_granularity = &(ssd->granularity_head[i]);
		p_granularity->granularity_size = ssd->parameter->granularity_size[i];
		initialize_channels(ssd, p_granularity,i);
	}
	
	return ssd;
}

/*************************************************
*将page.parameters里面的参数导入到ssd->parameter里
*modify by zhouwen
*November 8,2011
**************************************************/
struct parameter_value* load_parameters(char parameter_file[60])
{
	FILE* fp;
	FILE* fp1;
	FILE* fp2;
	errno_t ferr;
	struct parameter_value* p;
	char buf[BUFSIZE];
	int i, j, k;
	int pre_eql, next_eql;
	int res_eql;
	char* ptr;
	int sum = 0;	//累加得到channel_number

	p = (struct parameter_value*)malloc(sizeof(struct parameter_value));
	alloc_assert(p, "parameter_value");
	memset(p, 0, sizeof(struct parameter_value));
	p->queue_length = 5;
	memset(buf, 0, BUFSIZE);

	if ((ferr = fopen_s(&fp, parameter_file, "r")) != 0)
	{
		printf("the file parameter_file error!\n");
		return p;
	}
	if ((ferr = fopen_s(&fp1, "parameters_name.txt", "w")) != 0)
	{
		printf("the file parameter_name error!\n");
		return p;
	}
	if ((ferr = fopen_s(&fp2, "parameters_value.txt", "w")) != 0)
	{
		printf("the file parameter_value error!\n");
		return p;
	}



	while (fgets(buf, 200, fp)) {
		if (buf[0] == '#' || buf[0] == ' ') continue;
		ptr = strchr(buf, '=');
		if (!ptr) continue;

		pre_eql = ptr - buf;
		next_eql = pre_eql + 1;

		while (buf[pre_eql - 1] == ' ') pre_eql--;
		buf[pre_eql] = 0;
		if (res_eql = strcmp(buf, "granularity num") == 0) {
			sscanf(buf + next_eql, "%d", &p->granularity_num);
		}
	

		else if ((res_eql = strcmp(buf, "chip number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->chip_num);
		}
		else if ((res_eql = strcmp(buf, "dram capacity")) == 0) {
			sscanf(buf + next_eql, "%d", &p->dram_capacity);
		}
		else if ((res_eql = strcmp(buf, "cpu sdram")) == 0) {
			sscanf(buf + next_eql, "%d", &p->cpu_sdram);
		}
		else if ((res_eql = strcmp(buf, "channel number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->channel_number);
		}
		else if ((res_eql = strcmp(buf, "die number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->die_chip);
		}
		else if ((res_eql = strcmp(buf, "plane number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->plane_die);
		}
		else if ((res_eql = strcmp(buf, "block number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->block_plane);
		}
		else if ((res_eql = strcmp(buf, "page number")) == 0) {
			sscanf(buf + next_eql, "%d", &p->page_block);
		}
		else if ((res_eql = strcmp(buf, "subpage page")) == 0) {
			sscanf(buf + next_eql, "%d", &p->subpage_page);
		}
		else if ((res_eql = strcmp(buf, "page capacity")) == 0) {
			sscanf(buf + next_eql, "%d", &p->page_capacity);
		}
		else if ((res_eql = strcmp(buf, "block capacity")) == 0) {
			sscanf(buf + next_eql, "%d", &p->block_capacity);
		}
		else if ((res_eql = strcmp(buf, "subpage capacity")) == 0) {
			sscanf(buf + next_eql, "%d", &p->subpage_capacity);
		}
		else if ((res_eql = strcmp(buf, "t_PROG")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tPROG);
		}
		else if ((res_eql = strcmp(buf, "t_DBSY")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tDBSY);
		}
		else if ((res_eql = strcmp(buf, "t_BERS")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tBERS);
		}
		else if ((res_eql = strcmp(buf, "t_CLS")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCLS);
		}
		else if ((res_eql = strcmp(buf, "t_CLH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCLH);
		}
		else if ((res_eql = strcmp(buf, "t_CS")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCS);
		}
		else if ((res_eql = strcmp(buf, "t_CH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCH);
		}
		else if ((res_eql = strcmp(buf, "t_WP")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tWP);
		}
		else if ((res_eql = strcmp(buf, "t_ALS")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tALS);
		}
		else if ((res_eql = strcmp(buf, "t_ALH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tALH);
		}
		else if ((res_eql = strcmp(buf, "t_DS")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tDS);
		}
		else if ((res_eql = strcmp(buf, "t_DH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tDH);
		}
		else if ((res_eql = strcmp(buf, "t_WC")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tWC);
		}
		else if ((res_eql = strcmp(buf, "t_WH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tWH);
		}
		else if ((res_eql = strcmp(buf, "t_ADL")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tADL);
		}
		else if ((res_eql = strcmp(buf, "t_R")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tR);
		}
		else if ((res_eql = strcmp(buf, "t_AR")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tAR);
		}
		else if ((res_eql = strcmp(buf, "t_CLR")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCLR);
		}
		else if ((res_eql = strcmp(buf, "t_RR")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRR);
		}
		else if ((res_eql = strcmp(buf, "t_RP")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRP);
		}
		else if ((res_eql = strcmp(buf, "t_WB")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tWB);
		}
		else if ((res_eql = strcmp(buf, "t_RC")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRC);
		}
		else if ((res_eql = strcmp(buf, "t_REA")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tREA);
		}
		else if ((res_eql = strcmp(buf, "t_CEA")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCEA);
		}
		else if ((res_eql = strcmp(buf, "t_RHZ")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRHZ);
		}
		else if ((res_eql = strcmp(buf, "t_CHZ")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCHZ);
		}
		else if ((res_eql = strcmp(buf, "t_RHOH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRHOH);
		}
		else if ((res_eql = strcmp(buf, "t_RLOH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRLOH);
		}
		else if ((res_eql = strcmp(buf, "t_COH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tCOH);
		}
		else if ((res_eql = strcmp(buf, "t_REH")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tREH);
		}
		else if ((res_eql = strcmp(buf, "t_IR")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tIR);
		}
		else if ((res_eql = strcmp(buf, "t_RHW")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRHW);
		}
		else if ((res_eql = strcmp(buf, "t_WHR")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tWHR);
		}
		else if ((res_eql = strcmp(buf, "t_RST")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_characteristics.tRST);
		}
		else if ((res_eql = strcmp(buf, "erase limit")) == 0) {
			sscanf(buf + next_eql, "%d", &p->ers_limit);
		}
		else if ((res_eql = strcmp(buf, "flash operating current")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->operating_current);
		}
		else if ((res_eql = strcmp(buf, "flash supply voltage")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->supply_voltage);
		}
		else if ((res_eql = strcmp(buf, "dram active current")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->dram_active_current);
		}
		else if ((res_eql = strcmp(buf, "dram standby current")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->dram_standby_current);
		}
		else if ((res_eql = strcmp(buf, "dram refresh current")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->dram_refresh_current);
		}
		else if ((res_eql = strcmp(buf, "dram voltage")) == 0) {
			sscanf(buf + next_eql, "%lf", &p->dram_voltage);
		}
		else if ((res_eql = strcmp(buf, "address mapping")) == 0) {
			sscanf(buf + next_eql, "%d", &p->address_mapping);
		}
		else if ((res_eql = strcmp(buf, "wear leveling")) == 0) {
			sscanf(buf + next_eql, "%d", &p->wear_leveling);
		}
		else if ((res_eql = strcmp(buf, "gc")) == 0) {
			sscanf(buf + next_eql, "%d", &p->gc);
		}
		else if ((res_eql = strcmp(buf, "clean in background")) == 0) {
			sscanf(buf + next_eql, "%d", &p->clean_in_background);
		}
		else if ((res_eql = strcmp(buf, "overprovide")) == 0) {
			sscanf(buf + next_eql, "%f", &p->overprovide);
		}
		else if ((res_eql = strcmp(buf, "gc threshold")) == 0) {
			sscanf(buf + next_eql, "%f", &p->gc_threshold);
		}
		else if ((res_eql = strcmp(buf, "buffer management")) == 0) {
			sscanf(buf + next_eql, "%d", &p->buffer_management);
		}
		else if ((res_eql = strcmp(buf, "scheduling algorithm")) == 0) {
			sscanf(buf + next_eql, "%d", &p->scheduling_algorithm);
		}
		else if ((res_eql = strcmp(buf, "quick table radio")) == 0) {
			sscanf(buf + next_eql, "%f", &p->quick_radio);
		}
		else if ((res_eql = strcmp(buf, "related mapping")) == 0) {
			sscanf(buf + next_eql, "%d", &p->related_mapping);
		}
		else if ((res_eql = strcmp(buf, "striping")) == 0) {
			sscanf(buf + next_eql, "%d", &p->striping);
		}
		else if ((res_eql = strcmp(buf, "interleaving")) == 0) {
			sscanf(buf + next_eql, "%d", &p->interleaving);
		}
		else if ((res_eql = strcmp(buf, "pipelining")) == 0) {
			sscanf(buf + next_eql, "%d", &p->pipelining);
		}
		else if ((res_eql = strcmp(buf, "time_step")) == 0) {
			sscanf(buf + next_eql, "%d", &p->time_step);
		}
		else if ((res_eql = strcmp(buf, "small large write")) == 0) {
			sscanf(buf + next_eql, "%d", &p->small_large_write);
		}
		else if ((res_eql = strcmp(buf, "active write threshold")) == 0) {
			sscanf(buf + next_eql, "%d", &p->threshold_fixed_adjust);
		}
		else if ((res_eql = strcmp(buf, "threshold value")) == 0) {
			sscanf(buf + next_eql, "%f", &p->threshold_value);
		}
		else if ((res_eql = strcmp(buf, "active write")) == 0) {
			sscanf(buf + next_eql, "%d", &p->active_write);
		}
		else if ((res_eql = strcmp(buf, "gc hard threshold")) == 0) {
			sscanf(buf + next_eql, "%f", &p->gc_hard_threshold);
		}
		else if ((res_eql = strcmp(buf, "allocation")) == 0) {
			sscanf(buf + next_eql, "%d", &p->allocation_scheme);
		}
		else if ((res_eql = strcmp(buf, "static_allocation")) == 0) {
			sscanf(buf + next_eql, "%d", &p->static_allocation);
		}
		else if ((res_eql = strcmp(buf, "dynamic_allocation")) == 0) {
			sscanf(buf + next_eql, "%d", &p->dynamic_allocation);
		}
		else if ((res_eql = strcmp(buf, "advanced command")) == 0) {
			sscanf(buf + next_eql, "%d", &p->advanced_commands);
		}
		else if ((res_eql = strcmp(buf, "advanced command priority")) == 0) {
			sscanf(buf + next_eql, "%d", &p->ad_priority);
		}
		else if ((res_eql = strcmp(buf, "advanced command priority2")) == 0) {
			sscanf(buf + next_eql, "%d", &p->ad_priority2);
		}
		else if ((res_eql = strcmp(buf, "greed CB command")) == 0) {
			sscanf(buf + next_eql, "%d", &p->greed_CB_ad);
		}
		else if ((res_eql = strcmp(buf, "greed MPW command")) == 0) {
			sscanf(buf + next_eql, "%d", &p->greed_MPW_ad);
		}
		else if ((res_eql = strcmp(buf, "aged")) == 0) {
			sscanf(buf + next_eql, "%d", &p->aged);
		}
		else if ((res_eql = strcmp(buf, "aged ratio")) == 0) {
			sscanf(buf + next_eql, "%f", &p->aged_ratio);
		}
		else if ((res_eql = strcmp(buf, "queue_length")) == 0) {
			sscanf(buf + next_eql, "%d", &p->queue_length);
		}
		else if ((res_eql = strncmp(buf, "chip number", 11)) == 0)
		{
			sscanf(buf + 12, "%d", &i);
			sscanf(buf + next_eql, "%d", &p->chip_channel[i]);
		}
		else if ((res_eql = strncmp(buf, "granularity channel",19)) == 0)
		{
			sscanf(buf + 20, "%d", &j);
			sscanf(buf + next_eql, "%d", &p->granularity_channel[j]);
			sum += p->granularity_channel[j];
		}
		else if ((res_eql = strncmp(buf, "granularity size",16)) == 0)
		{
			sscanf(buf + 17, "%d", &k);
			sscanf(buf + next_eql, "%d", &p->granularity_size[k]);
		}
		else {
			printf("don't match\t %s\n", buf);
		}

		memset(buf, 0, BUFSIZE);

	}



	//令channel_number 为所有粒度通道之和, 检验参数是否一致
	if (p->channel_number != sum)
	{
		printf("channel number don't match");
		exit(0);
	}




	fclose(fp);
	fclose(fp1);
	fclose(fp2);

	return p;
}
int Bear_time(struct ssd_info* ssd, int granularity)
{
	int time;
	switch (granularity)
	{
	case 2048:
		time = 700000;
		break;
	case 4096:
		time = 3000000;
		break;
	case 8192:
		time = 3800000;
		break;
	case 16384:
		time = 15000000;
		break;
	
	}
	return time;
}
int Prog_time(struct ssd_info* ssd, int granularity)
{
	int time;
	switch (granularity)
	{
	case 2048:
		time = 200000;
		break;
	case 4096:
		time = 900000;
		break;
	case 8192:
		time = 1300000;
		break;
	case 16384:
		time = 3500000;
		break;

	}
	return time;
}
int Read_time(struct ssd_info* ssd, int granularity)
{
	int time;
	switch (granularity)
	{
	case 2048:
		time = 25000;
		break;
	case 4096:
		time = 50000;
		break;
	case 8192:
		time = 75000;
		break;
	case 16384:
		time = 88000;
		break;

	}
	return time;
}
float earse_time(struct ssd_info* ssd, int granularity)
{
	int time;
	switch (granularity)
	{
	case 2048:
		time = 14000;
		break;
	case 4096:
		time = 6000;
		break;
	case 8192:
		time = 4000;
		break;
	case 16384:
		time = 2000;
		break; 

	}
	return time;
}
	float earse_time_1(struct ssd_info* ssd, int granularity)
	{
		int time;
		switch (granularity)
		{
		case 2048:
			time = 100000;
			break;
		case 4096:
			time = 5000;
			break;
		case 8192:
			time = 3000;
			break;
		case 16384:
			time = 2000;
			break;

		}
		return time;
	}