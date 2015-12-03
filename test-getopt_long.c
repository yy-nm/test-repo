/*=============================================================================
#     FileName: test-getopt_long.c
#         Desc: parse args both with -a and --all
#       Author: mardyu
#        Email: michealyxd@hotmail.com

#      Version: 0.0.1
#   LastChange: 2015-12-03 20:17:41
#      History:
=============================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

/*
 * ./getopt_long -0 0 --aaa 11 --bbb 22 --ccc --ddd 444
 * output:
 * key 48, value: 0
 * key 0, value: 11
 * key 98, value: (null)
 * key 99, value: (null)
 * key 100, value: 444
 *
 * -----------------------
 *  option:
 *
 *
 * */
int main(int argc, char **args)
{
	struct option opts[] = {
		{"aaa", required_argument, 0, 0},
		{"bbb", no_argument, 0, 'b'},
		{"ccc", optional_argument, 0, 'c'},
		{"ddd", required_argument, 0, 'd'},
		{0, 0, 0, 0}
	};
	int op;
	while (0 <= (op =  getopt_long(argc, args, "0:1", opts, 0))) {
		fprintf(stdout, "key %d, value: %s\n", op, optarg);
	}

	return 0;
}

