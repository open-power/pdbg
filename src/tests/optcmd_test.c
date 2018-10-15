#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ccan/array_size/array_size.h>

#include "../optcmd.h"
#include "../parsers.h"

/* The OPTCMD_TEST_BREAK_BUILD* defines can be used to test that we catch type
 * mistmatch errors between function/flag definitions and parsers */
#ifdef OPTCMD_TEST_BREAK_BUILD1
struct flags {
	bool test_bool;
	int test_num;
};
#else
struct flags {
	bool test_bool;
	uint64_t test_num;
};
#endif

/* Format: (<flag>, <field>, <parser>, <default value>) */
#define FLAG_TEST_BOOL ("--test-bool", test_bool, parse_flag_noarg, false)
#define FLAG_TEST_NUM ("--test-num", test_num, parse_number64, 10)

/* Format: (<parser>, <default value>)
 *
 * <default value> may be NULL if argument must be supplied.
 */
#define ARG_NUM (parse_number64, NULL)
#define ARG_NUM_OPT (parse_number64, "2")

static uint64_t num, opt, flag;
static bool bool_flag;

static int test(void)
{
	bool_flag = true;

	return 0;
}
OPTCMD_DEFINE_CMD(test, test);

#ifdef OPTCMD_TEST_BREAK_BUILD2
static int test_args(int num_arg, int opt_arg)
{
	return 0;
}
#else
static int test_args(uint64_t num_arg, uint64_t opt_arg)
{
	num = num_arg;
	opt = opt_arg;

	return 0;
}
#endif
OPTCMD_DEFINE_CMD_WITH_ARGS(test_args, test_args, (ARG_NUM, ARG_NUM_OPT));

static int test_flags(uint64_t num_arg, uint64_t opt_arg, struct flags flags)
{
	num = num_arg;
	opt = opt_arg;
	flag = flags.test_num;
	bool_flag = flags.test_bool;

	return 0;
}
OPTCMD_DEFINE_CMD_WITH_FLAGS(test_flags, test_flags, (ARG_NUM, ARG_NUM_OPT),
			     flags, (FLAG_TEST_BOOL, FLAG_TEST_NUM));

static int test_only_flags(struct flags flags)
{
	flag = flags.test_num;
	bool_flag = flags.test_bool;

	return 0;
}
OPTCMD_DEFINE_CMD_ONLY_FLAGS(test_only_flags, test_only_flags, flags,
			     (FLAG_TEST_BOOL, FLAG_TEST_NUM));

int parse_argv(const char *argv[], int argc)
{
	int i, rc;
	void **args, **flags;
	struct optcmd_cmd *cmds[] = { &optcmd_test, &optcmd_test_args, &optcmd_test_flags,
				      &optcmd_test_only_flags };
	optcmd_cmd_t *cmd;

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (!strcmp(argv[0], cmds[i]->cmd)) {
			/* Found our command */
			cmd = optcmd_parse(cmds[i], &argv[1], argc - 1, &args, &flags);
			if (cmd) {
				rc = cmd(args, flags);
				return rc;
			}
		}
	}

	return -1;
}

int main(void)
{
	/* Tests */
	const char *test1_argv[] = { "test" };
	bool_flag = false;
	assert(!parse_argv(test1_argv, ARRAY_SIZE(test1_argv)));
	assert(bool_flag);

	const char *test2_argv[] = { "test_args", "1" };
	assert(!parse_argv(test2_argv, ARRAY_SIZE(test2_argv)));
	assert(num == 1);
	assert(opt == 2);

	const char *test3_argv[] = { "test_args", "2", "3" };
	assert(!parse_argv(test3_argv, ARRAY_SIZE(test3_argv)));
	assert(num == 2);
	assert(opt == 3);

	const char *test4_argv[] = { "test_flags", "4", "5" };
	assert(!parse_argv(test4_argv, ARRAY_SIZE(test4_argv)));
	assert(num == 4);
	assert(opt == 5);
	assert(flag == 10);

	bool_flag = false;

	const char *test5_argv[] = { "test_flags", "5", "6" };
	assert(!parse_argv(test5_argv, ARRAY_SIZE(test5_argv)));
	assert(num == 5);
	assert(opt == 6);
	assert(flag == 10);
	assert(!bool_flag);

	const char *test6_argv[] = { "test_flags", "7", "8", "--test-num=9" };
	assert(!parse_argv(test6_argv, ARRAY_SIZE(test6_argv)));
	assert(num == 7);
	assert(opt == 8);
	assert(flag == 9);
	assert(!bool_flag);

	const char *test7_argv[] = { "test_flags", "8", "9", "--test-bool" };
	assert(!parse_argv(test7_argv, ARRAY_SIZE(test7_argv)));
	assert(num == 8);
	assert(opt == 9);
	assert(flag == 10);
	assert(bool_flag);

	bool_flag = false;

	const char *test8_argv[] = { "test_flags", "9", "10", "--test-bool", "--test-num=11" };
	assert(!parse_argv(test8_argv, ARRAY_SIZE(test8_argv)));
	assert(num == 9);
	assert(opt == 10);
	assert(flag == 11);
	assert(bool_flag);

	/* This should fail, too many arguments */
	const char *test9_argv[] = { "test_flags", "9", "10", "11", "--test-bool", "--test-num=11" };
	assert(parse_argv(test9_argv, ARRAY_SIZE(test9_argv)));

	/* So should this, unknown flag */
	const char *test10_argv[] = { "test_flags", "9", "10", "--test-blah", "--test-num=11" };
	assert(parse_argv(test10_argv, ARRAY_SIZE(test10_argv)));

	const char *test11_argv[] = { "test_only_flags" };
	assert(!parse_argv(test11_argv, ARRAY_SIZE(test11_argv)));
	assert(flag == 10);
	assert(!bool_flag);

	const char *test12_argv[] = { "test_only_flags", "--test-num=12" };
	assert(!parse_argv(test12_argv, ARRAY_SIZE(test12_argv)));
	assert(flag == 12);
	assert(!bool_flag);

	/* Should fail because we're passing a positional argument */
	const char *test13_argv[] = { "test_only_flags", "--test-num=12", "13" };
	assert(parse_argv(test13_argv, ARRAY_SIZE(test13_argv)));

	return 0;
}
