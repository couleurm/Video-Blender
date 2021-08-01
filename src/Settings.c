#include "Settings.h"

#include <stdlib.h>
#include <string.h>

#include "utils/Utils.h"
#include "utils/Queue.h"

int set_default_values(Settings* settings)
{
	if (!settings->input_file)
	{
		printf("no input file\n");
		return 1;
	}
	if (!settings->output_file)
		settings->output_file = (char*)"default.mp4";
	if (!settings->num)
	{
		settings->num = 1;
		settings->den = 60;
	}
	if (!settings->w_count)
	{
		settings->w_count = 2;
		settings->weights = malloc(sizeof(int) * settings->w_count);
		if (!settings->weights)
		{
			printf("can't alloc\n");
			return 1;
		}
		for (int i = 0; i < settings->w_count; i++)
		{
			settings->weights[i] = 1;
		}
	}
	if (settings->opengl)
		settings->threads = 1;
	if (!settings->threads)
		settings->threads = get_available_threads();
	if (!settings->decoder)
		settings->decoder = AV_CODEC_ID_H264;
	if (!settings->preset)
		settings->preset = "ultrafast";
	if (!settings->crf)
		settings->crf = "0";
	if (!settings->pix_fmt)
		settings->pix_fmt = AV_PIX_FMT_YUV444P;

	return 0;
}

