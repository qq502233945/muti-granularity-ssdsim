/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName�� ssd.c
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
Zhiming Zhu     2012/07/19        2.1.1         Correct erase_planes()   812839842@qq.com  
*****************************************************************************************************************************/

#define _CRTDBG_MAP_ALLOC
//#define DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include "initialize.h"
#include "ssd.h"
#include "pagemap.h"
#include "flash.h"
#include <direct.h>
#include <math.h>
#include <corecrt_io.h>
#include <crtdbg.h>
/********************************************************************************************************************************
1��main������initiatio()����������ʼ��ssd,��2��make_aged()����ʹSSD��Ϊaged��aged��ssd�൱��ʹ�ù�һ��ʱ���ssd��������ʧЧҳ��
non_aged��ssd���µ�ssd����ʧЧҳ��ʧЧҳ�ı��������ڳ�ʼ�����������ã�3��pre_process_page()������ǰɨһ������󣬰Ѷ�����
��lpn<--->ppnӳ���ϵ���Ƚ����ã�д�����lpn<--->ppnӳ���ϵ��д��ʱ���ٽ�����Ԥ����trace��ֹ�������Ƕ��������ݣ�4��simulate()��
���Ĵ�������trace�ļ��Ӷ�������������ɶ��������������ɣ�5��statistic_output()������ssd�ṹ�е���Ϣ���������ļ����������
ͳ�����ݺ�ƽ�����ݣ�����ļ���С��trace_output�ļ���ܴ����ϸ��6��free_all_node()�����ͷ�����main����������Ľڵ�
*********************************************************************************************************************************/
float Space_co = 0.5;
float Trans_co = 0.5;
int  main()
{


	FILE* fp;
	int count = 0;
	char parameter_file[5][70]; 
	char parameter_file_set[5][70];
	char parameter_file_out[5][70];
	char parameter_file_outs[5][70];
	char* chCurPath = _getcwd(NULL, 0);              // ��ǰ����Ŀ¼
	printf("current work path: %s\n", chCurPath);
	int ret = _chdir("E:\\code\\traces\\origin\\output\\");
	if (ret < 0)
	{
		perror("E:\\code\\traces\\origin\\output\\");
	}
	char* newPath = _getcwd(NULL, 0);
	printf("new work path: %s\n", newPath);
	
	struct _finddata_t data;
	struct _finddata_t set;
	long hnd = _findfirst("*.ascii", &data);
	if (hnd < 0)
	{
		perror("*.ascii");
	}

	int  nRet = (hnd < 0) ? -1 : 1;

	while (nRet >= 0)
	{
		char temp[60];
		char tt[30];
		char ttt[30];
		char* p;
		int l;
		memset(temp, '\0', sizeof(temp));
		memset(tt, '\0', sizeof(tt));
		memset(ttt, '\0', sizeof(ttt));
		if (data.attrib == _A_SUBDIR)  // �����Ŀ¼
			printf("   [%s]*\n", data.name);
		else
		{

			strcpy(temp, "E:\\code\\traces\\origin\\output\\");
			strcat(temp, data.name);

			strcpy(parameter_file[count], temp);

			p = strchr(data.name, '.');
			l = p - data.name;
			strcpy(parameter_file_out[count], "E:\\code\\traces\\result\\");
			strcpy(parameter_file_outs[count], "E:\\code\\traces\\result\\");

			strncpy(tt, data.name, l);
			strncpy(ttt, data.name, l);

			strcat(parameter_file_out[count], tt);
			strcat(parameter_file_outs[count], ttt);
			strcat(parameter_file_out[count], "_m_g.out");
			strcat(parameter_file_outs[count], "_rel_m_g.dat");
			count++;
		}


		nRet = _findnext(hnd, &data);
	}
	_findclose(hnd);
	count = 0;
	long hnd2 = _findfirst("*.parameters", &set);
	if (hnd2 < 0)
	{
		perror("*.ascii");
	}

	int  nRet2 = (hnd2 < 0) ? -1 : 1;

	while (nRet2 >= 0)
	{
		char temp[60];
		memset(temp, '\0', sizeof(temp));
		if (set.attrib == _A_SUBDIR)  // �����Ŀ¼
			printf("   [%s]*\n", set.name);
		else
		{
			strcpy(temp, "E:\\code\\traces\\origin\\output\\");
			strcat(temp, set.name);
			strcpy(parameter_file_set[count], temp);
			count++;
		}


		nRet2 = _findnext(hnd2, &set);

	}
	_findclose(hnd2);
	_chdir(newPath);
	free(newPath);
	for (int iii =0; iii <5; iii++)
	{

		printf("%s\n", parameter_file[iii]);
		printf("%s\n", parameter_file_set[iii]);
		printf("%s\n", parameter_file_out[iii]);
		printf("%s\n", parameter_file_outs[iii]);


		unsigned  int i, j, k, r, m;
		struct ssd_info* ssd;

#ifdef DEBUG
		printf("enter main\n");
#endif

		ssd = (struct ssd_info*)malloc(sizeof(struct ssd_info));
		alloc_assert(ssd, "ssd");
		memset(ssd, 0, sizeof(struct ssd_info));
		ssd->program_count_gc = 0;
		ssd->program_count_level = 0;
		ssd->program_count_pre = 0;
		ssd->count_valid = 0;
		ssd->count_valid0 = 0;
		ssd->count_valid1 = 0;
		ssd->count_valid2 = 0;
		ssd->count_valid3 = 0;
		ssd->count_valid4 = 0;
		ssd->gc_num = 0;
		ssd->count_write = 0;
		strcpy_s(ssd->parameterfilename , 70, parameter_file_set[iii]);
		strcpy_s(ssd->tracefilename, 70, parameter_file[iii]);
		strcpy_s(ssd->outputfilename, 70, parameter_file_out[iii]);
		strcpy_s(ssd->statisticfilename, 70, parameter_file_outs[iii]);


		ssd = initiation(ssd);
		make_aged(ssd);
		pre_process_page(ssd);


		for (r = 0; r < ssd->parameter->granularity_num; r++)
			for (i = 0; i < ssd->parameter->granularity_channel[r]; i++)
				for (m = 0; m < ssd->parameter->die_chip; m++)
					for (j = 0; j < ssd->parameter->die_chip; j++)
						for (k = 0; k < ssd->parameter->plane_die; k++)
						{
							printf("granularity:%d,channel:%d,chip:%d,die:%d,plane:%d have free page: %d\n", r, i, m, j, k, ssd->granularity_head[r].channel_head[i].chip_head[m].die_head[j].plane_head[k].free_page);

						}


		fprintf(ssd->outputfile, "\t\t\t\t\t\t\t\t\tOUTPUT\n");
		fprintf(ssd->outputfile, "****************** TRACE INFO ******************\n");

		ssd = simulate(ssd);


		printf("count_valid0: %d\n", ssd->count_valid0);
		printf("count_valid1: %d\n", ssd->count_valid1);
		printf("count_valid2: %d\n", ssd->count_valid2);
		printf("count_valid3: %d\n", ssd->count_valid3);
		
	\
		statistic_output(ssd);
		free_all_node(ssd);

		printf("\n");
		printf("the simulation is completed!\n");
	
	}
	
	system("pause");
	
/* 	_CrtDumpMemoryLeaks(); */

}

/******************simulate() *********************************************************************
*simulate()�Ǻ��Ĵ���������Ҫʵ�ֵĹ��ܰ���
*1,��trace�ļ��л�ȡһ�����󣬹ҵ�ssd->request
*2������ssd�Ƿ���dram�ֱ�������������󣬰���Щ�������Ϊ��д�����󣬹ҵ�ssd->channel����ssd��
*3�������¼����Ⱥ���������Щ��д������
*4�����ÿ������������󶼴������������Ϣ��outputfile�ļ���
**************************************************************************************************/
struct ssd_info *simulate(struct ssd_info *ssd)
{
	int flag=1,flag1=0;
	double output_step=0;
	unsigned int a=0,b=0;
	errno_t err;

	printf("\n");
	printf("begin simulating.......................\n");
	printf("\n");
	printf("\n");
	printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");

	if((err=fopen_s(&(ssd->tracefile),ssd->tracefilename,"r"))!=0)
	{  
		printf("the trace file can't open\n");
		return NULL;
	}

	fprintf(ssd->outputfile,"      arrive           lsn     size ope     begin time    response time    process time\n");	
	fflush(ssd->outputfile);

	while(flag!=100)      
	{
        
		flag=get_requests(ssd);

		if(flag == 1)
		{   
			//printf("once\n");
			if (ssd->parameter->dram_capacity!=0)
			{
				buffer_management(ssd);  
				distribute(ssd); 
			} 
			else
			{
				no_buffer_distribute(ssd);
			}		
		}

		process(ssd);    
		trace_output(ssd);
		if(flag == 0 && ssd->request_queue == NULL)
			flag = 100;
	}

	fclose(ssd->tracefile);
	return ssd;
}



