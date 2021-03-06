COMPONENT_NAME = shell_commands

SRC_FILES = \
	../components/shell/src/commands/cmd_exit.c \
	../components/shell/src/commands/cmd_info.c \
	../components/shell/src/commands/cmd_memdump.c

TEST_SRC_FILES = \
	src/shell/test_shell_commands.cpp

INCLUDE_DIRS += \
	../components/logging/include \
	../components/shell/src \
	../components/shell/include

include test_runners/MakefileRunner.mk
