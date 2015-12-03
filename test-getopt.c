/*=============================================================================
#     FileName: test-getopt.c
#         Desc: simple demo for getopt func to parse args
#       Author: mardyu
#        Email: michealyxd@hotmail.com

#      Version: 0.0.1
#   LastChange: 2015-12-03 19:52:15
#      History:
=============================================================================*/


#include <stdio.h>
#include <unistd.h>
#include <getopt.h>


int main(int argc, char **args)
{
	int op;
	/*
	 * params:
	 *	without arg: -a, -b -e
	 *	with arg: -c, -d, -f
	 * if character of optstring with ':'(colon) or '?' behind means params
	 * with arg
	 * use ? if without args op='param', optarg=NULL
	 * use : if without args have error msg send to stderr, op='?',
	 *	optarg=NULL
	 * */
	while(0 <= (op = getopt(argc, args, "abc?d:ef:"))) {
		fprintf(stdout, "key: %c value: %s\n", op, optarg);
	}

	return 0;
}