/********    get_request    ******************************************************
*	1.get requests that arrived already
*	2.add those request node to ssd->reuqest_queue
*	return	0: reach the end of the trace
*			-1: no request has been added
*			1: add one request to list
*SSDģ����������������ʽ:ʱ������(��ȷ��̫��) �¼�����(���������) trace����()��
*���ַ�ʽ�ƽ��¼���channel/chip״̬�ı䡢trace�ļ�����ﵽ��
*channel/chip״̬�ı��trace�ļ����󵽴���ɢ����ʱ�����ϵĵ㣬ÿ�δӵ�ǰ״̬����
*��һ��״̬��Ҫ���������һ��״̬��ÿ����һ����ִ��һ��process
********************************************************************************/
int get_requests(struct ssd_info *ssd)  
{  
	char buffer[200];
	unsigned int lsn=0;
	int device,  size, ope,large_lsn, i = 0,j=0;
	struct request *request1;
	int flag = 1;
	long filepoint; 
	__int64 time_t = 0;
	__int64 nearest_event_time;    
	int stage=2;
	int io = 0;
	#ifdef DEBUG
	printf("enter get_requests,  current time:%I64u\n",ssd->current_time);
	#endif


	
	if(feof(ssd->tracefile)){
		return 100; 
	}
	

	filepoint = ftell(ssd->tracefile);	
	fgets(buffer, 200, ssd->tracefile); 
	sscanf(buffer,"%I64u %d %d %d %d" ,&time_t,&device,&lsn,&size,&ope);




	if ((device<0)&&(lsn<0)&&(size<0)&&(ope<0))
	{
		return 100;
	}
	if (lsn<ssd->min_lsn) 
		ssd->min_lsn=lsn;
	if (lsn>ssd->max_lsn)
		ssd->max_lsn=lsn;
	/******************************************************************************************************
	*�ϲ��ļ�ϵͳ���͸�SSD���κζ�д��������������֣�LSN��size�� LSN���߼������ţ������ļ�ϵͳ���ԣ����������Ĵ�
	*���ռ���һ�����Ե������ռ䡣���磬������260��6����ʾ������Ҫ��ȡ��������Ϊ260���߼�������ʼ���ܹ�6��������
	*large_lsn: channel�����ж��ٸ�subpage�������ٸ�sector��overprovideϵ����SSD�в��������еĿռ䶼���Ը��û�ʹ�ã�
	*����32G��SSD������10%�Ŀռ䱣�������������ã����Գ���1-provide
	***********************************************************************************************************/
	large_lsn=(int)(((ssd->parameter->granularity_size[0]/512)*(ssd->parameter->block_capacity/ ssd->parameter->granularity_size[0])*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_channel[0]*ssd->parameter->channel_number)*(1-ssd->parameter->overprovide));
	lsn = lsn%large_lsn;
	nearest_event_time = find_nearest_event(ssd);
	
	if (device == 1)
	{
		if (lsn != 123)//��ͬ���㿪ʼ����Ҫ�ж���һ���׶��Ƿ�ִ����
		{
			if (lsn == 777)
			{
				stage = 0;
			}
			else if (lsn == 888)
			{
				if (ssd->request_queue == NULL)
				{
					stage = 0;
				}
				else
				{
					fseek(ssd->tracefile, filepoint, 0);
					if (nearest_event_time == 0x7fffffffffffffff)
					{
						ssd->current_time += 50;//ָ��ʱ��
						return -1;
					}
					else
					{
						ssd->current_time = nearest_event_time;
						return -1;
					}

				}

			}
			else if (lsn == 999)
			{
				if (ssd->request_queue == NULL)
				{
					stage = 0;
				}
				else
				{
					fseek(ssd->tracefile, filepoint, 0);
					if (nearest_event_time == 0x7fffffffffffffff)
					{
						ssd->current_time += 50;//ָ��ʱ��
						return -1;
					}
					else
					{
						ssd->current_time = nearest_event_time;
						return -1;
					}
				}
			}
		}
		else//ת������ ��Ҫ�ж�io����
		{
			if (ssd->request_queue == NULL)
			{
				stage = 1;
			}
			else
			{
				fseek(ssd->tracefile, filepoint, 0);
				if (nearest_event_time == 0x7fffffffffffffff)
				{
					ssd->current_time += 50;//ָ��ʱ��
					return -1;
				}
				else
				{
					ssd->current_time = nearest_event_time;
					return -1;
				}
			}
		}
		switch (stage)
		{
		case 0: //ioing
			io = 1;
			break;
		case 1://computing
			io = 1;
			ssd->current_time += time_t;
			break;
		case 2://blocing

			break;
		}

	}
	else
	{
		io = 0;
		time_t = ssd->current_time;
	}
	if (io)
	{
		filepoint = ftell(ssd->tracefile);
		fgets(buffer, 200, ssd->tracefile);
		sscanf(buffer, "%I64u %d %d %d %d", &time_t, &device, &lsn, &size, &ope);
		time_t = ssd->current_time;

		if (device == 1)
		{
			filepoint = ftell(ssd->tracefile);
			fgets(buffer, 200, ssd->tracefile);
			sscanf(buffer, "%I64u %d %d %d %d", &time_t, &device, &lsn, &size, &ope);
			time_t = ssd->current_time+50;
		}

	}

	if (nearest_event_time==0x7fffffffffffffff)
	{
		ssd->current_time=time_t;           
		                                                  
		//if (ssd->request_queue_length>ssd->parameter->queue_length)    //���������еĳ��ȳ����������ļ��������õĳ���                     
		//{
			//printf("error in get request , the queue length is too long\n");
		//}
	}
	else
	{   
		if(nearest_event_time<time_t)
		{
			//printf("backing\nbacking\nbacking\nbacking\nbacking\nbacking\nbacking\n");
			/*******************************************************************************
			*�ع��������û�а�time_t����ssd->current_time����trace�ļ��Ѷ���һ����¼�ع�
			*filepoint��¼��ִ��fgets֮ǰ���ļ�ָ��λ�ã��ع����ļ�ͷ+filepoint��
			*int fseek(FILE *stream, long offset, int fromwhere);���������ļ�ָ��stream��λ�á�
			*���ִ�гɹ���stream��ָ����fromwhere��ƫ����ʼλ�ã��ļ�ͷ0����ǰλ��1���ļ�β2��Ϊ��׼��
			*ƫ��offset��ָ��ƫ���������ֽڵ�λ�á����ִ��ʧ��(����offset�����ļ������С)���򲻸ı�streamָ���λ�á�
			*�ı��ļ�ֻ�ܲ����ļ�ͷ0�Ķ�λ��ʽ���������д��ļ���ʽ��"r":��ֻ����ʽ���ı��ļ�	
			**********************************************************************************/
			fseek(ssd->tracefile,filepoint,0); 
			if(ssd->current_time<=nearest_event_time)
				ssd->current_time=nearest_event_time;
			return -1;
		}
		else
		{
			if (ssd->request_queue_length>=ssd->parameter->queue_length)
			{
				fseek(ssd->tracefile,filepoint,0);
				ssd->current_time=nearest_event_time;
				return -1;
			} 
			else
			{
				ssd->current_time=time_t;
			}
		}
	}

	if(time_t < 0)
	{
		printf("error!\n");
		while(1){}
	}
	if(feof(ssd->tracefile)){
		request1 = NULL;
		return 100; 
	}
	
	request1 = (struct request*)malloc(sizeof(struct request));
	alloc_assert(request1,"request");
	memset(request1,0, sizeof(struct request));
	
	request1->time = time_t;
	request1->lsn = lsn;
	request1->size = size;
	request1->operation = ope;	
	request1->begin_time = time_t;
	request1->response_time = 0;	
	request1->energy_consumption = 0;	
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;         //record the count of lsn served by buffer
	filepoint = ftell(ssd->tracefile);		// set the file point

	if(ssd->request_queue == NULL)          //The queue is empty
	{
		ssd->request_queue = request1;
		ssd->request_tail = request1;
		ssd->request_queue_length++;
	}
	else
	{			

		(ssd->request_tail)->next_node = request1;	
		ssd->request_tail = request1;			
		ssd->request_queue_length++;
	}

	if (request1->operation==1)             //����ƽ�������С 1Ϊ�� 0Ϊд
	{
		ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+request1->size)/(ssd->read_request_count+1);
	} 
	else
	{
		ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+request1->size)/(ssd->write_request_count+1);
	}

	
	//filepoint = ftell(ssd->tracefile);	
	//fgets(buffer, 200, ssd->tracefile);    //Ѱ����һ������ĵ���ʱ��
	//sscanf(buffer,"%I64u %d %d %d %d",&time_t,&device,&lsn,&size,&ope);
	//ssd->next_request_time=time_t;
	//fseek(ssd->tracefile,filepoint,0);

	return 1;
}
unsigned int req_grannnum(struct ssd_info* ssd,  unsigned int granu)
{
	unsigned int granu_size,i;
	granu_size = granu;
	for (i = 0;i < ssd->parameter->granularity_num;i++)
	{
		if (granu_size == ssd->parameter->granularity_size[i])
		{
			return i;
		}
	}
}


