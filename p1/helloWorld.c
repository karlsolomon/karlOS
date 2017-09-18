#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*
Use Cases

a > b		set stdout of a to b
b < a		set stdin of b to a
a >> b		set stderr of a to b
likely a recursive solution here (front to back)

a | b

probably parse the whole line and determine sequence of actions.
pipe has the highest priority

first split on pipes
then split on file directors (>, >>, <)
then execute command

char*** pipes = [numPipes + 2]; //n pipes for n+1 commands, + null-terminator






a > b | c > d


*/




int main(void) {
        printf("Hello World!\n");
        int c;
        bool quit = false;
        while(!quit) {
                printf( "Enter a value :");
                c = getchar( );
                if(c == '!') {
                        quit = true;
                }
                printf( "\nYou entered: ");
                putchar( c );
        }
        return 0;
}
