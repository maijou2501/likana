bin_PROGRAMS   = likana
likana_SOURCES = likana.c
likana_CFLAGS  = -O3 -s -fstack-protector-all -fPIE -pie -ffunction-sections -fdata-sections -pthread
likana_LDFLAGS = -Wl,-z,now,-z,relro,--gc-sections

TESTS=likana_test