/**********************************************************************************************************************************************
*��������Ĵ�С���ض�Ӧ�������� (�����������ѡ����Ҫ���ǿռ������ʡ���ȡ�ٶ�)
***********************************************************************************************************************************************/
struct request* request_granularity(struct ssd_info* ssd,struct request* new_request)
{
	unsigned int i=0,request_siez,granularity=0,p1=0,p2=0;
	request_siez = new_request->size*512; //����lsn�������������С��ÿ��lsn��СΪ512B
	float spaceloss1, spaceloss2, program_time1, program_time2, loss1, loss2;

	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		if (i == 0)
		{
			if (request_siez <= (ssd->parameter->granularity_size[i]))
			{
				new_request->granularity = ssd->parameter->granularity_size[i];
				break;
			}
			else
			{

				if((request_siez > ssd->parameter->granularity_size[i]) &&(request_siez < ssd->parameter->granularity_size[i+1]))
				{
					spaceloss1 = abs(ssd->parameter->granularity_size[i] * (ceil(request_siez / ssd->parameter->granularity_size[i])) - request_siez);
					spaceloss2 = ssd->parameter->granularity_size[i + 1] - request_siez;
					program_time1 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i]);
					program_time2 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i+1]);
					
					loss1 = spaceloss1 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time1 ) / ((float)Prog_time(ssd,ssd->parameter->granularity_size[ssd->parameter->granularity_num-1])) * Trans_co * ceil(request_siez / ssd->parameter->granularity_size[i]);
					loss2 = spaceloss2 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time2 ) / (((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]))) * Trans_co;
					if (loss1 < loss2)
					{
						new_request->granularity = ssd->parameter->granularity_size[0];
						break;
					}
						
					else
					{
						new_request->granularity = ssd->parameter->granularity_size[1];
						break;
					}
						
				}

			}





		}
		else if (i != ssd->parameter->granularity_num - 1)
		{
			if (request_siez == (ssd->parameter->granularity_size[i]))
			{
				new_request->granularity = ssd->parameter->granularity_size[i];
				break;
			}
			if ((request_siez > ssd->parameter->granularity_size[i]) && (request_siez < ssd->parameter->granularity_size[i + 1]))
			{
				spaceloss1 = abs(ssd->parameter->granularity_size[i] * (ceil(request_siez / ssd->parameter->granularity_size[i])) - request_siez);
				spaceloss2 = ssd->parameter->granularity_size[i + 1] - request_siez;
				program_time1 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i]);
				program_time2 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i+1]);
				loss1 = spaceloss1 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time1 ) / ((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) * Trans_co * ceil(request_siez / ssd->parameter->granularity_size[i]);
				loss2 = spaceloss2 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time2 ) / ((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) * Trans_co;
				if (loss1 < loss2)
				{
					new_request->granularity = ssd->parameter->granularity_size[i];
					break;
				}
				
				else
				{
					new_request->granularity = ssd->parameter->granularity_size[i+1];
					break;
				}
			}

		}
		else
		{
			new_request->granularity = ssd->parameter->granularity_size[i];
			break;
		}

	}
 
	return new_request;
}
unsigned int request_granularity_sub(struct ssd_info* ssd, struct sub_request* new_request)
{
	unsigned int i = 0, request_siez, granularity = 0, p1 = 0, p2 = 0;
	request_siez = new_request->size * 512; //����lsn�������������С��ÿ��lsn��СΪ512B
	float spaceloss1, spaceloss2, program_time1, program_time2, loss1, loss2;
	if (new_request->lpn == 1024)
	{
		printf("sss");
	}
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		if (i == 0)
		{	
			if (request_siez <= (ssd->parameter->granularity_size[i]))
			{
				granularity = 0;
				break;
			}
			else
			{

				if ((request_siez > ssd->parameter->granularity_size[i]) && (request_siez < ssd->parameter->granularity_size[i + 1]))
				{
					spaceloss1 = abs(ssd->parameter->granularity_size[i] * (ceil(request_siez / ssd->parameter->granularity_size[i])) - request_siez);
					spaceloss2 = ssd->parameter->granularity_size[i + 1] - request_siez;
					program_time1 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i]);
					program_time2 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i + 1]);

					loss1 = spaceloss1 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time1) / ((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) * Trans_co * ceil(request_siez / ssd->parameter->granularity_size[i]);
					loss2 = spaceloss2 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time2) / (((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]))) * Trans_co;
					if (loss1 < loss2)
					{
						granularity = 0;
						break;
					}

					else
					{
						granularity = 1;
						break;
					}

				}

			}


		}
		else if (i != ssd->parameter->granularity_num - 1)
		{
			if (request_siez == (ssd->parameter->granularity_size[i]))
			{
				granularity = i;
				break;
			}
			if ((request_siez > ssd->parameter->granularity_size[i]) && (request_siez < ssd->parameter->granularity_size[i + 1]))
			{
				spaceloss1 = abs(ssd->parameter->granularity_size[i] * (ceil(request_siez / ssd->parameter->granularity_size[i])) - request_siez);
				spaceloss2 = ssd->parameter->granularity_size[i + 1] - request_siez;
				program_time1 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i]);
				program_time2 = (float)Prog_time(ssd, ssd->parameter->granularity_size[i+1]);
				loss1 = spaceloss1 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time1) / ((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) * Trans_co * ceil(request_siez / ssd->parameter->granularity_size[i]);
				loss2 = spaceloss2 / (ssd->parameter->granularity_size[i + 1] - ssd->parameter->granularity_size[i] - 1) * Space_co + (program_time2) / ((float)Prog_time(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1])) * Trans_co;
				if (loss1 < loss2)
				{
					granularity = i;
					break;
				}

				else
				{
					granularity = i+1;
					break;
				}
			}
		}
		else
		{
			granularity = i;
		}
			
	}
	return granularity;
}
/**********************************************************************************************************************************************
*����buffer�Ǹ�дbuffer������Ϊд�������ģ���Ϊ��flash��ʱ��tRΪ20us��дflash��ʱ��tprogΪ200us������Ϊд������ܽ�ʡʱ��
*  �����������������buffer����buffer������ռ��channel��I/O���ߣ�û������buffer����flash����ռ��channel��I/O���ߣ����ǲ���buffer��
*  д����������request�ֳ�sub_request����������Ƕ�̬���䣬sub_request�ҵ�ssd->sub_request�ϣ���Ϊ��֪��Ҫ�ȹҵ��ĸ�channel��sub_request��
*          ����Ǿ�̬������sub_request�ҵ�channel��sub_request����,ͬʱ���ܶ�̬���仹�Ǿ�̬����sub_request��Ҫ�ҵ�request��sub_request����
*		   ��Ϊÿ������һ��request����Ҫ��traceoutput�ļ�������������request����Ϣ��������һ��sub_request,�ͽ����channel��sub_request��
*		   ��ssd��sub_request����ժ����������traceoutput�ļ����һ���������request��sub_request����
*		   sub_request����buffer����buffer����д�����ˣ����ҽ���sub_page�ᵽbuffer��ͷ(LRU)����û��������buffer�������Ƚ�buffer��β��sub_request
*		   д��flash(������һ��sub_requestд���󣬹ҵ��������request��sub_request���ϣ�ͬʱ�Ӷ�̬���仹�Ǿ�̬����ҵ�channel��ssd��
*		   sub_request����),�ڽ�Ҫд��sub_pageд��buffer��ͷ
***********************************************************************************************************************************************/
//struct ssd_info *buffer_management(struct ssd_info *ssd)
//{   
//	unsigned int j,lsn,lpn,last_lpn,first_lpn,index,complete_flag=0, state,full_page;
//	unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;           
//	struct request *new_request;
//	struct buffer_group *buffer_node,key;
//	unsigned int mask=0,offset1=0,offset2=0;
//
//	#ifdef DEBUG
//	printf("enter buffer_management,  current time:%I64u\n",ssd->current_time);
//	#endif
//	ssd->dram->current_time=ssd->current_time;
//	full_page=~(0xffffffff<<ssd->parameter->subpage_page);
//	
//	new_request=ssd->request_tail;
//
//	lsn=new_request->lsn;
//	lpn=new_request->lsn/ssd->parameter->subpage_page;
//	last_lpn=(new_request->lsn+new_request->size-1)/ssd->parameter->subpage_page;
//	first_lpn=new_request->lsn/ssd->parameter->subpage_page;
//
//	/**/
//
//	new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
//	alloc_assert(new_request->need_distr_flag,"new_request->need_distr_flag");
//	memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
//	
//	if(new_request->operation==READ) 
//	{		
//		while(lpn<=last_lpn)      		
//		{
//			/************************************************************************************************
//			 *need_distb_flag��ʾ�Ƿ���Ҫִ��distribution������1��ʾ��Ҫִ�У�buffer��û�У�0��ʾ����Ҫִ��
//             *��1��ʾ��Ҫ�ַ���0��ʾ����Ҫ�ַ�����Ӧ���ʼȫ����Ϊ1
//			*************************************************************************************************/
//			need_distb_flag=full_page;   
//			key.group=lpn;
//			buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);		// buffer node 
//
//			while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->parameter->subpage_page)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
//			{             	
//				lsn_flag=full_page;
//				mask=1 << (lsn%ssd->parameter->subpage_page);
//				if(mask>31)
//				{
//					printf("the subpage number is larger than 32!add some cases");
//					getchar(); 		   
//				}
//				else if((buffer_node->stored & mask)==mask)
//				{
//					flag=1;
//					lsn_flag=lsn_flag&(~mask);
//				}
//
//				if(flag==1)				
//				{	//�����buffer�ڵ㲻��buffer�Ķ��ף���Ҫ������ڵ��ᵽ���ף�ʵ����LRU�㷨�������һ��˫����С�		       		
//					if(ssd->dram->buffer->buffer_head!=buffer_node)     
//					{		
//						if(ssd->dram->buffer->buffer_tail==buffer_node)								
//						{			
//							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
//							ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;							
//						}				
//						else								
//						{				
//							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
//							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
//						}								
//						buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;
//						ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
//						buffer_node->LRU_link_pre=NULL;			
//						ssd->dram->buffer->buffer_head=buffer_node;													
//					}						
//					ssd->dram->buffer->read_hit++;					
//					new_request->complete_lsn_count++;											
//				}		
//				else if(flag==0)
//					{
//						ssd->dram->buffer->read_miss_hit++;
//					}
//
//				need_distb_flag=need_distb_flag&lsn_flag;
//				
//				flag=0;		
//				lsn++;						
//			}	
//				
//			index=(lpn-first_lpn)/(32/ssd->parameter->subpage_page); 			
//			new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->parameter->subpage_page))*ssd->parameter->subpage_page));	
//			lpn++;
//			
//		}
//	}  
//	else if(new_request->operation==WRITE)
//	{
//		while(lpn<=last_lpn)           	
//		{	
//			need_distb_flag=full_page;
//			mask=~(0xffffffff<<(ssd->parameter->subpage_page));
//			state=mask;
//
//			if(lpn==first_lpn)
//			{
//				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-new_request->lsn);
//				state=state&(0xffffffff<<offset1);
//			}
//			if(lpn==last_lpn)
//			{
//				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(new_request->lsn+new_request->size));
//				state=state&(~(0xffffffff<<offset2));
//			}
//			
//			ssd=insert2buffer(ssd, lpn, state,NULL,new_request);
//			lpn++;
//		}
//	}
//	complete_flag = 1;
//	for(j=0;j<=(last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32;j++)
//	{
//		if(new_request->need_distr_flag[j] != 0)
//		{
//			complete_flag = 0;
//		}
//	}
//
//	/*************************************************************
//	*��������Ѿ���ȫ����buffer���񣬸�������Ա�ֱ����Ӧ��������
//	*�������dram�ķ���ʱ��Ϊ1000ns
//	**************************************************************/
//	if((complete_flag == 1)&&(new_request->subs==NULL))               
//	{
//		new_request->begin_time=ssd->current_time;
//		new_request->response_time=ssd->current_time+1000;            
//	}
//
//	return ssd;
//}
struct ssd_info* buffer_management(struct ssd_info* ssd)
{
	unsigned int j, lsn, lpn, last_lpn, first_lpn, index, complete_flag = 0;
	unsigned long full_page, need_distb_flag,mask = 0,state;
	unsigned int flag = 0, lsn_flag, flag1 = 1, active_region_flag = 0;
	struct request* new_request;
	struct buffer_group* buffer_node, key;
	unsigned int  offset1 = 0, offset2 = 0,balance_flag = 0;

#ifdef DEBUG
	printf("enter buffer_management,  current time:%I64u\n", ssd->current_time);
#endif
	ssd->dram->current_time = ssd->current_time;
	
