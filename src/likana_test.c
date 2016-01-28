#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
	printf("likana check/start\n");

	assert( WEXITSTATUS( system( "./likana -v" ) ) == 0 );

	assert( WEXITSTATUS( system( "./likana -h" ) ) == 0 );

	assert( WEXITSTATUS( system( "./likana" )    ) == 1 );

	printf("likana check/end\n");
	return 0;
}