int parse(Settings* settings, int argc, char** args)
{
	memset(settings, 0, sizeof(Settings));

	if (argc < 2)
	{
		print_usage();
		return 1;
	}
	for (int i = 1; i < argc; i++)
	{
		if (args[i][0] != '-')
		{
			printf(args[i]);
			printf("is not an argument\n");
			return 1;
		}
		if (args[i][1] == 'i')
		{
			i += 1;
			if (file_exists(args[i]))
			{
				settings->input_file = args[i];
				continue;
			}
			printf("input file does not exist\n");
			return 1;
		}
		if (args[i][1] == 'o')
		{
			i += 1;
			settings->output_file = args[i];
			continue;
		}
		if (!strcmp(&args[i][1], "hd"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-hd needs an argument, possible values : Nvidia, Intel\n");
				return 1;
			}
			if (str_equals_case_insensitive(args[i], "nvidia"))
			{
				settings->hardware_decoding = "Nvidia";
				continue;
			}
			if (str_equals_case_insensitive(args[i], "intel"))
			{
				settings->hardware_decoding = "Intel";
				continue;
			}
			printf(args[i]);
			printf(" unknown decoder\nnpossible values : Nvidia, Intel\n");
			return 1;
		}
		if (!strcmp(&args[i][1], "he"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-he needs an argument, possible values : Nvidia, Intel, AMD\n");
				return 1;
			}
			if (str_equals_case_insensitive(args[i], "nvidia"))
			{
				settings->hardware_encoding = "NVIDIA";
				continue;
			}
			if (str_equals_case_insensitive(args[i], "intel"))
			{
				settings->hardware_encoding = "Intel";
				continue;
			}
			if (str_equals_case_insensitive(args[i], "amd"))
			{
				settings->hardware_encoding = "AMD";
				continue;
			}
			printf(args[i]);
			printf(" unknown encoder\npossible values : Nvidia, Intel, AMD\n");
			return 1;
		}
		if (!strcmp(&args[i][1], "fps"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-fps needs an argument\n");
				return 1;
			}
			settings->num = 1;
			if (cstr_to_int(args[i], &settings->den))
			{
				printf("%s is not an integer\n", args[i]);
				return 1;
			}
			printf("fps %d\n", settings->den);
			continue;
		}
		if (!strcmp(&args[i][1], "timebase"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-timebase needs an argument\n");
				return 1;
			}
			int len = strlen(args[i]);
			if (len < 3)
				goto timebase_fail;
			int separator_index;
			if (char_index_in_str(args[i], '/', &separator_index))
				goto timebase_fail;
			if (separator_index == 0 || separator_index == (len - 1))
				goto timebase_fail;
			char* tmpstr = malloc(sizeof(char) * separator_index+1);
			if (!tmpstr)
			{
				printf("can't alloc\n");
				return 1;
			}
			memcpy(tmpstr, args[i], sizeof(char) * separator_index);
			tmpstr[separator_index] = '\0';
			if (cstr_to_int(tmpstr, &settings->num))
			{
				printf("invalid numerator %s\n", tmpstr);
				return 1;
			}
			if (settings->num <= 0)
			{
				printf("numerator must be > 0\n");
				return 1;
			}
			free(tmpstr);
			tmpstr = malloc(len - separator_index +1);
			if (!tmpstr)
			{
				printf("can't alloc\n");
				return 1;
			}
			memcpy(tmpstr, &args[i][separator_index + 1], sizeof(char)* (len - separator_index));
			tmpstr[len - separator_index] = '\0';
			if (cstr_to_int(tmpstr, &settings->den))
			{
				printf("invalid denomitor %s\n", tmpstr);
				return 1;
			}
			if (settings->den <= 0)
			{
				printf("denominator must be > 0\n");
				return 1;
			}
			free(tmpstr);
			printf("timebase : %d/%d\n", settings->num, settings->den);
			continue;

		timebase_fail:
			printf("%s timebase should be written : num/den\n", args[i]);
			return 1;
		}
		if (!strcmp(&args[i][1], "weights"))
		{
			Queue q;
			int value;
			init(&q, sizeof(int));

			while (1)
			{
				if (cstr_to_int(args[i+1], &value))
					break;
				add(&q, value);
				i += 1;
				if (i+1 >= argc)
					break;
			}
			if (q.size == 1)
			{
				front(&q, &value);
				if (value < 2)
				{
					printf("need at least 2 weights\n");
					return 1;
				}
				settings->w_count = value;
				settings->weights = malloc(sizeof(int) * value);
				if (!settings->weights)
				{
					printf("can't malloc\n");
					return 1;
				}
				for (int i = 0; i < value; i++)
				{
					settings->weights[i] = 1;
				}
				qremove(&q);
				continue;
			}
			if (q.size < 2)
			{
				printf("need at least 2 weights\n");
				return 1;
			}

			settings->w_count = q.size;
			value = sizeof(int) * q.size;
			settings->weights = malloc(value);
			if (!settings->weights)
			{
				printf("can't alloc\n");
				return 1;
			}
			int size = q.size;
			for (int j = 0; j < size; j++)
			{
				front(&q, &value);
				settings->weights[j] = value;
				qremove(&q);
			}
			continue;
		}
		if (!strcmp(&args[i][1], "threads"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-threads needs an argument\n");
				return 1;
			}
			if (cstr_to_int(args[i], &settings->threads))
			{
				printf("invalid thread count\n");
				return 1;
			}
			if (settings->threads < 1)
			{
				printf("it needs at least one thread\n");
				settings->threads = 1;
			}
			continue;
		}
		if (!strcmp(&args[i][1], "opengl"))
		{
			settings->opengl = 1;
			continue;
		}
		if (!strcmp(&args[i][1], "decoder"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-decoder needs an argument\n");
				return 1;
			}
			if (!strcmp(args[i], "h264"))
			{
				settings->decoder = AV_CODEC_ID_H264;
				continue;
			}
			if (!strcmp(args[i], "h265"))
			{
				settings->decoder = AV_CODEC_ID_H265;
				continue;
			}
			printf("invalid decoder\n");
			return 1;
		}
		if (!strcmp(&args[i][1], "preset"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-preset needs an argument\n");
				return 1;
			}
			settings->preset = args[i];
		}
		if (!strcmp(&args[i][1], "crf"))
		{
			i += 1;
			if (i >= argc)
			{
				printf("-crf needs an argument\n");
				return 1;
			}
			settings->crf = args[i];
		}
		printf("unknown argument : %s\n", args[i]);
		return 1;
	}

	if (set_default_values(settings))
	{
		return 1;
	}

	return 0;
}

void print_usage()
{
	printf(
		"file options :\n"
		"\t-i file \t input file\n"
		"\t-o file \t output file\n"
		"\n"
		"output framerate options :\n"
		"\t-fps int \t set timebase with framerate\n"
		"\t-timebase num/den \t set timebase ex:-timebase 1/60\n"
		"\n"
		"weights options :\n"
		"\t-weights int \t set the number of frames blended with weights 1\n"
		"\t-weights ints \t set weights ex:-weights 1 1\n"
		"\n"
		"processing options :\n"
		"\t-threads int \t set number of threads for blending\n"
		"\t-opengl \t use opengl to blend frames\n"
		"\n"
		"codec options :\n"
		"\t-decoder codec \t set decoder, possible values:h264,h265\n"
		"\t-preset preset \t set decoder preset\n"
		"\t-crf int \t set quality\n"
		"\t-hd dec \t set hardware decoder, possible values:Intel,Nvidia\n"
		"\t-he enc \t set hardware encoder, possible values:Intel,Nvidia,AMD\n"
		"\n"
	);
}