	balance_flag = balance_granularity(ssd);
	new_request = ssd->request_tail;
	request_granularity(ssd, new_request);
	//�������lsn�벻ͬ����lpn��ӳ���������ģ���ͨ��python����Ԥ������

	//full_page = ~(0xffffffff << (new_request->granularity / 512));
	if ((new_request->granularity / 512) != 32)
		full_page = ~(0xffffffff << (new_request->granularity / 512));
	else
	{
		full_page = 0xffffffff;
	}
	
	lsn = new_request->lsn;


	lpn = (new_request->lsn / (ssd->parameter->granularity_size[0] / 512))-(new_request->lsn/ (ssd->parameter->granularity_size[0] / 512))%(new_request->granularity / ssd->parameter->granularity_size[0]); //lpn��ַ�ռ������С�����������֣���ͨ�����ȷ��䡣
	//lpn = new_request->lsn / ssd->parameter->subpage_page;
	last_lpn = ((lsn+ new_request->size-1) / (ssd->parameter->granularity_size[0] / 512)) - ((lsn + new_request->size-1) / (ssd->parameter->granularity_size[0] / 512)) % (new_request->granularity / ssd->parameter->granularity_size[0]);//��ͬ�����߼�ҳ��ַ���������㣬�߼�ҳ������ҳ��ͬ
	first_lpn = lpn;

	/**/

	new_request->need_distr_flag = (unsigned long*)malloc(sizeof(unsigned long) * ((((lsn + new_request->size-1) / (ssd->parameter->granularity_size[0] / 512)) - first_lpn ) * (ssd->parameter->granularity_size[0] / 512) / 32 +1));
	alloc_assert(new_request->need_distr_flag, "new_request->need_distr_flag");
	memset(new_request->need_distr_flag, 0, sizeof(unsigned long) * ( (((lsn + new_request->size-1) / (ssd->parameter->granularity_size[0] / 512)) - first_lpn ) * (ssd->parameter->granularity_size[0] / 512) / 32 +1));
	

	if (new_request->operation == READ)
	{
		while (lpn <= last_lpn)
		{
			/************************************************************************************************
			 *need_distb_flag��ʾ�Ƿ���Ҫִ��distribution������1��ʾ��Ҫִ�У�buffer��û�У�0��ʾ����Ҫִ��
			 *��1��ʾ��Ҫ�ַ���0��ʾ����Ҫ�ַ�����Ӧ���ʼȫ����Ϊ1
			*************************************************************************************************/
			need_distb_flag = full_page;
			key.group = lpn;
			buffer_node = (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE*)&key);		// buffer node 

			while ((buffer_node != NULL) && (lsn < (lpn + 1) * (ssd->parameter->granularity_size[0] / 512)) && (lsn <= (new_request->lsn + new_request->size - 1)))
			{
				lsn_flag = full_page;
				mask = 1 << (lsn % (new_request->granularity / 512));
				//if (mask > 31)
				//{
				//	printf("the subpage number is larger than 32!add some cases");
				//	getchar();
				//}
				if ((buffer_node->stored & mask) == mask)
				{
					flag = 1;
					lsn_flag = lsn_flag & (~mask);
				}

				if (flag == 1) //LRU�㷨
				{	//�����buffer�ڵ㲻��buffer�Ķ��ף���Ҫ������ڵ��ᵽ���ף�ʵ����5LRU�㷨�������һ��˫����С�		       		
					if (ssd->dram->buffer->buffer_head != buffer_node)
					{
						if (ssd->dram->buffer->buffer_tail == buffer_node)
						{
							buffer_node->LRU_link_pre->LRU_link_next = NULL;
							ssd->dram->buffer->buffer_tail = buffer_node->LRU_link_pre;
						}
						else
						{
							buffer_node->LRU_link_pre->LRU_link_next = buffer_node->LRU_link_next;
							buffer_node->LRU_link_next->LRU_link_pre = buffer_node->LRU_link_pre;
						}
						buffer_node->LRU_link_next = ssd->dram->buffer->buffer_head;
						ssd->dram->buffer->buffer_head->LRU_link_pre = buffer_node;
						buffer_node->LRU_link_pre = NULL;
						ssd->dram->buffer->buffer_head = buffer_node;
					}
					ssd->dram->buffer->read_hit++;
					new_request->complete_lsn_count++;
				}
				else if (flag == 0)
				{
					ssd->dram->buffer->read_miss_hit++;
				}

				need_distb_flag = need_distb_flag & lsn_flag;

				flag = 0;
				lsn++;
			}

			index = (lpn - first_lpn) / (32 / (ssd->parameter->granularity_size[0] / 512));//����ڶ��ٸ�flag��ʾ��ǰҳ
			new_request->need_distr_flag[index] = new_request->need_distr_flag[index] | (need_distb_flag << (((lpn - first_lpn) % (32 / (ssd->parameter->granularity_size[0] / 512))) * (ssd->parameter->granularity_size[0] / 512)));
			lpn = lpn + (new_request->granularity / ssd->parameter->granularity_size[0]);

		}
	}
	else if (new_request->operation == WRITE)
	{
		if (balance_flag)
		{
			new_request->granularity = ssd->parameter->granularity_size[easre_least_granularity(ssd)];

		}
		while (lpn <= last_lpn)
		{
			need_distb_flag = full_page;
			

			if ((new_request->granularity / 512) != 32)
				mask = ~(0xffffffff << (new_request->granularity / 512));
			else
			{
				mask = 0xffffffff;
			}
			state = mask;

			if (lpn == first_lpn)
			{
				offset1 = (new_request->granularity / 512) - ((lpn + (new_request->granularity / ssd->parameter->granularity_size[0])) * (ssd->parameter->granularity_size[0] / 512) - new_request->lsn); //����д��������ȼ���������µ�ƫ����
				state = state & (0xffffffff << offset1); //��ʼƫ����
			}
			if (lpn == last_lpn)
			{
				offset2 = (new_request->granularity / 512) - ((lpn + (new_request->granularity / ssd->parameter->granularity_size[0])) * (ssd->parameter->granularity_size[0] / 512) - (new_request->lsn + new_request->size));
				state = state & (~(0xffffffff << offset2));//����ƫ����
			}

			ssd = insert2buffer(ssd, lpn, state, NULL, new_request);
			lpn = lpn + (new_request->granularity / ssd->parameter->granularity_size[0]);
		}
	}
	complete_flag = 1;
	for (j = 0; j <= (last_lpn - first_lpn + 1) * (ssd->parameter->granularity_size[0] / 512) / 32; j++)
	{
		if (new_request->need_distr_flag[j] != 0)
		{
			complete_flag = 0;
		}
	}

	/*************************************************************
	*��������Ѿ���ȫ����buffer���񣬸�������Ա�ֱ����Ӧ��������
	*�������dram�ķ���ʱ��Ϊ1000ns
	**************************************************************/
	if ((complete_flag == 1) && (new_request->subs == NULL))
	{
		new_request->begin_time = ssd->current_time;
		new_request->response_time = ssd->current_time + 1000;
	}

	return ssd;
}

/*****************************
*lpn��ppn��ת��
******************************/
unsigned int lpn2ppn(struct ssd_info *ssd,unsigned int lsn)
{
	int lpn, ppn;	
	struct entry *p_map = ssd->dram->map->map_entry;
#ifdef DEBUG
	printf("enter lpn2ppn,  current time:%I64u\n",ssd->current_time);
#endif
	lpn = lsn/ssd->parameter->subpage_page;			//lpn
	ppn = (p_map[lpn]).pn;
	return ppn;
}

/**********************************************************************************
*�����������������������ֻ���������д�����Ѿ���buffer_management()�����д�����
*����������к�buffer���еļ�飬��ÿ������ֽ�������󣬽���������й���channel�ϣ�
*��ͬ��channel���Լ������������
**********************************************************************************/

struct ssd_info *distribute(struct ssd_info *ssd) 
{
	unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0;
	unsigned long full_page;
	unsigned long j, k, sub_size;
	int i=0;
	struct request *req;
	struct sub_request *sub;
	unsigned int * complt;


	

	req = ssd->request_tail;
	//full_page = ~(0xffffffff << (req->granularity / 512));
	if ((req->granularity / 512) != 32)
		full_page = ~(0xffffffff << (req->granularity / 512));
	else
	{
		full_page = 0xffffffff;
}
	
	#ifdef DEBUG
	printf("enter distribute,  current time:%I64u, req_siez:%d, req_graunlarity:%d, req_lsn:%d,req_ope:%d\n", ssd->current_time,req->size,req->granularity,req->lsn,req->operation);
	#endif
	if(req->response_time != 0){
		return ssd;
	}
	if (req->operation==WRITE)
	{
		return ssd;
	}

	if(req != NULL)
	{
		if(req->distri_flag == 0)
		{
			//�������һЩ��������Ҫ����
			if(req->complete_lsn_count != ssd->request_tail->size)
			{		
				first_lsn = req->lsn;				
				last_lsn = first_lsn + req->size;
				complt = req->need_distr_flag;
				start = first_lsn - first_lsn % (ssd->parameter->granularity_size[0] / 512);
				end = ((last_lsn/ (ssd->parameter->granularity_size[0] / 512)) + 1) * (ssd->parameter->granularity_size[0] / 512);//��ҳ����lsn��
				i = (end - start)/32;	


				while(i >= 0)
				{	
					/*************************************************************************************
					*һ��32λ���������ݵ�ÿһλ����һ����ҳ��32/ssd->parameter->subpage_page�ͱ�ʾ�ж���ҳ��
					*�����ÿһҳ��״̬��������� req->need_distr_flag�У�Ҳ����complt�У�ͨ���Ƚ�complt��
					*ÿһ����full_page���Ϳ���֪������һҳ�Ƿ�����ɡ����û���������ͨ��creat_sub_request
					��������������
					*************************************************************************************/
					for(j=0; j<32/ (req->granularity / 512); j++)
					{	
						k = (complt[((end-start)/32-i)] >>((req->granularity / 512) *j)) & full_page;
						if (k !=0)
						{			
							lpn = (start / (ssd->parameter->granularity_size[0] / 512)) - (start / (ssd->parameter->granularity_size[0] / 512)) % (req->granularity / ssd->parameter->granularity_size[0])+ ((end - start) / 32 - i) * 32 / (ssd->parameter->granularity_size[0] / 512)+j*(req->granularity/ ssd->parameter->granularity_size[0]);
						
							sub_size=transfer_size(ssd,k,lpn,req);    
							if (sub_size==0) 
							{
								continue;
							}
							else
							{
								sub=creat_sub_request(ssd,lpn,sub_size,0,req,req->operation,req->granularity,0,16384);
							}	
						}
					}
					i = i-1;
				}

			}
			else
			{
				req->begin_time=ssd->current_time;
				req->response_time=ssd->current_time+1000;   
			}

		}
	}
	return ssd;
}


/**********************************************************************
*trace_output()��������ÿһ����������������󾭹�process()�����������
*��ӡ�����ص����н����outputfile�ļ��У�����Ľ����Ҫ�����е�ʱ��
**********************************************************************/
void trace_output(struct ssd_info* ssd){
	int flag = 1;	
	__int64 start_time, end_time;
	struct request *req, *pre_node;
	struct sub_request *sub, *tmp;

#ifdef DEBUG
	printf("enter trace_output,  current time:%I64u\n",ssd->current_time);
#endif

	pre_node=NULL;
	req = ssd->request_queue;
	start_time = 0;
	end_time = 0;

	if (req == NULL)
	{
		
		return;
	}
		

	while(req != NULL)	
	{
		sub = req->subs;
		flag = 1;
		start_time = 0;
		end_time = 0;

		if(req->response_time != 0)
		{
			
			fprintf(ssd->outputfile,"%16I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, req->response_time-req->time);
			fflush(ssd->outputfile);

			if(req->response_time-req->begin_time==0)
			{
				printf("the begin time is %d \n",req->begin_time);
				printf("the response time is 0?? \n");
				getchar();
			}

			if (req->operation==READ)
			{
				ssd->read_request_count++;
				ssd->read_avg=ssd->read_avg+(req->response_time-req->time);
			} 
			else
			{
				ssd->write_request_count++;
				ssd->write_avg=ssd->write_avg+(req->response_time-req->time);
			}

			if(pre_node == NULL)
			{
				if(req->next_node == NULL)
				{
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_queue = NULL;
					ssd->request_tail = NULL;
					ssd->request_queue_length--;
				}
				else
				{
					ssd->request_queue = req->next_node;
					pre_node = req;
					req = req->next_node;
					free(pre_node->need_distr_flag);
					pre_node->need_distr_flag=NULL;
					free((void *)pre_node);
					pre_node = NULL;
					ssd->request_queue_length--;
				}
			}
			else
			{
				if(req->next_node == NULL)
				{
					pre_node->next_node = NULL;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_tail = pre_node;
					ssd->request_queue_length--;
				}
				else
				{
					pre_node->next_node = req->next_node;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free((void *)req);
					req = pre_node->next_node;
					ssd->request_queue_length--;
				}
			}
		}
		else
		{
			flag=1;
			

			while(sub != NULL)
			{
				
				//printf("req time:%I64u, sub lpn:%d, sub size:%d,sub current_state:%d,sub next state:%d,next time:%I64u,currnet time:%I64u\n", req->begin_time, sub->lpn, sub->size, sub->current_state, sub->next_state, sub->next_state_predict_time, ssd->current_time);
				if(start_time == 0)
					start_time = sub->begin_time;
				//printf("granularity: %d --size %d--OP=%d\n", sub->granularity, sub->size,sub->operation);
				if(start_time > sub->begin_time)
					start_time = sub->begin_time;
				if(end_time < sub->complete_time)
					end_time = sub->complete_time;
				//printf("req time:%I64u,sub lpn:%d,sub current_state:%d,sub next state:%d,begin time:%I64u,next time:%I64u,currnet time:%I64u\n", req->begin_time, sub->lpn, sub->current_state, sub->next_state, sub->begin_time, sub->next_state_predict_time, ssd->current_time);

				if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))	// if any sub-request is not completed, the request is not completed
				{
					
					sub = sub->next_subs;
					
				}
				else
				{
					flag=0;
					break;
				}
				
			}

			if (flag == 1)
			{		
				fprintf(ssd->outputfile,"%16I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
				fflush(ssd->outputfile);

				if(end_time-start_time==0)
				{
					printf("the  time is %l \n", req->time);
					printf("the lsn is %d \n", req->lsn);
					printf("the size is %d \n", req->size);
					printf("start time %d \n", start_time);
					printf("end time %d \n", end_time);
					printf("the response time is 0?? \n");
					getchar();
				}

				if (req->operation==READ)
				{
					ssd->read_request_count++;
					ssd->read_avg=ssd->read_avg+(end_time-req->time);
				} 
				else
				{
					ssd->write_request_count++;
					ssd->write_avg=ssd->write_avg+(end_time-req->time);
				}

				while(req->subs!=NULL)
				{
					tmp = req->subs;
					req->subs = tmp->next_subs;
					if (tmp->update!=NULL)
					{
						free(tmp->update->location);
						tmp->update->location=NULL;
						free(tmp->update);
						tmp->update=NULL;
					}
					free(tmp->location);
					tmp->location=NULL;
					free(tmp);
					tmp=NULL;
					
				}
				
				if(pre_node == NULL)
				{
					if(req->next_node == NULL)
					{

						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_queue = NULL;
						ssd->request_tail = NULL;
						ssd->request_queue_length--;
					}
					else
					{
						ssd->request_queue = req->next_node;
						pre_node = req;

						req = req->next_node;
						free(pre_node->need_distr_flag);
						pre_node->need_distr_flag=NULL;
						free(pre_node);
						pre_node = NULL;
						ssd->request_queue_length--;
					}
				}
				else
				{
					if(req->next_node == NULL)
					{

						pre_node->next_node = NULL;
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_tail = pre_node;	
						ssd->request_queue_length--;
					}
					else
					{

						pre_node->next_node = req->next_node;
						if (req->begin_time == 0)
						{
							printf("000");
						}
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = pre_node->next_node;
						ssd->request_queue_length--;
					}

				}
			}
			else
			{	
				pre_node = req;
				req = req->next_node;
			}
		}		
	}
}


/*******************************************************************************
*statistic_output()������Ҫ�����������һ����������ش�����Ϣ��
*1�������ÿ��plane�Ĳ���������plane_erase���ܵĲ���������erase
*2����ӡmin_lsn��max_lsn��read_count��program_count��ͳ����Ϣ���ļ�outputfile�С�
*3����ӡ��ͬ����Ϣ���ļ�statisticfile��
*******************************************************************************/
void statistic_output(struct ssd_info *ssd)
{
	unsigned int lpn_count=0,i,j,c,k,r,m,plane_erase=0;
	float erase = 0;
	double gc_energy=0.0;
#ifdef DEBUG
	printf("enter statistic_output,  current time:%I64u\n",ssd->current_time);
#endif

	// for(i=0;i<ssd->parameter->channel_number;i++)
	// {
	// 	for(j=0;j<ssd->parameter->die_chip;j++)
	// 	{
	// 		for(k=0;k<ssd->parameter->plane_die;k++)
	// 		{
	// 			plane_erase=0;
	// 			for(m=0;m<ssd->parameter->block_plane;m++)
	// 			{
	// 				if(ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count>0)
	// 				{
	// 					erase=erase+ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
	// 					plane_erase+=ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
	// 				}
	// 			}
	// 			fprintf(ssd->outputfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,m,plane_erase);
	// 			fprintf(ssd->statisticfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,m,plane_erase);
	// 		}
	// 	}
	// }


	for ( r = 0; r < ssd->parameter->granularity_num; r++)
	{
		for(i=0;i<ssd->parameter->granularity_channel[r];i++)
		{
			for (c = 0;c < ssd->parameter->chip_channel[0];c++)
			{
				for (j = 0;j < ssd->parameter->die_chip;j++)
				{
					for (k = 0;k < ssd->parameter->plane_die;k++)
					{
						plane_erase = 0;
						for (m = 0;m < ssd->parameter->block_plane;m++)
						{
							if (ssd->granularity_head[r].channel_head[i].chip_head[c].die_head[j].plane_head[k].blk_head[m].erase_count > 0)
							{
								float s1, s2,s3,s4;
								s1 = ssd->granularity_head[r].channel_head[i].chip_head[c].die_head[j].plane_head[k].blk_head[m].erase_count;
								s2 = earse_time_1(ssd, ssd->parameter->granularity_size[r]);
								s3 =  earse_time_1(ssd, ssd->parameter->granularity_size[ssd->parameter->granularity_num - 1]);
								s4 = s2 / s3;
								erase += s1/s4;
								plane_erase += ssd->granularity_head[r].channel_head[i].chip_head[c].die_head[j].plane_head[k].blk_head[m].erase_count;
								//erase += ssd->granularity_head[r].channel_head[i].chip_head[c].die_head[j].plane_head[k].blk_head[m].erase_count;
							}
						}
						fprintf(ssd->outputfile, "the %d granularity,the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n", r,i, c,j, k,  plane_erase);
						fprintf(ssd->statisticfile, "the %d granularity,the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",r, i,c, j, k,  plane_erase);
					}
				}
			}

		}
	}
	




	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"---------------------------statistic data---------------------------\n");	 
	fprintf(ssd->outputfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->outputfile,"max lsn: %13d\n",ssd->max_lsn);
	fprintf(ssd->outputfile,"read count: %13d\n",ssd->read_count);	  
	fprintf(ssd->outputfile,"program count: %13d",ssd->program_count);	
	fprintf(ssd->outputfile,"                        include the flash write count leaded by read requests\n");
	fprintf(ssd->outputfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->outputfile,"erase count: %13d\n",ssd->erase_count);
	fprintf(ssd->outputfile,"direct erase count: %13d\n",ssd->direct_erase_count);
	fprintf(ssd->outputfile,"copy back count: %13d\n",ssd->copy_back_count);
	fprintf(ssd->outputfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	fprintf(ssd->outputfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	fprintf(ssd->outputfile,"interleave write count: %13d\n",ssd->interleave_count);
	fprintf(ssd->outputfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	fprintf(ssd->outputfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	fprintf(ssd->outputfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	fprintf(ssd->outputfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->outputfile,"write flash count: %13d\n",ssd->write_flash_count);
	fprintf(ssd->outputfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	fprintf(ssd->outputfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	fprintf(ssd->outputfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->outputfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->outputfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->outputfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->outputfile,"write request average size: %13f\n",ssd->ave_write_size);
	fprintf(ssd->outputfile,"read request average response time: %16I64u\n",ssd->read_avg/ssd->read_request_count);
	fprintf(ssd->outputfile,"write request average response time: %16I64u\n",ssd->write_avg/(ssd->write_request_count+1));
	fprintf(ssd->outputfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->outputfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->outputfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->outputfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->outputfile,"erase: %13lf\n",erase);
	fflush(ssd->outputfile);

	fclose(ssd->outputfile);


	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"---------------------------statistic data---------------------------\n");	
	fprintf(ssd->statisticfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->statisticfile,"max lsn: %13d\n",ssd->max_lsn);
	fprintf(ssd->statisticfile,"read count: %13d\n",ssd->read_count);	  
	fprintf(ssd->statisticfile,"program count: %13d",ssd->program_count);	  
	fprintf(ssd->statisticfile,"                        include the flash write count leaded by read requests\n");
	fprintf(ssd->statisticfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->statisticfile,"erase count: %13d\n",ssd->erase_count);	  
	fprintf(ssd->statisticfile,"direct erase count: %13d\n",ssd->direct_erase_count);
	fprintf(ssd->statisticfile,"copy back count: %13d\n",ssd->copy_back_count);
	fprintf(ssd->statisticfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	fprintf(ssd->statisticfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	fprintf(ssd->statisticfile,"interleave count: %13d\n",ssd->interleave_count);
	fprintf(ssd->statisticfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	fprintf(ssd->statisticfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	fprintf(ssd->statisticfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	fprintf(ssd->statisticfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->statisticfile,"write flash count: %13d\n",ssd->write_flash_count);
	fprintf(ssd->statisticfile,"waste page count: %13d\n",ssd->waste_page_count);
	fprintf(ssd->statisticfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	fprintf(ssd->statisticfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	fprintf(ssd->statisticfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->statisticfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->statisticfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->statisticfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->statisticfile,"write request average size: %13f\n",ssd->ave_write_size);
	fprintf(ssd->statisticfile,"read request average response time: %16I64u\n",ssd->read_avg/ssd->read_request_count);
	fprintf(ssd->statisticfile,"write request average response time: %16I64u\n",ssd->write_avg/(ssd->write_request_count+1));
	fprintf(ssd->statisticfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->statisticfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->statisticfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->statisticfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->statisticfile,"erase: %13lf\n",erase);
	fprintf(ssd->statisticfile, "count_pre: %13d\n", ssd->program_count_pre);
	fprintf(ssd->statisticfile, "count_gc: %13d\n", ssd->program_count_gc);
	fprintf(ssd->statisticfile, "count_level: %13d\n", ssd->program_count_level);
	fprintf(ssd->statisticfile, "count_get: %13d\n", ssd->program_count_get);
	fprintf(ssd->statisticfile, "write allo count: %13d\n", ssd->count_write);
	fprintf(ssd->statisticfile, "count_valid: %13d\n", ssd->count_valid);
	fprintf(ssd->statisticfile, "count_valid0: %13d\n", ssd->count_valid0);
	fprintf(ssd->statisticfile, "count_valid1: %13d\n", ssd->count_valid1);
	fprintf(ssd->statisticfile, "count_valid2: %13d\n", ssd->count_valid2);
	fprintf(ssd->statisticfile, "count_valid3: %13d\n", ssd->count_valid3);
	fprintf(ssd->statisticfile, "count_valid4: %13d\n", ssd->count_valid4);
	fprintf(ssd->statisticfile, "gc request: %13d\n", ssd->gc_num);
	fflush(ssd->statisticfile);

	fclose(ssd->statisticfile);
}


/***********************************************************************************
*����ÿһҳ��״̬�����ÿһ��Ҫ�������ҳ����Ŀ��Ҳ����һ����������Ҫ�������ҳ��ҳ��
************************************************************************************/
unsigned int size(unsigned int stored)
{
	unsigned int i,total=0,mask=0x80000000;

	//#ifdef DEBUG
	//printf("enter size\n");
	//#endif
	for(i=1;i<=32;i++)
	{
		if(stored & mask) total++;
		stored<<=1;
	}
	#ifdef DEBUG
	    printf("leave size\n");
    #endif
    return total;
}


/*********************************************************
*transfer_size()���������þ��Ǽ�������������Ҫ�����size
*�����е���������first_lpn��last_lpn�������ر��������Ϊ��
*��������º��п��ܲ��Ǵ���һ��ҳ���Ǵ���һҳ��һ���֣���
*Ϊlsn�п��ܲ���һҳ�ĵ�һ����ҳ��
*********************************************************/
unsigned int transfer_size(struct ssd_info *ssd,unsigned long need_distribute,unsigned int lpn,struct request *req)
{
	unsigned int first_lpn,last_lpn,state,trans_size;
	unsigned long mask=0,offset1=0,offset2=0;

	//first_lpn=req->lsn/ssd->parameter->subpage_page;
	//last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;

	first_lpn = (req->lsn / (ssd->parameter->granularity_size[0] / 512)) - (req->lsn / (ssd->parameter->granularity_size[0] / 512)) % (req->granularity / ssd->parameter->granularity_size[0]); //lpn��ַ�ռ������С�����������֣���ͨ�����ȷ��䡣
	//lpn = new_request->lsn / ssd->parameter->subpage_page;
	//last_lpn = lpn + ((req->size -1 )* 512) / req->granularity;//��ͬ�����߼�ҳ��ַ���������㣬�߼�ҳ������ҳ��ͬ
	last_lpn = ((req->lsn + req->size - 1) / (ssd->parameter->granularity_size[0] / 512)) - ((req->lsn + req->size-1) / (ssd->parameter->granularity_size[0] / 512)) % (req->granularity / ssd->parameter->granularity_size[0]);

	

	if ((req->granularity / 512) != 32)
		mask = ~(0xffffffff << (req->granularity / 512));
	else
	{
		mask = 0xffffffff;
	}
	state=mask;
	if(lpn==first_lpn)
	{
		offset1= (req->granularity / 512) -((lpn+(req->granularity/ ssd->parameter->granularity_size[0]))* (ssd->parameter->granularity_size[0] / 512) -req->lsn);
		state=state&(0xffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2= (req->granularity / 512) -((lpn+ (req->granularity / ssd->parameter->granularity_size[0]))* (ssd->parameter->granularity_size[0]/ 512) -(req->lsn+req->size));
		state=state&(~(0xffffffff<<offset2));
	}

	trans_size=size(state&need_distribute);

	return trans_size;
}


/**********************************************************************************************************  
*__int64 find_nearest_event(struct ssd_info *ssd)       
*Ѱ����������������絽����¸�״̬ʱ��,���ȿ��������һ��״̬ʱ�䣬���������¸�״̬ʱ��С�ڵ��ڵ�ǰʱ�䣬
*˵��������������Ҫ�鿴channel���߶�Ӧdie����һ״̬ʱ�䡣Int64���з��� 64 λ�����������ͣ�ֵ���ͱ�ʾֵ����
*-2^63 ( -9,223,372,036,854,775,808)��2^63-1(+9,223,372,036,854,775,807 )֮����������洢�ռ�ռ 8 �ֽڡ�
*channel,die���¼���ǰ�ƽ��Ĺؼ����أ������������ʹ�¼�������ǰ�ƽ���channel��die�ֱ�ص�idle״̬��die�е�
*������׼������
***********************************************************************************************************/
__int64 find_nearest_event(struct ssd_info *ssd) 
{
	unsigned int i,j,r,k;
	__int64 time=0x7fffffffffffffff;
	__int64 time1=0x7fffffffffffffff;
	__int64 time2=0x7fffffffffffffff;

	// for (i=0;i<ssd->parameter->channel_number;i++)
	// {
	// 	if (ssd->channel_head[i].next_state==CHANNEL_IDLE)
	// 		if(time1>ssd->channel_head[i].next_state_predict_time)
	// 			if (ssd->channel_head[i].next_state_predict_time>ssd->current_time)    
	// 				time1=ssd->channel_head[i].next_state_predict_time;
	// 	for (j=0;j<ssd->parameter->chip_channel[i];j++)
	// 	{
	// 		if ((ssd->channel_head[i].chip_head[j].next_state==CHIP_IDLE)||(ssd->channel_head[i].chip_head[j].next_state==CHIP_DATA_TRANSFER))
	// 			if(time2>ssd->channel_head[i].chip_head[j].next_state_predict_time)
	// 				if (ssd->channel_head[i].chip_head[j].next_state_predict_time>ssd->current_time)    
	// 					time2=ssd->channel_head[i].chip_head[j].next_state_predict_time;	
	// 	}   
	// } 
	for (j = 0; j < ssd->parameter->granularity_num; j++)
		for (i = 0; i < ssd->parameter->granularity_channel[j]; i++)
	{		
			if (ssd->granularity_head[j].channel_head[i].next_state==CHANNEL_IDLE)
				if(time1>ssd->granularity_head[j].channel_head[i].next_state_predict_time)
					if (ssd->granularity_head[j].channel_head[i].next_state_predict_time>ssd->current_time)
						time1=ssd->granularity_head[j].channel_head[i].next_state_predict_time;
			for (k=0;k<ssd->parameter->chip_channel[0];k++)
			{
				if ((ssd->granularity_head[j].channel_head[i].chip_head[k].next_state==CHIP_IDLE)||(ssd->granularity_head[j].channel_head[i].chip_head[k].next_state==CHIP_DATA_TRANSFER))
					if(time2>ssd->granularity_head[j].channel_head[i].chip_head[k].next_state_predict_time)
						if (ssd->granularity_head[j].channel_head[i].chip_head[k].next_state_predict_time>ssd->current_time)
							time2=ssd->granularity_head[j].channel_head[i].chip_head[k].next_state_predict_time;
			}   
	}
	
	


	/*****************************************************************************************************
	 *timeΪ���� A.��һ״̬ΪCHANNEL_IDLE����һ״̬Ԥ��ʱ�����ssd��ǰʱ���CHANNEL����һ״̬Ԥ��ʱ��
	 *           B.��һ״̬ΪCHIP_IDLE����һ״̬Ԥ��ʱ�����ssd��ǰʱ���DIE����һ״̬Ԥ��ʱ��
	 *		     C.��һ״̬ΪCHIP_DATA_TRANSFER����һ״̬Ԥ��ʱ�����ssd��ǰʱ���DIE����һ״̬Ԥ��ʱ��
	 *CHIP_DATA_TRANSFER��׼����״̬�������Ѵӽ��ʴ�����register����һ״̬�Ǵ�register����buffer�е���Сֵ 
	 *ע����� ��û������Ҫ���time����ʱtime����0x7fffffffffffffff ��
	*****************************************************************************************************/
	time=(time1>time2)?time2:time1;
	return time;
}

/***********************************************
*free_all_node()���������þ����ͷ���������Ľڵ�
************************************************/
void free_all_node(struct ssd_info *ssd)
{
	unsigned int i,j,k,r,l,n;
	struct buffer_group *pt=NULL;
	struct direct_erase * erase_node=NULL;
	// for (i=0;i<ssd->parameter->channel_number;i++)
	// {
	// 	for (j=0;j<ssd->parameter->chip_channel[0];j++)
	// 	{
	// 		for (k=0;k<ssd->parameter->die_chip;k++)
	// 		{
	// 			for (l=0;l<ssd->parameter->plane_die;l++)
	// 			{
	// 				for (n=0;n<ssd->parameter->block_plane;n++)
	// 				{
	// 					free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
	// 					ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head=NULL;
	// 				}
	// 				free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
	// 				ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head=NULL;
	// 				while(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node!=NULL)
	// 				{
	// 					erase_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
	// 					ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=erase_node->next_node;
	// 					free(erase_node);
	// 					erase_node=NULL;
	// 				}
	// 			}
				
	// 			free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
	// 			ssd->channel_head[i].chip_head[j].die_head[k].plane_head=NULL;
	// 		}
	// 		free(ssd->channel_head[i].chip_head[j].die_head);
	// 		ssd->channel_head[i].chip_head[j].die_head=NULL;
	// 	}
	// 	free(ssd->channel_head[i].chip_head);
	// 	ssd->channel_head[i].chip_head=NULL;
	// }

	for ( r = 0; r < ssd->parameter->granularity_num; r++)
	{
			for (i=0;i<ssd->parameter->granularity_channel[r];i++)
			{
				for (j=0;j<ssd->parameter->chip_channel[0];j++)
				{
					for (k=0;k<ssd->parameter->die_chip;k++)
					{
						for (l=0;l<ssd->parameter->plane_die;l++)
						{
							for (n=0;n<ssd->parameter->block_plane;n++)
							{			
								free(ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
								ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head=NULL;
							}
							free(ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
							ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head=NULL;
							while(ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node!=NULL)
							{
								erase_node=ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
								ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=erase_node->next_node;
								free(erase_node);
								erase_node=NULL;
							}
						}
				
						free(ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head);
						ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head=NULL;
					}
					free(ssd->granularity_head[r].channel_head[i].chip_head[j].die_head);
					ssd->granularity_head[r].channel_head[i].chip_head[j].die_head=NULL;
				}
				free(ssd->granularity_head[r].channel_head[i].chip_head);
				ssd->granularity_head[r].channel_head[i].chip_head=NULL;
			}
		free(ssd->granularity_head[r].channel_head);
		ssd->granularity_head[r].channel_head=NULL;
	}
	


	free(ssd->granularity_head);
	ssd->granularity_head=NULL;

	avlTreeDestroy( ssd->dram->buffer);
	ssd->dram->buffer=NULL;
	
	free(ssd->dram->map->map_entry);
	ssd->dram->map->map_entry=NULL;
	free(ssd->dram->map);
	ssd->dram->map=NULL;
	free(ssd->dram);
	ssd->dram=NULL;
	free(ssd->parameter);
	ssd->parameter=NULL;

	free(ssd);
	ssd=NULL;
}


/*****************************************************************************
*make_aged()���������þ���ģ����ʵ���ù�һ��ʱ���ssd��
*��ô���ssd����Ӧ�Ĳ�����Ҫ�ı䣬�����������ʵ���Ͼ��Ƕ�ssd�и��������ĸ�ֵ��
******************************************************************************/
struct ssd_info *make_aged(struct ssd_info *ssd)
{
	unsigned int i,j,k,r,l,m,n,ppn;
	int threshould,flag=0;
	
	if (ssd->parameter->aged==1)
	{
		printf("Make aged");


		for ( r = 0; r < ssd->parameter->granularity_num; r++)
			for (i=0;i<ssd->parameter->granularity_channel[r];i++)
				for (j=0;j<ssd->parameter->chip_channel[0];j++)
					for (k=0;k<ssd->parameter->die_chip;k++)
						for (l=0;l<ssd->parameter->plane_die;l++)
						{  
							flag=0;
							for (m=0;m<ssd->parameter->block_plane;m++)
							{  
								//threshold��ʾһ��plane���ж���ҳ��Ҫ��ǰ��ΪʧЧ
								threshould = (int)(ssd->parameter->block_plane * (ssd->parameter->block_capacity / ssd->parameter->granularity_size[r]) * ssd->parameter->aged_ratio);
								if (flag>=threshould)
								{
									break;
								}
								for (n=0;n<((ssd->parameter->block_capacity / ssd->parameter->granularity_size[r]) *ssd->parameter->aged_ratio+1);n++)
								{  
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state=0;        //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state=0;         //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn=0;  //��valid_state free_state lpn����Ϊ0��ʾҳʧЧ������ʱ�������⣬����lpn=0��������Чҳ
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
									ssd->granularity_head[r].channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
									flag++;

									ppn=find_ppn(ssd,r,i,j,k,l,m,n);
							
								}
							} 
						}
		 
	}  
	else
	{
		return ssd;
	}

	return ssd;
}


/*********************************************************************************************
*no_buffer_distribute()�����Ǵ���ssdû��dram��ʱ��
*���Ƕ�д����Ͳ�������Ҫ��buffer����Ѱ�ң�ֱ������creat_sub_request()���������������ٴ���
*********************************************************************************************/
struct ssd_info *no_buffer_distribute(struct ssd_info *ssd)
{
	unsigned int lsn,lpn,last_lpn,first_lpn,complete_flag=0, state,full_page,i;
	unsigned int flag=0,flag1=1,active_region_flag=0, balance_flag=0;           //to indicate the lsn is hitted or not
	struct request *req=NULL;
	struct sub_request *sub=NULL,*sub_r=NULL,*update=NULL;
	struct local *loc=NULL;
	struct channel_info *p_ch=NULL;
	
	
	unsigned int mask=0; 
	unsigned int offset1=0, offset2=0;
	unsigned int sub_size=0;
	unsigned int sub_state=0;
	
	ssd->dram->current_time=ssd->current_time;

	req = ssd->request_tail;
	request_granularity(ssd, req);

	if ((req->granularity / 512) != 32)
		full_page = ~(0xffffffff << (req->granularity / 512));
	else
	{
		full_page = 0xffffffff;
	}

	lsn = req->lsn;

	lsn=req->lsn;
	lpn = (req->lsn / (ssd->parameter->granularity_size[0] / 512)) - (req->lsn / (ssd->parameter->granularity_size[0] / 512)) % (req->granularity / ssd->parameter->granularity_size[0]); //lpn��ַ�ռ������С�����������֣���ͨ�����ȷ��䡣
	//lpn = new_request->lsn / ssd->parameter->subpage_page;
	last_lpn = ((lsn + req->size - 1) / (ssd->parameter->granularity_size[0] / 512)) - ((lsn + req->size - 1) / (ssd->parameter->granularity_size[0] / 512)) % (req->granularity / ssd->parameter->granularity_size[0]);//��ͬ�����߼�ҳ��ַ���������㣬�߼�ҳ������ҳ��ͬ
	first_lpn = lpn;

	if(req->operation==READ)        
	{		
		while(lpn<=last_lpn) 		
		{
			sub_state=(ssd->dram->map->map_entry[lpn].state&0xffffffff);
			sub_size=size(sub_state);

			sub=creat_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation, req->granularity,0,16384);
			lpn = lpn + (req->granularity / ssd->parameter->granularity_size[0]);
		}
	}
	else if(req->operation==WRITE)
	{
		unsigned int  remap_fla=0;
		int map_lpn[5];
		int lpn_num = 0;
		balance_flag = balance_granularity(ssd);

		if (balance_flag&&((req->size)>(2*req->granularity/512*ssd->parameter->granularity_channel[req_grannnum(ssd,req->granularity)])))//��������������򽫴�������еĲ��ְ�ͨ�������Լ���д�ٶȷ��䵽��С���ȵ�����
		//if (balance_flag)
		{
			
			if (req->granularity == 16384)//�����tlc��tlcĥ����������ط��䣬������Ҫ�����lpn����
			{
				ssd->count_valid0++;
				for (i = 0; i <= ssd->parameter->granularity_num; i++)
				{
					map_lpn[i] = round((req->size * ssd->map_factor[i])/(req->granularity/512)) ;
					//map_lpn[i] = 3*ssd->parameter->granularity_channel[i];
					//map_lpn[i] = ssd->parameter->granularity_channel[i];;
				}
				//balance_flag = 0;
			}
			else//����ǽ�С���ȵ������򲻽����ط���
			{
				balance_flag = 0;
			}

			//int xxxx = easre_underleast_granularity(ssd, req_grannnum(ssd, req->granularity));
			//printf("origin:%d -> %d\n", req->granularity, ssd->parameter->granularity_size[xxxx]);
				
		}
		else if (balance_flag && ((req->size) > 32*need_map_size(ssd))&&(req->size<= 2*( req->granularity / 512 * ssd->parameter->granularity_channel[req_grannnum(ssd, req->granularity)])))
		{
			if (req->granularity == 16384)//�����tlc��tlcĥ����������ط��䣬������Ҫ�����lpn����
			{
				ssd->count_valid1++;
				//printf("req:size %d\n", req->size);
				for (i = 0; i <= ssd->parameter->granularity_num; i++)
				{
					map_lpn[i] = ssd->parameter->granularity_channel[i];
					//map_lpn[i] = 2;
				}
				//balance_flag = 0;
			}
			else//����ǽ�С���ȵ������򲻽����ط���
			{
				balance_flag = 0;
			}
		}
		//else if(balance_flag && ((req->size) <= 32 * need_map_size(ssd)) && (req->size > ssd->parameter->granularity_num*32))
		//{
		//	if (req->granularity == 16384)//�����tlc��tlcĥ����������ط��䣬������Ҫ�����lpn����
		//	{
		//		ssd->count_valid2++;
		//		for (i = 0; i <= ssd->parameter->granularity_num; i++)
		//		{
		//			map_lpn[i] = ssd->parameter->granularity_channel[i];;
		//		}
		//		//balance_flag = 0;
		//	}
		//	else//����ǽ�С���ȵ������򲻽����ط���
		//	{
		//		balance_flag = 0;
		//	}
		//}
		//else if (balance_flag &&  (req->size <= ssd->parameter->granularity_num * 32))
		//{
		//	if (req->granularity == 16384)//�����tlc��tlcĥ����������ط��䣬������Ҫ�����lpn����
		//	{
		//		ssd->count_valid3++;
		//		for (i = 0; i <= ssd->parameter->granularity_num; i++)
		//		{
		//			map_lpn[i] = ssd->parameter->granularity_channel[i];;
		//		}
		//		//balance_flag = 0;
		//	}
		//	else//����ǽ�С���ȵ������򲻽����ط���
		//	{
		//		balance_flag = 0;
		//	}
		//}
		else 
		{
			balance_flag = 0;
		}
		int temp = 0;
		balance_flag = 0;
		//balance_flag = 1;
		//map_lpn[0] = ceil(req->size / (req->granularity / 512))/3;
		//map_lpn[1] = ceil(req->size / (req->granularity / 512))- map_lpn[0];
		while (lpn <= last_lpn)
		{
			if (balance_flag)//��Ҫ��ӳ��
			{
				//printf("page num:%d\n", map_lpn[temp]);
				if (map_lpn[temp] == 0)
				{
					temp++;
				}
				if (ssd->parameter->granularity_size[temp] != 16384)//��ǰ��ûӳ�䵽tlc����
				{
					if (map_lpn[temp] > 0 )
					{
						remap_fla = 1;
					}
					else
					{
						
						remap_fla = 0;
					}
				}
				else//�����tlc��������Ҫӳ��
				{
					balance_flag = 0;
				}

			}

			if ((req->granularity / 512) != 32)
				mask = ~(0xffffffff << (req->granularity / 512));
			else
			{
				mask = 0xffffffff;
			}
			state = mask;

			if (lpn == first_lpn)
			{
				
				offset1 = (req->granularity / 512) - ((lpn + (req->granularity / ssd->parameter->granularity_size[0])) * (ssd->parameter->granularity_size[0] / 512) - req->lsn); //����д��������ȼ���������µ�ƫ����
				state = state & (0xffffffff << offset1); //��ʼƫ����
			}
			if (lpn == last_lpn)
			{
				
				offset2 = (req->granularity / 512) - ((lpn + (req->granularity / ssd->parameter->granularity_size[0])) * (ssd->parameter->granularity_size[0] / 512) - (req->lsn + req->size));
				state = state & (~(0xffffffff << offset2));//����ƫ����
			}
			sub_size = size(state);
			sub = creat_sub_request(ssd, lpn, sub_size, state, req, WRITE, req->granularity, remap_fla,ssd->parameter->granularity_size[temp]);
			
			lpn = lpn + (req->granularity / ssd->parameter->granularity_size[0]);
			map_lpn[temp]--;
			
		}
		temp = 0;
	}

	return ssd;
}

unsigned int easre_least_channle(struct ssd_info* ssd, unsigned int granu)
{
	int i,j, least = 10000;
	for (i = 0; i < ssd->parameter->granularity_channel[granu]; i++)
	{
		

		if (ssd->granularity_head[granu].channel_head[i].erase_count < least)
		{
			j = i;
			least = ssd->granularity_head[i].channel_head->erase_count;
		}
	}
	//printf("granu: %d: earse least channel:%d \n",granu , j);
	return j;
}
unsigned int easre_most_channle(struct ssd_info* ssd, unsigned int granu)
{
	int i, j=0, most = 0;
	for (i = 0; i < ssd->parameter->granularity_channel[granu]; i++)
	{


		if (ssd->granularity_head[granu].channel_head[i].erase_count > most)
		{
			j = i;
			most = ssd->granularity_head[granu].channel_head[i].erase_count;
		}
	}
	//printf("granu: %d: earse least channel:%d \n", granu, j);
	return j;
}

unsigned int easre_least_chip(struct ssd_info* ssd, unsigned int granu, unsigned int channel)
{
	int i, least = 100000, l;
	if (ssd->granularity_head[granu].channel_head[channel].chip_head[0].erase_count >= ssd->granularity_head[granu].channel_head[channel].chip_head[1].erase_count)
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}


unsigned int easre_least_granularity(struct ssd_info* ssd)
{
	int i, least = 10000000, l;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{


		if (ssd->granularity_head[i].erase_count *ssd->earse_factor[i]/ ssd->parameter->granularity_channel[i] < least)
		{
			l = i;
			least = ssd->granularity_head[i].erase_count* ssd->earse_factor[i] / ssd->parameter->granularity_channel[i];
		}

	}
	
	return l;
}
unsigned int easre_underleast_granularity(struct ssd_info* ssd,unsigned int granu)
{
	int i, least = 10000000, l;
	for (i = 0; i < granu+1; i++)
	{


		if ((ssd->granularity_head[i].erase_count  * ssd->earse_factor[i] / ssd->parameter->granularity_channel[i]) < least)
		{
			l = i;
			least = ssd->granularity_head[i].erase_count  * ssd->earse_factor[i] / ssd->parameter->granularity_channel[i];
		}

	}
	return l;
}
unsigned int balance_granularity(struct ssd_info* ssd)
{
	int i, least = 10000000, erase[4],total = 0 ,avg = 0;
	double bz = 0.0;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		total += ssd->granularity_head[i].erase_count * ssd->earse_factor[i] / ssd->parameter->granularity_channel[i] ;
		erase[i] = ssd->granularity_head[i].erase_count * ssd->earse_factor[i] / ssd->parameter->granularity_channel[i];

	}
	avg = total / ssd->parameter->granularity_num;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{	 
		bz += pow(erase[i] - avg, 2);

	}
	bz = sqrt(bz / ssd->parameter->granularity_num);
	//printf("bz: %lf\n",bz);
	//if (bz < 1)
	//{
	//	return 0;
	//}
	//else
	//{
	//	return 1;
	//}
	return 1;
}

unsigned int pre_least_channel(struct ssd_info* ssd, unsigned int granu)
{
	int i, j, least = 1000000000,  l;

	for (i = 0; i < ssd->parameter->granularity_channel[granu]; i++)
	{

		if ((ssd->granularity_head[granu].channel_head[i].program_count) < least)
		{
			l = i;
			least = ssd->granularity_head[granu].channel_head[i].program_count;
		}

	}
	//printf("pre write least channel:%d \n",l);
	return l;
}
unsigned int pre_least_granu(struct ssd_info* ssd)
{
	int i,j, least = 10000000,graun_totol=0, l;

	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{

		if ((ssd->granularity_head[i].progranm_count*(ssd->parameter->granularity_size[i]/ssd->parameter->granularity_size[0]) / (ssd->parameter->granularity_channel[i])) < least)
		{
			l = i;
			least = (ssd->granularity_head[i].progranm_count * (ssd->parameter->granularity_size[i] / ssd->parameter->granularity_size[0]) / (ssd->parameter->granularity_channel[i]));
		}

	}

	return l;
}
unsigned int pre_avg_granu(struct ssd_info* ssd)
{
	int i, least = 10000000, program[4], total = 0, avg = 0;
	double pro = 0.0;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		total += (ssd->granularity_head[i].progranm_count * (ssd->parameter->granularity_size[i] / ssd->parameter->granularity_size[0]) / (ssd->parameter->granularity_channel[i]));
		program[i] = (ssd->granularity_head[i].progranm_count * (ssd->parameter->granularity_size[i] / ssd->parameter->granularity_size[0]) / (ssd->parameter->granularity_channel[i]));

	}
	avg = total / ssd->parameter->granularity_num;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		pro += pow(program[i] - avg, 2);

	}
	pro = sqrt(pro / ssd->parameter->granularity_num);
	//printf("pro:%f\n", pro);
	if (pro < 10)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

unsigned int need_map_size(struct ssd_info* ssd)
{
	unsigned int i,total=0;
	for (i = 0; i < ssd->parameter->granularity_num; i++)
	{
		if (ssd->parameter->granularity_size[i] != 16384)
		{
			total += ssd->parameter->granularity_channel[i] * 2;
		}
	}
	return total;
}